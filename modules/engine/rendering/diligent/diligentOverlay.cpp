#include "diligentOverlay.h"

#include <EngineFactoryOpenGL.h>
#include <Shader.h>

#include "logger.h"

namespace BreadEngine {
    /// What the pixel shader of an effect that samples a texture has to call it. Resolved once
    /// per effect, so a draw costs a null check rather than a lookup by name.
    constexpr const char *OVERLAY_TEXTURE_NAME = "g_OverlayTexture";

    namespace {
        [[nodiscard]] Diligent::PRIMITIVE_TOPOLOGY toNative(const OverlayTopology topology)
        {
            return topology == OverlayTopology::Lines
                       ? Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST
                       : Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }

        /// Points a pipeline's static variable at @p buffer wherever the shader still has one.
        /// A block a stage does not read is eliminated by the compiler, which leaves the lookup
        /// returning null rather than failing - and every effect is offered more than it reads,
        /// because what it reads is its own business.
        void setStaticBuffer(Diligent::IPipelineState *pipeline, const Diligent::SHADER_TYPE stage,
                             const char *name, Diligent::IBuffer *buffer)
        {
            if (buffer == nullptr) return;
            if (auto *variable = pipeline->GetStaticVariableByName(stage, name)) variable->Set(buffer);
        }
    }

    void OverlayPass::initialize(Diligent::IRenderDevice *device, Diligent::IDeviceContext *context)
    {
        _device = device;
        _context = context;

        Diligent::BufferDesc constantsDesc;
        constantsDesc.Name = "Overlay frame constants";
        constantsDesc.Usage = Diligent::USAGE_DYNAMIC;
        constantsDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
        constantsDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        constantsDesc.Size = sizeof(FrameConstants);
        _device->CreateBuffer(constantsDesc, nullptr, &_frameConstants);
    }

    void OverlayPass::shutdown()
    {
        _meshes.clear();
        _effects.clear();
        _frameConstants.Release();

        _context = nullptr;
        _device = nullptr;
    }

    OverlayEffectHandle OverlayPass::createEffect(const OverlayEffectDesc &desc)
    {
        if (!_device) return {};

        // The client's own directory first, then the engine's - which is what lets a shader
        // shipped beside the editor include overlay.fxh from beside the engine.
        const std::string applicationDirectory = GetApplicationDirectory();
        const std::string searchPath = applicationDirectory + desc.shaderDirectory + ";" +
                                       applicationDirectory + SHADER_DIRECTORY;
        Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> sources;
        Diligent::GetEngineFactoryOpenGL()->CreateDefaultShaderSourceStreamFactory(searchPath.c_str(), &sources);

        Diligent::ShaderCreateInfo shaderInfo;
        shaderInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        shaderInfo.pShaderSourceStreamFactory = sources;

        Diligent::RefCntAutoPtr<Diligent::IShader> vertexShader;
        shaderInfo.Desc = {"Overlay VS", Diligent::SHADER_TYPE_VERTEX, true};
        shaderInfo.FilePath = desc.vertexShader.c_str();
        _device->CreateShader(shaderInfo, &vertexShader);

        Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
        shaderInfo.Desc = {"Overlay PS", Diligent::SHADER_TYPE_PIXEL, true};
        shaderInfo.FilePath = desc.pixelShader.c_str();
        _device->CreateShader(shaderInfo, &pixelShader);

        if (!vertexShader || !pixelShader)
        {
            Logger::LogError("Diligent failed to compile the overlay shaders in " + desc.shaderDirectory);
            return {};
        }

        EffectSlot slot;
        slot.parameterSize = desc.parameterSize;
        if (desc.parameterSize > 0)
        {
            Diligent::BufferDesc parametersDesc;
            parametersDesc.Name = "Overlay effect parameters";
            parametersDesc.Usage = Diligent::USAGE_DYNAMIC;
            parametersDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
            parametersDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
            // Rounded up to the vector a uniform block is laid out in: the block the shader
            // declares is padded to a multiple of it, so a buffer sized to the byte would be
            // smaller than the shader's own idea of the same block.
            parametersDesc.Size = (desc.parameterSize + 15u) & ~15u;
            _device->CreateBuffer(parametersDesc, nullptr, &slot.parameters);
        }

        // OverlayVertex's own order, interleaved in one buffer. The colour arrives as four
        // bytes and is normalized on the way in, which is what lets it stay a raylib Color on
        // the engine's side of the seam.
        constexpr Diligent::LayoutElement vertexLayout[]{
            {0, 0, 3, Diligent::VT_FLOAT32, Diligent::False}, // position
            {1, 0, 2, Diligent::VT_FLOAT32, Diligent::False}, // uv
            {2, 0, 4, Diligent::VT_UINT8, Diligent::True}, // color
        };

        Diligent::GraphicsPipelineStateCreateInfo pipelineInfo;
        pipelineInfo.PSODesc.Name = "Overlay effect";
        pipelineInfo.pVS = vertexShader;
        pipelineInfo.pPS = pixelShader;

        auto &graphics = pipelineInfo.GraphicsPipeline;
        graphics.NumRenderTargets = 1;
        graphics.RTVFormats[0] = SCENE_OUTPUT_FORMAT;
        graphics.DSVFormat = SCENE_DEPTH_FORMAT;
        graphics.PrimitiveTopology = toNative(desc.topology);
        graphics.RasterizerDesc.CullMode = desc.cullBackFaces ? Diligent::CULL_MODE_BACK : Diligent::CULL_MODE_NONE;
        // Set whether or not this effect culls: which winding faces front is the other half of
        // that state, and the engine's geometry is wound the way raylib winds its own.
        graphics.RasterizerDesc.FrontCounterClockwise = Diligent::True;
        graphics.DepthStencilDesc.DepthEnable = desc.depth != OverlayDepthMode::Disabled;
        graphics.DepthStencilDesc.DepthWriteEnable = desc.depth == OverlayDepthMode::TestAndWrite;
        graphics.InputLayout.LayoutElements = vertexLayout;
        graphics.InputLayout.NumElements = static_cast<Diligent::Uint32>(std::size(vertexLayout));

        auto &blend = graphics.BlendDesc.RenderTargets[0];
        blend.BlendEnable = desc.blend == OverlayBlendMode::Alpha;
        blend.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
        blend.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
        // Nothing downstream reads the output texture's alpha, and letting a transparent
        // overlay write its own would leave a hole in an otherwise opaque frame.
        blend.RenderTargetWriteMask = Diligent::COLOR_MASK_RGB;

        // Dynamic because one effect draws with as many textures as the client has - a glyph
        // atlas and an icon sheet through one pipeline - and neither a static nor a mutable
        // variable can be re-pointed once it has been set.
        const Diligent::ShaderResourceVariableDesc variables[]{
            {Diligent::SHADER_TYPE_PIXEL, OVERLAY_TEXTURE_NAME, Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
        };
        pipelineInfo.PSODesc.ResourceLayout.Variables = variables;
        pipelineInfo.PSODesc.ResourceLayout.NumVariables = static_cast<Diligent::Uint32>(std::size(variables));

        _device->CreateGraphicsPipelineState(pipelineInfo, &slot.pipeline);
        if (!slot.pipeline)
        {
            Logger::LogError("Diligent failed to create an overlay pipeline state for " + desc.pixelShader);
            return {};
        }

        setStaticBuffer(slot.pipeline, Diligent::SHADER_TYPE_VERTEX, "OverlayFrameConstants", _frameConstants);
        setStaticBuffer(slot.pipeline, Diligent::SHADER_TYPE_PIXEL, "OverlayFrameConstants", _frameConstants);
        setStaticBuffer(slot.pipeline, Diligent::SHADER_TYPE_VERTEX, "OverlayParameters", slot.parameters);
        setStaticBuffer(slot.pipeline, Diligent::SHADER_TYPE_PIXEL, "OverlayParameters", slot.parameters);

        slot.pipeline->CreateShaderResourceBinding(&slot.binding, true);
        slot.texture = slot.binding->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, OVERLAY_TEXTURE_NAME);

        return _effects.add(std::move(slot));
    }

    void OverlayPass::destroyEffect(const OverlayEffectHandle handle)
    {
        // The slot owns its pipeline, its binding and its constants, so clearing it releases them.
        _effects.remove(handle);
    }

    OverlayMeshHandle OverlayPass::createMesh(const OverlayMeshData &data)
    {
        if (!_device || data.isEmpty()) return {};

        MeshSlot slot;
        slot.vertexCount = static_cast<Diligent::Uint32>(data.vertices.size());
        slot.indexCount = static_cast<Diligent::Uint32>(data.indices.size());

        Diligent::BufferDesc bufferDesc;
        bufferDesc.Usage = Diligent::USAGE_IMMUTABLE;

        bufferDesc.Name = "Overlay vertices";
        bufferDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
        bufferDesc.Size = data.vertices.size() * sizeof(OverlayVertex);
        const Diligent::BufferData vertexData{data.vertices.data(), bufferDesc.Size};
        _device->CreateBuffer(bufferDesc, &vertexData, &slot.vertices);

        if (slot.indexCount > 0)
        {
            bufferDesc.Name = "Overlay indices";
            bufferDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
            bufferDesc.Size = data.indices.size() * sizeof(uint32_t);
            const Diligent::BufferData indexData{data.indices.data(), bufferDesc.Size};
            _device->CreateBuffer(bufferDesc, &indexData, &slot.indices);
        }

        if (!slot.vertices || (slot.indexCount > 0 && !slot.indices))
        {
            Logger::LogError("Diligent failed to upload an overlay mesh");
            return {};
        }

        return _meshes.add(std::move(slot));
    }

    void OverlayPass::destroyMesh(const OverlayMeshHandle handle)
    {
        _meshes.remove(handle);
    }

    void OverlayPass::begin(const CameraView &camera, const Matrix &viewProjection,
                            Diligent::ITexture *output, Diligent::ITexture *depth)
    {
        if (!_context || output == nullptr || depth == nullptr) return;

        auto *renderTarget = output->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        auto *depthStencil = depth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
        // Neither is cleared: the colour is the finished frame this pass draws over, and the
        // depth is the scene's own, which is the whole reason geometry occludes the overlay.
        _context->SetRenderTargets(1, &renderTarget, depthStencil, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const FrameConstants frame{
            .viewProjection = MatrixToFloatV(viewProjection),
            .inverseViewProjection = MatrixToFloatV(MatrixInvert(viewProjection)),
            .cameraPosition = {camera.position.x, camera.position.y, camera.position.z, 1.0f}
        };
        uploadConstants(_context, _frameConstants, &frame, sizeof(frame));
    }

    void OverlayPass::draw(const OverlayDrawDesc &desc, Diligent::ITextureView *texture)
    {
        if (!_context) return;

        const auto *effect = _effects.get(desc.effect);
        const auto *mesh = _meshes.get(desc.mesh);
        if (effect == nullptr || mesh == nullptr) return;

        if (effect->parameterSize > 0 && desc.parameters != nullptr)
        {
            uploadConstants(_context, effect->parameters, desc.parameters, effect->parameterSize);
        }

        _context->SetPipelineState(effect->pipeline);
        if (effect->texture != nullptr) effect->texture->Set(texture);
        _context->CommitShaderResources(effect->binding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Diligent::IBuffer *vertices = mesh->vertices;
        constexpr Diligent::Uint64 vertexOffset = 0;
        _context->SetVertexBuffers(0, 1, &vertices, &vertexOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                   Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);

        if (mesh->indexCount > 0)
        {
            _context->SetIndexBuffer(mesh->indices, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            Diligent::DrawIndexedAttribs drawAttribs;
            drawAttribs.IndexType = Diligent::VT_UINT32;
            drawAttribs.NumIndices = mesh->indexCount;
            drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
            _context->DrawIndexed(drawAttribs);
            return;
        }

        Diligent::DrawAttribs drawAttribs;
        drawAttribs.NumVertices = mesh->vertexCount;
        drawAttribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        _context->Draw(drawAttribs);
    }
} // namespace BreadEngine
