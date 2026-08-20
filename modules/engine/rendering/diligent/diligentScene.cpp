#include "diligentInternal.h"

#include <algorithm>
#include <chrono>
#include <cstring>

#include <Sampler.h>
#include <Shader.h>

#include "logger.h"
#include "utils/workerPool.h"

namespace BreadEngine {
    /// Shader variable names of the material texture slots, in MaterialDesc's own order.
    constexpr const char *MATERIAL_TEXTURE_NAMES[]{"g_Albedo", "g_Normal", "g_Orm", "g_Emission"};

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

        const auto shaderSources = createShaderSources();

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
            Logger::LogError("Diligent failed to compile the scene shaders");
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
        // what MUTABLE means here. The two environment cubes live on the binding as well but
        // are DYNAMIC, because a rebaked sky replaces them and neither of the other two kinds
        // can be re-set; everything left - the constant buffers, the shadow arrays and the BRDF
        // table - is static and stays bound for the pipeline's life.
        Diligent::ShaderResourceVariableDesc pixelVariables[MATERIAL_TEXTURE_COUNT + 2];
        for (size_t slot = 0; slot < MATERIAL_TEXTURE_COUNT; ++slot)
        {
            pixelVariables[slot] = {Diligent::SHADER_TYPE_PIXEL, MATERIAL_TEXTURE_NAMES[slot],
                                    Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE};
        }
        pixelVariables[MATERIAL_TEXTURE_COUNT] = {Diligent::SHADER_TYPE_PIXEL, "g_Irradiance",
                                                  Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};
        pixelVariables[MATERIAL_TEXTURE_COUNT + 1] = {Diligent::SHADER_TYPE_PIXEL, "g_Prefiltered",
                                                      Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};
        pipelineInfo.PSODesc.ResourceLayout.Variables = pixelVariables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(std::size(pixelVariables));

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
        // The BRDF table depends on nothing but the shading model, so it never changes and
        // belongs to the pipeline rather than to any one environment. Guarded because it is
        // built by a pass of its own, and a renderer that failed to build it should say so
        // rather than take the process down here.
        if (_brdfLut)
        {
            _scenePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_BrdfLut")
                          ->Set(_brdfLut->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        }

        createMaterialFallbacks();
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

    void DiligentRenderer::submitDraws()
    {
        if (!_scenePipeline || _draws.empty()) return;

        const auto ambient = ColorNormalize(_ambientColor);
        const auto forward = Vector3Normalize(Vector3Subtract(_camera.target, _camera.position));
        // The same inverse the background pass turns its view ray by, so a reflection lands
        // where the sky it reflects is drawn.
        const auto skyRotation = QuaternionInvert(_skyRotation);
        const bool hasAmbientMap = _ambientMaps.get(_ambientMap) != nullptr;
        const SceneFrameConstants frame{
            .viewProjection = MatrixToFloatV(_viewProjection),
            .cameraPosition = {_camera.position.x, _camera.position.y, _camera.position.z, 1.0f},
            .cameraForward = {forward.x, forward.y, forward.z, 0.0f},
            .ambientColor = {ambient.x, ambient.y, ambient.z, _ambientEnergy},
            .skyRotation = {skyRotation.x, skyRotation.y, skyRotation.z, skyRotation.w},
            .ambientParams = {hasAmbientMap ? 1.0f : 0.0f, static_cast<float>(PREFILTERED_CUBE_MIPS - 1), 0.0f, 0.0f}
        };
        uploadConstants(_frameConstants, &frame, sizeof(frame));
        uploadLights();

        _context->SetPipelineState(_scenePipeline);

        for (const auto &[mesh, material, model, castShadows]: _draws)
        {
            const auto *slot = _meshes.get(mesh);
            auto *surface = _materials.get(material);
            if (slot == nullptr || surface == nullptr) continue;

            // Compared rather than waited on: nothing announces that the environment was
            // rebaked, and a binding left pointing at the previous one keeps a freed cube alive
            // and lights the surface with a sky that is no longer in the scene.
            if (surface->ambientMap != _ambientMap) bindAmbientMap(*surface, _ambientMap);

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
            _context->CommitShaderResources(surface->binding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            Diligent::DrawIndexedAttribs drawAttribs;
            drawAttribs.IndexType = Diligent::VT_UINT32;
            drawAttribs.NumIndices = slot->indexCount;
            drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
            _context->DrawIndexed(drawAttribs);
        }
    }

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

        MaterialSlot material;
        _scenePipeline->CreateShaderResourceBinding(&material.binding, true);
        if (!material.binding)
        {
            Logger::LogError("Diligent failed to create a material's resource binding");
            return {};
        }

        const TextureHandle handles[MATERIAL_TEXTURE_COUNT]{desc.albedo, desc.normal, desc.orm, desc.emission};
        for (size_t slot = 0; slot < MATERIAL_TEXTURE_COUNT; ++slot)
        {
            // Null when the shader does not sample this slot - the sources are compiled from
            // disk at runtime, so which variables exist is not fixed at build time.
            auto *variable = material.binding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, MATERIAL_TEXTURE_NAMES[slot]);
            if (variable == nullptr) continue;

            variable->Set(materialTextureView(handles[slot], _materialFallbacks[slot]));
        }

        material.irradiance = material.binding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Irradiance");
        material.prefiltered = material.binding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Prefiltered");
        // Unconditionally, unlike the per-draw call: a dynamic variable starts out pointing at
        // nothing, which is not a state a draw can validate.
        bindAmbientMap(material, _ambientMap);

        return _materials.add(std::move(material));
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
} // namespace BreadEngine
