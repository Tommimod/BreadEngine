#include "diligentRenderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include <Sampler.h>
#include <Utilities/interface/DiligentFXShaderSourceStreamFactory.hpp>
#include <Shader.h>

#include "logger.h"
#include "rendering/sky/hosekWilkie.h"
#include "utils/workerPool.h"

namespace BreadEngine {
    /// Environment cubes are float for the same reason the scene target is: a sky carries a sun,
    /// and the whole point of baking one is that the values above white survive to light with.
    constexpr Diligent::TEXTURE_FORMAT SKY_FORMAT = Diligent::TEX_FORMAT_RGBA16_FLOAT;

    /// What the sky model's physical radiance is divided by on its way into the engine's own
    /// units. Hosek-Wilkie returns absolute radiance, in W / (m^2 sr nm); a Light's intensity is
    /// an authored multiplier with no unit at all. The two have to be reconciled somewhere, and
    /// it is done here rather than by moving the lights, because every intensity already
    /// authored in a scene stays valid this way and none of them would survive the alternative.
    /// The value puts a clear day's zenith near 1, so a scene reads before any exposure is set.
    constexpr float SKY_RADIANCE_SCALE = 1.0f / 25.0f;

    /// Face size of the diffuse irradiance cube, and how many directions each of its texels
    /// integrates over. It holds a cosine convolution of the whole sky, so there is nothing in
    /// it finer than a slow gradient and a small face resolves it completely.
    constexpr int IRRADIANCE_CUBE_SIZE = 64;
    constexpr float IRRADIANCE_SAMPLE_COUNT = 256.0f;

    /// Face size of the reflection cube, and how many samples each of its texels averages.
    constexpr int PREFILTERED_CUBE_SIZE = 128;
    constexpr float PREFILTERED_SAMPLE_COUNT = 128.0f;

    /// The preintegrated GGX table, indexed by the cosine of the viewing angle and by
    /// roughness. Both axes are smooth, which is why so small a table is enough; the sample
    /// count is generous because it is paid once at startup.
    constexpr Diligent::Uint32 BRDF_LUT_SIZE = 256;
    constexpr Diligent::Uint32 BRDF_LUT_SAMPLE_COUNT = 512;

    /// Bounds on the cube a loaded equirectangular image is unwrapped into. A face covers a
    /// quarter turn where the source spans a full one, so half the source's height is the size
    /// at which neither is resolving detail the other does not have.
    constexpr int MIN_SKY_RESOLUTION = 64;
    constexpr int MAX_SKY_RESOLUTION = 2048;

    void EnvironmentMaps::initializeAmbient(Diligent::IRenderDevice *device, Diligent::IDeviceContext *context)
    {
        _device = device;
        _context = context;

        createIblPipelines();
    }

    void EnvironmentMaps::initializeSky()
    {
        createSkyPipelines();
    }

    void EnvironmentMaps::shutdown()
    {
        // Every slot the pool is about to drop may still have a decode running into it, and
        // the future does not wait on its own. There is no later frame to reap one on.
        _cubemaps.forEachAlive([](CubemapSlot &slot)
        {
            if (slot.decodeJob.valid()) slot.decodeJob.get();
        });
        _cubemaps.clear();
        _ambientMaps.clear();

        _skyboxBinding.Release();
        _skyboxPipeline.Release();
        _skyboxConstants.Release();
        _equirectBakeBinding.Release();
        _equirectBakePipeline.Release();
        _skyBakeBinding.Release();
        _skyBakePipeline.Release();
        _skyBakeConstants.Release();
        _prefilterBinding.Release();
        _prefilterPipeline.Release();
        _irradianceBinding.Release();
        _irradiancePipeline.Release();
        _iblBakeConstants.Release();
        _ambientFallback.Release();
        _brdfLut.Release();

        _context = nullptr;
        _device = nullptr;
    }

    void EnvironmentMaps::setSettings(const EnvironmentSettings &settings)
    {
        _sky = settings.background.sky;
        _skyRotation = settings.background.rotation;
        _skyEnergy = settings.background.energy;
        _skyBlur = settings.background.skyBlur;

        _ambientColor = settings.ambient.color;
        _ambientEnergy = settings.ambient.energy;
        _ambientMap = settings.ambient.map;
    }

    AmbientLookup EnvironmentMaps::ambientLookup() const
    {
        const auto color = ColorNormalize(_ambientColor);
        // The inspector's rotation turns the sky and this turns the direction it is sampled
        // with, and those are opposites.
        const auto rotation = QuaternionInvert(_skyRotation);
        const bool hasAmbientMap = _ambientMaps.get(_ambientMap) != nullptr;

        return {
            .prefiltered = prefilteredView(_ambientMap),
            .color = {color.x, color.y, color.z, _ambientEnergy},
            .rotation = {rotation.x, rotation.y, rotation.z, rotation.w},
            .params = {hasAmbientMap ? 1.0f : 0.0f, static_cast<float>(PREFILTERED_CUBE_MIPS - 1), 0.0f, 0.0f}
        };
    }

    void EnvironmentMaps::createSkyPipelines()
    {
        if (!_device) return;

        Diligent::BufferDesc constantsDesc;
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        constantsDesc.Name = "Sky bake constants";
        constantsDesc.Size = sizeof(SkyBakeConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_skyBakeConstants);
        constantsDesc.Name = "Skybox constants";
        constantsDesc.Size = sizeof(SkyboxConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_skyboxConstants);

        const auto shaderSources = createShaderSources();
        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = shaderSources;

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        shaderInfo.Desc = {"Fullscreen VS", Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.FilePath = "fullscreen.vsh";
        _device->CreateShader(shaderInfo, &vertexShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> proceduralShader;
        shaderInfo.Desc = {"Procedural sky PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "skyProcedural.psh";
        _device->CreateShader(shaderInfo, &proceduralShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> equirectangularShader;
        shaderInfo.Desc = {"Equirectangular sky PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "skyEquirect.psh";
        _device->CreateShader(shaderInfo, &equirectangularShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> skyboxShader;
        shaderInfo.Desc = {"Skybox PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "skybox.psh";
        _device->CreateShader(shaderInfo, &skyboxShader);

        if (!vertexShader || !proceduralShader || !equirectangularShader || !skyboxShader)
        {
            Logger::LogError("Diligent failed to compile the sky shaders");
            return;
        }

        // All three passes are the same one triangle over a whole target; only what they sample
        // and where they land differ.
        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
        pipelineInfo.pVS = vertexShader;
        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = SKY_FORMAT;
        graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
        graphics.DepthStencilDesc.DepthEnable = Diligent::False;

        pipelineInfo.PSODesc.Name = "Procedural sky bake";
        pipelineInfo.pPS = proceduralShader;
        _device->CreateGraphicsPipelineState(pipelineInfo, &_skyBakePipeline);

        // Dynamic for the same reason the composite pass's input is: the source changes every
        // time a different image is loaded, and a mutable variable cannot be re-pointed.
        const Diligent::ShaderResourceVariableDesc equirectangularVariables[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_Equirect", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };
        pipelineInfo.PSODesc.Name = "Equirectangular sky bake";
        pipelineInfo.pPS = equirectangularShader;
        pipelineInfo.PSODesc.ResourceLayout.Variables = equirectangularVariables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = 1;
        _device->CreateGraphicsPipelineState(pipelineInfo, &_equirectBakePipeline);

        // The background pass lands in the scene target instead, at the far plane under a
        // LESS_EQUAL test, which is what confines it to the pixels no geometry reached. It
        // writes no depth: the editor's overlay tests against this buffer, and a sky is not
        // something the grid should be hidden behind.
        const Diligent::ShaderResourceVariableDesc skyboxVariables[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_Sky", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };
        pipelineInfo.PSODesc.Name = "Skybox";
        pipelineInfo.pPS = skyboxShader;
        pipelineInfo.PSODesc.ResourceLayout.Variables = skyboxVariables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = 1;
        graphics.RTVFormats[0] = SCENE_COLOR_FORMAT;
        graphics.DSVFormat = SCENE_DEPTH_FORMAT;
        graphics.DepthStencilDesc.DepthEnable = Diligent::True;
        graphics.DepthStencilDesc.DepthWriteEnable = Diligent::False;
        graphics.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;
        _device->CreateGraphicsPipelineState(pipelineInfo, &_skyboxPipeline);

        if (!_skyBakePipeline || !_equirectBakePipeline || !_skyboxPipeline)
        {
            Logger::LogError("Diligent failed to create the sky pipeline states");
            return;
        }

        _skyBakePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "SkyConstants")->Set(_skyBakeConstants);
        _equirectBakePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "SkyConstants")->Set(_skyBakeConstants);
        _skyboxPipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "SkyboxConstants")->Set(_skyboxConstants);

        _skyBakePipeline->CreateShaderResourceBinding(&_skyBakeBinding, true);
        _equirectBakePipeline->CreateShaderResourceBinding(&_equirectBakeBinding, true);
        _skyboxPipeline->CreateShaderResourceBinding(&_skyboxBinding, true);
    }

    void EnvironmentMaps::createIblPipelines()
    {
        if (!_device) return;

        Diligent::BufferDesc constantsDesc;
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        constantsDesc.Name = "IBL bake constants";
        constantsDesc.Size = sizeof(IblBakeConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_iblBakeConstants);

        // Ahead of anything that can fail. The scene pipeline binds the table for its lifetime
        // and every material binds the fallback cube, so a shader that does not compile has to
        // leave this renderer without ambient maps - not without the two things every draw
        // needs whether or not an environment was ever precomputed.
        createAmbientFallbacks();

        const auto shaderSources = createShaderSources();
        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = shaderSources;

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        shaderInfo.Desc = {"Fullscreen VS", Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.FilePath = "fullscreen.vsh";
        _device->CreateShader(shaderInfo, &vertexShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> irradianceShader;
        shaderInfo.Desc = {"Irradiance bake PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "iblIrradiance.psh";
        _device->CreateShader(shaderInfo, &irradianceShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> prefilterShader;
        shaderInfo.Desc = {"Reflection bake PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "iblSpecular.psh";
        _device->CreateShader(shaderInfo, &prefilterShader);

        if (!vertexShader || !irradianceShader || !prefilterShader)
        {
            Logger::LogError("Diligent failed to compile the image-based lighting shaders");
            return;
        }

        // Both passes are the same one triangle over a whole cube face; only the distribution
        // they sample the source with differs.
        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
        pipelineInfo.pVS = vertexShader;
        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = SKY_FORMAT;
        graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
        graphics.DepthStencilDesc.DepthEnable = Diligent::False;

        // Dynamic for the same reason the background pass's cube is: the source is a different
        // texture every time the sky is rebaked, and a mutable variable cannot be re-pointed.
        const Diligent::ShaderResourceVariableDesc environmentVariables[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_Environment", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };
        pipelineInfo.PSODesc.ResourceLayout.Variables = environmentVariables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = 1;

        pipelineInfo.PSODesc.Name = "Irradiance bake";
        pipelineInfo.pPS = irradianceShader;
        _device->CreateGraphicsPipelineState(pipelineInfo, &_irradiancePipeline);

        pipelineInfo.PSODesc.Name = "Reflection bake";
        pipelineInfo.pPS = prefilterShader;
        _device->CreateGraphicsPipelineState(pipelineInfo, &_prefilterPipeline);

        if (!_irradiancePipeline || !_prefilterPipeline)
        {
            Logger::LogError("Diligent failed to create the image-based lighting pipeline states");
            return;
        }

        _irradiancePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "IblBakeConstants")->Set(_iblBakeConstants);
        _prefilterPipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "IblBakeConstants")->Set(_iblBakeConstants);
        _irradiancePipeline->CreateShaderResourceBinding(&_irradianceBinding, true);
        _prefilterPipeline->CreateShaderResourceBinding(&_prefilterBinding, true);
    }

    void EnvironmentMaps::createAmbientFallbacks()
    {
        // One texel per face. It is bound wherever a scene has no environment map, where the
        // shader branches away from it before any fetch - but a draw still validates every
        // binding it has, so the variable cannot be left pointing at nothing.
        constexpr Diligent::Uint64 emptyFace = 0;
        Diligent::TextureSubResData faces[CUBE_FACE_COUNT];
        for (auto &face: faces) face = {&emptyFace, sizeof(emptyFace)};
        const Diligent::TextureData fallbackData{faces, CUBE_FACE_COUNT};

        Diligent::TextureDesc fallbackDesc;
        fallbackDesc.Name = "Ambient fallback cube";
        fallbackDesc.Type = Diligent::RESOURCE_DIM_TEX_CUBE;
        fallbackDesc.Width = fallbackDesc.Height = 1;
        fallbackDesc.ArraySize = CUBE_FACE_COUNT;
        fallbackDesc.MipLevels = 1;
        fallbackDesc.Format = SKY_FORMAT;
        fallbackDesc.BindFlags = Diligent::BIND_SHADER_RESOURCE;
        _device->CreateTexture(fallbackDesc, &fallbackData, &_ambientFallback);
        restoreRaylibPixelStore();
        if (!_ambientFallback) Logger::LogError("Diligent failed to create the ambient fallback cube");

        precomputeBrdfLut();
    }

    void EnvironmentMaps::precomputeBrdfLut()
    {
        Diligent::TextureDesc desc;
        desc.Name = "Preintegrated GGX";
        desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        desc.Width = desc.Height = BRDF_LUT_SIZE;
        desc.MipLevels = 1;
        // Two terms and nothing else: the scale and the offset the split sum applies to f0.
        desc.Format = Diligent::TEX_FORMAT_RG16_FLOAT;
        desc.BindFlags = Diligent::BIND_SHADER_RESOURCE | Diligent::BIND_RENDER_TARGET;
        _device->CreateTexture(desc, nullptr, &_brdfLut);
        if (!_brdfLut)
        {
            Logger::LogError("Diligent failed to create the preintegrated GGX table");
            return;
        }

        // Both stages come straight out of DiligentFX. Nothing about this integral is specific
        // to the engine, and the table is read at exactly the two coordinates it is written at.
        const auto samples = std::to_string(BRDF_LUT_SAMPLE_COUNT) + "u";
        const Diligent::ShaderMacro macros[]{{"NUM_SAMPLES", samples.c_str()}};

        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = &Diligent::DiligentFXShaderSourceStreamFactory::GetInstance();
        shaderInfo.Macros = {macros, 1};

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        shaderInfo.Desc = {"Full screen triangle VS", Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.EntryPoint = "FullScreenTriangleVS";
        shaderInfo.FilePath = "FullScreenTriangleVS.fx";
        _device->CreateShader(shaderInfo, &vertexShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
        shaderInfo.Desc = {"Precompute BRDF PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.EntryPoint = "PrecomputeBRDF_PS";
        shaderInfo.FilePath = "PrecomputeBRDF.psh";
        _device->CreateShader(shaderInfo, &pixelShader);

        if (!vertexShader || !pixelShader)
        {
            Logger::LogError("Diligent failed to compile the BRDF integration shaders");
            return;
        }

        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
        pipelineInfo.PSODesc.Name = "Precompute BRDF";
        pipelineInfo.pVS = vertexShader;
        pipelineInfo.pPS = pixelShader;
        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = desc.Format;
        graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
        graphics.DepthStencilDesc.DepthEnable = Diligent::False;

        // Local: the table outlives the pass that fills it, and nothing ever fills it again.
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline;
        _device->CreateGraphicsPipelineState(pipelineInfo, &pipeline);
        if (!pipeline)
        {
            Logger::LogError("Diligent failed to create the BRDF integration pipeline state");
            return;
        }

        auto *target = _brdfLut->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        _context->SetRenderTargets(1, &target, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->SetPipelineState(pipeline);

        Diligent::DrawAttribs drawAttribs;
        drawAttribs.NumVertices = 3;
        drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        _context->Draw(drawAttribs);

        // Clamped on both axes: the table is indexed by a cosine and by a roughness, and
        // wrapping either would fold a grazing view back onto a head-on one.
        Diligent::SamplerDesc samplerDesc;
        samplerDesc.MinFilter = samplerDesc.MagFilter = samplerDesc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
        samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
        Diligent::RefCntAutoPtr<Diligent::ISampler> sampler;
        _device->CreateSampler(samplerDesc, &sampler);
        _brdfLut->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sampler);
    }

    CubemapHandle EnvironmentMaps::loadCubemap(const std::string &path, const SkyboxCubemapParameters &settings,
                                              const float encoding)
    {
        if (!_equirectBakePipeline) return {};

        const auto ground = toSceneLinear(settings.groundAlbedo, encoding);
        CubemapSlot slot;
        slot.ground = {ground.x, ground.y, ground.z, settings.fillBelowHorizon ? 1.0f : 0.0f};
        slot.path = path;

        const auto handle = _cubemaps.add(std::move(slot));
        auto *pending = _cubemaps.get(handle);

        // A four-thousand-pixel environment image takes seconds to decode, and doing it here
        // would stall every frame in which a sky is picked or an unrelated setting is touched.
        // Decoding needs no device, so it runs off the render thread; finalizeCubemaps does
        // the half that does. Pool slots keep a stable address, so the job captures one.
        // Nothing is reported from in here. Logger appends to a shared buffer and notifies
        // the editor's console, neither of which is synchronised, so the job leaves a null
        // loader behind and finalizeCubemaps says so from the render thread.
        pending->decodeJob = WorkerPool::submit([pending]
        {
            Diligent::TextureLoadInfo loadInfo;
            loadInfo.Name = "Equirectangular sky";
            loadInfo.GenerateMips = false;
            // An .hdr already holds linear radiance; an eight-bit image is encoded. This is the
            // only chance to say which, because the cube it is unwrapped into is float and
            // nothing downstream can tell the two apart afterwards.
            loadInfo.IsSRGB = !pending->path.ends_with(".hdr") && !pending->path.ends_with(".HDR");

            Diligent::CreateTextureLoaderFromFile(pending->path.c_str(), Diligent::IMAGE_FILE_FORMAT_UNKNOWN,
                                                  loadInfo, &pending->loader);
        });

        return handle;
    }

    bool EnvironmentMaps::isCubemapReady(const CubemapHandle handle) const
    {
        const auto *slot = _cubemaps.get(handle);
        return slot != nullptr && slot->texture;
    }

    void EnvironmentMaps::finalizeCubemaps()
    {
        // Slots whose owner let go mid-decode, freed on whichever frame the job lands. First,
        // so a cube nobody wants is never baked.
        _cubemaps.removeIf([](CubemapSlot &slot)
        {
            if (!slot.abandoned) return false;
            if (slot.decodeJob.valid())
            {
                if (slot.decodeJob.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return false;
                slot.decodeJob.get();
            }

            return true;
        });

        _cubemaps.forEachAlive([this](CubemapSlot &slot)
        {
            if (slot.abandoned || !slot.decodeJob.valid()) return;
            // Polled rather than waited on: the whole point of the job is that the frame it
            // was queued in does not stop for it.
            if (slot.decodeJob.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;

            slot.decodeJob.get();
            // Reported here rather than from the job, which runs on a worker thread.
            if (!slot.loader)
            {
                Logger::LogError("Diligent failed to load the skybox image " + slot.path);
                return;
            }

            Diligent::RefCntAutoPtr<Diligent::ITexture> equirectangular;
            slot.loader->CreateTexture(_device, &equirectangular);
            restoreRaylibPixelStore();
            // The decoded pixels live in the loader, and the texture now owns its own copy.
            slot.loader.Release();
            if (!equirectangular) return;

            const auto &sourceDesc = equirectangular->GetDesc();
            if (sourceDesc.Type != Diligent::RESOURCE_DIM_TEX_2D)
            {
                Logger::LogError("A skybox image must be a single equirectangular picture: " + slot.path);
                return;
            }

            Diligent::SamplerDesc samplerDesc;
            samplerDesc.MinFilter = samplerDesc.MagFilter = samplerDesc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
            // Wrapped across the seam and clamped at the poles, which is how the projection
            // runs: longitude comes back around, latitude stops.
            samplerDesc.AddressU = Diligent::TEXTURE_ADDRESS_WRAP;
            samplerDesc.AddressV = samplerDesc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
            Diligent::RefCntAutoPtr<Diligent::ISampler> sampler;
            _device->CreateSampler(samplerDesc, &sampler);
            auto *sourceView = equirectangular->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
            sourceView->SetSampler(sampler);

            _equirectBakeBinding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Equirect")->Set(sourceView);

            // A face covers a quarter turn where the source spans a full one, so half the
            // source's height is the size at which neither resolves detail the other lacks.
            const int size = std::clamp(static_cast<int>(sourceDesc.Height) / 2, MIN_SKY_RESOLUTION, MAX_SKY_RESOLUTION);
            SkyBakeConstants constants{};
            constants.ground = slot.ground;
            bakeCubemap(slot, "Skybox cubemap", size, _equirectBakePipeline, _equirectBakeBinding, constants);
        });
    }

    CubemapHandle EnvironmentMaps::createProceduralSky(const int size, const SkyboxProceduralParameters &sky,
                                                      const float encoding)
    {
        if (!_skyBakePipeline) return {};

        // The light's forward is the direction sunlight travels, so the direction *to* the sun
        // is its opposite - and that is what both the model's elevation and the disc need.
        const auto toSun = Vector3Normalize(Vector3Negate(sky.sunDirection));
        const float elevation = std::asin(std::clamp(toSun.y, -1.0f, 1.0f));

        const auto ground = toSceneLinear(sky.groundAlbedo, encoding);
        const auto cooked = cookHosekWilkieSky(sky.turbidity, Vector3{ground.x, ground.y, ground.z}, elevation);

        const auto tint = toSceneLinear(sky.skyTint, encoding);
        const auto sunColor = toSceneLinear(sky.sunColor, encoding);
        const float sunScale = sky.sunIntensity * sky.sunEnergy;

        SkyBakeConstants constants{};
        for (size_t index = 0; index < std::size(constants.coefficients); ++index)
        {
            constants.coefficients[index] = {
                cooked.coefficients[0][index], cooked.coefficients[1][index], cooked.coefficients[2][index], 0.0f
            };
        }
        constants.radiance = {
            cooked.radiance[0] * SKY_RADIANCE_SCALE, cooked.radiance[1] * SKY_RADIANCE_SCALE,
            cooked.radiance[2] * SKY_RADIANCE_SCALE, 0.0f
        };
        constants.sun = {toSun.x, toSun.y, toSun.z, std::cos(std::max(sky.sunSize, 0.0f) * DEG2RAD)};
        constants.sunColor = {sunColor.x * sunScale, sunColor.y * sunScale, sunColor.z * sunScale, 0.0f};
        constants.tint = {tint.x, tint.y, tint.z, sky.skyEnergy};
        constants.ground = {ground.x, ground.y, ground.z, 0.0f};

        CubemapSlot slot;
        bakeCubemap(slot, "Procedural sky", size, _skyBakePipeline, _skyBakeBinding, constants);
        if (!slot.texture) return {};

        return _cubemaps.add(std::move(slot));
    }

    void EnvironmentMaps::bakeCubemap(CubemapSlot &slot, const char *name, const int size,
                                       Diligent::IPipelineState *pipeline,
                                       Diligent::IShaderResourceBinding *binding, SkyBakeConstants &constants)
    {
        Diligent::TextureDesc desc;
        desc.Name = name;
        desc.Type = Diligent::RESOURCE_DIM_TEX_CUBE;
        desc.Width = desc.Height = static_cast<Diligent::Uint32>(size);
        desc.ArraySize = CUBE_FACE_COUNT;
        // The whole chain: a blurred sky is a coarser mip of the same cube, and the image-based
        // lighting this feeds will want the chain too.
        desc.MipLevels = 0;
        desc.Format = SKY_FORMAT;
        desc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
        desc.MiscFlags = Diligent::MISC_TEXTURE_FLAG_GENERATE_MIPS;

        _device->CreateTexture(desc, nullptr, &slot.texture);
        if (!slot.texture)
        {
            Logger::LogError(std::string("Diligent failed to create the ") + name);
            return;
        }

        _context->SetPipelineState(pipeline);
        for (Diligent::Uint32 face = 0; face < CUBE_FACE_COUNT; ++face)
        {
            Diligent::TextureViewDesc faceDesc;
            faceDesc.Name = "Cube face target";
            faceDesc.ViewType = Diligent::TEXTURE_VIEW_RENDER_TARGET;
            faceDesc.FirstArraySlice = face;
            faceDesc.NumArraySlices = 1;
            Diligent::RefCntAutoPtr<Diligent::ITextureView> faceTarget;
            slot.texture->CreateView(faceDesc, &faceTarget);

            Diligent::ITextureView *target = faceTarget;
            _context->SetRenderTargets(1, &target, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            constants.faceRight = {CUBE_FACE_RIGHT[face].x, CUBE_FACE_RIGHT[face].y, CUBE_FACE_RIGHT[face].z, 0.0f};
            constants.faceUp = {CUBE_FACE_UP[face].x, CUBE_FACE_UP[face].y, CUBE_FACE_UP[face].z, 0.0f};
            constants.faceForward = {CUBE_FACE_FORWARD[face].x, CUBE_FACE_FORWARD[face].y, CUBE_FACE_FORWARD[face].z, 0.0f};
            uploadConstants(_context, _skyBakeConstants, &constants, sizeof(constants));

            _context->CommitShaderResources(binding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            Diligent::DrawAttribs drawAttribs;
            drawAttribs.NumVertices = 3;
            drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
            _context->Draw(drawAttribs);
        }

        auto *cubeView = slot.texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
        _context->GenerateMips(cubeView);

        Diligent::SamplerDesc samplerDesc;
        samplerDesc.MinFilter = samplerDesc.MagFilter = samplerDesc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
        samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
        Diligent::RefCntAutoPtr<Diligent::ISampler> sampler;
        _device->CreateSampler(samplerDesc, &sampler);
        cubeView->SetSampler(sampler);
    }

    void EnvironmentMaps::drawSkybox(const CameraView &camera, const Matrix &viewProjection)
    {
        if (!_skyboxPipeline) return;

        const auto *slot = _cubemaps.get(_sky);
        if (slot == nullptr || !slot->texture) return;

        // The inspector's rotation turns the sky; the shader turns the direction it is sampled
        // with, and those are opposites.
        const auto rotation = QuaternionInvert(_skyRotation);
        const auto mipCount = static_cast<float>(slot->texture->GetDesc().MipLevels);

        const SkyboxConstants constants{
            .inverseViewProjection = MatrixToFloatV(MatrixInvert(viewProjection)),
            .cameraPosition = {camera.position.x, camera.position.y, camera.position.z, 1.0f},
            .rotation = {rotation.x, rotation.y, rotation.z, rotation.w},
            .params = {_skyEnergy, std::clamp(_skyBlur, 0.0f, 1.0f) * std::max(mipCount - 1.0f, 0.0f), 0.0f, 0.0f}
        };
        uploadConstants(_context, _skyboxConstants, &constants, sizeof(constants));

        _context->SetPipelineState(_skyboxPipeline);
        _skyboxBinding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Sky")
                      ->Set(slot->texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        _context->CommitShaderResources(_skyboxBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Diligent::DrawAttribs drawAttribs;
        drawAttribs.NumVertices = 3;
        drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        _context->Draw(drawAttribs);
    }

    void EnvironmentMaps::destroyCubemap(const CubemapHandle handle)
    {
        auto *slot = _cubemaps.get(handle);
        if (slot == nullptr) return;

        // A job still writing into this slot cannot have it recycled underneath it, and waiting
        // for one here would put the multi-second freeze back into the one gesture the
        // asynchronous load exists for: picking a second image while the first is still
        // decoding. finalizeCubemaps frees it once the job lands.
        if (slot->decodeJob.valid())
        {
            slot->abandoned = true;
            return;
        }

        // The slot owns the texture, so clearing it releases it.
        _cubemaps.remove(handle);
    }

    AmbientMapHandle EnvironmentMaps::createAmbientMap(const CubemapHandle cubemap)
    {
        if (!_irradiancePipeline || !_prefilterPipeline) return {};

        const auto *source = _cubemaps.get(cubemap);
        if (source == nullptr || !source->texture) return {};

        auto *sourceView = source->texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
        _irradianceBinding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Environment")->Set(sourceView);
        _prefilterBinding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Environment")->Set(sourceView);

        // Both passes read the source's own dimensions to decide which of its mips a sample
        // comes from, which is what keeps a sun disc from arriving as a scatter of hot pixels.
        const auto &sourceDesc = source->texture->GetDesc();
        IblBakeConstants constants{};
        constants.filter = {
            0.0f, static_cast<float>(sourceDesc.Width), static_cast<float>(sourceDesc.MipLevels), IRRADIANCE_SAMPLE_COUNT
        };

        AmbientMapSlot slot;
        slot.irradiance = bakeIblCube("Irradiance cube", IRRADIANCE_CUBE_SIZE, 1,
                                      _irradiancePipeline, _irradianceBinding, constants);
        constants.filter.w = PREFILTERED_SAMPLE_COUNT;
        slot.prefiltered = bakeIblCube("Reflection cube", PREFILTERED_CUBE_SIZE, PREFILTERED_CUBE_MIPS,
                                       _prefilterPipeline, _prefilterBinding, constants);
        if (!slot.irradiance || !slot.prefiltered) return {};

        return _ambientMaps.add(std::move(slot));
    }

    Diligent::RefCntAutoPtr<Diligent::ITexture> EnvironmentMaps::bakeIblCube(
        const char *name, const int size, const Diligent::Uint32 mipCount, Diligent::IPipelineState *pipeline,
        Diligent::IShaderResourceBinding *binding, IblBakeConstants &constants)
    {
        Diligent::TextureDesc desc;
        desc.Name = name;
        desc.Type = Diligent::RESOURCE_DIM_TEX_CUBE;
        desc.Width = desc.Height = static_cast<Diligent::Uint32>(size);
        desc.ArraySize = CUBE_FACE_COUNT;
        // Exactly the levels this fills. Nothing generates the rest, and a level left unwritten
        // would be sampled as black wherever a roughness selected it.
        desc.MipLevels = mipCount;
        desc.Format = SKY_FORMAT;
        desc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;

        Diligent::RefCntAutoPtr<Diligent::ITexture> cube;
        _device->CreateTexture(desc, nullptr, &cube);
        if (!cube)
        {
            Logger::LogError(std::string("Diligent failed to create the ") + name);
            return {};
        }

        _context->SetPipelineState(pipeline);
        for (Diligent::Uint32 mip = 0; mip < mipCount; ++mip)
        {
            // Mip 0 stands for a mirror and the last for a fully rough surface, which is the
            // ramp the scene pass reverses when it picks a level from a roughness. A cube with
            // one level is the irradiance one, and its roughness means nothing.
            constants.filter.x = mipCount > 1 ? static_cast<float>(mip) / static_cast<float>(mipCount - 1) : 0.0f;

            for (Diligent::Uint32 face = 0; face < CUBE_FACE_COUNT; ++face)
            {
                Diligent::TextureViewDesc faceDesc;
                faceDesc.Name = "Cube face target";
                faceDesc.ViewType = Diligent::TEXTURE_VIEW_RENDER_TARGET;
                faceDesc.FirstArraySlice = face;
                faceDesc.NumArraySlices = 1;
                faceDesc.MostDetailedMip = mip;
                Diligent::RefCntAutoPtr<Diligent::ITextureView> faceTarget;
                cube->CreateView(faceDesc, &faceTarget);

                Diligent::ITextureView *target = faceTarget;
                _context->SetRenderTargets(1, &target, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                constants.faceRight = {CUBE_FACE_RIGHT[face].x, CUBE_FACE_RIGHT[face].y, CUBE_FACE_RIGHT[face].z, 0.0f};
                constants.faceUp = {CUBE_FACE_UP[face].x, CUBE_FACE_UP[face].y, CUBE_FACE_UP[face].z, 0.0f};
                constants.faceForward = {CUBE_FACE_FORWARD[face].x, CUBE_FACE_FORWARD[face].y, CUBE_FACE_FORWARD[face].z, 0.0f};
                uploadConstants(_context, _iblBakeConstants, &constants, sizeof(constants));

                _context->CommitShaderResources(binding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

                Diligent::DrawAttribs drawAttribs;
                drawAttribs.NumVertices = 3;
                drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
                _context->Draw(drawAttribs);
            }
        }

        // Trilinear, so a roughness between two levels reads between the two roughnesses they
        // were filtered for rather than snapping to the nearer one.
        Diligent::SamplerDesc samplerDesc;
        samplerDesc.MinFilter = samplerDesc.MagFilter = samplerDesc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
        samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
        Diligent::RefCntAutoPtr<Diligent::ISampler> sampler;
        _device->CreateSampler(samplerDesc, &sampler);
        cube->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sampler);

        return cube;
    }

    Diligent::ITextureView *EnvironmentMaps::irradianceView(const AmbientMapHandle map) const
    {
        const auto *ambient = _ambientMaps.get(map);
        auto *cube = ambient != nullptr ? ambient->irradiance.RawPtr() : _ambientFallback.RawPtr();
        return cube != nullptr ? cube->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE) : nullptr;
    }

    Diligent::ITextureView *EnvironmentMaps::prefilteredView(const AmbientMapHandle map) const
    {
        const auto *ambient = _ambientMaps.get(map);
        auto *cube = ambient != nullptr ? ambient->prefiltered.RawPtr() : _ambientFallback.RawPtr();
        return cube != nullptr ? cube->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE) : nullptr;
    }

    Diligent::ITextureView *EnvironmentMaps::brdfLutView() const
    {
        return _brdfLut ? _brdfLut->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE) : nullptr;
    }

    void EnvironmentMaps::destroyAmbientMap(const AmbientMapHandle handle)
    {
        // The slot owns both cubes, so clearing it releases them. A material binding still
        // holding one keeps it alive until the next draw notices the handle changed.
        _ambientMaps.remove(handle);
    }

    CubemapHandle DiligentRenderer::loadCubemap(const std::string &path, const SkyboxCubemapParameters &settings)
    {
        return _environment.loadCubemap(path, settings, _outputEncoding);
    }

    bool DiligentRenderer::isCubemapReady(const CubemapHandle handle) const
    {
        return _environment.isCubemapReady(handle);
    }

    CubemapHandle DiligentRenderer::createProceduralSky(const int size, const SkyboxProceduralParameters &sky)
    {
        return _environment.createProceduralSky(size, sky, _outputEncoding);
    }

    void DiligentRenderer::destroyCubemap(const CubemapHandle handle)
    {
        _environment.destroyCubemap(handle);
    }

    AmbientMapHandle DiligentRenderer::createAmbientMap(const CubemapHandle cubemap)
    {
        return _environment.createAmbientMap(cubemap);
    }

    void DiligentRenderer::destroyAmbientMap(const AmbientMapHandle handle)
    {
        _environment.destroyAmbientMap(handle);
    }
} // namespace BreadEngine
