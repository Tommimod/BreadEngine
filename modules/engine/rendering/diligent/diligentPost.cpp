#include "diligentInternal.h"

#include <algorithm>
#include <cmath>

#include <Shader.h>

#include "logger.h"

namespace BreadEngine {
    /// Levels of the bloom chain, and so how far the glow can reach: the first is half the
    /// scene's size and each after it half the one before, so the widest blur the effect can
    /// produce is the coarsest level stretched back over the frame. Capped because a level
    /// small enough costs two passes to contribute a flat colour.
    constexpr int BLOOM_MAX_LEVELS = 8;

    /// The smallest either side of a chain level is allowed to get. Below this a level is
    /// shaped more by the clamping at its own edges than by the image in it.
    constexpr int BLOOM_MIN_LEVEL_SIZE = 8;

    /// How much of a bloom level is replaced by the blur of the level below it on the way back
    /// up the chain. A half weights every scale equally, and - the reason this is a blend
    /// rather than a sum - it leaves the total weight at one: a chain built from a flat colour
    /// comes back as that same colour however long it is, so the level count decides how far
    /// the glow spreads and never how bright it is.
    constexpr float BLOOM_UPSAMPLE_BLEND = 0.5f;

    /// Mirrors composite.psh's cbuffer, and float4-only for the same reason the scene's blocks
    /// are: it is the only member layout the struct and the shader cannot drift apart over.
    struct PostConstants
    {
        /// x is the TonemapMode, y the exposure, z the reference white point.
        Vector4 tonemap;
        /// x is brightness, y contrast, z saturation, w the exponent the result leaves through.
        Vector4 grading;
    };

    /// Mirrors fog.psh's cbuffer, float4-only like the blocks above it.
    struct FogConstants
    {
        float16 inverseViewProjection;
        Vector4 cameraPosition;
        /// rgb is the fog colour in the scene's linear space, a how much of the fog reaches
        /// the pixels no geometry claimed.
        Vector4 color;
        /// x is the FogMode, y the distance the linear mode starts fogging at, z the reciprocal
        /// of the span it takes to reach full, w the two exponential modes' density.
        Vector4 params;
        /// x is the world height the fog is at full density up to, y how fast it thins above.
        Vector4 height;
    };

    /// Mirrors the cbuffer bloomDownsample.psh and bloomUpsample.psh both declare. One block
    /// for two shaders because they are two halves of one effect and every step of it uploads
    /// the whole thing anyway; float4-only like the blocks above it.
    struct BloomConstants
    {
        /// x is the brightness a pixel has to carry before it glows, y how wide the soft
        /// shoulder below that is, z the radius of the upsample tent in the source's texels,
        /// w whether this step is the one that filters by brightness.
        Vector4 filter;
        /// x is how strongly the glow reaches its target: one while the chain sums into
        /// itself, the authored intensity on the step that reaches the scene.
        Vector4 combine;
    };

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

    void DiligentRenderer::createFogPipeline()
    {
        if (!_device) return;

        Diligent::BufferDesc constantsDesc;
        constantsDesc.Name = "Fog constants";
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        constantsDesc.Size = sizeof(FogConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_fogConstants);

        const auto shaderSources = createShaderSources();

        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = shaderSources;

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        shaderInfo.Desc = {"Fog VS", Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.FilePath = "fullscreen.vsh";
        _device->CreateShader(shaderInfo, &vertexShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
        shaderInfo.Desc = {"Fog PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "fog.psh";
        _device->CreateShader(shaderInfo, &pixelShader);

        if (!vertexShader || !pixelShader)
        {
            Logger::LogError("Diligent failed to compile the fog shaders");
            return;
        }

        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
        pipelineInfo.PSODesc.Name = "Scene fog";
        pipelineInfo.pVS = vertexShader;
        pipelineInfo.pPS = pixelShader;

        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = SCENE_COLOR_FORMAT;
        graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
        graphics.DepthStencilDesc.DepthEnable = Diligent::False;

        auto &blend = graphics.BlendDesc.RenderTargets[0];
        blend.BlendEnable = Diligent::True;
        blend.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
        blend.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
        blend.RenderTargetWriteMask = Diligent::COLOR_MASK_RGB;

        const Diligent::ShaderResourceVariableDesc variables[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_SceneDepth", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };
        pipelineInfo.PSODesc.ResourceLayout.Variables = variables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(std::size(variables));

        _device->CreateGraphicsPipelineState(pipelineInfo, &_fogPipeline);
        if (!_fogPipeline)
        {
            Logger::LogError("Diligent failed to create the fog pipeline state");
            return;
        }

        _fogPipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "FogConstants")->Set(_fogConstants);
        _fogPipeline->CreateShaderResourceBinding(&_fogBinding, true);
    }

    void DiligentRenderer::createBloomPipelines()
    {
        if (!_device) return;

        Diligent::BufferDesc constantsDesc;
        constantsDesc.Name = "Bloom constants";
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        constantsDesc.Size = sizeof(BloomConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_bloomConstants);

        const auto shaderSources = createShaderSources();

        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = shaderSources;

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        shaderInfo.Desc = {"Bloom VS", Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.FilePath = "fullscreen.vsh";
        _device->CreateShader(shaderInfo, &vertexShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> downsampleShader;
        shaderInfo.Desc = {"Bloom downsample PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "bloomDownsample.psh";
        _device->CreateShader(shaderInfo, &downsampleShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> upsampleShader;
        shaderInfo.Desc = {"Bloom upsample PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = "bloomUpsample.psh";
        _device->CreateShader(shaderInfo, &upsampleShader);

        if (!vertexShader || !downsampleShader || !upsampleShader)
        {
            Logger::LogError("Diligent failed to compile the bloom shaders");
            return;
        }

        // Every kernel here reads between its source's texels - that is what halving and
        // doubling a resolution means - so the source has to be filtered rather than point
        // sampled. Immutable rather than set on the view, because the scene target is one of
        // the sources and the composite reads that same view one texel to one pixel: a
        // pipeline's immutable sampler is written into the binding when it is created and is
        // deliberately left alone when the texture behind it is set, so the two coexist.
        Diligent::SamplerDesc sourceSampler;
        sourceSampler.MinFilter = sourceSampler.MagFilter = sourceSampler.MipFilter = Diligent::FILTER_TYPE_LINEAR;
        sourceSampler.AddressU = sourceSampler.AddressV = sourceSampler.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
        const Diligent::ImmutableSamplerDesc immutableSamplers[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_BloomSource", sourceSampler}
        };

        // Dynamic because every step of the chain points it somewhere else.
        const Diligent::ShaderResourceVariableDesc variables[]{
            {Diligent::SHADER_TYPE_PIXEL, "g_BloomSource", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };

        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
        pipelineInfo.pVS = vertexShader;
        pipelineInfo.PSODesc.ResourceLayout.Variables = variables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(std::size(variables));
        pipelineInfo.PSODesc.ResourceLayout.ImmutableSamplers = immutableSamplers;
        pipelineInfo.PSODesc.ResourceLayout.NumImmutableSamplers = static_cast<Diligent::Uint32>(std::size(immutableSamplers));

        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = SCENE_COLOR_FORMAT;
        graphics.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        graphics.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
        graphics.DepthStencilDesc.DepthEnable = Diligent::False;

        pipelineInfo.PSODesc.Name = "Bloom downsample";
        pipelineInfo.pPS = downsampleShader;
        _device->CreateGraphicsPipelineState(pipelineInfo, &_bloomDownsamplePipeline);

        pipelineInfo.PSODesc.Name = "Bloom upsample";
        pipelineInfo.pPS = upsampleShader;
        auto &blend = graphics.BlendDesc.RenderTargets[0];
        blend.BlendEnable = Diligent::True;
        // The alpha the shader leaves is a blend factor for the combine below and nothing
        // else, so no target here ever wants it written.
        blend.RenderTargetWriteMask = Diligent::COLOR_MASK_RGB;
        // The shader hands over the blur already scaled by the blend weight and that weight
        // in the alpha, so this is exactly an interpolation between a level and the blur of
        // the one below it - which is what keeps the chain's total weight at one.
        blend.SrcBlend = Diligent::BLEND_FACTOR_ONE;
        blend.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
        _device->CreateGraphicsPipelineState(pipelineInfo, &_bloomUpsamplePipeline);

        // The combines, in BloomMode's own order past Disabled. All three receive the glow
        // already scaled by the authored intensity, with that intensity in the alpha, and
        // differ only in what they do with it.
        const Diligent::BLEND_FACTOR sourceFactors[BLOOM_BLEND_MODE_COUNT]{
            // Mix interpolates toward the glow, so what the scene keeps is the rest of it.
            Diligent::BLEND_FACTOR_ONE,
            Diligent::BLEND_FACTOR_ONE,
            // Screen adds only into the headroom a pixel has left, which is what keeps an
            // already-bright area from blowing out further while a dark one still lights up.
            Diligent::BLEND_FACTOR_INV_DEST_COLOR
        };
        const Diligent::BLEND_FACTOR destinationFactors[BLOOM_BLEND_MODE_COUNT]{
            Diligent::BLEND_FACTOR_INV_SRC_ALPHA,
            Diligent::BLEND_FACTOR_ONE,
            Diligent::BLEND_FACTOR_ONE
        };

        pipelineInfo.PSODesc.Name = "Bloom combine";
        for (size_t mode = 0; mode < BLOOM_BLEND_MODE_COUNT; ++mode)
        {
            blend.SrcBlend = sourceFactors[mode];
            blend.DestBlend = destinationFactors[mode];
            _device->CreateGraphicsPipelineState(pipelineInfo, &_bloomCombinePipelines[mode]);
            if (!_bloomCombinePipelines[mode])
            {
                Logger::LogError("Diligent failed to create a bloom combine pipeline state");
                return;
            }
        }

        if (!_bloomDownsamplePipeline || !_bloomUpsamplePipeline)
        {
            Logger::LogError("Diligent failed to create the bloom pipeline states");
            return;
        }

        _bloomDownsamplePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "BloomConstants")->Set(_bloomConstants);
        _bloomDownsamplePipeline->CreateShaderResourceBinding(&_bloomDownsampleBinding, true);
        _bloomUpsamplePipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "BloomConstants")->Set(_bloomConstants);
        _bloomUpsamplePipeline->CreateShaderResourceBinding(&_bloomUpsampleBinding, true);

        for (size_t mode = 0; mode < BLOOM_BLEND_MODE_COUNT; ++mode)
        {
            _bloomCombinePipelines[mode]->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "BloomConstants")->Set(_bloomConstants);
            _bloomCombinePipelines[mode]->CreateShaderResourceBinding(&_bloomCombineBindings[mode], true);
        }
    }

    void DiligentRenderer::drawFog()
    {
        if (!_fogPipeline || _fog.mode == FogMode::Disabled) return;

        auto *renderTarget = _sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        _context->SetRenderTargets(1, &renderTarget, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const auto color = toSceneLinear(_fog.color, _post.encoding);
        const FogConstants constants{
            .inverseViewProjection = MatrixToFloatV(MatrixInvert(_viewProjection)),
            .cameraPosition = {_camera.position.x, _camera.position.y, _camera.position.z, 1.0f},
            .color = {color.x, color.y, color.z, std::clamp(_fog.skyAffect, 0.0f, 1.0f)},
            .params = {
                static_cast<float>(_fog.mode), _fog.start,
                1.0f / std::max(_fog.end - _fog.start, 1e-4f), _fog.density
            },
            .height = {_fog.height, std::max(_fog.heightFalloff, 0.0f), 0.0f, 0.0f}
        };
        uploadConstants(_fogConstants, &constants, sizeof(constants));

        _context->SetPipelineState(_fogPipeline);
        _fogBinding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_SceneDepth")
                   ->Set(_sceneDepth->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        _context->CommitShaderResources(_fogBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Diligent::DrawAttribs drawAttribs;
        drawAttribs.NumVertices = 3;
        drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        _context->Draw(drawAttribs);
    }

    void DiligentRenderer::resizeBloomChain()
    {
        const auto sceneWidth = static_cast<int>(_sceneColor->GetDesc().Width);
        const auto sceneHeight = static_cast<int>(_sceneColor->GetDesc().Height);

        // How many levels this target has room for. The chain starts at half the scene's size
        // - the finest level of a blur has nothing to gain from resolving what the scene
        // already did - and stops once a side would fall below what is still an image.
        int levelWidth = sceneWidth / 2;
        int levelHeight = sceneHeight / 2;
        int available = 0;
        while (available < BLOOM_MAX_LEVELS && std::min(levelWidth, levelHeight) >= BLOOM_MIN_LEVEL_SIZE)
        {
            ++available;
            levelWidth /= 2;
            levelHeight /= 2;
        }

        if (available == 0)
        {
            _bloomChain.clear();
            return;
        }

        const auto wanted = static_cast<size_t>(std::clamp(
            static_cast<int>(std::lround(_bloom.levels * static_cast<float>(available))), 1, available));

        if (_bloomChain.size() == wanted &&
            _bloomChain.front()->GetDesc().Width == static_cast<Diligent::Uint32>(sceneWidth / 2))
        {
            return;
        }

        _bloomChain.clear();
        _bloomChain.reserve(wanted);

        Diligent::TextureDesc levelDesc;
        levelDesc.Name = "Bloom level";
        levelDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        levelDesc.MipLevels = 1;
        // The same format the scene is shaded in: the glow is scene radiance blurred, and
        // clamping it to a displayable range here would take the brightest highlights - the
        // ones bloom exists for - out of the effect.
        levelDesc.Format = SCENE_COLOR_FORMAT;
        levelDesc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;

        levelWidth = sceneWidth / 2;
        levelHeight = sceneHeight / 2;
        for (size_t level = 0; level < wanted; ++level)
        {
            levelDesc.Width = static_cast<Diligent::Uint32>(levelWidth);
            levelDesc.Height = static_cast<Diligent::Uint32>(levelHeight);

            Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
            _device->CreateTexture(levelDesc, nullptr, &texture);
            if (!texture)
            {
                Logger::LogError("Diligent failed to create a bloom chain level");
                _bloomChain.clear();
                return;
            }

            _bloomChain.push_back(std::move(texture));
            levelWidth /= 2;
            levelHeight /= 2;
        }
    }

    void DiligentRenderer::drawBloomStep(Diligent::IPipelineState *pipeline, Diligent::IShaderResourceBinding *binding,
                                         Diligent::ITexture *source, Diligent::ITextureView *target)
    {
        // Every step covers its whole target, so nothing is cleared - and the viewport comes
        // with the target, which is what lets one triangle serve every level whatever its size.
        _context->SetRenderTargets(1, &target, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        _context->SetPipelineState(pipeline);
        binding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_BloomSource")
               ->Set(source->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
        _context->CommitShaderResources(binding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Diligent::DrawAttribs drawAttribs;
        drawAttribs.NumVertices = 3;
        drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        _context->Draw(drawAttribs);
    }

    void DiligentRenderer::drawBloom()
    {
        const auto blendMode = static_cast<size_t>(_bloom.mode);
        if (blendMode == 0 || blendMode > BLOOM_BLEND_MODE_COUNT || _bloom.intensity <= 0.0f) return;

        const auto blendIndex = blendMode - 1;
        // Guarded on the binding rather than the pipeline: the bindings are the last thing
        // createBloomPipelines makes and it makes them all together, so one of them standing
        // is the whole path standing - including the pipelines it returned early on.
        if (!_bloomCombineBindings[blendIndex]) return;

        resizeBloomChain();
        if (_bloomChain.empty()) return;

        BloomConstants constants{
            .filter = {
                std::max(_bloom.threshold, 0.0f), std::clamp(_bloom.softThreshold, 0.0f, 1.0f),
                std::max(_bloom.filterRadius, 0.0f), 1.0f
            },
            // Nothing scales what goes down the chain; the two steps that put one level onto
            // another rewrite this before they run.
            .combine = {1.0f, 0.0f, 0.0f, 0.0f}
        };

        // Down the chain. The first step reads the scene and is the only one that filters by
        // brightness - what glows has to be decided before any averaging, or a bright pixel
        // would arrive already diluted by its dark neighbours.
        for (size_t level = 0; level < _bloomChain.size(); ++level)
        {
            constants.filter.w = level == 0 ? 1.0f : 0.0f;
            uploadConstants(_bloomConstants, &constants, sizeof(constants));

            drawBloomStep(_bloomDownsamplePipeline, _bloomDownsampleBinding,
                          level == 0 ? _sceneColor : _bloomChain[level - 1],
                          _bloomChain[level]->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET));
        }

        // And back up it, each level interpolated toward the blur of the one below it so that
        // every scale contributes and the total stays at one. Nothing changes between these
        // steps, so the constants are uploaded once for all of them.
        constants.filter.w = 0.0f;
        constants.combine.x = BLOOM_UPSAMPLE_BLEND;
        uploadConstants(_bloomConstants, &constants, sizeof(constants));

        for (size_t level = _bloomChain.size() - 1; level > 0; --level)
        {
            drawBloomStep(_bloomUpsamplePipeline, _bloomUpsampleBinding, _bloomChain[level],
                          _bloomChain[level - 1]->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET));
        }

        // The last step up is the one that reaches the scene: the same tent over the finest
        // level, at the authored strength, through whichever blend the mode asks for.
        constants.combine.x = _bloom.intensity;
        uploadConstants(_bloomConstants, &constants, sizeof(constants));

        drawBloomStep(_bloomCombinePipelines[blendIndex], _bloomCombineBindings[blendIndex], _bloomChain.front(),
                      _sceneColor->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET));
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

        _fog.mode = settings.fog.mode;
        _fog.color = settings.fog.color;
        _fog.start = settings.fog.start;
        _fog.end = settings.fog.end;
        _fog.density = settings.fog.density;
        _fog.height = settings.fog.height;
        _fog.heightFalloff = settings.fog.heightFalloff;
        _fog.skyAffect = settings.fog.skyAffect;

        _bloom.mode = settings.bloom.mode;
        _bloom.levels = settings.bloom.levels;
        _bloom.intensity = settings.bloom.intensity;
        _bloom.threshold = settings.bloom.threshold;
        _bloom.softThreshold = settings.bloom.softThreshold;
        _bloom.filterRadius = settings.bloom.filterRadius;
    }
} // namespace BreadEngine
