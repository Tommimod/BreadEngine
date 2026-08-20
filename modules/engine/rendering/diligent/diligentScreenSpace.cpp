#include "diligentScreenSpace.h"

#include <algorithm>

#include <Sampler.h>
#include <Shader.h>

#include "logger.h"

namespace BreadEngine {
    /// Mirrors the cbuffer occlusion.psh and occlusionBlur.psh both declare. One block for two
    /// shaders because they are two halves of one measurement and every frame uploads the whole
    /// thing anyway; float4-only because that is the only member layout the struct and the
    /// shader cannot drift apart over.
    struct OcclusionConstants
    {
        float16 viewProjection;
        float16 inverseViewProjection;
        Vector4 cameraPosition;
        Vector4 cameraForward;
        /// x is the radius, y the depth bias, z the intensity and w the exponent.
        Vector4 params;
        /// x is the sample count; yz is one texel of the target.
        Vector4 target;
    };

    /// Mirrors reflections.psh's cbuffer, float4-only like the block above it.
    struct ReflectionConstants
    {
        float16 viewProjection;
        float16 inverseViewProjection;
        Vector4 cameraPosition;
        Vector4 cameraForward;
        /// x is the step size, y the thickness, z the maximum distance and w the edge fade.
        Vector4 rayParams;
        /// x is the step budget, y the bisection budget.
        Vector4 rayLimits;
        Vector4 ambientColor;
        Vector4 ambientRotation;
        Vector4 ambientParams;
    };

    /// Compiles the fullscreen vertex shader every screen-space pass draws through, together
    /// with one pixel shader of the engine's own. Either can be null on return, which each
    /// caller reports before giving up on its pipeline.
    void createScreenSpaceShaders(Diligent::IRenderDevice *device, const char *pixelShaderFile, const char *name,
                                  Diligent::RefCntAutoPtr<Diligent::IShader> &vertexShader,
                                  Diligent::RefCntAutoPtr<Diligent::IShader> &pixelShader)
    {
        // Callers build several pipelines from one pair of handles, and creating into a handle
        // that still holds the previous shader is what Diligent calls an overwritten reference.
        vertexShader.Release();
        pixelShader.Release();

        const auto shaderSources = createShaderSources();

        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = shaderSources;

        const std::string vertexName = std::string(name) + " VS";
        shaderInfo.Desc = {vertexName.c_str(), Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.FilePath = "fullscreen.vsh";
        device->CreateShader(shaderInfo, &vertexShader);

        const std::string pixelName = std::string(name) + " PS";
        shaderInfo.Desc = {pixelName.c_str(), Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = pixelShaderFile;
        device->CreateShader(shaderInfo, &pixelShader);
    }

    /// Points one of @p binding's texture variables at @p view, where the shader still has
    /// that variable. It can lose one without anything failing: the sources are compiled from
    /// disk at runtime, and a term the pixel shader stops depending on takes its textures with
    /// it through dead-code elimination - after which there is nothing left to point at.
    void bindTexture(Diligent::IShaderResourceBinding *binding, const char *name, Diligent::ITextureView *view)
    {
        if (auto *variable = binding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, name)) variable->Set(view);
    }

    /// The pipeline description every pass here starts from: one fullscreen triangle, no depth
    /// buffer of any kind, and no culling to keep it away from the state rlgl believes it owns
    /// more than necessary.
    Diligent::GraphicsPipelineStateCreateInfo screenSpacePipelineInfo(Diligent::TEXTURE_FORMAT format)
    {
        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;

        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = format;
        graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
        graphics.DepthStencilDesc.DepthEnable = Diligent::False;

        return pipelineInfo;
    }

    void ScreenSpaceEffects::initialize(Diligent::IRenderDevice *device, Diligent::IDeviceContext *context)
    {
        _device = device;
        _context = context;

        createOcclusionFallback();
        createOcclusionPipelines();
        createReflectionPipeline();
    }

    void ScreenSpaceEffects::shutdown()
    {
        releaseTargets();
        _occlusionFallback.Release();

        _occlusionPipeline.Release();
        _occlusionBinding.Release();
        _occlusionBlurPipeline.Release();
        _occlusionBlurBinding.Release();
        _occlusionApplyPipeline.Release();
        _occlusionApplyBinding.Release();
        _occlusionConstants.Release();

        _reflectionPipeline.Release();
        _reflectionBinding.Release();
        _reflectionConstants.Release();

        _context = nullptr;
        _device = nullptr;
    }

    void ScreenSpaceEffects::setSettings(const EnvironmentSettings &settings)
    {
        _occlusionState.enabled = settings.ssao.enabled;
        _occlusionState.intensity = settings.ssao.intensity;
        _occlusionState.power = settings.ssao.power;
        _occlusionState.radius = settings.ssao.radius;
        _occlusionState.bias = settings.ssao.bias;
        _occlusionState.sampleCount = settings.ssao.sampleCount;

        _reflectionState.enabled = settings.ssr.enabled;
        _reflectionState.stepSize = settings.ssr.stepSize;
        _reflectionState.thickness = settings.ssr.thickness;
        _reflectionState.maxDistance = settings.ssr.maxDistance;
        _reflectionState.edgeFade = settings.ssr.edgeFade;
        _reflectionState.maxRaySteps = settings.ssr.maxRaySteps;
        _reflectionState.binarySteps = settings.ssr.binarySteps;
    }

    void ScreenSpaceEffects::releaseTargets()
    {
        // Whatever the reflection pass was going to read is one of the two being dropped.
        _ambientAccess = nullptr;
        _occlusionRaw.Release();
        _occlusion.Release();
    }

    void ScreenSpaceEffects::createOcclusionFallback()
    {
        constexpr Diligent::Uint8 unoccluded = 255;

        Diligent::TextureDesc desc;
        desc.Name = "Ambient access fallback";
        desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.Format = OCCLUSION_FORMAT;
        desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

        Diligent::TextureSubResData level{&unoccluded, sizeof(unoccluded)};
        const Diligent::TextureData data{&level, 1};
        _device->CreateTexture(desc, &data, &_occlusionFallback);

        restoreRaylibPixelStore();
        if (!_occlusionFallback) Logger::LogError("Diligent failed to create the ambient access fallback");
    }

    void ScreenSpaceEffects::createOcclusionPipelines()
    {
        Diligent::BufferDesc constantsDesc;
        constantsDesc.Name = "Occlusion constants";
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        constantsDesc.Size = sizeof(OcclusionConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_occlusionConstants);

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
        createScreenSpaceShaders(_device, "occlusion.psh", "Occlusion", vertexShader, pixelShader);
        if (!vertexShader || !pixelShader)
        {
            Logger::LogError("Diligent failed to compile the ambient occlusion shaders");
            return;
        }

        // Every texture here is one of the scene's own, and those are rebuilt whenever the
        // target resizes - so nothing may be bound for the pipeline's lifetime.
        const Diligent::ShaderResourceVariableDesc occlusionVariables[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_SceneDepth", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {Diligent::SHADER_TYPE_PIXEL, "g_SceneSurface", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };

        auto pipelineInfo = screenSpacePipelineInfo(OCCLUSION_FORMAT);
        pipelineInfo.PSODesc.Name = "Ambient occlusion";
        pipelineInfo.pVS = vertexShader;
        pipelineInfo.pPS = pixelShader;
        pipelineInfo.PSODesc.ResourceLayout.Variables = occlusionVariables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(std::size(occlusionVariables));
        _device->CreateGraphicsPipelineState(pipelineInfo, &_occlusionPipeline);

        if (!_occlusionPipeline)
        {
            Logger::LogError("Diligent failed to create the ambient occlusion pipeline state");
            return;
        }
        _occlusionPipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "OcclusionConstants")->Set(_occlusionConstants);
        _occlusionPipeline->CreateShaderResourceBinding(&_occlusionBinding, true);

        createScreenSpaceShaders(_device, "occlusionBlur.psh", "Occlusion blur", vertexShader, pixelShader);
        if (!vertexShader || !pixelShader)
        {
            Logger::LogError("Diligent failed to compile the ambient occlusion blur shaders");
            return;
        }

        const Diligent::ShaderResourceVariableDesc blurVariables[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_SceneDepth", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {Diligent::SHADER_TYPE_PIXEL, "g_Occlusion", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };

        pipelineInfo.PSODesc.Name = "Ambient occlusion blur";
        pipelineInfo.pVS = vertexShader;
        pipelineInfo.pPS = pixelShader;
        pipelineInfo.PSODesc.ResourceLayout.Variables = blurVariables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(std::size(blurVariables));
        _device->CreateGraphicsPipelineState(pipelineInfo, &_occlusionBlurPipeline);

        if (!_occlusionBlurPipeline)
        {
            Logger::LogError("Diligent failed to create the ambient occlusion blur pipeline state");
            return;
        }
        _occlusionBlurPipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "OcclusionConstants")->Set(_occlusionConstants);
        _occlusionBlurPipeline->CreateShaderResourceBinding(&_occlusionBlurBinding, true);

        createScreenSpaceShaders(_device, "occlusionApply.psh", "Occlusion apply", vertexShader, pixelShader);
        if (!vertexShader || !pixelShader)
        {
            Logger::LogError("Diligent failed to compile the ambient occlusion apply shaders");
            return;
        }

        const Diligent::ShaderResourceVariableDesc applyVariables[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_SceneAmbient", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {Diligent::SHADER_TYPE_PIXEL, "g_Occlusion", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };

        auto applyInfo = screenSpacePipelineInfo(SCENE_COLOR_FORMAT);
        applyInfo.PSODesc.Name = "Ambient occlusion apply";
        applyInfo.pVS = vertexShader;
        applyInfo.pPS = pixelShader;
        applyInfo.PSODesc.ResourceLayout.Variables = applyVariables;
        applyInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(std::size(applyVariables));

        // The pass writes the light to remove rather than the colour to keep, so the blend
        // subtracts it from what is already there. Alpha is left alone: the scene pass owns it.
        auto &blend = applyInfo.GraphicsPipeline.BlendDesc.RenderTargets[0];
        blend.BlendEnable = Diligent::True;
        blend.SrcBlend = Diligent::BLEND_FACTOR_ONE;
        blend.DestBlend = Diligent::BLEND_FACTOR_ONE;
        blend.BlendOp = Diligent::BLEND_OPERATION_REV_SUBTRACT;
        blend.RenderTargetWriteMask = Diligent::COLOR_MASK_RGB;

        _device->CreateGraphicsPipelineState(applyInfo, &_occlusionApplyPipeline);
        if (!_occlusionApplyPipeline)
        {
            Logger::LogError("Diligent failed to create the ambient occlusion apply pipeline state");
            return;
        }
        _occlusionApplyPipeline->CreateShaderResourceBinding(&_occlusionApplyBinding, true);
    }

    void ScreenSpaceEffects::createReflectionPipeline()
    {
        Diligent::BufferDesc constantsDesc;
        constantsDesc.Name = "Reflection constants";
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        constantsDesc.Size = sizeof(ReflectionConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_reflectionConstants);

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
        createScreenSpaceShaders(_device, "reflections.psh", "Reflections", vertexShader, pixelShader);
        if (!vertexShader || !pixelShader)
        {
            Logger::LogError("Diligent failed to compile the reflection shaders");
            return;
        }

        // The environment cube is dynamic for the reason the scene pass's is: a rebaked sky
        // replaces the texture rather than editing it, which neither of the other kinds allows.
        const Diligent::ShaderResourceVariableDesc variables[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_SceneColor", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {Diligent::SHADER_TYPE_PIXEL, "g_SceneDepth", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {Diligent::SHADER_TYPE_PIXEL, "g_SceneSurface", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {Diligent::SHADER_TYPE_PIXEL, "g_Occlusion", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
            {Diligent::SHADER_TYPE_PIXEL, "g_Prefiltered", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };

        auto pipelineInfo = screenSpacePipelineInfo(SCENE_COLOR_FORMAT);
        pipelineInfo.PSODesc.Name = "Screen space reflections";
        pipelineInfo.pVS = vertexShader;
        pipelineInfo.pPS = pixelShader;
        pipelineInfo.PSODesc.ResourceLayout.Variables = variables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(std::size(variables));

        // The pass writes the difference between the two reflections rather than either of
        // them, so it is added to the frame - and that difference is signed, which only the
        // floating point scene target can hold.
        auto &blend = pipelineInfo.GraphicsPipeline.BlendDesc.RenderTargets[0];
        blend.BlendEnable = Diligent::True;
        blend.SrcBlend = Diligent::BLEND_FACTOR_ONE;
        blend.DestBlend = Diligent::BLEND_FACTOR_ONE;
        blend.RenderTargetWriteMask = Diligent::COLOR_MASK_RGB;

        _device->CreateGraphicsPipelineState(pipelineInfo, &_reflectionPipeline);
        if (!_reflectionPipeline)
        {
            Logger::LogError("Diligent failed to create the reflection pipeline state");
            return;
        }

        _reflectionPipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "ReflectionConstants")->Set(_reflectionConstants);
        _reflectionPipeline->CreateShaderResourceBinding(&_reflectionBinding, true);
    }

    void ScreenSpaceEffects::resizeOcclusionTargets(const Diligent::ITexture *sceneColor)
    {
        const auto &scene = sceneColor->GetDesc();
        if (_occlusion && _occlusion->GetDesc().Width == scene.Width && _occlusion->GetDesc().Height == scene.Height)
        {
            return;
        }

        releaseTargets();

        Diligent::TextureDesc desc;
        desc.Name = "Ambient access";
        desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        desc.Width = scene.Width;
        desc.Height = scene.Height;
        desc.MipLevels = 1;
        desc.Format = OCCLUSION_FORMAT;
        desc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
        _device->CreateTexture(desc, nullptr, &_occlusionRaw);
        _device->CreateTexture(desc, nullptr, &_occlusion);

        if (!_occlusionRaw || !_occlusion)
        {
            Logger::LogError("Diligent failed to create the ambient occlusion targets");
            releaseTargets();
            return;
        }

        // Every reader of these two walks them a texel at a time, at the same resolution they
        // were written: the blur steps by whole texels and the two passes after it read the
        // pixel they are shading. Filtering any of that would only blur the answer twice.
        Diligent::SamplerDesc samplerDesc;
        samplerDesc.MinFilter = samplerDesc.MagFilter = samplerDesc.MipFilter = Diligent::FILTER_TYPE_POINT;
        samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
        Diligent::RefCntAutoPtr<Diligent::ISampler> sampler;
        _device->CreateSampler(samplerDesc, &sampler);
        _occlusionRaw->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sampler);
        _occlusion->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE)->SetSampler(sampler);
    }

    void ScreenSpaceEffects::drawAmbientOcclusion(Diligent::ITexture *sceneColor, Diligent::ITexture *sceneAmbient,
                                                  Diligent::ITexture *sceneDepth, Diligent::ITexture *sceneSurface,
                                                  const CameraView &camera, const Matrix &viewProjection)
    {
        // Set before the effect has a chance to give up, so that every way out of this leaves
        // the reflection pass reading a texture that says the frame is not occluded at all.
        _ambientAccess = _occlusionFallback;

        if (!_occlusionPipeline || !_occlusionBlurPipeline || !_occlusionApplyPipeline) return;
        if (!_occlusionState.enabled || _occlusionState.sampleCount <= 0) return;

        resizeOcclusionTargets(sceneColor);
        if (!_occlusion) return;

        const auto &scene = sceneColor->GetDesc();
        const auto forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        const OcclusionConstants constants{
            .viewProjection = MatrixToFloatV(viewProjection),
            .inverseViewProjection = MatrixToFloatV(MatrixInvert(viewProjection)),
            .cameraPosition = {camera.position.x, camera.position.y, camera.position.z, 1.0f},
            .cameraForward = {forward.x, forward.y, forward.z, 0.0f},
            .params = {
                std::max(_occlusionState.radius, 1e-4f), _occlusionState.bias,
                std::max(_occlusionState.intensity, 0.0f), std::max(_occlusionState.power, 1e-4f)
            },
            .target = {
                static_cast<float>(_occlusionState.sampleCount),
                1.0f / static_cast<float>(scene.Width), 1.0f / static_cast<float>(scene.Height), 0.0f
            }
        };
        uploadConstants(_context, _occlusionConstants, &constants, sizeof(constants));

        Diligent::DrawAttribs drawAttribs;
        drawAttribs.NumVertices = 3;
        drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;

        auto *rawTarget = _occlusionRaw->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        _context->SetRenderTargets(1, &rawTarget, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->SetPipelineState(_occlusionPipeline);
        bindTexture(_occlusionBinding, "g_SceneDepth", sceneDepth->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        bindTexture(_occlusionBinding, "g_SceneSurface", sceneSurface->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        _context->CommitShaderResources(_occlusionBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->Draw(drawAttribs);

        auto *blurTarget = _occlusion->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        _context->SetRenderTargets(1, &blurTarget, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->SetPipelineState(_occlusionBlurPipeline);
        bindTexture(_occlusionBlurBinding, "g_SceneDepth", sceneDepth->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        bindTexture(_occlusionBlurBinding, "g_Occlusion", _occlusionRaw->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        _context->CommitShaderResources(_occlusionBlurBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->Draw(drawAttribs);

        // The scene's depth is a shader resource of the two passes above and an attachment of
        // nothing here: this one writes colour alone, so the depth buffer stays unbound.
        auto *colorTarget = sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        _context->SetRenderTargets(1, &colorTarget, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->SetPipelineState(_occlusionApplyPipeline);
        bindTexture(_occlusionApplyBinding, "g_SceneAmbient", sceneAmbient->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        bindTexture(_occlusionApplyBinding, "g_Occlusion", _occlusion->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        _context->CommitShaderResources(_occlusionApplyBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->Draw(drawAttribs);

        _ambientAccess = _occlusion;
    }

    void ScreenSpaceEffects::drawReflections(Diligent::ITexture *sceneColor, Diligent::ITexture *sceneDepth,
                                             Diligent::ITexture *sceneSurface, const CameraView &camera,
                                             const Matrix &viewProjection, const AmbientLookup &ambient)
    {
        if (!_reflectionPipeline || !_reflectionState.enabled) return;
        // The pass reproduces what the scene pass reflected before replacing it, and with no
        // cube to reproduce it from there is nothing it could subtract.
        if (ambient.prefiltered == nullptr || _ambientAccess == nullptr) return;

        const auto forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        const ReflectionConstants constants{
            .viewProjection = MatrixToFloatV(viewProjection),
            .inverseViewProjection = MatrixToFloatV(MatrixInvert(viewProjection)),
            .cameraPosition = {camera.position.x, camera.position.y, camera.position.z, 1.0f},
            .cameraForward = {forward.x, forward.y, forward.z, 0.0f},
            .rayParams = {
                std::max(_reflectionState.stepSize, 1e-4f), std::max(_reflectionState.thickness, 0.0f),
                std::max(_reflectionState.maxDistance, 0.0f), std::clamp(_reflectionState.edgeFade, 1e-4f, 1.0f)
            },
            .rayLimits = {
                static_cast<float>(std::max(_reflectionState.maxRaySteps, 0)),
                static_cast<float>(std::max(_reflectionState.binarySteps, 0)), 0.0f, 0.0f
            },
            .ambientColor = ambient.color,
            .ambientRotation = ambient.rotation,
            .ambientParams = ambient.params
        };
        uploadConstants(_context, _reflectionConstants, &constants, sizeof(constants));

        auto *colorTarget = sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        _context->SetRenderTargets(1, &colorTarget, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        _context->SetPipelineState(_reflectionPipeline);

        // The scene's colour is the reflected radiance and the target of this pass at once. It
        // works because a ray only ever lands on a pixel other than the one being shaded, and
        // because the two reflections being swapped are both of the frame as it stands.
        bindTexture(_reflectionBinding, "g_SceneColor", sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        bindTexture(_reflectionBinding, "g_SceneDepth", sceneDepth->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        bindTexture(_reflectionBinding, "g_SceneSurface", sceneSurface->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        bindTexture(_reflectionBinding, "g_Occlusion", _ambientAccess->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        bindTexture(_reflectionBinding, "g_Prefiltered", ambient.prefiltered);
        _context->CommitShaderResources(_reflectionBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Diligent::DrawAttribs drawAttribs;
        drawAttribs.NumVertices = 3;
        drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        _context->Draw(drawAttribs);
    }
} // namespace BreadEngine
