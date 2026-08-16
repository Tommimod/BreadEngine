# Migration plan: r3d → DiligentEngine

**This is the working document**: where the migration stands, the rules that must not be broken, and what is still planned. Keep it short enough to read in full at the start of every session.

The record of how each completed phase went — what broke, how it was found, what was verified — lives in [DILIGENT_MIGRATION_LOG.md](DILIGENT_MIGRATION_LOG.md). That file is history: open the section for an area before working in it, never end to end.

## Status

**Phase 3 — DiligentRenderer MVP — COMPLETE.** 3.a (backend skeleton, device attach, scene target, compositing), 3.b (geometry, first PSO, constant buffers, first HLSL shader) and 3.c (textures, material SRB, one directional light) are done and verified. Phases 0, 1 (build system) and 2 (the seam) are complete.

**Phase 3.5 — settled.** The engine keeps its own shader; DiligentFX is adopted component by component instead. See the phase entry for the spike's measurements and the component list.

**Next, in order:**

1. **Phase 3.6** — remove R3D.
2. **Phase 4** — its real remainder only: `MaterialHandle` over a per-material SRB, and refcounting in `TextureAsset`.
3. **Phase 5** — six primitive generators and the assimp importer.

**What renders today under Diligent:** the cube primitive, textured, with normal mapping, lit by one directional light, in the editor viewport and in the standalone game. Everything else is a stub — models, the other primitives, skybox and cubemaps, ambient/IBL, all post-processing, shadows, and any light past the first directional one.

**Screenshot-diffing against R3D is over.** It ended with 3.c: nothing past textures and basic lighting was ever built on r3d on a real scene, so there is no reference left to diff against. From here, verify against known-good reference output and the scene's own expected behaviour.

## Open decisions (owner: project owner, not the agent)

- **GL 3.3 or a raylib rebuilt as `GRAPHICS_API_OPENGL_43`** — deferred to Phase 7, where IBL forces it. 3.3 covers everything built so far; what it does not cover is listed under that phase.
- **Point/spot shadow priority** — Phase 6, and genre-relevant for a horror game. Do not auto-deprioritize.
- **Who owns the shipped game's 2D** — the gap under Phase 9. Must be answered before the game loses rlgl.
- **A separate MSVC toolchain for D3D** — spike 0.1 found ATL is missing in clang64, so D3D11/D3D12 cannot build here at all. Vulkan+GL covers PC and SteamDeck.

## Invariants

Rules already paid for in bugs. Breaking one of these does not produce a compile error — it produces a silent, visual, or data-loss failure weeks later. The log has the incident behind each.

**Serialization**

- **Enums serialize by *declaration index*** (`magic_enum::enum_index`), not by name and not by underlying value. **Append only, and delete only from the tail** — removing a middle value silently reindexes everything after it. An out-of-range index deserializes to the field's default rather than crashing.
- **New inspector fields are backward compatible.** `InspectorStruct::deserialize` guards every property with `if (node[prop.name])`, so a config written before a field existed keeps the C++ initializer. Removing or reordering is what breaks.
- **The assets registry must stay relocatable.** `_projectPath` and `_fullPath` are derived and never serialized; `_pathFromRoot` is the only source of truth. Every site that assigns either must go through `childPathFromRoot` / `toFullPath` — a divergence makes the registry unloadable, which strips the tree and re-registers every file with fresh GUIDs, breaking every asset link in every scene.

**Matrices and geometry**

- **Upload `MatrixToFloatV(m).v`, never the bytes of the `Matrix` struct** — they are transposes of each other. Shaders are written column-vector, `mul(M, v)`, which lines up with HLSL's default cbuffer packing and with what rlgl itself uploads.
- **The scene projection stays OpenGL's `[-1, 1]` depth range** for as long as the editor's rlgl overlay depth-tests against the scene's depth buffer. Near and far come from `rlGetCullDistanceNear/Far` so the two projections agree.
- **Vertex layout is the contract for every generator and the importer**: position, normal, uv, and a `float3` tangent with no handedness. The bitangent is `cross(tangent, normal)` — the direction v increases in, and what DiligentFX's own shaders build their TBN with. Front faces are counter-clockwise.

**Sharing an OpenGL context with raylib**

- **rlgl tracks GL state in software and only issues the calls it believes are needed.** Anything Diligent changes that rlgl also tracks must be put back in `yieldToRaylib()` — currently blending, the depth test, bound sampler objects and `GL_FRAMEBUFFER_SRGB`. Culling and depth writes happen to match rlgl's defaults today; that is coincidence, not design.
- **`InvalidateState()` resets Diligent's *cache* of GL state, not the GL state itself.** It is needed before the pass as well as after, and it is never a substitute for an explicit restore.
- **Pixel-unpack state must be restored at every upload site, not in `yieldToRaylib()`.** Diligent leaves `GL_UNPACK_ROW_LENGTH` set to its last stride; raylib assumes the default. Fonts load during editor init, before any frame is yielded — the symptom is the whole UI rendering correctly *except* that text is completely absent.
- **The GL context is exactly 3.3, and that is raylib's decision.** No SSBOs, no compute, no image load/store. If something needs 4.x, the fix is rebuilding the vendored raylib as `GRAPHICS_API_OPENGL_43`, not working around it in the backend.

**Shaders**

- **HLSL that is legal for D3D is not automatically legal through the GL converter.** No C-style casts, no implicit conversions (`float4 v = 0;`), avoid `float3` members in cbuffer structs, do not pass structs to functions. `HLSL2GLSLConverterImpl.hpp`'s known-issues header comment is the list — read it before writing anything non-trivial.
- Sources live as `.hlsl` files next to the executable and are compiled at runtime, so a shader edit does not need a rebuild.

**Structure**

- **The backend switch is compile-time.** Exactly one backend's `.cpp` files compile; `modules/engine/CMakeLists.txt` filters `rendering/backends/` and adds back only the selected directory.
- **The seam holds**: r3d appears in engine/editor/game code in exactly one place outside `rendering/backends/r3d/` — the `#include` in `rendering/renderer.cpp`. Phase 3.6's deletion depends on this staying true.
- **Never use bare `include_directories()`** — always `target_include_directories()`, scoped to the target that needs it. A blanket call once made assimp's `texture.h` shadow DiligentCore's `Texture.h`.
- **Background work must be waited on explicitly.** A dropped `std::future` does not wait, where a `jthread` destructor did. Every path that frees or recycles a slot a job writes into has to join it first.

## How to work on this

**Build** with CLion's bundled CMake, never a system `cmake` — `find_package` resolves differently between them and has already produced a build that configured under one and failed to link under the other:

```
"D:/Programms/CLion 2026.1.1/bin/cmake/win/x64/bin/cmake.exe" --build <build-dir> --target BreadEditor -j 14
```

Two build directories exist until Phase 3.6: `cmake-build-debug` (R3D) and `cmake-build-debug-diligent`, configured with `-DBREAD_RENDER_BACKEND=Diligent` and otherwise identical.

**"It compiles" and "it launches" prove nothing here.** Every real bug in this migration so far was silent at build time and visible only on screen: a shader that failed to compile, a UI that went black, text that vanished, geometry lit from the wrong side. The acceptance criterion is a screenshot or a pixel measurement, never an exit code. Prefer sampling specific pixel values over reading a full screenshot — it is cheaper and more precise. Read the image only when you do not yet know what you are looking for.

**Delegate broad searches to a subagent.** Anything shaped like "find every use of X", "which file sets Y", or "summarize how Z is done across this tree" — especially across the vendored DiligentCore/DiligentFX/raylib sources — should go to a subagent that reads a lot and returns a few lines. Keep decisions, diagnosis and implementation in the main session: a subagent starts cold and would have to re-read this file and the seam to do them, which costs more than it saves.

**The test scene** (`games/example_game/assets/`) is checked in: a Capsule at `x = -3`, a Cube at the origin with a deliberately non-trivial transform and the scene's three textures, the `scene.gltf` model at `x = +3`, and a directional SunLight — one node per rendering path the migration has to keep working. The Cube's transform is not decorative: with an identity matrix, a wrong per-draw model matrix and no model matrix at all look the same. **To update the scene, edit it in the editor and copy `Root.nd`, `assets_registry.cnf`, `project_settings.cnf` and `global_light_settings.cnf` back out of `bin/assets/game/`** — the build only copies what is absent and will never overwrite build-directory state.

**Code left behind carries no migration narrative.** Comments about phases, about what used to be, or about what will replace this belong in this file or the log, never in a header. A comment earns its place only by explaining something non-obvious about the code as it stands.

**At the end of every session**: update the Status section here, move any newly discovered load-bearing rule into Invariants, and put the incident itself in the log. This pair of files is the only handoff between sessions.

---

## Context

BreadEngine's rendering is currently 100% OpenGL, locked in via r3d (a PBR renderer built on raylib's rlgl abstraction). There is no path from here to Vulkan/D3D, which blocks the long-term goal of a future-proof renderer for shipping a real game (horror/adventure) on PC and eventually SteamDeck/consoles. DiligentEngine has been chosen as the target RHI (D3D11/D3D12/Vulkan/Metal/GL, plus a GLTF PBR Renderer + deferred shading + SSAO/Bloom in DiligentFX/DiligentSamples). This plan sequences that migration so BreadEngine stays buildable and runnable at every step (strangler-fig, not big-bang), given this is solo part-time hobby work with eventual AI-agent assistance on implementation, while architectural decisions stay with you.

## Phase 0 — settled

The four spikes are done; their findings are in the log. What they settled, and what still holds:

- **Windowing stays raylib.** It keeps the window and input; Diligent takes the device. Proven to coexist on one HWND.
- **The editor's 2D chrome stays raylib/raygui/rlgl permanently.** The editor never ships — only the 3D viewport content and gizmos move to Diligent. This deliberately avoids rewriting a 26-file UI toolkit that has nothing to do with the goal.
- **The editor process runs Diligent's OpenGL backend** so the scene is a plain GL texture the viewport can blit without a copy. CPU readback is the recorded fallback if that ever stops being possible — see Phase 8.
- **~~`PBR_Renderer` is the integration target~~ — reversed by Phase 3.5.** Both it and `GLTF_PBR_Renderer` are reference-only; the engine's own shader is the integration target, and BreadEngine's asset pipeline is not reshaped around glTF's loader.

## Phases 1–9 — Backend cutover (feature parity with today's R3D rendering)

Each phase keeps the tree buildable; Phases 2–8 keep R3D alive behind the new seam via a `BREAD_RENDER_BACKEND=R3D|Diligent` switch, so regressions are checked by screenshot-diffing the same scene under both backends — this is the QA method that substitutes for not having a rendering engineer's eye.

1. **Build system.** Add DiligentCore/DiligentTools/DiligentFX as git submodules (Diligent's documented approach — its CMake uses custom macros that plain `add_subdirectory` handles better than `FetchContent`), wired in alongside the existing vendored raylib/r3d/assimp blocks in root `CMakeLists.txt` — remove nothing yet. Collapse the three duplicated DLL-copy blocks (root/editor/example_game) into one reusable CMake function while here.
2. **Insert the seam.** New `modules/engine/rendering/IRenderer.h` + opaque handle types (`MeshHandle`, `MaterialHandle`, `TextureHandle`, `LightHandle` — plain index/generation structs, not per-object virtual dispatch, since only one backend is active at a time). Move every R3D call site (`meshRendererSystem.cpp`, `lightSystem.cpp`, `globalLightSystem.cpp`, `textureAsset.cpp`, `material.cpp`, `meshRenderer.cpp`, `spriteRenderer.cpp`, `editor.cpp`, `example_game/main.cpp`) into one `R3DRenderer : IRenderer`. Components (`MeshRenderer`, `SpriteRenderer`, `Light`, `Material`, `TextureAsset`, `Camera`) hold handles, not raw R3D types. **Before touching `Light`/`Material`: confirm whether `.yaml` scene files bake `R3D_LightType`'s numeric enum value** (check `BaseYamlConfig`/inspector serialization) — if so, either keep the enum's underlying values stable across the swap or write a one-time migration for existing scene assets. This phase should be a pure refactor with zero visual change — validate with a before/after screenshot diff.
3. **DiligentRenderer MVP.** Forward-shaded (not deferred) single PBR pipeline: PSO + SRB, one directional light, no shadows/IBL/post yet. Wire the existing `Camera3D`/`rcamera.h` math into Diligent's per-frame constant buffer (matrix-convention conversion is the only new math needed). Milestone: a textured lit cube renders equivalently via both backends. This is where the "not a rendering engineer" learning curve is steepest (PSOs/SRBs are conceptually unlike raylib's globals-based rlgl API) — budget accordingly, and treat DiligentSamples Tutorials 01–05 as the reference path.
3.5. **`PBR_Renderer` adoption — DECIDED: not adopted.** *(numbered .5 on purpose: renumbering 4–11 would invalidate every cross-reference in this document.)* The engine keeps the metallic-roughness shader 3.c wrote, and **spike 0.4's "drive `PBR_Renderer` directly" is reversed**: it is reference-only from here. The spike's measurements are in the log; what the rest of this plan depends on:

   **DiligentFX is adopted component by component instead.** Most of it does not go through `PBR_Renderer` at all, so refusing the renderer costs far less than refusing the library. The list is explicit so it does not get re-argued every phase:
   - **Phase 6** — `Components/ShadowMapManager.hpp` for CSM, with `Shaders/Common/public/PCF.fxh` and `Shadows.fxh` included into the engine's own shader.
   - **Phase 7** — `PostProcess/` (Bloom, DepthOfField, ScreenSpaceAmbientOcclusion, ScreenSpaceReflection, TemporalAntiAliasing), `Components/ToneMapping.hpp`, and `Components/EnvMapRenderer.hpp` for the skybox.
   - **Phase 8** — `Components/CoordinateGridRenderer.hpp` is what the grid becomes when it moves out of rlgl and into the renderer as real scene geometry.
   - **Not available without `PBR_Renderer`, and therefore ours to write:** the IBL precompute (its `PrecomputeCubemaps` is a method on the renderer, though the shaders it drives — `PBR/private/ComputeIrradianceMap.psh`, `PrefilterEnvMap.psh` — are usable directly), order-independent transparency, and the glTF extension lobes (clearcoat, sheen, anisotropy, iridescence, transmission).
   - Including a DiligentFX `.fxh` from an engine shader needs `CreateCompoundShaderSourceFactory({&DiligentFXShaderSourceStreamFactory::GetInstance(), <the file factory>})` — its shaders are compiled into the static library, not shipped as files.

   **What the decision releases.** The deadline this phase existed to meet — settle the conventions before Phase 5 writes six generators — is met by keeping them: `MeshVertex` and the material struct stay as they are, and **Phase 5 is unblocked with no changes to either**.

3.6. **Remove R3D.** *(new after 3.c.)* Pulled forward from Phase 9 on the reasoning that less code is less to break, now that the R3D screenshot diff has served its purpose. **This is mechanical by construction**: Phase 2 left r3d in exactly one place outside `rendering/backends/r3d/` — the `#include` in `rendering/renderer.cpp` that selects the backend. Delete that directory, the `lib/engine/r3d/` tree, the `R3D_DLL`/`R3D_LIB` CMake wiring and the r3d include directories, and decide what happens to the now-single-valued `BREAD_RENDER_BACKEND` option and the `backends/` source filtering it drives.

   ⚠️ **"Safe" here means "breaks nothing that is not already missing", not "no capability is lost".** The two backends are compile-time exclusive, so deleting R3D cannot break the Diligent build. What it does remove is the only configuration in which the editor is currently *complete*: under Diligent, models, nine of ten primitives, skybox and cubemaps, ambient/IBL maps, every post-effect, shadows and all lighting past one directional light are still stubs. Deleting R3D makes that the only reality until Phases 5–7 land. That is the trade being accepted deliberately — it is not a hidden cost.

   Three specifics that are easy to miss:
   - **rlgl does not go with it.** Phase 9 pairs "remove R3D" with "remove rlgl", but the Diligent backend is *built on* rlgl interop by design (Phase 0.3's GL sharing, 2.d's overlay brackets, 3.a's framebuffer over Diligent's textures, 3.b/3.c's state restores). rlgl is load-bearing and stays; only r3d leaves. Phase 9 keeps the rlgl half, and with it the still-open question of who owns the shipped game's 2D.
   - **assimp does not go with it either.** The plan scopes assimp to import-time in Phase 5; right now `loadModel` is a Diligent stub and the engine has no model loading at all, so nothing about assimp changes here.
   - ⚠️ **`applyDefaultEnvironment` loses its only real implementation.** The R3D backend seeds environment defaults from `R3D_ENVIRONMENT_BASE`; Diligent's is an empty body. Once R3D is gone, environment defaults come from the engine's own field initializers alone — so this phase has to either bake sensible values into those initializers or give the Diligent backend something to seed. Revisit the 2.c decision that made defaults backend-supplied while here; the reason it was made no longer applies once there is one backend.

4. **Materials & textures cutover.** Replace `TextureAsset`'s raw `Image`/`Texture2D`/`R3D_Cubemap` members with `TextureHandle` (DiligentTools' `TextureLoader` loads PNG/JPG/HDR/KTX/DDS straight into an `ITexture`, likely a cleaner reuse point than raylib decode + manual upload — confirm in Phase 0.4). Replace `Material`'s `R3D_Material` wrapping with an engine-native albedo/normal/ORM/emission struct shaped to glTF's metallic-roughness convention, since `meshAsset`'s existing texture-slot auto-wiring already approximates it.
   **Mostly landed early — read the 3.c notes before starting.** Phase 2 already did the `TextureHandle` and engine-native `Material` half, and 3.c adopted `TextureLoader` and built the material texture binding. What is genuinely left for Phase 4: a real `MaterialHandle` backed by a per-material SRB (3.c re-points dynamic variables per draw instead), and refcounting in `TextureAsset` so one material's `unload()` stops invalidating textures another material shares.
5. **Mesh primitives, MeshRenderer, SpriteRenderer.** Reimplement the remaining `R3D_GenMesh*` generators as engine-native geometry producing Diligent vertex/index buffers — genuinely new code (no Diligent equivalent exists), but standard parametric math, good AI-assist candidate.
   **Scope cut, decided after 3.c: `Slope`, `Torus` and `FreePoly` are dropped rather than ported.** That leaves **six** generators for this phase — sphere, hemisphere, cylinder, capsule, plane, quad — on top of the cube 3.b already produced. Two things make the cut cheap, and both are worth checking still hold before doing it:
   - **The three dropped types are the last three enumerators of `MeshPrimitiveType`** (`Slope = 8, Torus = 9, FreePoly = 10`). Since the enum is serialized by declaration index, deleting from the tail leaves every surviving value's index untouched — the same property that made dropping `R3D_LIGHT_TYPE_COUNT` and `R3D_TONEMAP_COUNT` safe. **Delete only from the tail; removing `Slope` alone would silently reindex `Torus` and `FreePoly`.**
   - A scene that already uses one of them degrades rather than crashes: `magic_enum::enum_cast` on an out-of-range index returns nothing, so the field keeps its default (`None`) and that node renders no mesh.
   Deleting each type means its `*PrimitiveData` header, its arm in `сreatePrimitiveCommand`'s `createData` factory, and its entry in the editor's create menu. The `forward` parameter on `createPrimitive`/`generatePrimitive` **stays** — `Quad` still needs it, even though `FreePoly` was its other user. Port `meshAsset`'s assimp import to build an engine-native cached mesh format directly (today assimp/R3D only transiently enumerate material count, geometry isn't cached) — this also lets Phase 9 scope assimp to import-time/editor-only, dropping it from the shipped game's runtime deps entirely.
6. **Lights.** Replace `lightSystem.cpp`'s full R3D light diff-sync with `IRenderer` calls. Use DiligentFX's `ShadowMapManager` (CSM) for directional shadows first — closest match to an existing DiligentFX component. Point/spot shadows are custom follow-up with less direct reuse; given the horror/adventure genre, flag this as a genre-relevant priority call for you, not something to auto-deprioritize.
7. **Environment / post-processing / skybox.** Cleanest phase given the existing abstraction: only the `toNative`/`fromNative` bodies in `configs/light/*.h` (background/ambient/ssao/ssil/ssgi/ssr/bloom/fog/dof/tonemap/color) need retargeting to DiligentFX PostFX settings — `globalLightSystem.cpp` call sites shouldn't need to change. `R3D_GenProceduralSky` has no Diligent equivalent (it's an analytic sky shader, not an HDRI) — port a known public model (Preetham or Hosek-Wilkie) as an HLSL shader. Close the previously-stubbed Cubemap/Custom skybox modes now that Phase 4 gives a real texture/cubemap pipeline to hang them on.

   ⚠️ **This is where the GL-version question comes due**, and it is the project owner's call. The 3.5 spike found the one thing GL 3.3 cannot compile: DiligentFX's BRDF LUT precompute uses `bitfieldReverse`, which needs GLSL 400 or `GL_ARB_gpu_shader5`. IBL needs that LUT. Three ways out, in rising order of scope: write the LUT with the engine's own shader (it is one full-screen pass and the Hammersley sequence is trivial to write without `bitfieldReverse`); ask for the extension, which this machine's NVIDIA driver exposes even in a 3.3 context but Intel/AMD may not; or rebuild the vendored raylib as `GRAPHICS_API_OPENGL_43`, which also buys compute shaders and SSBOs for the PostFX effects and removes the whole class of "DiligentFX assumes more than 3.3" problems. Check what the PostFX components actually need before deciding — several of them are compute-based.
8. **Editor viewport integration.** Implement whatever Phase 0.3 spiked (GL-sharing or readback) so `viewportWindow.cpp`'s `R3D_SetResolution`/`DrawTexturePro` pattern becomes Diligent-scene-into-raygui-panel. Leave `raygizmo.c` (confirmed rlgl-based: `rlBegin`/`rlVertex3f`/`rlGetMatrixModelview`) as a raylib/rlgl overlay drawn after the Diligent blit into the same panel — continuing the existing compositing pattern rather than porting a third-party gizmo lib.

   **The GL half of this already landed early, in 2.d and 3.a**, and works: the renderer owns the scene target, `drawSceneTexture` blits it into the panel zero-copy, and the overlay brackets put rlgl's grid and gizmos into Diligent's own colour and depth attachments. What is actually left for this phase is the part below.

   ### What a non-GL backend costs the editor

   Consolidated here on purpose — these consequences were previously scattered across 0.2, 0.3 and 2.d and could not be seen together. Nothing here needs doing until the runtime actually moves off OpenGL; it is recorded so the cost is known in advance rather than discovered.

   - **The shipped game is unaffected.** It never calls `beginSceneOverlay`/`endSceneOverlay` and never blits into a panel — it renders the scene and presents. Moving it to Vulkan/D3D is a matter of implementing device and swap-chain creation in the backend instead of `AttachToActiveGLContext`.
   - **The editor's 2D chrome is unaffected either.** raygui keeps drawing through raylib's own GL context regardless of what renders the viewport; spike 0.1 ran raylib-GL and Diligent-Vulkan on the same HWND for 60 frames with no conflict. Phase 0.2's "keep raylib/raygui forever" decision survives a backend change intact.
   - ⚠️ **Exactly one seam breaks, and it breaks in two places — both in the viewport.**
     1. **Zero-copy delivery has no non-GL equivalent.** Today `ITexture::GetNativeHandle()` returns a GL texture name that gets wrapped as a raylib `Texture2D`. Under Vulkan/D3D there is no shared GL object. Fallback is the one spike 0.3 already named: CPU readback — slow, but acceptable for a preview pane. API-specific interop extensions are the faster alternative and considerably more work.
     2. **The depth-shared overlay stops being possible at all.** The grid and gizmos currently draw through rlgl *into Diligent's own depth buffer*, which only works because `rlLoadFramebuffer` can be pointed at Diligent's GL texture names. Without GL there is nothing to point it at. **The fix is the one 2.d's depth trap already identified as the better long-term answer regardless: the grid moves into the renderer as real scene geometry, and gizmos draw on top after the blit.** A grid is scene content, so this is a correction rather than a loss.
   - **D3D has a separate, earlier blocker that has nothing to do with rlgl:** DiligentCore's `try_compile` check for `atlbase.h` fails on this toolchain (MSYS2 clang64 ships MinGW-w64 headers, which have no ATL), so `D3D11_SUPPORTED`/`D3D12_SUPPORTED` are false. Vulkan+GL is what this toolchain can build; D3D needs a separate MSVC toolchain first. See spike 0.1.

   ⚠️ **Rejected, so it does not get re-proposed: keeping the editor permanently on Diligent-GL while the game ships Vulkan/D3D.** It is technically the cheapest path — the whole seam above keeps working untouched — and it was turned down by the project owner on the grounds that matter more than cost: **the editor viewport has to show the game as the shipped build renders it, one to one or as close as achievable.** A permanently different device path means shader and PSO divergence between the two would only ever surface in the game build, which defeats the point of having a viewport. The actual approach (readback, interop extensions, or something else) gets chosen when the move off OpenGL is actually on the table; it is deliberately not being decided now.
9. **Remove rlgl from engine+game.** **The R3D half of this phase moved to Phase 3.6** — what is left is dropping rlgl from engine/game code and `ASSIMP_DLL` from CMake (if Phase 5 scoped assimp to editor-only). This is the harder half: the Diligent backend's GL interop is written on rlgl, so removing it means the shipped game stops sharing raylib's context and owns a swap chain directly. Editor keeps raylib/raygui/rlgl for its own chrome per Phase 0.2.
   ⚠️ **Gap found in 2.d — the shipped game's 2D has no owner in this plan.** Phase 0.2 keeps raylib/raygui only for the *editor's* chrome, and this phase strips rlgl from the game — but the game draws its own HUD with raylib 2D (`DrawText`, `DrawFPS`) and has no replacement anywhere in Phases 1–11. A real horror/adventure game needs a HUD, menus and text, so this is a genuine missing subsystem, not a detail: either a Diligent-drawn 2D/UI layer for the runtime, or an explicit decision to keep raylib in the shipped game for 2D only (which conflicts with the SteamDeck/console dependency-trimming goal). **Decide before Phase 9, since that is where the game loses rlgl.** `example_game`'s currently-invisible 2D (see the 2.d notes) is the first thing that should be re-tested once this lands — it is the existing smoke test for it.

## Phases 10–11 — Greenfield (no porting burden, new subsystems)

10. **Skeletal/bone animation.** No existing BreadEngine code to port (confirmed zero hits for Animation/Skeleton/Bone). Use DiligentFX's `GLTF_PBR_Renderer`/DiligentSamples' GLTF viewer skinning support as the reference (joint indices/weights, joint matrix buffers, GPU skinning) rather than inventing it from scratch. Extend the Phase 5 assimp importer to bake skeleton hierarchy + bind pose + clips. This is a full subsystem (importer + runtime pose blending + GPU skinning + any editor preview tooling) — the GPU-skinning mechanics are well-referenced (good AI-assist fit), but animation runtime design (blending, state machines) is new architecture only you should own.
11. **Shader scripting/authoring.** Fills the two currently-dead placeholder fields (`Material::_shaderPath`, `GlobalLightSettings::_skyboxShaderPath`). Diligent shaders are HLSL, cross-compiled via DXC to SPIRV/GLSL/MSL, with `IShaderSourceInputStreamFactory` for `#include` resolution — a natural foundation. Compile user HLSL fragments into fixed material "shader slots" at runtime; hot-reload by watching file mtime and recreating the affected `IPipelineState` (cheap in Diligent, a common pattern). Scope is your call: a fixed-uniform HLSL-snippet system vs. a node-graph editor are very different sizes — the compile/hot-reload mechanics are good AI-assist material, but the authoring UX is a first-party design decision.

## Cross-cutting notes

- **Genre priority:** for horror/adventure, shadow quality (point/spot, not just directional) and post-processing (bloom/DOF/fog/color grading) likely matter more to final look than deferred shading or high light counts. Treat deferred shading (via Diligent's render-passes/subpass pattern) as an optional upgrade after Phase 9, not a Phase 3 requirement — don't conflate "reach parity with today's R3D rendering" with "modernize further."
- **Assimp** should end up import-time/editor-only after Phase 5, removing it from the shipped game's runtime dependencies — a free win for the SteamDeck-shipping goal.

## Rough effort (solo, part-time, order of magnitude)

Phase 0: days–2wk · 1: 2–4wk · 2: 3–6wk · 3: 4–8wk · 3.5: done in a day · 3.6: <1wk · 4: 2–4wk · 5: 4–6wk (was 4–6wk for ten generators; three are now dropped and one is done, so the mesh half shrinks) · 6: 4–8wk · 7: 2–3wk · 8: its GL half landed early in 2.d/3.a; what remains only becomes work when the runtime moves off OpenGL · 9: <1wk · 10: 2–4mo · 11: 1–4mo (scope-dependent). Phases 1–9 (full parity cutover) land around **6–12 months** part-time; Phases 10–11 are additional and can interleave once Phase 5's mesh pipeline exists.

AI-agent assistance fits best where there's dense reference material and no visual judgment call: PSO/SRB boilerplate, mesh-generator math, assimp import patterns, GPU-skinning mechanics, shader hot-reload plumbing. Keep for yourself: visual correctness review at every phase gate, the handle/interface boundary design in Phase 2, resource-lifetime/sync decisions, and any UX/authoring-surface design in Phases 10–11.

## Verification

- After Phase 2: load an existing scene under both `BREAD_RENDER_BACKEND=R3D` and confirm zero behavior change (same screenshot, same serialized files load without migration).
- After each of Phases 3–7: screenshot-diff the same test scene under R3D vs. Diligent backends; visually confirm lighting/shadows/post-fx look equivalent before moving on. **Superseded after sub-phase 3.c** — see the ⚠️ note under the Phase 3 sub-phase split: past textures and basic lighting there is nothing on the R3D side to diff against, so Phases 4–8 verify against reference output instead.
- After Phase 3.6: confirm editor and `example_game` both build and run with R3D fully removed from every link target, editor still runs its raylib/raygui chrome, and the scene renders exactly as it did under the Diligent build before the deletion — that last part is the whole test, since the deletion is meant to change nothing.
- After Phase 9: the same check for rlgl, with the shipped game owning its own swap chain.
- Phases 10–11: no direct R3D comparison possible (greenfield) — verify against known-good reference output (e.g. a public glTF skinned sample model) instead.

### Critical files
- `CMakeLists.txt`, `modules/engine/CMakeLists.txt`, `modules/editor/CMakeLists.txt`, `games/example_game/CMakeLists.txt`
- `modules/engine/engine.h` / `engine.cpp`
- `modules/engine/component/meshRenderer.h/.cpp`, `spriteRenderer.h/.cpp`, `light.h`, `camera.h`
- `modules/engine/data/material.h/.cpp`
- `modules/engine/configs/assets/textureAsset.h/.cpp`, `meshAsset.h/.cpp`
- `modules/engine/configs/light/*.h/.cpp`, `globalLightSettings.h`
- `modules/engine/systems/meshRendererSystem.cpp`, `lightSystem.cpp`, `globalLightSystem.cpp`, `cameraSystem.cpp`
- `modules/editor/editor.cpp`, `modules/editor/windows/viewportWindow.cpp`, `modules/editor/systems/gizmoSystem.cpp`, `lib/editor/raygizmo.c`
- `games/example_game/main.cpp`
