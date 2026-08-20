*Part of the [migration log](../../DILIGENT_MIGRATION_LOG.md). History, not instructions — read it before working in this area, not otherwise. Anything here that must not be re-broken belongs in [DILIGENT_MIGRATION.md](../../DILIGENT_MIGRATION.md)'s Invariants instead.*

### Phase 6.d — omni shadows

The last of Phase 6, and the first shadow kind with no DiligentFX component behind it: `ShadowMapManager` fits cascades to a camera and `PCF.fxh` filters a 2D map, neither of which a cube is. Three choices went to the project owner before any code and all three took the recommendation — projected depth reconstructed from the major axis over a distance-in-a-colour-cube, four casters at 512² faces, and a five-tap disk whose radius `shadowSoftness` drives.

**Two API facts were confirmed in the vendored sources first, because the whole phase rests on them.** `HLSL2GLSLConverterImpl.cpp:575` registers `SampleCmpTexCubeArr_3` for `samplerCubeArrayShadow`, and `GLSLDefinitions.h:974` expands it to `texture(Tex, vec4(dir), compare)` — but **only inside the `FRAGMENT_SHADER` branch**; outside it, line 952 replaces it with the literal `0.0`, so a cube shadow read from any other stage silently returns "fully shadowed". And `TextureCubeArray_GL::AttachToFramebuffer` takes its `NumArraySlices == 1` path through `glFramebufferTextureLayer`, indexing layer-faces — so one DSV per face is not a workaround, it is the supported shape. Two smaller ones settled details: `TextureBase.cpp:529` auto-corrects a DSV on a cube array to `RESOURCE_DIM_TEX_2D_ARRAY`, which is why the view desc needs nothing said about its dimension, and `RenderDeviceGLImpl.cpp:333` enables `GL_TEXTURE_CUBE_MAP_SEAMLESS`, so filter taps near a face edge read across it instead of clamping.

**No matrix reaches the shader, which is the shape worth keeping.** A 90° square projection writes `depthScale - depthScale·near/d`, where `d` is the distance along the face's own forward axis — and for any point inside a face's frustum that is exactly the largest component of the direction to it. So the scene pass rebuilds the comparison depth from `max(|x|,|y|,|z|)` and never learns which face the lookup lands on, never mind that face's transform. Two coefficients travel per light instead of six matrices, and the same expression is correct under D3D: the NDC ranges differ, but the *window* depth a given distance produces does not.

**The engine side needed nothing.** `Light` already showed `withShadows` for every type and `lightSystem.cpp` already forwarded it, so 6.d is entirely inside `rendering/diligent/` — `assignShadowSlots` grew a third branch, `SceneLight::Shadow.y` names the cube, `g_LightCount.z` says how far the array is live, and `renderShadowCasters` was reused with no change at all, six times per light.

`createShadowArray` was factored out while here: the spot array and the omni array differ only in dimension, resolution and slice count, and a `std::span` of the destination views carries the count so nothing has to be passed twice.

**The one defect, and it took a screenshot to find.** First run: the floor's shadows were correct in shape and position, while every *caster* was uniformly dark with a bright one-texel rim along its silhouette. The floor being right is what named the bug — it carries `castShadows: false`, so it is the only surface in the scene that cannot self-shadow. Everything in the map was shadowing itself completely; the rims were the texels the object no longer fully covered, reading the cleared far depth.

Two things were wrong, and the first is the one that matters:

- **The bias went the wrong way.** Depth grows with distance from the light, so a point is lit exactly where its own depth is the *smaller* of the two — the compared point has to be drawn back *toward* the light. It was being pushed away, which makes every surface fail its own comparison. The spot path had this right (`uvDepth.z - samplingError`); the cube path inverted it while restating it in distance rather than in depth.
- **The slope term had the wrong units.** It was `max(|ddx(dist)|, |ddy(dist)|)` — how much world distance one *screen* pixel spans, which has nothing to do with how much depth one *shadow-map texel* spans. It was also what lit the silhouettes: at a grazing view the derivative explodes and the bias with it. Replaced by a normal offset scaled by the texel's own footprint at that distance and by the sine of the incidence angle — correct units, and no derivative in the omni path at all.

The project owner's first read was that it looked like the 6.c near-plane defect, and the symptom sentence was word for word the one recorded there ("brightest areas dark, edges lit"). It was a different cause with the same signature, which is worth knowing: **that appearance means "the comparison is failing where it should pass", not specifically "the near plane is wrong".**

**Verified in the editor by the project owner** on the test scene with `SunLight` deactivated and one omni light at intensity 200: casters lit cleanly with no acne and no rim, shadows meeting their casters at the base, and cascades and spots unchanged with the sun restored. Face orientation was confirmed by the *first* screenshot rather than the fixed one — the floor shadows were the right shape and fell away from the light, which a mirrored face would not have produced.

---

### Between 6.c and 6.d — raylib 5.5 → 6.0, and the context to OpenGL 4.3

**Started as a blocker found while designing 6.d, and ended as the Phase 7 decision being taken early.** The plan's 6.d entry specifies `RESOURCE_DIM_TEX_CUBE_ARRAY`. Three facts, read in order, showed that was unreachable on the context we had:

1. `RenderDeviceGLImpl.cpp:919` — `CubemapArraysSupported = IsGL43OrAbove || CheckExtension("GL_ARB_texture_cube_map_array")`.
2. `HLSL2GLSLConverterImpl.cpp:575` — `TextureCubeArray.SampleCmp` converts to `texture(samplerCubeArrayShadow, …)`, which is GLSL 400.
3. `RenderDeviceGLImpl.cpp:364` — `MaxShaderVersion.GLSL = APIVersion`, so a 3.3 context emits `#version 330 core`; and `GLSLUtils.cpp:284` appends `#extension` lines **only when `IsES`**.

So even on a driver exposing the ARB extension — this machine's does — the shader could not have compiled, because nothing would have enabled it. There is a `ShaderCreateInfo::GLSLExtensions` field (`Shader.h:580`) that injects directives right after the version line, which would have made it work here and nowhere guaranteed. Four ways forward were written up for the project owner: that extension plus a runtime `CubemapArraysSupported` gate compiling the omni path out; a 2D array of six slices per light with analytic face selection; N separate non-array `TextureCube`s; or moving raylib to 4.3. **The owner chose 4.3 before the question was asked**, on the grounds that Phase 7 needs it anyway — and additionally asked that omni shadows be re-planned as their own step after it, rather than bundled in.

**What was actually done.** raylib tag `6.0` cloned into a scratch directory in the repo root, configured `-DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=OFF -DOPENGL_VERSION=4.3` with the project's own clang64, four headers and the DLL copied into `lib/engine/`, scratch directory deleted. No submodule, at the owner's request — the shape of `lib/engine/` is unchanged. `LibraryConfigurations.cmake:199` is what turns `OPENGL_VERSION=4.3` into `GRAPHICS_API_OPENGL_43`, and the configure output prints the resulting `GRAPHICS=` so it can be confirmed rather than assumed.

**Four checks made before the swap, and each of them saved work.**

- **The import table.** `llvm-objdump -p` on old and new DLL: byte-for-byte the same fifteen imports — five Win32 libraries and the UCRT stubs. A DLL built with a different toolchain would have dragged a second C runtime in, and that would have surfaced as a load failure rather than a link error.
- **The public API diff.** `RLAPI` declarations, 581 → 600, with eleven changed: seven `Text*` helpers, `DrawModelPoints`/`Ex`, `UnloadModelAnimation`, `UpdateModelAnimationBones`, plus three in rlgl (`rlCompileShader`, `rlLoadShaderCode`, `rlLoadComputeShaderProgram`, renamed per the changelog's `-WARNING-` entry). **None is used anywhere in `modules/`, `games/` or `lib/editor/`** — raygui's `TextSplit` is its own static, not raylib's.
- **`rlgl.h` under `_33` versus `_43`.** The public header is lines 1–818 and every `GRAPHICS_API_OPENGL_43` reference in it is version-selection plumbing; all real branches are past the `RLGL_IMPLEMENTATION` guard. So **no compile definition had to be added to any target** — which was the thing most likely to be got wrong, since a mismatched define between DLL and consumer is exactly the kind of failure that shows up as corruption much later.
- **The changelog, for the interop's own assumptions.** One entry mattered: rlgl's default clip range moved from 0.01/1000 to 0.05/4000. It cost nothing, because both sites that need it call `rlGetCullDistanceNear/Far`. That is now stated as a rule in Invariants rather than left as luck.

**The only code change in the whole swap was one comment** — `SHADOW_DISTANCE`'s, which asserted the camera's far plane was "a thousand units". It now names rlgl as the source instead of a number, which is true under both versions.

**What this releases downstream.** Cube map arrays for 6.d; `bitfieldReverse` for Phase 7's BRDF LUT; compute shaders and SSBOs for the compute-based PostFX components. The GLSL the engine's own shaders compile as went from `#version 330 core` to `#version 430 core` as a side effect — Diligent derives it from the context, so no shader source changed.
