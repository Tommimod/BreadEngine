# 8.b — the editor gizmo, off raylib

Landed 2026-08-23. `lib/editor/raygizmo.c` stops being linked; the editor draws and hit-tests the transform handles itself, through the overlay channel 8.a built.

## The four decisions, all the owner's

- **Always on top** — the gizmo keeps `raygizmo.c`'s `rlDisableDepthTest()` behaviour, as `OverlayDepthMode::Disabled`. The plan's own text had said "depth-tested against the renderer's own depth buffer"; that line was wrong and is now corrected. A handle standing inside the object it moves is still the handle the pointer has to reach.
- **Static unit meshes plus a per-draw model matrix**, over a dynamic mesh rebuilt each frame. This is the question 8.a left open, and the cheap answer won: the model matrix goes in the *client's own* parameter block, which is already uploaded per draw, so the seam did not change at all. `updateOverlayMesh` and dynamic-usage buffers stay unbuilt until the UI phase needs them for geometry that is genuinely variable.
- **`IRenderer::getViewProjection()`** over the editor rebuilding the projection. `BeginMode3D` was the source of `rlGetMatrixProjection`/`rlGetMatrixModelview` for the hit-test, and removing it removed that. A getter keeps the renderer the single source of truth for the aspect and the clip range it drew with — the invariant already says near and far are asked for and never repeated.
- **Same layout, better lines and heads.** Identical proportions, colours and placement; cone arrowheads instead of four-sided pyramids, and no hardware lines anywhere.

## Why no lines at all

`raygizmo.c` asked rlgl for 2.5-pixel lines. **GL core profile ignores `glLineWidth` above 1.0**, so a faithful port would have come out *thinner* than what it replaced, not equal to it. Every stroke is a solid instead: a shaft is a thin square prism, a rotation ring is a square-section torus. Because the whole gizmo is already scaled by its distance from the eye, a constant world thickness *is* a constant screen thickness — no camera-facing ribbon, no viewport size in the frame block, and one plain `mul(viewProj, mul(model, p))` vertex shader.

The ring being a tube rather than a flat annulus is what keeps the X rotation ring visible when it is seen edge-on, which is most of the time.

Six meshes, not five: `buildRing` is called twice, because a ring is placed by one uniform scale that carries its tube with it, and the free-move ring is a tenth of the rotation ring's radius. Ten draws per frame in translate or scale, three in rotate.

## Ordering without a depth test

Solid handles made painter's ordering matter in a way lines never did. The handles are collected into `_handles` and **stable-sorted back to front** by the squared distance from the eye to a point standing for each, then issued. Stable so that handles sitting on the same point — the three rings, all centred on the origin — keep submission order and the frame stays identical between runs, which is what `diff.ps1` rests on. Interlocking rings still cross wrongly at six points; so did `raygizmo.c`, and per-segment sorting is the only fix.

## What was dropped, deliberately

`setMode` has no callers anywhere in the tree, so translate is all that has ever been reachable. Rather than port translate alone and lose the rest with the file, all three modes were rewritten — as **exclusive** modes, not `raygizmo.c`'s bit flags:

- **Combined `GIZMO_TRANSLATE | GIZMO_SCALE`** is gone. It is what forced the halved shaft lengths and the plane-handle action override, about a third of the layout's complexity, and nothing could reach it.
- **`GIZMO_VIEW`** axis orientation is gone. `GizmoSpace::World`/`Local` remains, because scale genuinely needs local.
- **Scale's drag math was corrected.** `raygizmo.c` forced scale into local mode for *drawing* and then projected the pointer onto the **world** axes for the arithmetic, so a rotated object's scale handles moved the wrong components. `applyDrag` projects onto the handle's own direction. Unreachable today, which is why the risk of changing it is nil.
- **A click that grabs a handle without moving it no longer pushes an undo command.** `endDrag` compares against the transform the drag began from.

## What broke: the GL viewport

**Symptom: the whole editor UI drawn into the bottom-left corner of the window, squashed, with the top and right of the screen blank.** The 3D viewport, the panels and the text were all internally correct — just confined to a rectangle.

`yieldToRaylib` restores nine pieces of GL state and the viewport was not among them, because **`EndTextureMode` had been putting it back by coincidence**: the editor's `beginSceneOverlay`/`endSceneOverlay` pair wrapped the gizmo's rlgl drawing, and raylib's `EndTextureMode` ends with `rlViewport(0, 0, <render size>)`. Deleting the gizmo's rlgl path deleted the only call that reset it, and every Diligent pass sets the viewport to whatever target it draws into — so raylib inherited the overlay pass's.

This is the sixth bug of exactly the shape the Invariants describe: state rlgl neither tracks nor reissues, left behind by a pass, surfacing somewhere that looks unrelated to rendering.

Two details:

- **The value has to come from `GetRenderWidth()`/`GetRenderHeight()`, not `rlGetFramebufferWidth()`/`Height()`.** The first attempt used rlgl's, and the UI came back at exactly 1920×1080 in a 2560×1369 window — `InitWindow`'s requested size. rlgl's tracked framebuffer size is not updated by `MaximizeWindow`; raylib's render size is, and is what `EndTextureMode` itself uses.
- **The `#ifndef NDEBUG` state guard now checks it**, in the "does not fit the single-integer shape" group beside the colour write mask and the sampler bindings, so the next omission of this kind is named on the first run rather than found by eye.

## The numbers

Editor at 2560×1369, the `Cube` node selected, its gizmo about 158 px from origin to arrow tip:

- **UI extent** — dark content reaches `maxX = 2559`, `minY = 0`. Before the fix: `maxX = 1791`, `minY = 484`, i.e. a 1792×885 viewport sitting at the bottom-left. The intermediate rlgl-sourced attempt: `maxX = 1919`, `minY = 289`.
- **Drag on the X handle**, 180 px right: `Local Pos` went `0, 0.5, 0` → `2.3, 0.5, 0`. Y and Z untouched, so the axis constraint holds. Predicted from the geometry: 180 px on the pointer plane is `180/158 = 1.139` gizmo sizes, and `size ≈ 2.0` world units at that camera distance gives `2.28`.
- **The state guard is silent** on a fresh run, and no Diligent error appears on either stream.
- All three modes captured: cone-tipped arrows with two visible plane handles and the free-move ring; three rotation rings with the edge-on X ring surviving as a line; cube-tipped scale handles.

## The thing that cost the most time

The squashed viewport also broke the P/Invoke click harness, and in a way that read as a gizmo bug: clicks landed nowhere near the widgets they appeared to be over, so the node tree could not be driven and the gizmo "did not draw". Two wasted rounds went into calibrating a mouse mapping (`OS y = capture y - 484`) that was an artefact of the bug being investigated. **When the editor's own input appears mis-aimed, check the viewport before the input.**
