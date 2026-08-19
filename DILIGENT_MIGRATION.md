# Migration plan: r3d → DiligentEngine

**The working document**: where the migration stands, the rules that must not be broken, what is still planned. Read it in full at the start of every session — it is kept short enough for that.

**History lives in [DILIGENT_MIGRATION_LOG.md](DILIGENT_MIGRATION_LOG.md)**: what each phase did, what broke, how it was found, what was verified. Open the section for an area before working in it; never read it end to end. Anything here that is short is short *because* the log has the long version.

**Why any of this**: rendering was 100% OpenGL through r3d, with no path to Vulkan/D3D — which blocks shipping a horror/adventure game on PC and eventually SteamDeck. Diligent is the target RHI. Strangler-fig, not big-bang: the tree stays buildable and runnable at every step. Architecture decisions stay with the project owner.

## Status

**Done:** Phases 1–6 (build system, the `IRenderer` seam, the Diligent MVP, materials and textures, meshes and the assimp importer, all three light types with cascaded/spot/omni shadows). R3D is deleted (3.6). `PBR_Renderer` was evaluated and **not** adopted (3.5) — DiligentFX is taken component by component instead. Materials became `.mat` project assets between phases. raylib is 6.0 built as `GRAPHICS_API_OPENGL_43`, so the context is 4.3 core.

**Phase 7 — in progress.** 7.a (HDR, tone mapping, colour grading), 7.b (Hosek-Wilkie procedural sky + equirectangular skybox) and 7.c (image-based ambient: irradiance cube, prefiltered reflection cube, preintegrated BRDF table) are built and verified on screen. Environment images decode on a worker thread; the sky rebuilds by comparing its inputs, and the ambient precompute waits for them to settle.

**Next:** 7.d fog · 7.e bloom · 7.f the normal+roughness target with SSAO and SSR on it. Then 8 (editor viewport), 9 (remove rlgl), 10 (skeletal animation), 11 (shader authoring).

**What renders today:** all seven primitives and imported models, textured, normal-mapped, lit by up to 32 directional/spot/omni lights with range and cone falloff; cascaded shadows from one directional light, maps for up to four spots and cube maps for up to four omni lights; ambient from a sky through its own irradiance and reflection maps; resolved through tone mapping and colour grading. In the editor viewport and in the standalone game alike. Still stubbed: every post-effect other than tone mapping.

⚠️ **Standing obligation:** the Hosek-Wilkie dataset is third-party, BSD-3-clause (`modules/engine/rendering/sky/LICENSE-HosekWilkie.txt`). Commercial use is fine, but **the shipped game's documentation must reproduce the notice**. Nothing in the build enforces this.

## Open decisions — the project owner's, not the agent's

- **Where the shadow-quality knobs live.** `SHADOW_DISTANCE`, cascade count, three shadow-map resolutions and two caster budgets are constants in `diligentRenderer.cpp`; `GlobalLightSettings` is where they belong if they should be per-project. Worth pricing now: four omni casters are 24 full-scene depth passes per frame, and nothing culls a caster against a light's range.
- **Who owns the shipped game's 2D** — the gap under Phase 9. The game draws its HUD with raylib 2D and Phase 9 strips rlgl from it. Must be answered before that.
- **A separate MSVC toolchain for D3D.** MSYS2 clang64 has no ATL, so `D3D11_SUPPORTED`/`D3D12_SUPPORTED` are false and D3D cannot build here at all. Vulkan+GL covers PC and SteamDeck.

## Invariants

Rules already paid for in bugs. Breaking one produces no compile error — it produces a silent, visual or data-loss failure weeks later. The log has the incident behind each.

**Serialization**

- A field that keeps its **name** through a restructure but changes meaning is worse than one that disappears: `deserialize` guards on `if (node[prop.name])`, so a removed name is harmless and a surviving one is loaded however little sense it makes. Migrate a restructured block by **deleting it from the config**; prevent it by renaming anything whose meaning changed.
- **Enums serialize by declaration index** (`magic_enum::enum_index`), not by name or underlying value. Append only, delete only from the tail. An out-of-range index falls back to the field's default.
- **New inspector fields are backward compatible** — a config written before a field existed keeps the C++ initializer. Removing or reordering is what breaks.
- **The assets registry must stay relocatable.** `_pathFromRoot` is the only source of truth; `_projectPath`/`_fullPath` are derived and never serialized. Assign either only through `childPathFromRoot`/`toFullPath` — a divergence makes the registry unloadable, which re-registers every file with fresh GUIDs and breaks every asset link in every scene.

**Matrices and geometry**

- **Upload `MatrixToFloatV(m).v`, never the bytes of a `Matrix`** — they are transposes of each other, so `memcpy` between them is a silent transpose. Everything crossing the Diligent boundary goes through `float16`.
- **A Diligent `float4x4` copies straight into a `float16` and stays usable as `mul(M, v)`** — the two conventions differ by one transpose each and they cancel — *provided the producer was asked not to transpose*. `DistributeCascadeInfo::PackMatrixRowMajor` defaults to false; the renderer sets it **true**. Wrong, and every shadow lands somewhere plausible but wrong.
- **DiligentFX's camera space runs +Z forward; the engine's runs −Z.** Handing `DistributeCascades` the engine's view matrix puts every cascade behind the camera. `renderCascades` builds a left-handed basis instead, and the pixel shader gets camera-space depth from `dot(worldPos - cameraPos, cameraForward)` — handedness-free.
- **The scene projection stays OpenGL's `[-1, 1]` depth range** while the rlgl overlay depth-tests against the scene's buffer. Near and far are *asked for* via `rlGetCullDistanceNear/Far`, never repeated — which is why raylib 6.0 changing those defaults cost nothing.
- **Vertex layout is the contract for every generator and the importer**: position, normal, uv, `float3` tangent with no handedness. Bitangent is `cross(tangent, normal)`. Front faces are counter-clockwise.

**Sharing an OpenGL context with raylib**

- **rlgl tracks GL state in software and only issues calls it believes are needed.** Anything Diligent changes that rlgl also tracks goes back in `yieldToRaylib()`: blending, depth test, *both halves of* face culling, bound sampler objects, `GL_FRAMEBUFFER_SRGB`. Depth writes match rlgl's default by coincidence, not design.
- **Face culling is two pieces of state; restoring one without the other is worse than neither.** `FrontCounterClockwise` defaults to *false*, so any pipeline that does not ask for CCW leaves `glFrontFace(GL_CW)` and every raylib triangle becomes a back face. **Signature: the whole UI vanishes and only lines survive** — lines are not subject to the cull test.
- **`InvalidateState()` resets Diligent's *cache* of GL state, not the state** — and not the rasterizer cache at all (`m_RSState`: cull mode, front face, depth bias survive it). Needed before a pass as well as after, and never a substitute for an explicit restore. rlgl's `rlEnableBackfaceCulling` (called by `raygizmo.c` on selection) is what changes those behind Diligent's back.
- **Anything that draws outside a frame hands the context back itself.** `initialize()` integrates the BRDF table with a real pass before any frame exists; raylib then loads fonts and draws into that pass's framebuffer and state unless `yieldToRaylib()` runs first.
- **Pixel-unpack state is restored at every upload site, not in `yieldToRaylib()`.** Diligent leaves `GL_UNPACK_ROW_LENGTH` at its last stride and raylib assumes the default. Fonts load during editor init, before any frame is yielded — symptom is the whole UI correct *except* that text is absent.
- **The GL context's version is raylib's decision and only raylib's.** `rlgl.h`'s public header is identical under `_33` and `_43`, so no consumer defines it. Anything needing more than the context gives is a raylib rebuild, never a backend workaround.

**Shaders**

- **Everything a draw needs unconditionally is built before anything that can fail.** The 1×1 ambient fallback cube and the BRDF table (`createAmbientFallbacks()`) come before shader compilation and pipeline creation, which return early. Wrong order and a shader typo becomes a null dereference inside `initialize()` instead of a logged failure.
- **A shader resource the GLSL compiler eliminates makes `GetStaticVariableByName` return null, and every call site dereferences it unchecked.** Dead-code elimination is enough: a `scene.psh` whose output stops depending on `color` drops the lights, shadow arrays, material textures and BRDF table together. **Reachable without a rebuild** — keep every resource alive in a probe with a vanishing term (`probe + color * 1e-9`). Symptom: a log that stops mid-initialization, because stdout has not flushed.
- **HLSL legal for D3D is not automatically legal through the GL converter.** No C-style casts, no implicit conversions (`float4 v = 0;`), **no scalar exponent to `pow`** (GLSL has no `pow(vec3, float)`), avoid `float3` cbuffer members, do not pass structs to functions. `HLSL2GLSLConverterImpl.hpp`'s known-issues comment is the list.
- Shader sources ship next to the executable and compile at runtime — **a shader edit needs no rebuild**.
- **`GL_SUPPORTED` must be defined by hand for any DiligentFX shader header.** Nothing on the GL path defines it, and an undefined macro is zero — so `PCF.fxh` silently takes its D3D branch and emits something with no GLSL counterpart. The renderer passes it from `IsGLDevice()`.
- **Including a DiligentFX `.fxh` needs the compound source factory** — those files are compiled into the static library, not shipped: `CreateCompoundShaderSourceFactory({&DiligentFXShaderSourceStreamFactory::GetInstance(), <file factory>})`.
- **Screen-space derivatives are only defined where control flow is uniform across a quad**, so `ddx`/`ddy` work is resolved *before* the per-light loop. A branch on a constant-buffer value is uniform and safe; a branch on the loop's light is not.
- **`Shadows.fxh` passes `ShadowMapAttribs` into its functions**, which the converter says never to do. It works only because none of them multiplies by a matrix member. Not licence to pass structs generally.
- **`TextureCubeArray.SampleCmp` works only from a pixel shader and fails silently elsewhere** — mapped to the literal `0.0` outside `FRAGMENT_SHADER`, so a cube shadow read from a vertex or compute stage reports *fully shadowed* with nothing on any output stream. Same for `Texture2DArray` and `TextureCube`.
- **MUTABLE and STATIC shader variables are both set once.** A static is set through the pipeline and an SRB created afterwards takes a copy. Anything *replaced* rather than edited must be DYNAMIC and set per binding — material textures are the first case, the environment cubes the second.

**The colour pipeline**

- **The scene target is linear, unbounded, floating point; the composite pass is the only encode.** Anything writing into the scene target writes linear values; anything reading the *output* texture reads an encoded image — which is why the rlgl overlay draws after the composite. A second encode anywhere is a washed-out frame that still looks plausible.
- **An authored `Color` written into the linear scene target is decoded by the exact inverse of the encode the composite will apply**, not by the piecewise sRGB curve — `toSceneLinear` raises by `1/encoding`, which also makes `OutputColorSpace::Linear` an identity both ways.
- **Ambient and light colours are *not* decoded; the background and the sky are.** `_ambientColor` goes through `ColorNormalize`. Deliberate and older than HDR, but it means a `SOLID_COLOR` scene is ~4× brighter in ambient than the same colour as a background — any prediction of an ambient-only pixel must use the undecoded number.
- **The overlay framebuffer's colour attachment is the output texture and its depth is the scene's own.** `rlUnloadFramebuffer` only deletes what it finds on the *depth* attachment, which is why `releaseSceneTarget` detaches depth and nothing else.
- **An equirectangular sky image is not guaranteed to hold data down to its own horizon.** The test HDRI goes black two rows *above* the midpoint, so clamping to exactly `v = 0.5` samples the black it was meant to replace; `skyEquirect.psh` clamps two degrees higher. Wider rule: **when a sky looks wrong below the horizon, decode the file before suspecting the projection** — both times the projection was right.
- **The sky model is in physical units and the engine's lights are not, and the sky is the side that converts.** `SKY_RADIANCE_SCALE` reconciles them where the model crosses into the engine — moving the lights onto a physical scale would invalidate every intensity already authored.
- **The irradiance cube holds E/π, not E**, because the estimator divides by the cosine-hemisphere pdf. So a constant-radiance environment convolves *to itself*, and prefilters to itself at every roughness — which is why substituting the flat ambient colour for both fetches is the same arithmetic, not a second lighting model, and why there is one ambient formula rather than two.
- **`background.energy` scales the sky as drawn; `ambient.energy` scales the light it casts.** Never let them compound. `background.rotation` applies to *both* — as a rotation of the ambient lookup in the shader, not a rebake, so it stays free.
- **`TonemapMode` reaches the shader as a number and is branched on there.** Uniform across the pass, so it costs nothing and the operator list stays exactly the serialized enum.

**Structure and lifetime**

- **The seam holds.** No Diligent type appears in engine, editor or game code: `rendering/diligent/` is the only directory including a Diligent header, and `renderer.cpp` names `DiligentRenderer` exactly once. Phase 8's move off OpenGL is only cheap while that stays true.
- **The renderer takes geometry, never a description of it.** `createMesh(const MeshData &)` is the only route to the GPU; generating primitives and importing models happen engine-side. That is what lets Phase 9 drop assimp from the shipped runtime.
- **A config's `isChangedFromEditor` is one flag for every block the inspector shows** — `GlobalLightSettings` carries the sky and five post-effect blocks alike. Anything whose rebuild is expensive compares the inputs it actually consumes; the comparison belongs in `operator==` next to the fields, because a field added and not compared becomes an inspector control that does nothing.
- **The inspector also mutates without raising that flag at all** — list `+`/`-` buttons and asset-link fields write straight through member pointers. Anything derived from that state re-derives by comparison. `MaterialAsset::getHandle()`, `MeshRenderer::getMaterials()`, `SpriteRenderer::isQuadStale()` are the precedents.
- **A snapshot taken to decide "has this changed" records the decision, not the ingredients for it.** `BakedSky` stores `isFlat`; deciding the branch again at rebuild is how a snapshot and its rebuild come to disagree.
- **An asset held across frames is compared by guid, never by address** — assets are rebuilt on project reload, and a freed pointer can compare equal to a different asset at the same address.
- **Decoding an image on the render thread is a multi-second stall.** `WorkerPool` plus a pool slot with a stable address is the pattern (`createTexture`, `loadCubemap`). A resource that arrives late needs a way to say so — `isCubemapReady` — and its dependents re-derive by comparison.
- **Nothing may reach `Logger` from a worker thread**: it appends to a static vector and invokes `OnLog`, which the editor console subscribes to; neither is synchronised. A job records the failure in its own slot and the render thread reports it.
- **Background work must be waited on explicitly** — a dropped `std::future` does not wait. Every path that frees or recycles a slot a job writes into joins it first.
- **…but "wait" and "free now" conflict on a frame path.** `destroyCubemap` marks the slot abandoned and `finalizeCubemaps` reaps it once the job lands; `ResourcePool::removeIf` exists because `forEachAlive` cannot name the slot it is standing on. Shutdown still waits — there is no later frame to reap on.
- **A component that owns a renderer handle releases it in `onDestroy()`.** `onDispose` runs at engine shutdown only, so relying on it alone leaks on every node deletion.
- **A component owning a renderer handle needs `noexcept` moves**, enforced by `static_assert`: `ComponentChunk` relocates through `move_if_noexcept`, and a copy deliberately drops GPU state. The trap: every component declares `~T() override = default`, and a user-declared destructor **suppresses the implicit moves**.
- **A copy of a render component carries what it is authored with and none of its GPU state** — handles, acquired-asset pointers and loaded flags are dropped and re-resolved next frame. Copy-assignment releases the destination first.
- **A destructor, and `unload()`, may reach the renderer only through `Renderer::isAlive()`.** `get()` throws once the renderer is down, and nodes and assets are destroyed after `Renderer::shutdown()`.
- **A component reaches its node only through `_owner`, and every add path must set it** — `ComponentsProvider::addImpl` once did not, so a component added in code crashed the moment it touched its `Transform`.
- **A light is the only render resource the renderer keeps between frames**, so skipping an inactive one leaves the last state applied rather than stopping it drawing. Anything else that becomes retained state inherits this.
- **The update phase skips inactive nodes at the runner; start-frame and end-frame do not.** A system that must observe deactivation cannot be an update system. Which phase is right depends on **where the resource is consumed**: lights and draws are read by `endScene` (start-frame is right), the camera by `beginScene` — and the two hosts disagree on order, so the deactivated-camera fix went into `CameraDirector::getActiveCamera()` instead. Do not generalise the light fix to the camera.
- **A light reaches the shader only through `_visibleLights`**, rebuilt every frame in `endScene` before the shadow maps are filled. Past `MAX_SCENE_LIGHTS`, directional wins and the rest go by distance minus range. **A frame with no directional caster leaves `iNumCascades` at zero**, which `Shadows.fxh` reads as fully lit — that is the guard, not a branch.
- **A loaded mesh always draws with a valid material** — the linked one or `Renderer::defaultMaterial()`. `endScene` skips a draw whose material does not resolve, so an unlinked slot would be *invisible* rather than untextured.
- **A material slot from a mesh source is never a safe index into the material list** — the inspector can shorten it without a flag. Every draw clamps (`std::min(materialSlot, lastSlot)`).
- **A material is a project file and nothing else is.** `MaterialAsset` (`.mat`) is the only thing holding a surface; `MeshRenderer`/`SpriteRenderer` hold a `MaterialLink`. A model brings geometry and material *slots* only, so **an imported model draws with the default material until a `.mat` is linked per slot** — the owner's call, not a bug to fix.
- **One `.mat` is one `MaterialHandle`, shared by every slot linking it.** That sharing is the point. A per-renderer instance is not a checkbox away, because a material's texture set is fixed once it exists — `MaterialLink::_isInstanced` is inert until something builds a second material.
- **`TextureAsset::acquire()`/`release()` are paired by whoever stores the handle, and the reference goes back to the asset it was taken from** — never to whatever an asset-link field names at release time, since the inspector rewrites those under the holder. The registry's eager load is uncounted, so the count governs sharing, not residency.
- **`MeshRenderer::_acquiredAsset` is both the release target and the ownership flag** — the inspector rewrites `_meshAsset` directly, and non-null is also what distinguishes borrowed parts from a primitive this component generated.
- **A texture's colour space is set by hand and by nobody else.** Nothing may write a `TextureAsset`'s `_withColor`, filter or wrap on its behalf — those are serialized and shared between materials. Accepted consequence: **a data map arrives with `_withColor = true` and is sampled through sRGB until unchecked by hand**, which is true of the test scene's own normal and ORM maps.
- **An asset's data lives in exactly one of two places, and `Asset::isStoredInOwnFile()` says which** — a texture's settings in `assets_registry.cnf`, a material's in its `.mat`. Storing both is silent data loss: the registry's copy deserializes second and wins.
- **A self-stored asset must be read outside a deserialization phase** — the generic deserializer *defers* an `ASSET_LINK` while one is open, and a deferred link is only ever delivered to a component. Load order satisfies it: `restoreEngineAssetsByFiles` runs after `AssetsDeserializer` closes its phase.
- **assimp flips V when importing glTF, so `aiProcess_FlipUVs` is mandatory.** It cannot desynchronize the tangent basis — only V flips, the tangent follows +u, the bitangent is derived.
- **A cube face's axes are the direction-to-texel rule read backwards, and all six have v pointing the way that feels upside down** (for +X: `s = -z, t = -y`, so v runs down along −Y). Derive the span from `NormalizedDeviceXYToTexUV(ndc) * 2 - 1`, not from the triangle's NDC — which way v runs against NDC is exactly what differs between GL and D3D. Paid for twice already.
- **A shadow bias moves the compared point *toward* the light, never away.** Push it the other way and every caster shadows itself — symptom is a caster uniformly dark with a bright one-texel silhouette rim, while a `castShadows: false` surface stays correct.
- **A slope-scaled bias is measured in shadow-map texels, never screen pixels.** The 2D paths take derivatives of shadow-map UV *and* depth together; the cube path has no UV and offsets along the normal by the texel's footprint instead.
- **A shadow projection's near plane is a constant, never a fraction of the light's `range`** — `range` is routinely thousands of units, and with `DepthClipEnable = False` the failure is silent.
- **"Brightest areas dark, edges lit" names a symptom, not a cause.** A near plane past every caster and a bias with the wrong sign produced identical appearances. Read it as "the comparison is failing where it should pass" and check each input.
- **A constant buffer's size and what is written into it are two separate declarations and nothing checks them.** A struct that outgrew its buffer corrupts driver memory and crashes somewhere unrelated frames later. `uploadConstants` now refuses an oversized write — the only reason this class of bug is loud.
- **`MeshPrimitiveType` is the one enum not serialized through `magic_enum`** (`static_cast<int>` into the primitive blob). The append-only rule still holds, and the two mechanisms agree only because its enumerators are contiguous from `None = 0` — never give one an explicit value.
- **A capsule's `height` is its straight section** (total `height + 2 * radius`). The serialized defaults are only coherent under that reading.
- **Node ids in `.nd` scenes are not arbitrary.** Hand-written ids 100–105 crashed scene loading with nothing on either output stream; 10–15 loaded fine. Cause never chased; check ids first when a hand-authored scene fails for no visible reason.
- **raylib and Diligent collide on two names.** `raymath.h` defines `PI` as a macro (`diligentRenderer.h` wraps its `ShadowMapManager.hpp` include in `push_macro`/`undef`/`pop_macro`), and it declares `Vector2/3/4` and `Matrix` *unguarded*, so it must come after `raylib.h`.
- **Never use bare `include_directories()`** — always `target_include_directories()`. A blanket call once made assimp's `texture.h` shadow DiligentCore's `Texture.h`.

## How to work on this

**Build with CLion's bundled CMake, never a system one** — `find_package` resolves differently and has already produced a build that configured under one and failed to link under the other:

```
"D:/Programms/CLion 2026.1.1/bin/cmake/win/x64/bin/cmake.exe" --build <build-dir> --target BreadEditor -j 14
```

**Build every target you are about to run.** `BreadEditor` and `ExampleGame` share one `bin/`, and the shader copy is a post-build step on each — so building only one refreshes the shaders the *other* will load. A stale exe against fresh shaders looks exactly like a renderer bug.

**Only one build at a time per directory.** Two ninja processes, or one killed mid-flight, leave **zero-filled `.obj` files** ninja then considers up to date: compilation "succeeds" and the link fails with `undefined symbol` for functions plainly defined. A valid COFF object starts `64 86`, a corrupt one `00 00`. Delete those and any `*.obj.tmp`, rebuild; no reconfigure needed.

**"It compiles" and "it launches" prove nothing here.** Every real bug in this migration was silent at build time and visible only on screen. The acceptance criterion is a pixel measurement or a screenshot, never an exit code. Prefer sampling specific pixels over reading a whole image; read the image only when you do not yet know what you are looking for.

**raylib is a build output, not a black box.** `lib/engine/` holds four headers and one `libraylib.dll`, deliberately not a submodule. To change its version or GL level: clone the tag into a throwaway directory, configure with the project's own clang64, copy those five files over, delete the directory.

```
cmake -S raylib -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=C:/msys64/clang64/bin/clang.exe \
      -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=OFF -DOPENGL_VERSION=4.3
```

**Compare the resulting DLL's import table against the one it replaces** — anything new there is a second C runtime that would have to ship alongside it. The targets link the `.dll` directly; no import library is needed.

**Delegate broad searches to a subagent** — "find every use of X", "how is Z done across this tree", especially across the vendored DiligentCore/DiligentFX/raylib sources. Keep decisions, diagnosis and implementation in the main session: a subagent starts cold and would have to re-read this file to do them.

**The test scene** (`games/example_game/assets/`) is one node per rendering path the migration must keep working: a Capsule at `x = -3`, a Cube at the origin with a deliberately non-trivial transform, `scene.gltf` at `x = +3`, a 40×40 Floor with `castShadows: false`, a directional SunLight, and a 4K sky HDRI. The Cube's transform is not decorative — under an identity matrix, a wrong model matrix and no model matrix look the same.

**Its ambient colour is not authored**: `GlobalLightSystem` copies the active camera's background colour into it while the mode is `SOLID_COLOR`, so `Root.nd`'s `_backgroundColor` is the real knob and `energy` is the only ambient field surviving from `global_light_settings.cnf`.

**To update the scene, edit it in the editor and copy `Root.nd`, `assets_registry.cnf`, `project_settings.cnf` and `global_light_settings.cnf` back out of `bin/assets/game/`** — the build only copies what is absent and never overwrites build-directory state.

**Code left behind carries no migration narrative.** Comments about phases, about what used to be, or about what will replace this belong in these two files, never in a header. A comment earns its place only by explaining something non-obvious about the code as it stands.

**At the end of every session**: update Status here, move any newly discovered load-bearing rule into Invariants, put the incident itself in the log. This pair of files is the only handoff between sessions.

## What remains

**7.d–7.f — the rest of the environment phase.** The parameter blocks in `configs/light/` are already engine-native and serialized, and `setEnvironment` is the single place that maps them onto DiligentFX's PostFX components — call sites should not change. **DiligentFX is adopted component by component** (3.5's decision, kept here so it is not re-argued): `PostProcess/` — Bloom, DepthOfField, ScreenSpaceAmbientOcclusion, ScreenSpaceReflection, TemporalAntiAliasing — and `Components/ToneMapping.hpp` are what 7.d–7.f reach for; `Components/ShadowMapManager.hpp` was Phase 6's. The IBL precompute was **ours to write** because `PrecomputeCubemaps` is a method on `PBR_Renderer`, and 7.c wrote it. 7.f's SSAO and SSR commit the scene pass to a second render target (normal+roughness). ⚠️ The append-only serialization rule is **lifted for `configs/light/` for the duration of Phase 7**, by the owner: blocks there may be restructured, and SSGI/SSIL may be pruned.

**8 — editor viewport.** `Components/CoordinateGridRenderer.hpp` is what the grid becomes once it moves out of rlgl and into the renderer as real scene geometry. The GL half already landed in 2.d/3.a and works: the renderer owns the scene target, `drawSceneTexture` blits it zero-copy, and the overlay brackets put rlgl's grid and gizmos into Diligent's own colour and depth attachments. **Nothing here needs doing until the runtime moves off OpenGL** — recorded so the cost is known in advance:

- The shipped game is unaffected (it never blits into a panel), and so is the editor's 2D chrome (raygui keeps its own context).
- **Exactly one seam breaks, in two places, both in the viewport.** Zero-copy delivery has no non-GL equivalent — `GetNativeHandle()` returns a GL texture name today; the fallback is CPU readback, slow but acceptable for a preview pane. And the depth-shared overlay stops being possible at all, because `rlLoadFramebuffer` has nothing to point at. **The fix is the one 2.d already identified as better regardless: the grid becomes real scene geometry in the renderer, and gizmos draw on top after the blit.**
- ⚠️ **Rejected, so it is not re-proposed: keeping the editor permanently on Diligent-GL while the game ships Vulkan/D3D.** Technically the cheapest path, turned down by the owner — the viewport has to show the game as the shipped build renders it, and a permanently different device path means shader and PSO divergence that only ever surfaces in the game build.

**9 — remove rlgl from engine and game**, and `ASSIMP_DLL` from CMake. The harder half: the Diligent backend's GL interop is written on rlgl, so removing it means the shipped game owns a swap chain directly. The editor keeps raylib/raygui/rlgl for its own chrome, permanently. Blocked on the open decision about the game's 2D.

**10 — skeletal animation.** Nothing to port (zero hits for Animation/Skeleton/Bone). Reference DiligentFX's `GLTF_PBR_Renderer` and the DiligentSamples GLTF viewer for joint indices/weights, joint matrix buffers and GPU skinning; extend the Phase 5 assimp importer for skeleton, bind pose and clips. The GPU-skinning mechanics are well-referenced; **animation runtime design (blending, state machines) is new architecture the owner should own.**

**11 — shader authoring.** Fills `MaterialAsset::_shaderPath` and `GlobalLightSettings::_skyboxShaderPath`, both dead today. Recorded shape, requested by the owner:

- **One file per shader, not two.** `scene.vsh`/`scene.psh` are one shader split across two files; the asset is a single file holding both stages.
- **Its fields draw in the inspector like a component's**, through `INSPECT_FIELD`/`INSPECTOR_BEGIN`. **What those fields are is the open question**: a fixed set the engine declares, or a set parsed from the shader source. Today a `Property` list is built once per *type*, statically — per-*asset* fields are new ground.
- **`MaterialAsset` links it like it links a texture** — `ShaderAsset *` as an `ASSET_LINK`, so it resolves, serializes and gets a picker for free, and the material rebuilds by the same comparison `getHandle()` already does.
- It inherits the material-asset rules: own data in its own file, read outside a deserialization phase.

**Genre priority, for scope calls:** for horror/adventure, shadow quality and post-processing matter more to the final look than deferred shading or high light counts. Deferred is an optional upgrade after Phase 9, not a requirement — do not conflate "parity with what R3D did" with "modernize further."

**Remaining effort, order of magnitude (solo, part-time):** 7 has 2–3 weeks left · 8 only becomes work when the runtime moves off OpenGL · 9 <1 week · 10 is 2–4 months · 11 is 1–4 months, scope-dependent.

## Verification

**Screenshot-diffing against R3D is over** — there has been no R3D to diff against since 3.6, and it stopped being useful at 3.c. Verify against known-good reference output and the scene's own expected behaviour instead. Every phase gate so far was met by pixel measurement; the measured values and what each one proved are in the log.

Four checks are worth repeating whenever the environment path is touched, because each isolates one link:

1. **Procedural sky** — a vertical scan zenith to horizon reads `bedbff` → `dbf4ff`, blue with B highest throughout.
2. **HDRI sky and its below-horizon fill** — the band under the horizon reads the horizon's own colour dimmed by the ground albedo, varying along it, not `000000`.
3. **Frames during a decode** — capture the game ~8 s in: floor, objects and HUD rendering while the background is still the flat clear colour.
4. **Rebuild economy** — log every sky rebuild and ambient precompute, run ~20 s: exactly one of each.

And one for the renderer's failure path: **rename a symbol in a shader so it will not compile, launch, and confirm the renderer logs and keeps running** instead of dying inside `initialize()`.

### Critical files

- `CMakeLists.txt` and the three module/target ones
- `modules/engine/engine.h`/`.cpp`
- `modules/engine/rendering/IRenderer.h`, `renderer.cpp`, `resourcePool.h`, `diligent/diligentRenderer.{h,cpp}`, `diligent/shaders/`
- `modules/engine/rendering/geometry/{primitiveGenerator,modelImporter,meshData}.*`, `rendering/sky/hosekWilkie.*`
- `modules/engine/component/{meshRenderer,spriteRenderer,light,camera,cameraDirector}.*`, `component/core/componentChunk.h` (its `resize` growth is what forces the `noexcept` moves)
- `modules/engine/data/materialLink.*`, `configs/assets/{materialAsset,textureAsset,meshAsset}.*`, `configs/light/*`
- `modules/engine/systems/{meshRendererSystem,lightSystem,globalLightSystem,cameraSystem}.*`, `systems/core/systemsRegistry.cpp` (which phases skip inactive nodes)
- `modules/editor/editor.cpp`, `windows/viewportWindow.cpp`, `systems/gizmoSystem.cpp`, `lib/editor/raygizmo.c`
- `games/example_game/main.cpp`
