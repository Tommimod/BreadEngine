#include "gridRenderer.h"

#include "rendering/renderer.h"

namespace BreadEditor {
    /// Mirrors grid.psh's own constant block, and float4-only for the reason the renderer's
    /// blocks are: that is the only member layout the struct and the shader cannot drift
    /// apart over.
    struct GridParameters
    {
        Vector4 majorColor;
        Vector4 minorColor;
        Vector4 xAxisColor;
        Vector4 zAxisColor;
        /// x is the finest cell in world units, y how many of them make up the next level, z
        /// the width of a minor line and w of a major one, both in pixels.
        Vector4 cell;
        /// x is the width of an axis line in pixels, y the distance the grid starts fading at
        /// and z the one it has gone by, w how few pixels apart the finest cell may get.
        Vector4 axis;
    };

    /// How the grid is drawn. Colours are in the space the output texture already holds -
    /// the overlay draws after the composite, so what is written here is what is shown.
    constexpr GridParameters GRID{
        .majorColor = {0.62f, 0.62f, 0.62f, 0.75f},
        .minorColor = {0.40f, 0.40f, 0.40f, 0.50f},
        .xAxisColor = {0.85f, 0.28f, 0.32f, 0.90f},
        .zAxisColor = {0.30f, 0.50f, 0.90f, 0.90f},
        .cell = {1.0f, 10.0f, 1.0f, 1.6f},
        .axis = {1.8f, 60.0f, 260.0f, 3.0f}
    };

    void GridRenderer::initialize()
    {
        auto &renderer = BreadEngine::Renderer::get();

        _effect = renderer.createOverlayEffect(BreadEngine::OverlayEffectDesc{
            .shaderDirectory = "shaders/editor",
            .vertexShader = "grid.vsh",
            .pixelShader = "grid.psh",
            .parameterSize = sizeof(GridParameters),
            // A line of the grid is a shade of the pixel it lands on rather than a shape, so
            // the pass blends; it tests the scene's depth and writes none of its own, which
            // is what lets whatever the editor draws next stand in front of it.
            .blend = BreadEngine::OverlayBlendMode::Alpha,
            .depth = BreadEngine::OverlayDepthMode::Test,
            // One triangle laid over the viewport, and which way its corners happen to be
            // wound is no part of what it draws.
            .cullBackFaces = false
        });

        // The corners are already in normalized device space: the pixel shader finds the
        // ground plane by itself, so the geometry only has to reach every pixel.
        BreadEngine::OverlayMeshData triangle;
        triangle.vertices = {
            BreadEngine::OverlayVertex{.position = {-1.0f, -1.0f, 0.0f}},
            BreadEngine::OverlayVertex{.position = {3.0f, -1.0f, 0.0f}},
            BreadEngine::OverlayVertex{.position = {-1.0f, 3.0f, 0.0f}}
        };
        _mesh = renderer.createOverlayMesh(triangle);
    }

    void GridRenderer::shutdown()
    {
        if (!BreadEngine::Renderer::isAlive()) return;

        auto &renderer = BreadEngine::Renderer::get();
        renderer.destroyOverlayMesh(_mesh);
        renderer.destroyOverlayEffect(_effect);
        _mesh = {};
        _effect = {};
    }

    void GridRenderer::render() const
    {
        if (!_effect.isValid() || !_mesh.isValid()) return;

        BreadEngine::Renderer::get().drawOverlay(BreadEngine::OverlayDrawDesc{
            .mesh = _mesh,
            .effect = _effect,
            .parameters = &GRID
        });
    }
} // namespace BreadEditor
