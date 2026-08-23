# 8.a — the editor grid, and the overlay channel under it

Landed 2026-08-23. The first piece of Phase 8: `DrawGrid(1000, 1.0f)` is gone and the editor draws an analytic grid through Diligent, using a shader it ships itself.

## The decision: what the seam looks like

The constraint set by the owner is that the engine knows nothing about the editor — so the editor cannot ask the renderer for "a grid". Three shapes were put up:

- **A generic overlay channel with the grid as vertex-coloured lines.** Engine learns "unlit lines" and nothing else; the grid would have been today's quality plus a fade.
- **A generic overlay channel plus an escape hatch for a client-supplied shader pair** — chosen. The editor ships `grid.vsh`/`grid.psh` and gets a proper analytic grid, and the channel is the same one the gizmo and the UI will draw through.
- **DiligentFX's `CoordinateGridRenderer` adopted engine-side.** Priced properly this time and it is *cheap* — unlike the components rejected in 7.e and 7.f it needs no `PostFXContext`, only a colour RTV, a depth SRV and a `CameraAttribs`. It lost on architecture alone: `drawGrid()` on `IRenderer` is the engine learning an editor concept. It would also not have removed the work, only the grid's share of it, since the gizmo needs the generic channel regardless.

The second question — how wide to design the channel before the UI work — was answered "design for both, implement 3D now": the vertex layout, the effect/mesh/draw split, the blend and depth modes and the texture binding are settled now; only the world-space depth-tested path is exercised. The texture binding in particular shaped the resource layout (DYNAMIC, because one effect will draw a glyph atlas and an icon sheet), which is the expensive half to change later. It is unexercised until the text pass.

**What was deliberately left out:** a per-draw model matrix, and the viewport size in the frame block. Neither is needed by a full-screen grid, both are two-line additions to a struct with no serialization contract, and the gizmo is the thing that should decide the shape of the first.

## Why DiligentFX's grid *math* was not reused either

`CoordinateGrid.fxh` is a shader header, so including it would have kept the architecture intact — the editor's own shader, DiligentFX's math. It was rejected for a concrete reason: `ComputeCoordinateGrid` takes `CameraAttribs` **by value and multiplies by its matrix members**, which is exactly the case the converter's known-issues list says not to do. `Shadows.fxh` gets away with passing a struct only because none of its functions multiplies by a matrix member; this one does it three times. The grid math is written directly against the constant block instead, which also avoids taking on an Apache-2.0 attribution obligation next to the Hosek-Wilkie one.

## What the shader does

One triangle covering the viewport, corners already in normalized device space. Per pixel: unproject the near and far points, intersect the ray with `y = 0`, take `fwidth` of the hit position, and measure the distance to the nearest line **in pixels** — which is what keeps a line one width wide at any distance. The cell size is `finest * subdivision^floor(level)` where `level` comes from how many world units a pixel covers, so it steps up whenever the finest cells would fall below three pixels apart; the fractional part fades the finest level out while the medium one takes on the major-line emphasis. Axes are the same measurement against `|x|` and `|z|`.

Then `SV_Depth` is written from the hit point through the same view-projection the scene used, which is what makes the hardware depth test occlude the grid. **`SV_Depth` does convert on the GL path** — `HLSL2GLSLConverterImpl.cpp` maps `sv_depth` to `_SET_GL_FRAG_DEPTH`, i.e. `gl_FragDepth`. Depth writes stay off, so the value is used for the test only and whatever the editor draws next still stands in front of the grid.

Hardware depth testing is what let the pass keep the scene depth as an *attachment* rather than a shader resource. DiligentFX's component takes the other route (sample the depth, compare by hand, soft visibility) and would have forced the channel to expose the depth buffer as a texture to client shaders — a much wider seam for no gain here.

`discard` runs **last**, after both derivatives and both outputs, because a derivative is only defined where the whole quad took the same path.

## What broke

The `#ifndef NDEBUG` GL-state guard from 7.e caught both regressions on the first run, which is the entire reason it exists:

- **`glDepthMask`** — the plan's own note said depth writes matched rlgl's default *by coincidence, not design*, because no pipeline had ever turned them off. The overlay's does. Not restoring it leaves rlgl drawing with depth writes off.
- **`glColorMask`** — the overlay writes `COLOR_MASK_RGB` so a transparent line cannot punch a hole in the output texture's alpha, which is the channel raylib blends the finished frame into the window through. The mask is global state, so rlgl inherited it and drew the whole UI with no alpha. Visible in the first capture as a frame with no sky band and a washed viewport.

Both are now explicit restores in `yieldToRaylib`.

## The numbers

Measured on the capture, viewport in the physical 2560×1440 frame:

- **Z axis** at `x = 1269`: `94,140,232`. Authored `(0.30, 0.50, 0.90)` at opacity 0.9 over the lit floor — consistent, and blue-dominant along its whole length.
- **X axis** at `y = 624 / 622 / 611` across three columns: `217,86,96`. Authored `(0.85, 0.28, 0.32)` at 0.9 → `217,71,82` over the floor. The line slopes with perspective, which is why the row differs per column.
- Falloff neighbours (`208,130,135`, `203,184,183`) confirm the one-pixel antialias rather than a hard edge.
- Second run: no `GL state left for raylib is wrong` line on either stream, and no Diligent error.
