#include "diligentRenderer.h"

// Before anything that reaches for a GL entry point: GLEW insists on declaring them first.
#include <GL/glew.h>

#include <algorithm>
#include <cmath>

#include <EngineFactoryOpenGL.h>
#include <Shader.h>

#include "logger.h"
#include "rlgl.h"

namespace BreadEngine {
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
        uploadConstants(_context, _lightConstants, &constants, sizeof(constants));
    }

    // --- shadows ---

    constexpr Diligent::TEXTURE_FORMAT SHADOW_MAP_FORMAT = Diligent::TEX_FORMAT_D32_FLOAT;
    constexpr Diligent::Uint32 SHADOW_MAP_RESOLUTION = 2048;
    constexpr Diligent::Uint32 SHADOW_CASCADE_COUNT = 4;

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

    void ShadowPass::initializeMaps(Diligent::IRenderDevice *device, Diligent::IDeviceContext *context)
    {
        _device = device;
        _context = context;
        if (!_device) return;

        Diligent::BufferDesc constantsDesc;
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        constantsDesc.Name = "Shadow constants";
        constantsDesc.Size = sizeof(ShadowConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_constants);
        constantsDesc.Name = "Shadow pass constants";
        constantsDesc.Size = sizeof(Diligent::float4x4);
        _device->CreateBuffer(constantsDesc, nullptr, &_passConstants);

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
        _cascadeMaps.Initialize(_device, nullptr, initInfo);

        // Selects the world-space filter Shadows.fxh sizes per cascade, over the fixed 3x3 one.
        _data.cascades.iFixedFilterSize = 0;

        createArray("Spot shadow maps", Diligent::RESOURCE_DIM_TEX_2D_ARRAY, SPOT_SHADOW_RESOLUTION,
                    comparisonSampler, _spotMaps, _spotDSVs);
        // A cube array is indexed in layer-faces rather than in cubes, so it holds six slices
        // per light and a depth pass still writes exactly one of them.
        createArray("Omni shadow maps", Diligent::RESOURCE_DIM_TEX_CUBE_ARRAY, OMNI_SHADOW_RESOLUTION,
                    comparisonSampler, _omniMaps, _omniDSVs);
    }

    void ShadowPass::createArray(const char *name, const Diligent::RESOURCE_DIMENSION dimension,
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

    void ShadowPass::initializePipeline(Diligent::IBuffer *drawConstants)
    {
        _drawConstants = drawConstants;
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

        _device->CreateGraphicsPipelineState(pipelineInfo, &_pipeline);
        if (!_pipeline)
        {
            Logger::LogError("Diligent failed to create the shadow pipeline state");
            return;
        }

        _pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "ShadowPassConstants")->Set(_passConstants);
        _pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "DrawConstants")->Set(_drawConstants);
        _pipeline->CreateShaderResourceBinding(&_binding, true);
    }

    void ShadowPass::shutdown()
    {
        _binding.Release();
        _pipeline.Release();
        _cascadeMaps = {};
        _spotMaps.Release();
        _spotDSVs = {};
        _omniMaps.Release();
        _omniDSVs = {};
        _constants.Release();
        _passConstants.Release();

        _drawConstants = nullptr;
        _context = nullptr;
        _device = nullptr;
    }

    void ShadowPass::render(const std::span<VisibleLight> lights, const ShadowCasters &casters,
                            const CameraView &camera, const float aspect)
    {
        assignSlots(lights);

        if (_pipeline)
        {
            _context->SetPipelineState(_pipeline);
            for (const auto &[light, spotShadowSlice, omniShadowSlice, ownsCascades]: lights)
            {
                if (ownsCascades) renderCascades(*light, casters, camera, aspect);
                else if (spotShadowSlice >= 0) renderSpotMap(*light, spotShadowSlice, casters);
                else if (omniShadowSlice >= 0) renderOmniMap(*light, omniShadowSlice, casters);
            }
        }

        // Uploaded even when nothing was filled: a frame with no directional caster leaves the
        // cascade count at zero, which is what Shadows.fxh reads as fully lit.
        uploadConstants(_context, _constants, &_data, sizeof(_data));
    }

    void ShadowPass::assignSlots(const std::span<VisibleLight> lights)
    {
        _data.cascades.iNumCascades = 0;

        bool cascadesTaken = false;
        int nextSpotSlice = 0;
        int nextOmniSlice = 0;
        for (auto &visible: lights)
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

    void ShadowPass::renderCasters(Diligent::ITextureView *target, const float16 &worldToLightClip,
                                   const ShadowCasters &casters)
    {
        _context->SetRenderTargets(0, nullptr, target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->ClearDepthStencil(target, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        uploadConstants(_context, _passConstants, worldToLightClip.v, sizeof(worldToLightClip));

        for (const auto &[mesh, material, model, castShadows]: casters.draws)
        {
            if (!castShadows) continue;

            const auto *slot = casters.meshes.get(mesh);
            if (slot == nullptr) continue;

            const SceneDrawConstants draw{.model = MatrixToFloatV(model), .normalMatrix = {}};
            uploadConstants(_context, _drawConstants, &draw, sizeof(draw));

            Diligent::IBuffer *vertices = slot->vertices;
            constexpr Diligent::Uint64 vertexOffset = 0;
            _context->SetVertexBuffers(0, 1, &vertices, &vertexOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                       Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
            _context->SetIndexBuffer(slot->indices, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            _context->CommitShaderResources(_binding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            Diligent::DrawIndexedAttribs drawAttribs;
            drawAttribs.IndexType = Diligent::VT_UINT32;
            drawAttribs.NumIndices = slot->indexCount;
            drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
            _context->DrawIndexed(drawAttribs);
        }
    }

    void ShadowPass::renderCascades(const LightState &light, const ShadowCasters &casters, const CameraView &camera,
                                    const float aspect)
    {
        // Fitting the cascades is DiligentFX's, and its camera space runs +Z forward where the
        // engine's runs -Z. So the camera is handed over as a left-handed basis built here
        // rather than as the engine's own view matrix, and the pixel shader gets its
        // camera-space depth by projecting onto the forward axis rather than from that matrix.
        const auto forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        const auto right = Vector3Normalize(Vector3CrossProduct(camera.up, forward));
        const auto up = Vector3CrossProduct(forward, right);
        const Diligent::float4x4 cameraWorld{
            right.x, right.y, right.z, 0.0f,
            up.x, up.y, up.z, 0.0f,
            forward.x, forward.y, forward.z, 0.0f,
            camera.position.x, camera.position.y, camera.position.z, 1.0f
        };
        const auto cameraView = cameraWorld.Inverse();

        const auto nearPlane = static_cast<float>(rlGetCullDistanceNear());
        const auto farPlane = static_cast<float>(rlGetCullDistanceFar());
        const bool isGL = _device->GetDeviceInfo().IsGLDevice();
        const auto cameraProjection = camera.projection == ProjectionType::Orthographic
                                          ? Diligent::float4x4::Ortho(camera.fov * aspect, camera.fov, nearPlane, farPlane, isGL)
                                          : Diligent::float4x4::Projection(camera.fov * DEG2RAD, aspect, nearPlane, farPlane, isGL);

        const Diligent::float3 lightDirection{light.direction.x, light.direction.y, light.direction.z};
        _data.cascades.fFilterWorldSize = light.shadowSoftness * SHADOW_SOFTNESS_WORLD_SIZE;

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
        _cascadeMaps.DistributeCascades(cascadeInfo, _data.cascades);

        for (Diligent::Uint32 cascade = 0; cascade < SHADOW_CASCADE_COUNT; ++cascade)
        {
            // Copied rather than transposed: PackMatrixRowMajor above already left it in the
            // layout the shaders read.
            float16 worldToLightClip;
            std::memcpy(worldToLightClip.v, &_cascadeMaps.GetCascadeTransform(cascade).WorldToLightProjSpace, sizeof(worldToLightClip));
            renderCasters(_cascadeMaps.GetCascadeDSV(cascade), worldToLightClip, casters);
        }
    }

    void ShadowPass::renderSpotMap(const LightState &light, const int slice, const ShadowCasters &casters)
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
        std::memcpy(&_data.spotTransforms[slice], worldToLightClip.v, sizeof(Diligent::float4x4));
        const auto filterTexels = std::min(light.shadowSoftness * SPOT_SHADOW_FILTER_TEXELS, SPOT_SHADOW_MAX_FILTER_TEXELS);
        _data.spotParams[slice].x = filterTexels / static_cast<float>(SPOT_SHADOW_RESOLUTION);
        renderCasters(_spotDSVs[slice], worldToLightClip, casters);
    }

    void ShadowPass::renderOmniMap(const LightState &light, const int slice, const ShadowCasters &casters)
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
        _data.omniPosition[slice] = {light.position.x, light.position.y, light.position.z, depthScale};
        const float filterTexels = std::min(light.shadowSoftness * OMNI_SHADOW_FILTER_TEXELS, OMNI_SHADOW_MAX_FILTER_TEXELS);
        // A face spans twice the distance to the surface across its full width, so one texel is
        // that much of it divided by the resolution - which is what turns both the filter's
        // reach and the offset a lookup leaves the surface by from texels into world units, at
        // whatever distance they end up being applied.
        constexpr float texelsToWorld = 2.0f / static_cast<float>(OMNI_SHADOW_RESOLUTION);
        _data.omniParams[slice] = {
            depthScale * OMNI_SHADOW_NEAR,
            filterTexels * texelsToWorld,
            (OMNI_SHADOW_NORMAL_OFFSET_TEXELS + filterTexels) * texelsToWorld,
            0.0f
        };

        for (size_t face = 0; face < CUBE_FACE_COUNT; ++face)
        {
            const auto target = Vector3Add(light.position, faceDirections[face]);
            const auto view = MatrixLookAt(light.position, target, faceUps[face]);
            renderCasters(_omniDSVs[slice * CUBE_FACE_COUNT + face],
                          MatrixToFloatV(MatrixMultiply(view, projection)), casters);
        }
    }
} // namespace BreadEngine
