#include "diligentRenderer.h"

// Before everything else: GLEW insists on being the first to declare the GL entry points.
#include <GL/glew.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

#include <EngineFactoryOpenGL.h>
#include <Sampler.h>
#include <Shader.h>
#include <ShaderSourceFactoryUtils.hpp>
#include <Utilities/interface/DiligentFXShaderSourceStreamFactory.hpp>

#include "logger.h"
#include "raymath.h"
#include "rlgl.h"

#include "utils/workerPool.h"

namespace BreadEngine {
    /// What raylib's own LoadRenderTexture stamps on a depth attachment; the field is unused
    /// for depth but DrawTexturePro-style paths still read it.
    constexpr int DEPTH_PIXEL_FORMAT = 19;

    /// The scene target's formats are fixed, so every pipeline that renders into it can be
    /// built against them once instead of being rebuilt when the target is resized.
    constexpr Diligent::TEXTURE_FORMAT SCENE_COLOR_FORMAT = Diligent::TEX_FORMAT_RGBA8_UNORM;
    constexpr Diligent::TEXTURE_FORMAT SCENE_DEPTH_FORMAT = Diligent::TEX_FORMAT_D32_FLOAT;

    /// Where the engine's shader sources sit relative to the executable.
    constexpr const char *SHADER_DIRECTORY = "shaders";

    constexpr Diligent::TEXTURE_FORMAT SHADOW_MAP_FORMAT = Diligent::TEX_FORMAT_D32_FLOAT;
    constexpr Diligent::Uint32 SHADOW_MAP_RESOLUTION = 2048;
    constexpr Diligent::Uint32 SHADOW_CASCADE_COUNT = 4;

    /// A spot covers one cone rather than the whole visible world, so it needs far less of a map
    /// than a cascade does.
    constexpr Diligent::Uint32 SPOT_SHADOW_RESOLUTION = 1024;

    /// Near plane of a spot's shadow projection. Deliberately a constant and not a fraction of
    /// the light's range: range is how far the light reaches and is routinely thousands of
    /// units, which would push the near plane past every caster in the scene. A float depth
    /// buffer spends most of its precision near the eye, so a small near plane costs nothing
    /// where the casters actually are.
    constexpr float SPOT_SHADOW_NEAR = 0.05f;

    /// Width of the filter kernel a light's shadowSoftness of 1 produces, in shadow map texels,
    /// and the ceiling on it. A spot's map is a fixed projection rather than a fitted cascade,
    /// so its softness is expressed in texels where the directional one is expressed in world
    /// units - and nothing grows the map to keep a wide kernel affordable, so it is capped
    /// instead. The cap matches the 9x9 the varying filter is written for.
    constexpr float SPOT_SHADOW_FILTER_TEXELS = 3.0f;
    constexpr float SPOT_SHADOW_MAX_FILTER_TEXELS = 9.0f;

    /// One face of an omni light's cube. Lower than a spot's map even though a face covers a
    /// wider angle: there are six of them per light, and four lights may cast at once.
    constexpr Diligent::Uint32 OMNI_SHADOW_RESOLUTION = 512;

    /// Near plane of every cube face's projection. A constant for the same reason the spot's is
    /// - see SPOT_SHADOW_NEAR - and a cube gives that mistake six chances to show up.
    constexpr float OMNI_SHADOW_NEAR = 0.05f;

    /// Radius of the tap disk a light's shadowSoftness of 1 produces, in face texels, and the
    /// ceiling on it. Five taps spread over a wide radius band rather than blur, so the cap is
    /// tighter than the spot's - that one grows its tap count with its kernel.
    constexpr float OMNI_SHADOW_FILTER_TEXELS = 1.5f;
    constexpr float OMNI_SHADOW_MAX_FILTER_TEXELS = 4.0f;

    /// How far off the surface a shadow lookup steps before comparing, in face texels, before
    /// the filter's own reach is added to it. Two covers the 2x2 the comparison sampler already
    /// filters across, which is the closest a lookup can come to its own surface.
    constexpr float OMNI_SHADOW_NORMAL_OFFSET_TEXELS = 2.0f;

    /// How far from the camera the cascades reach. The camera's far plane is rlgl's own cull
    /// distance and sits thousands of units out; fitting cascades to that would spend the whole
    /// shadow map on distance nothing is ever shadowed at.
    constexpr float SHADOW_DISTANCE = 60.0f;

    /// Width, in world units, of the penumbra a light's shadowSoftness of 1 produces. Kept
    /// small on purpose: DiligentFX grows a cascade until the filter fits in 9x9 texels, so a
    /// softness expressed in whole units would trade away most of the shadow map's resolution.
    constexpr float SHADOW_SOFTNESS_WORLD_SIZE = 0.05f;

    /// Shader variable names of the material texture slots, in MaterialDesc's own order.
    constexpr const char *MATERIAL_TEXTURE_NAMES[]{"g_Albedo", "g_Normal", "g_Orm", "g_Emission"};

    /**
     * Mirrors scene.vsh's cbuffers. Everything is a float4 or a float4x4 on purpose: those
     * are the only members whose std140 placement is the same as their placement here, so the
     * struct and the shader cannot drift apart over padding.
     */
    struct SceneFrameConstants
    {
        float16 viewProjection;
        Vector4 cameraPosition;
        /// xyz is the direction the camera looks in. The pixel shader projects onto it to get
        /// the camera-space depth the cascade selection compares against.
        Vector4 cameraForward;
        Vector4 ambientColor;
        Vector4 outputEncoding;
    };

    struct SceneDrawConstants
    {
        float16 model;
        float16 normalMatrix;
    };

    /// How many lights the pixel shader loops over. The shader is told this number rather than
    /// repeating it, so the array and the loop cannot disagree.
    constexpr size_t MAX_SCENE_LIGHTS = 32;

    /// One light as the shader reads it, with everything the pixel shader would otherwise have
    /// to derive per pixel folded in on the CPU.
    struct SceneLight
    {
        /// xyz is the world position, meaningless for a directional light; w is the LightType.
        Vector4 positionType;
        /// xyz is the direction the light travels, so a surface points back along it.
        Vector4 direction;
        /// rgb is the colour, w the intensity.
        Vector4 color;
        /// x is 1/range², y the cosine of the spot's half-angle, z the reciprocal of the
        /// cosine span its falloff covers, w whether the cascades were fitted to this light.
        Vector4 attenuation;
        /// x is the slice of the spot shadow array rendered for this light, y the cube of the
        /// omni one; either is -1 when this light has no map of that kind.
        Vector4 shadow;
    };

    struct SceneLightConstants
    {
        /// x is how many entries of the array are live, y and z how far along the spot and omni
        /// shadow arrays this frame filled.
        Vector4 count;
        SceneLight lights[MAX_SCENE_LIGHTS];
    };

    void DiligentRenderer::initialize(const int sceneWidth, const int sceneHeight)
    {
        // Window.hWnd is deliberately left null. A non-null handle sends the Win32 GL backend
        // down its "create our own context" path, which calls SetPixelFormat on the window's
        // HDC - Windows rejects a second pixel format on the same HDC, and raylib has already
        // set one. Null takes the attach path, which adopts wglGetCurrentContext() as-is.
        const Diligent::EngineGLCreateInfo createInfo;
        Diligent::GetEngineFactoryOpenGL()->AttachToActiveGLContext(createInfo, &_device, &_context);

        if (!_device || !_context)
        {
            Logger::LogError("Diligent failed to attach to the active OpenGL context");
            return;
        }

        // The attached context is raylib's, so its version is whatever GLFW negotiated rather
        // than anything this engine asked for - worth knowing when a feature is unavailable.
        const auto &apiVersion = _device->GetDeviceInfo().APIVersion;
        Logger::LogInfo("Diligent attached to OpenGL " + std::to_string(apiVersion.Major) + "." + std::to_string(apiVersion.Minor));

        createSceneTarget(sceneWidth, sceneHeight);
        // Before the scene pipeline: the cascade array is one of its static resources, so it
        // has to exist by the time that pipeline is built.
        createShadowMaps();
        createScenePipeline();
        createShadowPipeline();
    }

    void DiligentRenderer::shutdown()
    {
        releaseSceneTarget();

        _draws.clear();
        _materials.clear();
        _meshes.clear();
        // Cleared before the pool it points into.
        _visibleLights.clear();
        _lights.clear();
        // Every slot the pool is about to drop may still have a decode running into it, and
        // the future does not wait on its own.
        _textures.forEachAlive([](TextureSlot &slot)
        {
            if (slot.decodeJob.valid()) slot.decodeJob.get();
        });
        _textures.clear();
        _materialFallbacks = {};
        _scenePipeline.Release();
        _shadowBinding.Release();
        _shadowPipeline.Release();
        _shadowMap = {};
        _spotShadowSRV.Release();
        _spotShadowDSVs = {};
        _omniShadowSRV.Release();
        _omniShadowDSVs = {};
        _frameConstants.Release();
        _drawConstants.Release();
        _lightConstants.Release();
        _shadowConstants.Release();
        _shadowPassConstants.Release();

        _context.Release();
        _device.Release();
    }

    // --- frame ---

    void DiligentRenderer::createSceneTarget(const int width, const int height)
    {
        if (!_device || width <= 0 || height <= 0) return;
        if (_sceneColor &&
            _sceneColor->GetDesc().Width == static_cast<Diligent::Uint32>(width) &&
            _sceneColor->GetDesc().Height == static_cast<Diligent::Uint32>(height))
        {
            return;
        }

        releaseSceneTarget();

        Diligent::TextureDesc colorDesc;
        colorDesc.Name = "Scene color";
        colorDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        colorDesc.Width = static_cast<Diligent::Uint32>(width);
        colorDesc.Height = static_cast<Diligent::Uint32>(height);
        colorDesc.MipLevels = 1;
        colorDesc.Format = SCENE_COLOR_FORMAT;
        colorDesc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
        _device->CreateTexture(colorDesc, nullptr, &_sceneColor);

        Diligent::TextureDesc depthDesc = colorDesc;
        depthDesc.Name = "Scene depth";
        depthDesc.Format = SCENE_DEPTH_FORMAT;
        depthDesc.BindFlags = Diligent::BIND_DEPTH_STENCIL;
        _device->CreateTexture(depthDesc, nullptr, &_sceneDepth);

        if (!_sceneColor || !_sceneDepth)
        {
            Logger::LogError("Diligent failed to create the scene render target");
            return;
        }

        // The GL backend's native handle is the texture name itself, which is all raylib
        // needs to treat these as its own.
        _overlay.id = rlLoadFramebuffer();
        _overlay.texture = Texture2D{
            .id = static_cast<unsigned int>(_sceneColor->GetNativeHandle()),
            .width = width,
            .height = height,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        };
        _overlay.depth = Texture2D{
            .id = static_cast<unsigned int>(_sceneDepth->GetNativeHandle()),
            .width = width,
            .height = height,
            .mipmaps = 1,
            .format = DEPTH_PIXEL_FORMAT
        };

        rlFramebufferAttach(_overlay.id, _overlay.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(_overlay.id, _overlay.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferComplete(_overlay.id);
        rlDisableFramebuffer();
    }

    void DiligentRenderer::releaseSceneTarget()
    {
        if (_overlay.id != 0)
        {
            // rlUnloadFramebuffer deletes whatever texture it finds on the depth attachment,
            // and that one belongs to Diligent. Detaching first leaves it nothing to delete.
            rlFramebufferAttach(_overlay.id, 0, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
            rlUnloadFramebuffer(_overlay.id);
            _overlay = {};
        }

        _sceneColor.Release();
        _sceneDepth.Release();
    }

    void DiligentRenderer::createScenePipeline()
    {
        if (!_device) return;

        Diligent::BufferDesc constantsDesc;
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

        constantsDesc.Name = "Frame constants";
        constantsDesc.Size = sizeof(SceneFrameConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_frameConstants);
        constantsDesc.Name = "Draw constants";
        constantsDesc.Size = sizeof(SceneDrawConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_drawConstants);
        constantsDesc.Name = "Light constants";
        constantsDesc.Size = sizeof(SceneLightConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_lightConstants);
        constantsDesc.Name = "Shadow constants";
        constantsDesc.Size = sizeof(ShadowConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_shadowConstants);
        constantsDesc.Name = "Shadow pass constants";
        constantsDesc.Size = sizeof(Diligent::float4x4);
        _device->CreateBuffer(constantsDesc, nullptr, &_shadowPassConstants);

        Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> engineSources;
        const std::string shaderDirectory = std::string(GetApplicationDirectory()) + SHADER_DIRECTORY;
        Diligent::GetEngineFactoryOpenGL()->CreateDefaultShaderSourceStreamFactory(shaderDirectory.c_str(), &engineSources);
        // DiligentFX's .fxh files are compiled into the library rather than shipped next to the
        // executable, so an #include of one only resolves through its own factory.
        const auto shaderSources = Diligent::CreateCompoundShaderSourceFactory(
            {&Diligent::DiligentFXShaderSourceStreamFactory::GetInstance(), engineSources});

        const std::string maxLights = std::to_string(MAX_SCENE_LIGHTS);
        const std::string maxSpotShadows = std::to_string(MAX_SPOT_SHADOWS);
        const std::string maxOmniShadows = std::to_string(MAX_OMNI_SHADOWS);
        const std::string spotShadowResolution = std::to_string(SPOT_SHADOW_RESOLUTION);
        // PCF.fxh reads GL_SUPPORTED to avoid Texture2DArray.SampleCmpLevelZero, which has no
        // GLSL counterpart at all. Nothing defines it for us - an undefined macro is zero to the
        // preprocessor, which would silently select the branch that cannot be converted.
        const Diligent::ShaderMacro macros[]{
            {"MAX_SCENE_LIGHTS", maxLights.c_str()},
            {"MAX_SPOT_SHADOWS", maxSpotShadows.c_str()},
            {"MAX_OMNI_SHADOWS", maxOmniShadows.c_str()},
            {"SPOT_SHADOW_RESOLUTION", spotShadowResolution.c_str()},
            {"GL_SUPPORTED", _device->GetDeviceInfo().IsGLDevice() ? "1" : "0"}
        };

        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = shaderSources;
        shaderInfo.Macros = {macros, static_cast<Diligent::Uint32>(std::size(macros))};
        // Combined texture samplers - each Texture2D paired with a SamplerState named after
        // it plus "_sampler" - are what a GL device wants, and what DiligentFX asks for on one.

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        shaderInfo.Desc = {"Scene VS", Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.FilePath = "scene.vsh";
        _device->CreateShader(shaderInfo, &vertexShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
        shaderInfo.Desc = {"Scene PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "scene.psh";
        _device->CreateShader(shaderInfo, &pixelShader);

        if (!vertexShader || !pixelShader)
        {
            Logger::LogError("Diligent failed to compile the scene shaders from " + shaderDirectory);
            return;
        }

        // Slot, offset and stride are left to Diligent: one interleaved buffer in MeshVertex's
        // own order is the only layout the geometry side produces.
        constexpr Diligent::LayoutElement vertexLayout[]{
            {0, 0, 3, Diligent::VT_FLOAT32, Diligent::False}, // position
            {1, 0, 3, Diligent::VT_FLOAT32, Diligent::False}, // normal
            {2, 0, 2, Diligent::VT_FLOAT32, Diligent::False}, // uv
            {3, 0, 3, Diligent::VT_FLOAT32, Diligent::False}, // tangent
        };

        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
        pipelineInfo.PSODesc.Name = "Scene opaque";
        pipelineInfo.pVS = vertexShader;
        pipelineInfo.pPS = pixelShader;

        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = SCENE_COLOR_FORMAT;
        graphics.DSVFormat = SCENE_DEPTH_FORMAT;
        graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
        // Diligent defaults front faces to clockwise; the engine's geometry is wound the way
        // rlgl winds its own, so the overlay pass and the scene agree on which side is front.
        graphics.RasterizerDesc.FrontCounterClockwise = Diligent::True;
        graphics.DepthStencilDesc.DepthEnable = Diligent::True;
        graphics.InputLayout.LayoutElements = vertexLayout;
        graphics.InputLayout.NumElements = static_cast<Diligent::Uint32>(std::size(vertexLayout));

        // The material textures belong to the binding rather than to the pipeline, which is
        // what MUTABLE means here; everything else - the two constant buffers - is static and
        // stays bound for the pipeline's life.
        Diligent::ShaderResourceVariableDesc materialVariables[MATERIAL_TEXTURE_COUNT];
        for (size_t slot = 0; slot < MATERIAL_TEXTURE_COUNT; ++slot)
        {
            materialVariables[slot] = {Diligent::SHADER_TYPE_PIXEL, MATERIAL_TEXTURE_NAMES[slot],
                                       Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE};
        }
        pipelineInfo.PSODesc.ResourceLayout.Variables = materialVariables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(MATERIAL_TEXTURE_COUNT);

        _device->CreateGraphicsPipelineState(pipelineInfo, &_scenePipeline);
        if (!_scenePipeline)
        {
            Logger::LogError("Diligent failed to create the scene pipeline state");
            return;
        }

        // Both shaders read the frame block, and a static variable is per shader stage, so
        // binding it once for the vertex stage would leave the pixel stage's copy unset.
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "FrameConstants")->Set(_frameConstants);
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "FrameConstants")->Set(_frameConstants);
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "DrawConstants")->Set(_drawConstants);
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "LightConstants")->Set(_lightConstants);
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "ShadowConstants")->Set(_shadowConstants);
        // One cascade array for the whole scene, so it belongs to the pipeline rather than to
        // each material's binding.
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(_shadowMap.GetSRV());
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_SpotShadowMap")->Set(_spotShadowSRV);
        _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_OmniShadowMap")->Set(_omniShadowSRV);

        createMaterialFallbacks();
    }

    void DiligentRenderer::createShadowMaps()
    {
        if (!_device) return;

        // The scene pass samples the cascades through a comparison sampler, which is what turns
        // a fetch into "is this point in shadow" and lets the hardware filter the result.
        Diligent::SamplerDesc samplerDesc;
        samplerDesc.MinFilter = samplerDesc.MagFilter = samplerDesc.MipFilter = Diligent::FILTER_TYPE_COMPARISON_LINEAR;
        samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
        samplerDesc.ComparisonFunc = Diligent::COMPARISON_FUNC_LESS;
        Diligent::RefCntAutoPtr<Diligent::ISampler> comparisonSampler;
        _device->CreateSampler(samplerDesc, &comparisonSampler);

        Diligent::ShadowMapManager::InitInfo initInfo;
        initInfo.Format = SHADOW_MAP_FORMAT;
        initInfo.Resolution = SHADOW_MAP_RESOLUTION;
        initInfo.NumCascades = SHADOW_CASCADE_COUNT;
        // PCF needs nothing but the depth array, which is why the state cache the other modes
        // build their conversion pipelines through can be left null.
        initInfo.ShadowMode = SHADOW_MODE_PCF;
        initInfo.pComparisonSampler = comparisonSampler;
        _shadowMap.Initialize(_device, nullptr, initInfo);

        // Selects the world-space filter Shadows.fxh sizes per cascade, over the fixed 3x3 one.
        _shadowData.cascades.iFixedFilterSize = 0;

        createShadowArray("Spot shadow maps", Diligent::RESOURCE_DIM_TEX_2D_ARRAY, SPOT_SHADOW_RESOLUTION,
                          comparisonSampler, _spotShadowSRV, _spotShadowDSVs);
        // A cube array is indexed in layer-faces rather than in cubes, so it holds six slices
        // per light and a depth pass still writes exactly one of them.
        createShadowArray("Omni shadow maps", Diligent::RESOURCE_DIM_TEX_CUBE_ARRAY, OMNI_SHADOW_RESOLUTION,
                          comparisonSampler, _omniShadowSRV, _omniShadowDSVs);
    }

    void DiligentRenderer::createShadowArray(const char *name, const Diligent::RESOURCE_DIMENSION dimension,
                                             const Diligent::Uint32 resolution, Diligent::ISampler *comparisonSampler,
                                             Diligent::RefCntAutoPtr<Diligent::ITextureView> &srv,
                                             const std::span<Diligent::RefCntAutoPtr<Diligent::ITextureView>> sliceDSVs)
    {
        Diligent::TextureDesc arrayDesc;
        arrayDesc.Name = name;
        arrayDesc.Type = dimension;
        arrayDesc.Width = arrayDesc.Height = resolution;
        arrayDesc.MipLevels = 1;
        arrayDesc.ArraySize = static_cast<Diligent::Uint32>(sliceDSVs.size());
        arrayDesc.Format = SHADOW_MAP_FORMAT;
        arrayDesc.BindFlags = Diligent::BIND_SHADER_RESOURCE | Diligent::BIND_DEPTH_STENCIL;
        Diligent::RefCntAutoPtr<Diligent::ITexture> shadowMaps;
        _device->CreateTexture(arrayDesc, nullptr, &shadowMaps);
        if (!shadowMaps)
        {
            Logger::LogError(std::string("Diligent failed to create the ") + name);
            return;
        }

        srv = shadowMaps->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
        srv->SetSampler(comparisonSampler);
        // One view per slice: a depth pass writes a single light's map, not the whole array.
        for (Diligent::Uint32 slice = 0; slice < arrayDesc.ArraySize; ++slice)
        {
            Diligent::TextureViewDesc sliceDesc;
            sliceDesc.Name = "Shadow map slice";
            sliceDesc.ViewType = Diligent::TEXTURE_VIEW_DEPTH_STENCIL;
            sliceDesc.FirstArraySlice = slice;
            sliceDesc.NumArraySlices = 1;
            shadowMaps->CreateView(sliceDesc, &sliceDSVs[slice]);
        }
    }

    void DiligentRenderer::createShadowPipeline()
    {
        if (!_device) return;

        Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shaderSources;
        const std::string shaderDirectory = std::string(GetApplicationDirectory()) + SHADER_DIRECTORY;
        Diligent::GetEngineFactoryOpenGL()->CreateDefaultShaderSourceStreamFactory(shaderDirectory.c_str(), &shaderSources);

        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = shaderSources;
        shaderInfo.Desc = {"Shadow VS", Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.FilePath = "shadow.vsh";

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        _device->CreateShader(shaderInfo, &vertexShader);
        if (!vertexShader)
        {
            Logger::LogError("Diligent failed to compile the shadow shader from " + shaderDirectory);
            return;
        }

        // Position alone reaches the shadow map, but the buffer it comes from is still a
        // MeshVertex, so the stride has to be spelled out rather than inferred from one element.
        constexpr Diligent::LayoutElement vertexLayout[]{
            {0, 0, 3, Diligent::VT_FLOAT32, Diligent::False, 0, sizeof(MeshVertex)},
        };

        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
        pipelineInfo.PSODesc.Name = "Shadow cascade";
        pipelineInfo.pVS = vertexShader;

        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 0;
        graphics.DSVFormat = SHADOW_MAP_FORMAT;
        graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        // Nothing is culled: which winding faces the light depends on the handedness of the
        // light's own view basis, and a depth-only pass is cheap enough not to care.
        graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
        // A caster in front of the cascade's near plane still has to be recorded, so it is
        // clamped to the near plane instead of clipped away.
        graphics.RasterizerDesc.DepthClipEnable = Diligent::False;
        graphics.DepthStencilDesc.DepthEnable = Diligent::True;
        graphics.InputLayout.LayoutElements = vertexLayout;
        graphics.InputLayout.NumElements = static_cast<Diligent::Uint32>(std::size(vertexLayout));

        _device->CreateGraphicsPipelineState(pipelineInfo, &_shadowPipeline);
        if (!_shadowPipeline)
        {
            Logger::LogError("Diligent failed to create the shadow pipeline state");
            return;
        }

        _shadowPipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "ShadowPassConstants")->Set(_shadowPassConstants);
        _shadowPipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "DrawConstants")->Set(_drawConstants);
        _shadowPipeline->CreateShaderResourceBinding(&_shadowBinding, true);
    }

    void DiligentRenderer::restoreRaylibPixelStore()
    {
        // Diligent leaves GL_UNPACK_ROW_LENGTH at the stride of whatever it uploaded last, and
        // raylib sets only the alignment before its own uploads - it has always been able to
        // assume the default row length. Left dirty, raylib's next upload walks its source at
        // Diligent's stride: with a 1x1 texture behind us that is one pixel per row, which
        // reduces the editor's font atlas to nothing and makes every glyph render blank.
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    void DiligentRenderer::createMaterialFallbacks()
    {
        // In MATERIAL_TEXTURE_NAMES' order: white albedo, a flat tangent-space normal,
        // occlusion 1 / roughness 1 / metalness 0, and no emission.
        constexpr Diligent::Uint32 fallbackPixels[MATERIAL_TEXTURE_COUNT]{0xFFFFFFFF, 0xFFFF8080, 0xFF00FFFF, 0xFF000000};

        Diligent::TextureDesc desc;
        desc.Name = "Material fallback";
        desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
        desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

        for (size_t slot = 0; slot < MATERIAL_TEXTURE_COUNT; ++slot)
        {
            Diligent::TextureSubResData level{&fallbackPixels[slot], sizeof(Diligent::Uint32)};
            const Diligent::TextureData data{&level, 1};
            _device->CreateTexture(desc, &data, &_materialFallbacks[slot]);
        }

        restoreRaylibPixelStore();
    }

    void DiligentRenderer::resizeSceneTarget(const int width, const int height)
    {
        _hasExplicitTarget = true;
        createSceneTarget(width, height);
    }

    void DiligentRenderer::setOutputColorSpace(const OutputColorSpace colorSpace)
    {
        _outputEncoding = colorSpace == OutputColorSpace::Linear ? LINEAR_ENCODE_EXPONENT : GAMMA_ENCODE_EXPONENT;
    }

    void DiligentRenderer::beginScene(const CameraView &camera)
    {
        if (!_hasExplicitTarget) createSceneTarget(GetScreenWidth(), GetScreenHeight());

        _draws.clear();
        if (!_sceneColor) return;

        const auto &target = _sceneColor->GetDesc();
        const float aspect = static_cast<float>(target.Width) / static_cast<float>(target.Height);
        // The same clip range rlgl gives its own 3D passes. The editor's overlay draws into
        // this target's depth buffer through rlgl, so the two projections have to agree - and
        // that is also why the projection stays OpenGL's [-1, 1] depth convention.
        const auto nearPlane = static_cast<float>(rlGetCullDistanceNear());
        const auto farPlane = static_cast<float>(rlGetCullDistanceFar());

        Matrix projection;
        if (camera.projection == ProjectionType::Orthographic)
        {
            // Orthographic cameras spend fov as the visible height rather than an angle.
            const float halfHeight = camera.fov * 0.5f;
            projection = MatrixOrtho(-halfHeight * aspect, halfHeight * aspect, -halfHeight, halfHeight, nearPlane, farPlane);
        }
        else
        {
            projection = MatrixPerspective(camera.fov * DEG2RAD, aspect, nearPlane, farPlane);
        }

        _viewProjection = MatrixMultiply(MatrixLookAt(camera.position, camera.target, camera.up), projection);
        _camera = camera;
    }

    void DiligentRenderer::endScene()
    {
        if (!_sceneColor) return;

        // raylib has been drawing through the same context since the last frame ended, so
        // whatever Diligent remembers about the GL state it left behind is stale.
        _context->InvalidateState();

        // Ahead of the scene pass, which is the one that reads the result. A frame with no
        // directional caster leaves the cascade count at zero, which Shadows.fxh reads as
        // fully lit rather than as an error.
        selectVisibleLights(MAX_SCENE_LIGHTS);
        assignShadowSlots();
        renderShadowMaps();
        uploadConstants(_shadowConstants, &_shadowData, sizeof(_shadowData));

        // The pass is bound and cleared here rather than in beginScene because the engine
        // pushes the environment - and with it the background colour - from a start-frame
        // system that runs after beginScene has already returned.
        auto *renderTarget = _sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        auto *depthStencil = _sceneDepth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
        _context->SetRenderTargets(1, &renderTarget, depthStencil, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const auto clear = ColorNormalize(_clearColor);
        _context->ClearRenderTarget(renderTarget, &clear.x, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->ClearDepthStencil(depthStencil, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        submitDraws();

        yieldToRaylib();

        // Without a caller-sized target the scene is the frame, so it goes to the backbuffer
        // raylib is currently drawing into.
        if (!_hasExplicitTarget)
        {
            drawSceneTexture(Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())});
        }
    }

    void DiligentRenderer::uploadConstants(Diligent::IBuffer *buffer, const void *data, const size_t size)
    {
        // A mapped constant buffer is driver memory with nothing behind it, so writing past the
        // end corrupts whatever the driver keeps there and crashes somewhere else entirely,
        // frames later. Refusing the write turns that into one legible message.
        if (size > buffer->GetDesc().Size)
        {
            Logger::LogError(std::string("Constants for '") + buffer->GetDesc().Name + "' are larger than the buffer holding them");
            return;
        }

        void *mapped = nullptr;
        _context->MapBuffer(buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped);
        if (mapped == nullptr) return;

        std::memcpy(mapped, data, size);
        _context->UnmapBuffer(buffer, Diligent::MAP_WRITE);
    }

    void DiligentRenderer::submitDraws()
    {
        if (!_scenePipeline || _draws.empty()) return;

        const auto ambient = ColorNormalize(_ambientColor);
        const auto forward = Vector3Normalize(Vector3Subtract(_camera.target, _camera.position));
        const SceneFrameConstants frame{
            .viewProjection = MatrixToFloatV(_viewProjection),
            .cameraPosition = {_camera.position.x, _camera.position.y, _camera.position.z, 1.0f},
            .cameraForward = {forward.x, forward.y, forward.z, 0.0f},
            .ambientColor = {ambient.x, ambient.y, ambient.z, _ambientEnergy},
            .outputEncoding = {_outputEncoding, 0.0f, 0.0f, 0.0f}
        };
        uploadConstants(_frameConstants, &frame, sizeof(frame));
        uploadLights();

        _context->SetPipelineState(_scenePipeline);

        for (const auto &[mesh, material, model, castShadows]: _draws)
        {
            const auto *slot = _meshes.get(mesh);
            const auto *binding = _materials.get(material);
            if (slot == nullptr || binding == nullptr) continue;

            const SceneDrawConstants draw{
                .model = MatrixToFloatV(model),
                // Inverse transpose, so a non-uniform scale tilts the surface without taking
                // its normals off it.
                .normalMatrix = MatrixToFloatV(MatrixTranspose(MatrixInvert(model)))
            };
            uploadConstants(_drawConstants, &draw, sizeof(draw));

            Diligent::IBuffer *vertices = slot->vertices;
            constexpr Diligent::Uint64 vertexOffset = 0;
            _context->SetVertexBuffers(0, 1, &vertices, &vertexOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                       Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
            _context->SetIndexBuffer(slot->indices, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            // Committed after the draw constants were remapped, so the draw reads this
            // iteration's values and not the ones the previous one left bound.
            _context->CommitShaderResources(*binding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            Diligent::DrawIndexedAttribs drawAttribs;
            drawAttribs.IndexType = Diligent::VT_UINT32;
            drawAttribs.NumIndices = slot->indexCount;
            drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
            _context->DrawIndexed(drawAttribs);
        }
    }

    void DiligentRenderer::yieldToRaylib()
    {
        _context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->InvalidateState();
        _context->Flush();

        // rlgl tracks the GL state it expects in software and only issues the calls it thinks
        // are needed, so every piece of state the scene pipeline sets differently has to be put
        // back by hand. Culling and depth writes already match rlgl's own defaults; blending
        // and the depth test do not.
        rlEnableColorBlend();
        rlDisableDepthTest();

        // Diligent binds a sampler object per texture unit; rlgl uses none and relies on each
        // texture's own parameters. A sampler left bound overrides those, and its mipmapped
        // minification filter makes raylib's single-level textures incomplete - which samples
        // as opaque black, taking the whole editor UI with it.
        for (Diligent::Uint32 unit = 0; unit < MATERIAL_TEXTURE_COUNT; ++unit) glBindSampler(unit, 0);

        // Diligent enables sRGB framebuffer conversion once at device creation and leaves it
        // on. It is a no-op for its own non-sRGB targets, but raylib's colours are already
        // encoded, so leaving it on would gamma them a second time on the way to the window.
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    void DiligentRenderer::beginSceneOverlay()
    {
        if (_overlay.id == 0) return;

        BeginTextureMode(_overlay);
    }

    void DiligentRenderer::endSceneOverlay()
    {
        if (_overlay.id == 0) return;

        EndTextureMode();
    }

    void DiligentRenderer::drawSceneTexture(const Rectangle destination)
    {
        if (_overlay.id == 0) return;

        const auto width = static_cast<float>(_overlay.texture.width);
        const auto height = static_cast<float>(_overlay.texture.height);
        DrawTexturePro(_overlay.texture, Rectangle{0, 0, width, -height}, destination, Vector2{0, 0}, 0, WHITE);
    }

    // --- lights ---

    LightHandle DiligentRenderer::createLight(const LightType type)
    {
        return _lights.add(LightState{.type = type});
    }

    void DiligentRenderer::destroyLight(const LightHandle handle)
    {
        _lights.remove(handle);
    }

    bool DiligentRenderer::isLightValid(const LightHandle handle) const
    {
        return _lights.get(handle) != nullptr;
    }

    void DiligentRenderer::updateLight(const LightHandle handle, const LightState &state)
    {
        // Nothing is applied here: a light only exists as constants the scene pass reads, so
        // there is no GPU state to diff the incoming values against.
        if (auto *light = _lights.get(handle)) *light = state;
    }

    void DiligentRenderer::assignShadowSlots()
    {
        _shadowData.cascades.iNumCascades = 0;

        bool cascadesTaken = false;
        int nextSpotSlice = 0;
        int nextOmniSlice = 0;
        for (auto &visible: _visibleLights)
        {
            visible.spotShadowSlice = -1;
            visible.omniShadowSlice = -1;
            visible.ownsCascades = false;
            if (!visible.light->castShadows) continue;

            if (visible.light->type == LightType::Directional && !cascadesTaken)
            {
                visible.ownsCascades = true;
                cascadesTaken = true;
            }
            else if (visible.light->type == LightType::Spot && nextSpotSlice < static_cast<int>(MAX_SPOT_SHADOWS))
            {
                visible.spotShadowSlice = nextSpotSlice++;
            }
            else if (visible.light->type == LightType::Omni && nextOmniSlice < static_cast<int>(MAX_OMNI_SHADOWS))
            {
                visible.omniShadowSlice = nextOmniSlice++;
            }
        }
    }

    void DiligentRenderer::renderShadowCasters(Diligent::ITextureView *target, const float16 &worldToLightClip)
    {
        _context->SetRenderTargets(0, nullptr, target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->ClearDepthStencil(target, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        uploadConstants(_shadowPassConstants, worldToLightClip.v, sizeof(worldToLightClip));

        for (const auto &[mesh, material, model, castShadows]: _draws)
        {
            if (!castShadows) continue;

            const auto *slot = _meshes.get(mesh);
            if (slot == nullptr) continue;

            const SceneDrawConstants draw{.model = MatrixToFloatV(model), .normalMatrix = {}};
            uploadConstants(_drawConstants, &draw, sizeof(draw));

            Diligent::IBuffer *vertices = slot->vertices;
            constexpr Diligent::Uint64 vertexOffset = 0;
            _context->SetVertexBuffers(0, 1, &vertices, &vertexOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                       Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
            _context->SetIndexBuffer(slot->indices, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            _context->CommitShaderResources(_shadowBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            Diligent::DrawIndexedAttribs drawAttribs;
            drawAttribs.IndexType = Diligent::VT_UINT32;
            drawAttribs.NumIndices = slot->indexCount;
            drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
            _context->DrawIndexed(drawAttribs);
        }
    }

    void DiligentRenderer::renderShadowMaps()
    {
        if (!_shadowPipeline || !_sceneColor) return;

        _context->SetPipelineState(_shadowPipeline);
        for (const auto &[light, spotShadowSlice, omniShadowSlice, ownsCascades]: _visibleLights)
        {
            if (ownsCascades) renderCascades(*light);
            else if (spotShadowSlice >= 0) renderSpotShadow(*light, spotShadowSlice);
            else if (omniShadowSlice >= 0) renderOmniShadow(*light, omniShadowSlice);
        }
    }

    void DiligentRenderer::renderSpotShadow(const LightState &light, const int slice)
    {
        // A spot's cone is inscribed in a square perspective frustum of the same full angle, so
        // one map at the cone's own field of view covers it exactly.
        const auto target = Vector3Add(light.position, light.direction);
        // Any up vector will do except one along the cone's axis, which leaves the basis
        // degenerate; a light pointing straight down is the common case, not an edge one.
        const Vector3 up = std::abs(light.direction.y) > 0.99f ? Vector3{0.0f, 0.0f, 1.0f} : Vector3{0.0f, 1.0f, 0.0f};
        const auto view = MatrixLookAt(light.position, target, up);
        const auto projection = MatrixPerspective(light.spotAngle * DEG2RAD, 1.0f, SPOT_SHADOW_NEAR, std::max(light.range, SPOT_SHADOW_NEAR * 2.0f));

        const auto worldToLightClip = MatrixToFloatV(MatrixMultiply(view, projection));
        // Copied, not transposed: what the engine uploads and what Diligent stores are the same
        // sixteen floats for the same transform, which is what lets both stay mul(matrix, vector).
        std::memcpy(&_shadowData.spotTransforms[slice], worldToLightClip.v, sizeof(Diligent::float4x4));
        const auto filterTexels = std::min(light.shadowSoftness * SPOT_SHADOW_FILTER_TEXELS, SPOT_SHADOW_MAX_FILTER_TEXELS);
        _shadowData.spotParams[slice].x = filterTexels / static_cast<float>(SPOT_SHADOW_RESOLUTION);
        renderShadowCasters(_spotShadowDSVs[slice], worldToLightClip);
    }

    void DiligentRenderer::renderOmniShadow(const LightState &light, const int slice)
    {
        // The direction each face looks in, and the up vector that orients its image the way
        // the hardware's own direction-to-texel rule will read it back. Both tables are in the
        // cube's face order, and neither is free to be reordered or re-derived: the up vectors
        // are not the intuitive ones, and a wrong one flips a face without failing anywhere.
        static constexpr Vector3 faceDirections[CUBE_FACE_COUNT]{
            {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}
        };
        static constexpr Vector3 faceUps[CUBE_FACE_COUNT]{
            {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
            {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}
        };

        // Six 90 degree square frustums meet edge to edge, which is what makes them a cube.
        const float farPlane = std::max(light.range, OMNI_SHADOW_NEAR * 2.0f);
        const auto projection = MatrixPerspective(90.0f * DEG2RAD, 1.0f, OMNI_SHADOW_NEAR, farPlane);

        // What the scene pass needs to rebuild the depth these passes write, without a matrix
        // and without knowing which face a lookup lands on: the depth a perspective projection
        // leaves for a point at distance d along the face axis is scale - scale * near / d.
        const float depthScale = farPlane / (farPlane - OMNI_SHADOW_NEAR);
        _shadowData.omniPosition[slice] = {light.position.x, light.position.y, light.position.z, depthScale};
        const float filterTexels = std::min(light.shadowSoftness * OMNI_SHADOW_FILTER_TEXELS, OMNI_SHADOW_MAX_FILTER_TEXELS);
        // A face spans twice the distance to the surface across its full width, so one texel is
        // that much of it divided by the resolution - which is what turns both the filter's
        // reach and the offset a lookup leaves the surface by from texels into world units, at
        // whatever distance they end up being applied.
        constexpr float texelsToWorld = 2.0f / static_cast<float>(OMNI_SHADOW_RESOLUTION);
        _shadowData.omniParams[slice] = {
            depthScale * OMNI_SHADOW_NEAR,
            filterTexels * texelsToWorld,
            (OMNI_SHADOW_NORMAL_OFFSET_TEXELS + filterTexels) * texelsToWorld,
            0.0f
        };

        for (size_t face = 0; face < CUBE_FACE_COUNT; ++face)
        {
            const auto target = Vector3Add(light.position, faceDirections[face]);
            const auto view = MatrixLookAt(light.position, target, faceUps[face]);
            renderShadowCasters(_omniShadowDSVs[slice * CUBE_FACE_COUNT + face],
                                MatrixToFloatV(MatrixMultiply(view, projection)));
        }
    }

    void DiligentRenderer::renderCascades(const LightState &light)
    {
        // Fitting the cascades is DiligentFX's, and its camera space runs +Z forward where the
        // engine's runs -Z. So the camera is handed over as a left-handed basis built here
        // rather than as the engine's own view matrix, and the pixel shader gets its
        // camera-space depth by projecting onto the forward axis rather than from that matrix.
        const auto forward = Vector3Normalize(Vector3Subtract(_camera.target, _camera.position));
        const auto right = Vector3Normalize(Vector3CrossProduct(_camera.up, forward));
        const auto up = Vector3CrossProduct(forward, right);
        const Diligent::float4x4 cameraWorld{
            right.x, right.y, right.z, 0.0f,
            up.x, up.y, up.z, 0.0f,
            forward.x, forward.y, forward.z, 0.0f,
            _camera.position.x, _camera.position.y, _camera.position.z, 1.0f
        };
        const auto cameraView = cameraWorld.Inverse();

        const auto &target = _sceneColor->GetDesc();
        const float aspect = static_cast<float>(target.Width) / static_cast<float>(target.Height);
        const auto nearPlane = static_cast<float>(rlGetCullDistanceNear());
        const auto farPlane = static_cast<float>(rlGetCullDistanceFar());
        const bool isGL = _device->GetDeviceInfo().IsGLDevice();
        const auto cameraProjection = _camera.projection == ProjectionType::Orthographic
                                          ? Diligent::float4x4::Ortho(_camera.fov * aspect, _camera.fov, nearPlane, farPlane, isGL)
                                          : Diligent::float4x4::Projection(_camera.fov * DEG2RAD, aspect, nearPlane, farPlane, isGL);

        const Diligent::float3 lightDirection{light.direction.x, light.direction.y, light.direction.z};
        _shadowData.cascades.fFilterWorldSize = light.shadowSoftness * SHADOW_SOFTNESS_WORLD_SIZE;

        Diligent::ShadowMapManager::DistributeCascadeInfo cascadeInfo;
        cascadeInfo.pCameraView = &cameraView;
        cascadeInfo.pCameraWorld = &cameraWorld;
        cascadeInfo.pCameraProj = &cameraProjection;
        cascadeInfo.pLightDir = &lightDirection;
        // Row-major packing writes the matrices in the layout the engine's own shaders already
        // read - the one MatrixToFloatV produces - so they stay usable as mul(matrix, vector).
        cascadeInfo.PackMatrixRowMajor = true;
        cascadeInfo.AdjustCascadeRange = [](const int cascade, float &minZ, float &maxZ)
        {
            if (cascade < 0) maxZ = std::min(maxZ, SHADOW_DISTANCE);
        };
        _shadowMap.DistributeCascades(cascadeInfo, _shadowData.cascades);

        for (Diligent::Uint32 cascade = 0; cascade < SHADOW_CASCADE_COUNT; ++cascade)
        {
            // Copied rather than transposed: PackMatrixRowMajor above already left it in the
            // layout the shaders read.
            float16 worldToLightClip;
            std::memcpy(worldToLightClip.v, &_shadowMap.GetCascadeTransform(cascade).WorldToLightProjSpace, sizeof(worldToLightClip));
            renderShadowCasters(_shadowMap.GetCascadeDSV(cascade), worldToLightClip);
        }
    }

    void DiligentRenderer::selectVisibleLights(const size_t capacity)
    {
        _visibleLights.clear();
        _lights.forEachAlive([this](const LightState &light)
        {
            if (light.active) _visibleLights.push_back(VisibleLight{.light = &light});
        });

        if (_visibleLights.size() <= capacity) return;

        // Only reached once a scene has more lights than the buffer holds, and only then does
        // the order decide anything. A directional light lights everything, so it outranks any
        // punctual one; the rest go by how far the camera is from being inside their reach.
        const auto reach = [this](const LightState *light)
        {
            return Vector3Distance(_camera.position, light->position) - light->range;
        };
        std::ranges::sort(_visibleLights, [&reach](const VisibleLight &left, const VisibleLight &right)
        {
            const bool leftIsDirectional = left.light->type == LightType::Directional;
            if (leftIsDirectional != (right.light->type == LightType::Directional)) return leftIsDirectional;

            return reach(left.light) < reach(right.light);
        });
        _visibleLights.resize(capacity);
    }

    void DiligentRenderer::uploadLights()
    {
        SceneLightConstants constants{.count = {static_cast<float>(_visibleLights.size()), 0.0f, 0.0f, 0.0f}};
        int spotShadowCount = 0;
        int omniShadowCount = 0;
        for (size_t index = 0; index < _visibleLights.size(); ++index)
        {
            const auto &visible = _visibleLights[index];
            const auto &light = *visible.light;
            const auto color = ColorNormalize(light.color);
            // Both cosines are of the half-angle, since what the shader compares them against
            // is the angle between the cone's axis and the direction the surface lies in.
            const float outerCosine = std::cos(light.spotAngle * 0.5f * DEG2RAD);
            const float innerCosine = std::cos(light.spotAngle * (1.0f - light.spotBlend) * 0.5f * DEG2RAD);

            constants.lights[index] = SceneLight{
                .positionType = {light.position.x, light.position.y, light.position.z, static_cast<float>(light.type)},
                .direction = {light.direction.x, light.direction.y, light.direction.z, 0.0f},
                .color = {color.x, color.y, color.z, light.intensity},
                .attenuation = {
                    1.0f / std::max(light.range * light.range, 1e-4f),
                    outerCosine,
                    1.0f / std::max(innerCosine - outerCosine, 1e-4f),
                    // The cascades are fitted to exactly one light, so exactly one light in the
                    // array may read them.
                    visible.ownsCascades ? 1.0f : 0.0f
                },
                .shadow = {
                    static_cast<float>(visible.spotShadowSlice), static_cast<float>(visible.omniShadowSlice),
                    0.0f, 0.0f
                }
            };
            spotShadowCount = std::max(spotShadowCount, visible.spotShadowSlice + 1);
            omniShadowCount = std::max(omniShadowCount, visible.omniShadowSlice + 1);
        }

        // Slices past these hold whatever the last frame that used them left behind, so the
        // shader is told how far along each array is live rather than sampling all of it.
        constants.count.y = static_cast<float>(spotShadowCount);
        constants.count.z = static_cast<float>(omniShadowCount);
        uploadConstants(_lightConstants, &constants, sizeof(constants));
    }

    // --- textures ---

    Diligent::SamplerDesc DiligentRenderer::toNative(const TextureFilterMode filter, const TextureWrapMode wrap)
    {
        Diligent::SamplerDesc desc;
        desc.AddressU = desc.AddressV = desc.AddressW = toNative(wrap);

        switch (filter)
        {
            case TextureFilterMode::Point:
                desc.MinFilter = desc.MagFilter = desc.MipFilter = Diligent::FILTER_TYPE_POINT;
                break;
            case TextureFilterMode::Bilinear:
                desc.MinFilter = desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
                desc.MipFilter = Diligent::FILTER_TYPE_POINT;
                break;
            case TextureFilterMode::Anisotropic4x:
            case TextureFilterMode::Anisotropic8x:
            case TextureFilterMode::Anisotropic16x:
                desc.MinFilter = desc.MagFilter = desc.MipFilter = Diligent::FILTER_TYPE_ANISOTROPIC;
                desc.MaxAnisotropy = filter == TextureFilterMode::Anisotropic4x ? 4 : (filter == TextureFilterMode::Anisotropic8x ? 8 : 16);
                break;
            case TextureFilterMode::Trilinear:
            default:
                desc.MinFilter = desc.MagFilter = desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
                break;
        }

        return desc;
    }

    Diligent::TEXTURE_ADDRESS_MODE DiligentRenderer::toNative(const TextureWrapMode wrap)
    {
        switch (wrap)
        {
            case TextureWrapMode::Clamp: return Diligent::TEXTURE_ADDRESS_CLAMP;
            case TextureWrapMode::MirrorRepeat: return Diligent::TEXTURE_ADDRESS_MIRROR;
            case TextureWrapMode::MirrorClamp: return Diligent::TEXTURE_ADDRESS_MIRROR_ONCE;
            case TextureWrapMode::Repeat:
            default: return Diligent::TEXTURE_ADDRESS_WRAP;
        }
    }

    void DiligentRenderer::finalizeTexture(TextureSlot &slot)
    {
        if (slot.uploaded) return;
        if (slot.decodeJob.valid()) slot.decodeJob.get();

        slot.uploaded = true;
        if (!slot.loader) return;

        slot.loader->CreateTexture(_device, &slot.texture);
        restoreRaylibPixelStore();
        // The decoded pixels live in the loader, and the texture now owns its own copy.
        slot.loader.Release();
        if (!slot.texture) return;

        // A combined-sampler pipeline takes the sampler from the view, so this is where a
        // texture's own filter and wrap actually reach the GPU. CreateSampler de-duplicates
        // identical descriptions internally, so there is nothing to cache here.
        Diligent::RefCntAutoPtr<Diligent::ISampler> sampler;
        _device->CreateSampler(toNative(slot.desc.filter, slot.desc.wrap), &sampler);
        slot.texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sampler);
    }

    TextureHandle DiligentRenderer::createTexture(const TextureDesc &desc)
    {
        if (!_device) return {};

        const auto handle = _textures.add(TextureSlot{.desc = desc});
        auto *slot = _textures.get(handle);

        // Decoding needs no device, so it runs off the render thread; only the upload in
        // finalizeTexture does. Pool slots keep a stable address, so the job captures one.
        slot->decodeJob = WorkerPool::submit([slot]
        {
            Diligent::TextureLoadInfo loadInfo;
            loadInfo.Name = "Scene texture";
            loadInfo.IsSRGB = slot->desc.isColor;
            Diligent::CreateTextureLoaderFromFile(slot->desc.path.c_str(), Diligent::IMAGE_FILE_FORMAT_UNKNOWN, loadInfo, &slot->loader);
        });

        return handle;
    }

    void DiligentRenderer::destroyTexture(const TextureHandle handle)
    {
        auto *slot = _textures.get(handle);
        if (slot == nullptr) return;

        if (slot->decodeJob.valid()) slot->decodeJob.get();
        _textures.remove(handle);
    }

    TextureSize DiligentRenderer::getTextureSize(const TextureHandle handle)
    {
        auto *slot = _textures.get(handle);
        if (slot == nullptr) return {};

        finalizeTexture(*slot);
        if (!slot->texture) return {};

        const auto &desc = slot->texture->GetDesc();
        return {.width = static_cast<int>(desc.Width), .height = static_cast<int>(desc.Height)};
    }

    // --- materials ---

    Diligent::ITextureView *DiligentRenderer::materialTextureView(const TextureHandle handle, Diligent::ITexture *fallback)
    {
        Diligent::ITexture *texture = fallback;
        if (auto *entry = _textures.get(handle))
        {
            finalizeTexture(*entry);
            if (entry->texture) texture = entry->texture;
        }

        return texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
    }

    MaterialHandle DiligentRenderer::createMaterial(const MaterialDesc &desc)
    {
        if (!_scenePipeline) return {};

        MaterialSlot binding;
        _scenePipeline->CreateShaderResourceBinding(&binding, true);
        if (!binding)
        {
            Logger::LogError("Diligent failed to create a material's resource binding");
            return {};
        }

        const TextureHandle handles[MATERIAL_TEXTURE_COUNT]{desc.albedo, desc.normal, desc.orm, desc.emission};
        for (size_t slot = 0; slot < MATERIAL_TEXTURE_COUNT; ++slot)
        {
            // Null when the shader does not sample this slot - the sources are compiled from
            // disk at runtime, so which variables exist is not fixed at build time.
            auto *variable = binding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, MATERIAL_TEXTURE_NAMES[slot]);
            if (variable == nullptr) continue;

            variable->Set(materialTextureView(handles[slot], _materialFallbacks[slot]));
        }

        return _materials.add(std::move(binding));
    }

    void DiligentRenderer::destroyMaterial(const MaterialHandle handle)
    {
        // The slot owns the binding, so clearing it releases it.
        _materials.remove(handle);
    }

    // --- meshes ---

    MeshHandle DiligentRenderer::createMesh(const MeshData &geometry)
    {
        if (!_device) return {};
        if (geometry.isEmpty()) return {};

        MeshSlot slot;
        slot.indexCount = static_cast<Diligent::Uint32>(geometry.indices.size());

        Diligent::BufferDesc bufferDesc;
        bufferDesc.Usage = Diligent::USAGE_IMMUTABLE;

        bufferDesc.Name = "Mesh vertices";
        bufferDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
        bufferDesc.Size = geometry.vertices.size() * sizeof(MeshVertex);
        const Diligent::BufferData vertexData{geometry.vertices.data(), bufferDesc.Size};
        _device->CreateBuffer(bufferDesc, &vertexData, &slot.vertices);

        bufferDesc.Name = "Mesh indices";
        bufferDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
        bufferDesc.Size = geometry.indices.size() * sizeof(uint32_t);
        const Diligent::BufferData indexData{geometry.indices.data(), bufferDesc.Size};
        _device->CreateBuffer(bufferDesc, &indexData, &slot.indices);

        if (!slot.vertices || !slot.indices)
        {
            Logger::LogError("Diligent failed to upload a mesh's geometry");
            return {};
        }

        return _meshes.add(std::move(slot));
    }

    void DiligentRenderer::destroyMesh(const MeshHandle handle)
    {
        // The slot owns its buffers, so clearing it releases them.
        _meshes.remove(handle);
    }

    void DiligentRenderer::drawMesh(const MeshDrawDesc &draw)
    {
        if (_meshes.get(draw.mesh) == nullptr) return;

        const Matrix model = MatrixMultiply(MatrixMultiply(MatrixScale(draw.scale.x, draw.scale.y, draw.scale.z),
                                                           QuaternionToMatrix(draw.rotation)),
                                            MatrixTranslate(draw.position.x, draw.position.y, draw.position.z));
        _draws.push_back(DrawItem{.mesh = draw.mesh, .material = draw.material, .model = model, .castShadows = draw.castShadows});
    }

    // --- environment ---

    void DiligentRenderer::setEnvironment(const EnvironmentSettings &settings)
    {
        _clearColor = settings.background.color;
        _ambientColor = settings.ambient.color;
        _ambientEnergy = settings.ambient.energy;
    }

    CubemapHandle DiligentRenderer::loadCubemap(const std::string &path)
    {
        return {};
    }

    CubemapHandle DiligentRenderer::createProceduralSky(const int size, const SkyboxProceduralParameters &sky)
    {
        return {};
    }

    void DiligentRenderer::destroyCubemap(const CubemapHandle handle)
    {
    }

    AmbientMapHandle DiligentRenderer::createAmbientMap(const CubemapHandle cubemap)
    {
        return {};
    }

    void DiligentRenderer::destroyAmbientMap(const AmbientMapHandle handle)
    {
    }
} // namespace BreadEngine
