#include "r3dRenderer.h"

#include "data/primitives/capsulePrimitiveData.h"
#include "data/primitives/cubePrimitiveData.h"
#include "data/primitives/cylinderPrimitiveData.h"
#include "data/primitives/freePolyPrimitiveData.h"
#include "data/primitives/planePrimitiveData.h"
#include "data/primitives/slopePrimitiveData.h"
#include "data/primitives/spherePrimitiveData.h"
#include "data/primitives/torusPrimitiveData.h"
#include "utils/colorUtils.h"

namespace BreadEngine {
    void R3DRenderer::initialize(const int sceneWidth, const int sceneHeight)
    {
        R3D_Init(sceneWidth, sceneHeight);
        _defaultMaterial = R3D_GetDefaultMaterial();
    }

    void R3DRenderer::shutdown()
    {
        _lights.forEachAlive([](LightSlot &slot)
        {
            if (R3D_IsLightExist(slot.native)) R3D_DestroyLight(slot.native);
        });
        _textures.forEachAlive(releaseTexture);
        _meshes.forEachAlive([](R3D_Mesh &mesh)
        {
            if (R3D_IsMeshValid(mesh)) R3D_UnloadMesh(mesh);
        });
        _models.forEachAlive([](R3D_Model &model) { R3D_UnloadModel(model, false); });
        _cubemaps.forEachAlive([](R3D_Cubemap &cubemap) { R3D_UnloadCubemap(cubemap); });
        _ambientMaps.forEachAlive([](R3D_AmbientMap &map) { R3D_UnloadAmbientMap(map); });

        if (IsRenderTextureValid(_sceneTarget))
        {
            UnloadRenderTexture(_sceneTarget);
            _sceneTarget = {};
        }

        _lights.clear();
        _textures.clear();
        _meshes.clear();
        _models.clear();
        _cubemaps.clear();
        _ambientMaps.clear();

        R3D_Close();
    }

    // --- frame ---

    Camera3D R3DRenderer::toNative(const CameraView &camera)
    {
        return Camera3D{
            .position = camera.position,
            .target = camera.target,
            .up = camera.up,
            .fovy = camera.fov,
            .projection = camera.projection == ProjectionType::Orthographic ? CAMERA_ORTHOGRAPHIC : CAMERA_PERSPECTIVE
        };
    }

    void R3DRenderer::resizeSceneTarget(const int width, const int height)
    {
        if (width <= 0 || height <= 0) return;
        if (_sceneTarget.texture.width == width && _sceneTarget.texture.height == height) return;

        if (IsRenderTextureValid(_sceneTarget)) UnloadRenderTexture(_sceneTarget);

        _sceneTarget = LoadRenderTexture(width, height);
        R3D_SetResolution(width, height);
    }

    void R3DRenderer::beginScene(const CameraView &camera)
    {
        // A zero-id target makes r3d render to the backbuffer, which is what the game wants.
        R3D_BeginEx(_sceneTarget, toNative(camera));
    }

    void R3DRenderer::endScene()
    {
        R3D_End();
    }

    void R3DRenderer::beginSceneOverlay()
    {
        if (!IsRenderTextureValid(_sceneTarget)) return;

        BeginTextureMode(_sceneTarget);
    }

    void R3DRenderer::endSceneOverlay()
    {
        if (!IsRenderTextureValid(_sceneTarget)) return;

        EndTextureMode();
    }

    void R3DRenderer::drawSceneTexture(const Rectangle destination)
    {
        if (!IsRenderTextureValid(_sceneTarget)) return;

        const auto width = static_cast<float>(_sceneTarget.texture.width);
        const auto height = static_cast<float>(_sceneTarget.texture.height);
        DrawTexturePro(_sceneTarget.texture, Rectangle{0, 0, width, -height}, destination, Vector2{0, 0}, 0, WHITE);
    }

    // --- lights ---

    R3D_LightType R3DRenderer::toNative(const LightType type)
    {
        switch (type)
        {
            case LightType::Spot: return R3D_LIGHT_SPOT;
            case LightType::Omni: return R3D_LIGHT_OMNI;
            case LightType::Directional:
            default: return R3D_LIGHT_DIR;
        }
    }

    LightHandle R3DRenderer::createLight(const LightType type)
    {
        return _lights.add(LightSlot{.native = R3D_CreateLight(toNative(type))});
    }

    void R3DRenderer::destroyLight(const LightHandle handle)
    {
        const auto *slot = _lights.get(handle);
        if (slot == nullptr) return;

        if (R3D_IsLightExist(slot->native)) R3D_DestroyLight(slot->native);
        _lights.remove(handle);
    }

    bool R3DRenderer::isLightValid(const LightHandle handle) const
    {
        const auto *slot = _lights.get(handle);
        return slot != nullptr && R3D_IsLightExist(slot->native);
    }

    void R3DRenderer::updateLight(const LightHandle handle, const LightState &state)
    {
        auto *slot = _lights.get(handle);
        if (slot == nullptr) return;

        // A light's type is baked into the r3d object, so a type change means a new one.
        if (!R3D_IsLightExist(slot->native) || R3D_GetLightType(slot->native) != toNative(state.type))
        {
            if (R3D_IsLightExist(slot->native)) R3D_DestroyLight(slot->native);

            slot->native = R3D_CreateLight(toNative(state.type));
            slot->hasApplied = false;
        }

        const bool forceApply = !slot->hasApplied;
        const LightState &applied = slot->applied;

        if (forceApply || state.castShadows != applied.castShadows)
        {
            if (state.castShadows) R3D_EnableShadow(slot->native);
            else R3D_DisableShadow(slot->native);
        }

        if (forceApply || !ColorUtils::IsCompare(applied.color, state.color))
        {
            R3D_SetLightColor(slot->native, state.color);
        }

        if (forceApply || applied.range != state.range)
        {
            R3D_SetLightRange(slot->native, state.range);
        }

        if (state.castShadows && (forceApply || applied.shadowSoftness != state.shadowSoftness))
        {
            R3D_SetShadowSoftness(slot->native, state.shadowSoftness);
        }

        if (forceApply || applied.intensity != state.intensity)
        {
            R3D_SetLightEnergy(slot->native, state.intensity);
        }

        if (forceApply || applied.active != state.active)
        {
            R3D_SetLightActive(slot->native, state.active);
        }

        // Unconditional: r3d may key shadow-map refreshes off these setters, so skipping a
        // redundant call is not provably a no-op the way skipping a scalar setter is.
        if (state.type != LightType::Omni) R3D_SetLightDirection(slot->native, state.direction);
        if (state.type != LightType::Directional) R3D_SetLightPosition(slot->native, state.position);

        slot->applied = state;
        slot->hasApplied = true;
    }

    // --- textures ---

    TextureWrap R3DRenderer::toNative(const TextureWrapMode wrap)
    {
        switch (wrap)
        {
            case TextureWrapMode::Clamp: return TEXTURE_WRAP_CLAMP;
            case TextureWrapMode::MirrorRepeat: return TEXTURE_WRAP_MIRROR_REPEAT;
            case TextureWrapMode::MirrorClamp: return TEXTURE_WRAP_MIRROR_CLAMP;
            case TextureWrapMode::Repeat:
            default: return TEXTURE_WRAP_REPEAT;
        }
    }

    TextureFilter R3DRenderer::toNative(const TextureFilterMode filter)
    {
        switch (filter)
        {
            case TextureFilterMode::Bilinear: return TEXTURE_FILTER_BILINEAR;
            case TextureFilterMode::Trilinear: return TEXTURE_FILTER_TRILINEAR;
            case TextureFilterMode::Anisotropic4x: return TEXTURE_FILTER_ANISOTROPIC_4X;
            case TextureFilterMode::Anisotropic8x: return TEXTURE_FILTER_ANISOTROPIC_8X;
            case TextureFilterMode::Anisotropic16x: return TEXTURE_FILTER_ANISOTROPIC_16X;
            case TextureFilterMode::Point:
            default: return TEXTURE_FILTER_POINT;
        }
    }

    void R3DRenderer::finalizeTexture(TextureSlot &slot)
    {
        if (slot.uploaded) return;
        if (slot.decodeJob.joinable()) slot.decodeJob.join();

        const auto wrap = toNative(slot.desc.wrap);
        const auto filter = toNative(slot.desc.filter);
        if (IsImageValid(slot.decoded))
        {
            // R3D_LoadTextureFromImageEx takes ownership of the pixel data - unloading the
            // image afterwards is a double free. Dropping the handle is all that is left.
            slot.native = R3D_LoadTextureFromImageEx(slot.decoded, wrap, filter, slot.desc.isColor);
            slot.decoded = {};
        }
        else
        {
            slot.native = R3D_LoadTextureEx(slot.desc.path.c_str(), wrap, filter, slot.desc.isColor);
        }

        slot.uploaded = true;
    }

    void R3DRenderer::releaseTexture(TextureSlot &slot)
    {
        if (slot.decodeJob.joinable()) slot.decodeJob.join();
        // Only reached for an image that was decoded but never uploaded; once uploaded, the
        // pixel data belongs to r3d and slot.decoded has been cleared.
        if (IsImageValid(slot.decoded)) UnloadImage(slot.decoded);
        if (slot.uploaded) R3D_UnloadTexture(slot.native);
    }

    TextureHandle R3DRenderer::createTexture(const TextureDesc &desc)
    {
        const auto handle = _textures.add(TextureSlot{.desc = desc});
        auto *slot = _textures.get(handle);

        // Pool slots keep a stable address, so the decode job may capture one directly.
        slot->decodeJob = std::jthread([slot] { slot->decoded = LoadImage(slot->desc.path.c_str()); });
        return handle;
    }

    void R3DRenderer::destroyTexture(const TextureHandle handle)
    {
        auto *slot = _textures.get(handle);
        if (slot == nullptr) return;

        releaseTexture(*slot);
        _textures.remove(handle);
    }

    TextureSize R3DRenderer::getTextureSize(const TextureHandle handle)
    {
        auto *slot = _textures.get(handle);
        if (slot == nullptr) return {};

        finalizeTexture(*slot);
        return {.width = slot->native.width, .height = slot->native.height};
    }

    // --- materials ---

    void R3DRenderer::applyTexture(const TextureHandle handle, Texture2D &target)
    {
        auto *slot = _textures.get(handle);
        if (slot == nullptr) return;

        finalizeTexture(*slot);
        target = slot->native;
    }

    R3D_Material R3DRenderer::buildMaterial(const MaterialData &material)
    {
        auto native = _defaultMaterial;
        applyTexture(material.albedo, native.albedo.texture);
        applyTexture(material.normal, native.normal.texture);
        applyTexture(material.orm, native.orm.texture);
        applyTexture(material.emission, native.emission.texture);
        return native;
    }

    // --- meshes ---

    MeshHandle R3DRenderer::createPrimitive(const MeshPrimitiveData &data, const Vector3 forward)
    {
        R3D_Mesh mesh{};
        switch (data.getMeshType())
        {
            case MeshPrimitiveType::Cube:
            {
                const auto &cube = static_cast<const CubePrimitiveData &>(data);
                mesh = R3D_GenMeshCube(cube.width, cube.height, cube.depth);
                break;
            }
            case MeshPrimitiveType::Sphere:
            {
                const auto &sphere = static_cast<const SpherePrimitiveData &>(data);
                mesh = R3D_GenMeshSphere(sphere.radius, sphere.rings, sphere.slices);
                break;
            }
            case MeshPrimitiveType::HalfSphere:
            {
                const auto &sphere = static_cast<const SpherePrimitiveData &>(data);
                mesh = R3D_GenMeshHemiSphere(sphere.radius, sphere.rings, sphere.slices);
                break;
            }
            case MeshPrimitiveType::Cylinder:
            {
                const auto &cylinder = static_cast<const CylinderPrimitiveData &>(data);
                mesh = R3D_GenMeshCylinderEx(cylinder.bottomRadius, cylinder.topRadius, cylinder.height, cylinder.slices, cylinder.stacks, cylinder.bottomCap, cylinder.topCap);
                break;
            }
            case MeshPrimitiveType::Capsule:
            {
                const auto &capsule = static_cast<const CapsulePrimitiveData &>(data);
                mesh = R3D_GenMeshCapsule(capsule.radius, capsule.height, capsule.rings, capsule.slices);
                break;
            }
            case MeshPrimitiveType::Plane:
            {
                const auto &plane = static_cast<const PlanePrimitiveData &>(data);
                mesh = R3D_GenMeshPlane(plane.width, plane.height, plane.resX, plane.resZ);
                break;
            }
            case MeshPrimitiveType::Quad:
            {
                const auto &quad = static_cast<const PlanePrimitiveData &>(data);
                mesh = R3D_GenMeshQuad(quad.width, quad.height, quad.resX, quad.resZ, forward);
                break;
            }
            case MeshPrimitiveType::Slope:
            {
                const auto &slope = static_cast<const SlopePrimitiveData &>(data);
                mesh = R3D_GenMeshSlope(slope.width, slope.height, slope.length, slope.normal);
                break;
            }
            case MeshPrimitiveType::Torus:
            {
                const auto &torus = static_cast<const TorusPrimitiveData &>(data);
                mesh = R3D_GenMeshTorus(torus.radius, torus.size, torus.radiusSegments, torus.sides);
                break;
            }
            case MeshPrimitiveType::FreePoly:
            {
                const auto &poly = static_cast<const FreePolyPrimitiveData &>(data);
                mesh = R3D_GenMeshPoly(poly.sides, poly.size, forward);
                break;
            }
            case MeshPrimitiveType::None:
            default: return {};
        }

        if (!R3D_IsMeshValid(mesh)) return {};
        return _meshes.add(std::move(mesh));
    }

    void R3DRenderer::destroyMesh(const MeshHandle handle)
    {
        const auto *mesh = _meshes.get(handle);
        if (mesh == nullptr) return;

        if (R3D_IsMeshValid(*mesh)) R3D_UnloadMesh(*mesh);
        _meshes.remove(handle);
    }

    void R3DRenderer::drawMesh(const MeshHandle handle, const MaterialData &material, const Vector3 position, const Quaternion rotation, const Vector3 scale)
    {
        const auto *mesh = _meshes.get(handle);
        if (mesh == nullptr) return;

        R3D_DrawMeshEx(*mesh, buildMaterial(material), position, rotation, scale);
    }

    // --- models ---

    ModelHandle R3DRenderer::loadModel(const std::string &path)
    {
        auto model = R3D_LoadModelEx(path.c_str(), 0);
        if (model.meshes == nullptr) return {};

        return _models.add(std::move(model));
    }

    void R3DRenderer::destroyModel(const ModelHandle handle)
    {
        const auto *model = _models.get(handle);
        if (model == nullptr) return;

        // Materials are not unloaded here: setModelMaterial overwrites them with textures
        // the engine's TextureAssets own.
        R3D_UnloadModel(*model, false);
        _models.remove(handle);
    }

    int R3DRenderer::getModelMaterialCount(const std::string &path)
    {
        const auto model = R3D_LoadModel(path.c_str());
        const auto count = model.materialCount;
        R3D_UnloadModel(model, true);
        return count;
    }

    void R3DRenderer::setModelMaterial(const ModelHandle handle, const int slot, const MaterialData &material)
    {
        auto *model = _models.get(handle);
        if (model == nullptr || slot < 0 || slot >= model->materialCount) return;

        model->materials[slot] = buildMaterial(material);
    }

    void R3DRenderer::drawModel(const ModelHandle handle, const Vector3 position, const Quaternion rotation, const Vector3 scale)
    {
        const auto *model = _models.get(handle);
        if (model == nullptr) return;

        R3D_DrawModelEx(*model, position, rotation, scale);
    }
    // --- environment ---

    R3D_Bloom R3DRenderer::toNative(const BloomMode mode)
    {
        switch (mode)
        {
            case BloomMode::Mix: return R3D_BLOOM_MIX;
            case BloomMode::Additive: return R3D_BLOOM_ADDITIVE;
            case BloomMode::Screen: return R3D_BLOOM_SCREEN;
            case BloomMode::Disabled:
            default: return R3D_BLOOM_DISABLED;
        }
    }

    R3D_Fog R3DRenderer::toNative(const FogMode mode)
    {
        switch (mode)
        {
            case FogMode::Linear: return R3D_FOG_LINEAR;
            case FogMode::Exp2: return R3D_FOG_EXP2;
            case FogMode::Exp: return R3D_FOG_EXP;
            case FogMode::Disabled:
            default: return R3D_FOG_DISABLED;
        }
    }

    R3D_DoF R3DRenderer::toNative(const DepthOfFieldMode mode)
    {
        return mode == DepthOfFieldMode::Enabled ? R3D_DOF_ENABLED : R3D_DOF_DISABLED;
    }

    R3D_Tonemap R3DRenderer::toNative(const TonemapMode mode)
    {
        switch (mode)
        {
            case TonemapMode::Reinhard: return R3D_TONEMAP_REINHARD;
            case TonemapMode::Filmic: return R3D_TONEMAP_FILMIC;
            case TonemapMode::Aces: return R3D_TONEMAP_ACES;
            case TonemapMode::Agx: return R3D_TONEMAP_AGX;
            case TonemapMode::Linear:
            default: return R3D_TONEMAP_LINEAR;
        }
    }

    BloomMode R3DRenderer::fromNative(const R3D_Bloom mode)
    {
        switch (mode)
        {
            case R3D_BLOOM_MIX: return BloomMode::Mix;
            case R3D_BLOOM_ADDITIVE: return BloomMode::Additive;
            case R3D_BLOOM_SCREEN: return BloomMode::Screen;
            case R3D_BLOOM_DISABLED:
            default: return BloomMode::Disabled;
        }
    }

    FogMode R3DRenderer::fromNative(const R3D_Fog mode)
    {
        switch (mode)
        {
            case R3D_FOG_LINEAR: return FogMode::Linear;
            case R3D_FOG_EXP2: return FogMode::Exp2;
            case R3D_FOG_EXP: return FogMode::Exp;
            case R3D_FOG_DISABLED:
            default: return FogMode::Disabled;
        }
    }

    DepthOfFieldMode R3DRenderer::fromNative(const R3D_DoF mode)
    {
        return mode == R3D_DOF_ENABLED ? DepthOfFieldMode::Enabled : DepthOfFieldMode::Disabled;
    }

    TonemapMode R3DRenderer::fromNative(const R3D_Tonemap mode)
    {
        switch (mode)
        {
            case R3D_TONEMAP_REINHARD: return TonemapMode::Reinhard;
            case R3D_TONEMAP_FILMIC: return TonemapMode::Filmic;
            case R3D_TONEMAP_ACES: return TonemapMode::Aces;
            case R3D_TONEMAP_AGX: return TonemapMode::Agx;
            case R3D_TONEMAP_LINEAR:
            default: return TonemapMode::Linear;
        }
    }

    void R3DRenderer::applyDefaultEnvironment(EnvironmentSettings settings)
    {
        const R3D_Environment &defaults = *R3D_GetEnvironment();

        settings.background.color = defaults.background.color;
        settings.background.energy = defaults.background.energy;
        settings.background.skyBlur = defaults.background.skyBlur;
        settings.background.rotation = defaults.background.rotation;

        settings.ambient.color = defaults.ambient.color;
        settings.ambient.energy = defaults.ambient.energy;

        settings.ssao.intensity = defaults.ssao.intensity;
        settings.ssao.power = defaults.ssao.power;
        settings.ssao.radius = defaults.ssao.radius;
        settings.ssao.bias = defaults.ssao.bias;
        settings.ssao.sampleCount = defaults.ssao.sampleCount;
        settings.ssao.enabled = defaults.ssao.enabled;

        settings.ssil.radius = defaults.ssil.radius;
        settings.ssil.thickness = defaults.ssil.thickness;
        settings.ssil.intensity = defaults.ssil.intensity;
        settings.ssil.aoPower = defaults.ssil.aoPower;
        settings.ssil.sampleCount = defaults.ssil.sampleCount;
        settings.ssil.sliceCount = defaults.ssil.sliceCount;
        settings.ssil.denoiseSteps = defaults.ssil.denoiseSteps;
        settings.ssil.enabled = defaults.ssil.enabled;

        settings.ssgi.stepSize = defaults.ssgi.stepSize;
        settings.ssgi.thickness = defaults.ssgi.thickness;
        settings.ssgi.maxDistance = defaults.ssgi.maxDistance;
        settings.ssgi.intensity = defaults.ssgi.intensity;
        settings.ssgi.fadeStart = defaults.ssgi.fadeStart;
        settings.ssgi.fadeEnd = defaults.ssgi.fadeEnd;
        settings.ssgi.sampleCount = defaults.ssgi.sampleCount;
        settings.ssgi.maxRaySteps = defaults.ssgi.maxRaySteps;
        settings.ssgi.denoiseSteps = defaults.ssgi.denoiseSteps;
        settings.ssgi.enabled = defaults.ssgi.enabled;

        settings.ssr.stepSize = defaults.ssr.stepSize;
        settings.ssr.thickness = defaults.ssr.thickness;
        settings.ssr.maxDistance = defaults.ssr.maxDistance;
        settings.ssr.edgeFade = defaults.ssr.edgeFade;
        settings.ssr.maxRaySteps = defaults.ssr.maxRaySteps;
        settings.ssr.binarySteps = defaults.ssr.binarySteps;
        settings.ssr.enabled = defaults.ssr.enabled;

        settings.bloom.mode = fromNative(defaults.bloom.mode);
        settings.bloom.levels = defaults.bloom.levels;
        settings.bloom.intensity = defaults.bloom.intensity;
        settings.bloom.threshold = defaults.bloom.threshold;
        settings.bloom.softThreshold = defaults.bloom.softThreshold;
        settings.bloom.filterRadius = defaults.bloom.filterRadius;

        settings.fog.mode = fromNative(defaults.fog.mode);
        settings.fog.color = defaults.fog.color;
        settings.fog.start = defaults.fog.start;
        settings.fog.end = defaults.fog.end;
        settings.fog.density = defaults.fog.density;
        settings.fog.skyAffect = defaults.fog.skyAffect;

        settings.depthOfField.mode = fromNative(defaults.dof.mode);
        settings.depthOfField.focusPoint = defaults.dof.focusPoint;
        settings.depthOfField.focusScale = defaults.dof.focusScale;
        settings.depthOfField.nearScale = defaults.dof.nearScale;
        settings.depthOfField.maxBlurSize = defaults.dof.maxBlurSize;

        settings.tonemap.mode = fromNative(defaults.tonemap.mode);
        settings.tonemap.exposure = defaults.tonemap.exposure;
        settings.tonemap.white = defaults.tonemap.white;

        settings.finalColor.brightness = defaults.color.brightness;
        settings.finalColor.contrast = defaults.color.contrast;
        settings.finalColor.saturation = defaults.color.saturation;
    }

    void R3DRenderer::setEnvironment(EnvironmentSettings settings)
    {
        R3D_Environment &env = *R3D_GetEnvironment();

        const auto *sky = _cubemaps.get(settings.background.sky);
        const auto *ambientMap = _ambientMaps.get(settings.ambient.map);

        env.background = R3D_EnvBackground{
            .color = settings.background.color,
            .energy = settings.background.energy,
            .skyBlur = settings.background.skyBlur,
            .sky = sky != nullptr ? *sky : R3D_Cubemap{},
            .rotation = settings.background.rotation
        };

        env.ambient = R3D_EnvAmbient{
            .color = settings.ambient.color,
            .energy = settings.ambient.energy,
            .map = ambientMap != nullptr ? *ambientMap : R3D_AmbientMap{}
        };

        env.ssao = R3D_EnvSSAO{
            .sampleCount = settings.ssao.sampleCount,
            .intensity = settings.ssao.intensity,
            .power = settings.ssao.power,
            .radius = settings.ssao.radius,
            .bias = settings.ssao.bias,
            .enabled = settings.ssao.enabled
        };

        env.ssil = R3D_EnvSSIL{
            .sampleCount = settings.ssil.sampleCount,
            .sliceCount = settings.ssil.sliceCount,
            .radius = settings.ssil.radius,
            .thickness = settings.ssil.thickness,
            .intensity = settings.ssil.intensity,
            .aoPower = settings.ssil.aoPower,
            .denoiseSteps = settings.ssil.denoiseSteps,
            .enabled = settings.ssil.enabled
        };

        env.ssgi = R3D_EnvSSGI{
            .sampleCount = settings.ssgi.sampleCount,
            .maxRaySteps = settings.ssgi.maxRaySteps,
            .stepSize = settings.ssgi.stepSize,
            .thickness = settings.ssgi.thickness,
            .maxDistance = settings.ssgi.maxDistance,
            .intensity = settings.ssgi.intensity,
            .fadeStart = settings.ssgi.fadeStart,
            .fadeEnd = settings.ssgi.fadeEnd,
            .denoiseSteps = settings.ssgi.denoiseSteps,
            .enabled = settings.ssgi.enabled
        };

        env.ssr = R3D_EnvSSR{
            .maxRaySteps = settings.ssr.maxRaySteps,
            .binarySteps = settings.ssr.binarySteps,
            .stepSize = settings.ssr.stepSize,
            .thickness = settings.ssr.thickness,
            .maxDistance = settings.ssr.maxDistance,
            .edgeFade = settings.ssr.edgeFade,
            .enabled = settings.ssr.enabled
        };

        env.bloom = R3D_EnvBloom{
            .mode = toNative(settings.bloom.mode),
            .levels = settings.bloom.levels,
            .intensity = settings.bloom.intensity,
            .threshold = settings.bloom.threshold,
            .softThreshold = settings.bloom.softThreshold,
            .filterRadius = settings.bloom.filterRadius
        };

        env.fog = R3D_EnvFog{
            .mode = toNative(settings.fog.mode),
            .color = settings.fog.color,
            .start = settings.fog.start,
            .end = settings.fog.end,
            .density = settings.fog.density,
            .skyAffect = settings.fog.skyAffect
        };

        env.dof = R3D_EnvDoF{
            .mode = toNative(settings.depthOfField.mode),
            .focusPoint = settings.depthOfField.focusPoint,
            .focusScale = settings.depthOfField.focusScale,
            .nearScale = settings.depthOfField.nearScale,
            .maxBlurSize = settings.depthOfField.maxBlurSize
        };

        env.tonemap = R3D_EnvTonemap{
            .mode = toNative(settings.tonemap.mode),
            .exposure = settings.tonemap.exposure,
            .white = settings.tonemap.white
        };

        env.color = R3D_EnvColor{
            .brightness = settings.finalColor.brightness,
            .contrast = settings.finalColor.contrast,
            .saturation = settings.finalColor.saturation
        };
    }

    CubemapHandle R3DRenderer::loadCubemap(const std::string &path)
    {
        auto cubemap = R3D_LoadCubemap(path.c_str(), R3D_CUBEMAP_LAYOUT_AUTO_DETECT);
        return _cubemaps.add(std::move(cubemap));
    }

    CubemapHandle R3DRenderer::createProceduralSky(const int size, const SkyboxProceduralParameters &sky)
    {
        auto cubemap = R3D_GenProceduralSky(size, R3D_ProceduralSky{
            .skyTopColor = sky.skyTopColor,
            .skyHorizonColor = sky.skyHorizonColor,
            .skyHorizonCurve = sky.skyHorizonCurve,
            .skyEnergy = sky.skyEnergy,
            .groundBottomColor = sky.groundBottomColor,
            .groundHorizonColor = sky.groundHorizonColor,
            .groundHorizonCurve = sky.groundHorizonCurve,
            .groundEnergy = sky.groundEnergy,
            .sunDirection = sky.sunDirection,
            .sunColor = sky.sunColor,
            .sunSize = sky.sunSize,
            .sunEnergy = sky.sunEnergy
        });

        return _cubemaps.add(std::move(cubemap));
    }

    void R3DRenderer::destroyCubemap(const CubemapHandle handle)
    {
        const auto *cubemap = _cubemaps.get(handle);
        if (cubemap == nullptr) return;

        R3D_UnloadCubemap(*cubemap);
        _cubemaps.remove(handle);
    }

    AmbientMapHandle R3DRenderer::createAmbientMap(const CubemapHandle cubemap)
    {
        const auto *source = _cubemaps.get(cubemap);
        if (source == nullptr) return {};

        auto map = R3D_GenAmbientMap(*source, R3D_AMBIENT_ILLUMINATION | R3D_AMBIENT_REFLECTION);
        return _ambientMaps.add(std::move(map));
    }

    void R3DRenderer::destroyAmbientMap(const AmbientMapHandle handle)
    {
        const auto *map = _ambientMaps.get(handle);
        if (map == nullptr) return;

        R3D_UnloadAmbientMap(*map);
        _ambientMaps.remove(handle);
    }
} // namespace BreadEngine
