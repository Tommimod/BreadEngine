#include "diligentRenderer.h"

// Before everything else: GLEW insists on being the first to declare the GL entry points.
#include <GL/glew.h>

#include <algorithm>
#include <chrono>
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

#include "rendering/sky/hosekWilkie.h"
#include "utils/workerPool.h"

namespace BreadEngine {
    /// What raylib's own LoadRenderTexture stamps on a depth attachment; the field is unused
    /// for depth but DrawTexturePro-style paths still read it.
    constexpr int DEPTH_PIXEL_FORMAT = 19;

    /// The scene target's formats are fixed, so every pipeline that renders into it can be
    /// built against them once instead of being rebuilt when the target is resized. The scene
    /// shades into a float target because a light brighter than white has to survive as far as
    /// the tone mapper; only the composite pass's output is bounded to what a screen can show.
    constexpr Diligent::TEXTURE_FORMAT SCENE_COLOR_FORMAT = Diligent::TEX_FORMAT_RGBA16_FLOAT;
    constexpr Diligent::TEXTURE_FORMAT SCENE_DEPTH_FORMAT = Diligent::TEX_FORMAT_D32_FLOAT;
    constexpr Diligent::TEXTURE_FORMAT SCENE_OUTPUT_FORMAT = Diligent::TEX_FORMAT_RGBA8_UNORM;

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

    /// Face size of the reflection cube and how many of its mips are filled, which is also how
    /// many roughness steps the scene pass interpolates between. Mip 0 is a mirror and the last
    /// is fully rough; below four levels the steps become visible as bands on a curved surface.
    constexpr int PREFILTERED_CUBE_SIZE = 128;
    constexpr Diligent::Uint32 PREFILTERED_CUBE_MIPS = 6;
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
        /// rgb is the ambient colour used where no environment map is bound, w the energy.
        Vector4 ambientColor;
        /// Turns a world direction into the environment cube's space, as a quaternion. Already
        /// inverted: the authored rotation turns the sky, and this turns the lookup.
        Vector4 skyRotation;
        /// x is 1 while an environment map is bound, y the highest mip of the reflection cube.
        Vector4 ambientParams;
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

    /// Mirrors composite.psh's cbuffer, and float4-only for the same reason the scene's blocks
    /// are: it is the only member layout the struct and the shader cannot drift apart over.
    struct PostConstants
    {
        /// x is the TonemapMode, y the exposure, z the reference white point.
        Vector4 tonemap;
        /// x is brightness, y contrast, z saturation, w the exponent the result leaves through.
        Vector4 grading;
    };

    /// Faces of a cube map, in the order every graphics API agrees on, as the axes one face's
    /// texels span. They are the direction-to-texel rule read backwards rather than anything
    /// intuitive: for +X that rule is s = -z, t = -y, so u runs along -Z and v runs *down*
    /// along -Y. Every one of the six has v pointing the way that feels upside down, which is
    /// precisely why a wrong one mirrors a face without failing anywhere - the omni shadow cube
    /// pays for the same table.
    constexpr Vector3 CUBE_FACE_RIGHT[]{
        {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}
    };
    constexpr Vector3 CUBE_FACE_UP[]{
        {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
        {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}
    };
    constexpr Vector3 CUBE_FACE_FORWARD[]{
        {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}
    };

    /// An authored colour as the linear scene target needs it. A colour is picked in the
    /// encoded space a screen shows, and the composite pass encodes on the way back out, so
    /// what is written here has to be decoded by exactly the inverse of that encode - which
    /// also makes the whole round trip an identity when the output is asked to stay linear.
    Vector4 toSceneLinear(const Color color, const float encoding)
    {
        const auto normalized = ColorNormalize(color);
        const float exponent = 1.0f / encoding;
        return {
            std::pow(normalized.x, exponent), std::pow(normalized.y, exponent),
            std::pow(normalized.z, exponent), normalized.w
        };
    }

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
        // Also before the scene pipeline: the BRDF table is one of its static resources, and a
        // static variable can only be set while no binding has been created against it yet.
        createIblPipelines();
        createScenePipeline();
        createShadowPipeline();
        createCompositePipeline();
        createSkyPipelines();

        // The BRDF table is integrated by a real pass, so this is the first work that draws
        // before a frame has ever been opened. raylib goes on to load its fonts and draw the
        // editor's first frame through the same context, and it would do both into the table's
        // framebuffer with the pipeline's state still applied.
        yieldToRaylib();
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
        // Every slot the pool is about to drop may still have a decode running into it.
        _cubemaps.forEachAlive([](CubemapSlot &slot)
        {
            if (slot.decodeJob.valid()) slot.decodeJob.get();
        });
        _cubemaps.clear();
        _ambientMaps.clear();
        _brdfLut.Release();
        _ambientFallback.Release();
        _irradianceBinding.Release();
        _irradiancePipeline.Release();
        _prefilterBinding.Release();
        _prefilterPipeline.Release();
        _scenePipeline.Release();
        _compositeBinding.Release();
        _compositePipeline.Release();
        _skyBakeBinding.Release();
        _skyBakePipeline.Release();
        _equirectBakeBinding.Release();
        _equirectBakePipeline.Release();
        _skyboxBinding.Release();
        _skyboxPipeline.Release();
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
        _postConstants.Release();
        _skyBakeConstants.Release();
        _skyboxConstants.Release();
        _iblBakeConstants.Release();

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

        Diligent::TextureDesc outputDesc = colorDesc;
        outputDesc.Name = "Scene output";
        outputDesc.Format = SCENE_OUTPUT_FORMAT;
        _device->CreateTexture(outputDesc, nullptr, &_sceneOutput);

        if (!_sceneColor || !_sceneDepth || !_sceneOutput)
        {
            Logger::LogError("Diligent failed to create the scene render target");
            return;
        }

        // The composite pass reads this target one texel to one pixel, so point sampling is
        // not an approximation of the read - it is the read. Stated rather than left to the
        // backend's default, which filters and would soften the image by half a texel.
        Diligent::SamplerDesc sceneSampler;
        sceneSampler.MinFilter = sceneSampler.MagFilter = sceneSampler.MipFilter = Diligent::FILTER_TYPE_POINT;
        sceneSampler.AddressU = sceneSampler.AddressV = sceneSampler.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
        Diligent::RefCntAutoPtr<Diligent::ISampler> sceneColorSampler;
        _device->CreateSampler(sceneSampler, &sceneColorSampler);
        _sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sceneColorSampler);

        // The GL backend's native handle is the texture name itself, which is all raylib
        // needs to treat these as its own.
        _overlay.id = rlLoadFramebuffer();
        _overlay.texture = Texture2D{
            .id = static_cast<unsigned int>(_sceneOutput->GetNativeHandle()),
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
        _sceneOutput.Release();
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

    void DiligentRenderer::createCompositePipeline()
    {
        if (!_device) return;

        Diligent::BufferDesc constantsDesc;
        constantsDesc.Name = "Post constants";
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        constantsDesc.Size = sizeof(PostConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_postConstants);

        const auto shaderSources = createShaderSources();

        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = shaderSources;

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        shaderInfo.Desc = {"Composite VS", Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.FilePath = "fullscreen.vsh";
        _device->CreateShader(shaderInfo, &vertexShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
        shaderInfo.Desc = {"Composite PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "composite.psh";
        _device->CreateShader(shaderInfo, &pixelShader);

        if (!vertexShader || !pixelShader)
        {
            Logger::LogError("Diligent failed to compile the composite shaders");
            return;
        }

        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
        pipelineInfo.PSODesc.Name = "Scene composite";
        pipelineInfo.pVS = vertexShader;
        pipelineInfo.pPS = pixelShader;

        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = SCENE_OUTPUT_FORMAT;
        graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        // No input layout: the one triangle is built from the vertex index, so its winding is
        // an artefact of how the corners are indexed rather than something to depend on.
        graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
        // The pass covers every pixel of its target and runs with no depth attachment, because
        // the scene's depth has to reach the overlay that draws after it untouched.
        graphics.DepthStencilDesc.DepthEnable = Diligent::False;

        // Dynamic rather than mutable: the view changes every time the scene target is resized,
        // and a mutable variable cannot be re-pointed once it has been set.
        const Diligent::ShaderResourceVariableDesc variables[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_SceneColor", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };
        pipelineInfo.PSODesc.ResourceLayout.Variables = variables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(std::size(variables));

        _device->CreateGraphicsPipelineState(pipelineInfo, &_compositePipeline);
        if (!_compositePipeline)
        {
            Logger::LogError("Diligent failed to create the composite pipeline state");
            return;
        }

        _compositePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "PostConstants")->Set(_postConstants);
        _compositePipeline->CreateShaderResourceBinding(&_compositeBinding, true);
    }

    Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> DiligentRenderer::createShaderSources() const
    {
        Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> engineSources;
        const std::string shaderDirectory = std::string(GetApplicationDirectory()) + SHADER_DIRECTORY;
        Diligent::GetEngineFactoryOpenGL()->CreateDefaultShaderSourceStreamFactory(shaderDirectory.c_str(), &engineSources);

        // DiligentFX's .fxh files are compiled into the library rather than shipped next to the
        // executable, so an #include of one only resolves through its own factory.
        return Diligent::CreateCompoundShaderSourceFactory(
            {&Diligent::DiligentFXShaderSourceStreamFactory::GetInstance(), engineSources});
    }

    void DiligentRenderer::createSkyPipelines()
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

    void DiligentRenderer::createIblPipelines()
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

    void DiligentRenderer::createAmbientFallbacks()
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

    void DiligentRenderer::precomputeBrdfLut()
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
        _post.encoding = colorSpace == OutputColorSpace::Linear ? LINEAR_ENCODE_EXPONENT : GAMMA_ENCODE_EXPONENT;
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
        if (!_context) return;

        // raylib has been drawing through the same context since the last frame ended, so
        // whatever Diligent remembers about the GL state it left behind is stale.
        _context->InvalidateState();

        // Ahead of everything that binds the scene target, because baking a cube binds six
        // targets of its own - and ahead of the early return below, because a decode that has
        // landed should become usable whether or not there is a target to draw into this frame.
        finalizeCubemaps();

        if (!_sceneColor) return;

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

        const auto clear = toSceneLinear(_clearColor, _post.encoding);
        _context->ClearRenderTarget(renderTarget, &clear.x, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->ClearDepthStencil(depthStencil, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        submitDraws();
        drawSkybox();
        composite();

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

    void DiligentRenderer::composite()
    {
        if (!_compositePipeline || !_sceneOutput) return;

        auto *renderTarget = _sceneOutput->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        // Nothing is cleared: the triangle covers the whole target, so every pixel is written.
        _context->SetRenderTargets(1, &renderTarget, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const PostConstants constants{
            .tonemap = {static_cast<float>(_post.tonemap), _post.exposure, _post.whitePoint, 0.0f},
            .grading = {_post.brightness, _post.contrast, _post.saturation, _post.encoding}
        };
        uploadConstants(_postConstants, &constants, sizeof(constants));

        _context->SetPipelineState(_compositePipeline);
        _compositeBinding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_SceneColor")
                         ->Set(_sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        _context->CommitShaderResources(_compositeBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Diligent::DrawAttribs drawAttribs;
        drawAttribs.NumVertices = 3;
        drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        _context->Draw(drawAttribs);
    }

    void DiligentRenderer::yieldToRaylib()
    {
        _context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->InvalidateState();
        _context->Flush();

        // rlgl tracks the GL state it expects in software and only issues the calls it thinks
        // are needed, so every piece of state a pipeline sets differently has to be put back by
        // hand. Depth writes still match rlgl's own defaults; blending, the depth test and
        // both halves of the face-culling state do not.
        rlEnableColorBlend();
        rlDisableDepthTest();
        rlEnableBackfaceCulling();
        // Which winding faces front is the other half, and restoring the cull test without it
        // is worse than restoring neither: raylib winds counter-clockwise, so a pipeline that
        // left GL_CW makes every triangle raylib draws a back face. Lines are not subject to
        // the cull test at all, which is what makes that failure look like "only lines draw".
        glFrontFace(GL_CCW);

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

    void DiligentRenderer::setEnvironment(const EnvironmentSettings &settings)
    {
        _clearColor = settings.background.color;
        _sky = settings.background.sky;
        _skyRotation = settings.background.rotation;
        _skyEnergy = settings.background.energy;
        _skyBlur = settings.background.skyBlur;
        _ambientColor = settings.ambient.color;
        _ambientEnergy = settings.ambient.energy;
        _ambientMap = settings.ambient.map;

        // The encoding is deliberately not set here: it belongs to the project's output colour
        // space, which is pushed once at startup and is no part of the environment.
        _post.tonemap = settings.tonemap.mode;
        _post.exposure = settings.tonemap.exposure;
        _post.whitePoint = settings.tonemap.white;
        _post.brightness = settings.finalColor.brightness;
        _post.contrast = settings.finalColor.contrast;
        _post.saturation = settings.finalColor.saturation;
    }

    CubemapHandle DiligentRenderer::loadCubemap(const std::string &path, const SkyboxCubemapParameters &settings)
    {
        if (!_equirectBakePipeline) return {};

        const auto ground = toSceneLinear(settings.groundAlbedo, _post.encoding);
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

    bool DiligentRenderer::isCubemapReady(const CubemapHandle handle) const
    {
        const auto *slot = _cubemaps.get(handle);
        return slot != nullptr && slot->texture;
    }

    void DiligentRenderer::finalizeCubemaps()
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

    CubemapHandle DiligentRenderer::createProceduralSky(const int size, const SkyboxProceduralParameters &sky)
    {
        if (!_skyBakePipeline) return {};

        // The light's forward is the direction sunlight travels, so the direction *to* the sun
        // is its opposite - and that is what both the model's elevation and the disc need.
        const auto toSun = Vector3Normalize(Vector3Negate(sky.sunDirection));
        const float elevation = std::asin(std::clamp(toSun.y, -1.0f, 1.0f));

        const auto ground = toSceneLinear(sky.groundAlbedo, _post.encoding);
        const auto cooked = cookHosekWilkieSky(sky.turbidity, Vector3{ground.x, ground.y, ground.z}, elevation);

        const auto tint = toSceneLinear(sky.skyTint, _post.encoding);
        const auto sunColor = toSceneLinear(sky.sunColor, _post.encoding);
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

    void DiligentRenderer::bakeCubemap(CubemapSlot &slot, const char *name, const int size,
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
            uploadConstants(_skyBakeConstants, &constants, sizeof(constants));

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

    void DiligentRenderer::drawSkybox()
    {
        if (!_skyboxPipeline) return;

        const auto *slot = _cubemaps.get(_sky);
        if (slot == nullptr || !slot->texture) return;

        // The inspector's rotation turns the sky; the shader turns the direction it is sampled
        // with, and those are opposites.
        const auto rotation = QuaternionInvert(_skyRotation);
        const auto mipCount = static_cast<float>(slot->texture->GetDesc().MipLevels);

        const SkyboxConstants constants{
            .inverseViewProjection = MatrixToFloatV(MatrixInvert(_viewProjection)),
            .cameraPosition = {_camera.position.x, _camera.position.y, _camera.position.z, 1.0f},
            .rotation = {rotation.x, rotation.y, rotation.z, rotation.w},
            .params = {_skyEnergy, std::clamp(_skyBlur, 0.0f, 1.0f) * std::max(mipCount - 1.0f, 0.0f), 0.0f, 0.0f}
        };
        uploadConstants(_skyboxConstants, &constants, sizeof(constants));

        _context->SetPipelineState(_skyboxPipeline);
        _skyboxBinding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Sky")
                      ->Set(slot->texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        _context->CommitShaderResources(_skyboxBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Diligent::DrawAttribs drawAttribs;
        drawAttribs.NumVertices = 3;
        drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        _context->Draw(drawAttribs);
    }

    void DiligentRenderer::destroyCubemap(const CubemapHandle handle)
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

    AmbientMapHandle DiligentRenderer::createAmbientMap(const CubemapHandle cubemap)
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

    Diligent::RefCntAutoPtr<Diligent::ITexture> DiligentRenderer::bakeIblCube(
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
                uploadConstants(_iblBakeConstants, &constants, sizeof(constants));

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

    void DiligentRenderer::bindAmbientMap(MaterialSlot &slot, const AmbientMapHandle map)
    {
        const auto *ambient = _ambientMaps.get(map);
        auto *irradiance = ambient != nullptr ? ambient->irradiance.RawPtr() : _ambientFallback.RawPtr();
        auto *prefiltered = ambient != nullptr ? ambient->prefiltered.RawPtr() : _ambientFallback.RawPtr();
        // Only when the fallback itself failed to build, which createAmbientFallbacks has
        // already reported. Leaving the variables unset costs a validation message per draw;
        // dereferencing nothing costs the process.
        if (irradiance == nullptr || prefiltered == nullptr) return;

        if (slot.irradiance != nullptr)
        {
            slot.irradiance->Set(irradiance->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        }
        if (slot.prefiltered != nullptr)
        {
            slot.prefiltered->Set(prefiltered->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        }
        slot.ambientMap = map;
    }

    void DiligentRenderer::destroyAmbientMap(const AmbientMapHandle handle)
    {
        // The slot owns both cubes, so clearing it releases them. A material binding still
        // holding one keeps it alive until the next draw notices the handle changed.
        _ambientMaps.remove(handle);
    }
} // namespace BreadEngine
