*Part of the [migration log](../../DILIGENT_MIGRATION_LOG.md). History, not instructions — read it before working in this area, not otherwise. Anything here that must not be re-broken belongs in [DILIGENT_MIGRATION.md](../../DILIGENT_MIGRATION.md)'s Invariants instead.*

### After the HDRI — a stabilisation pass over everything 7.c touched

Requested by the project owner before moving on: a full review of the phase and the HDRI work that followed, for defects, leaks and lifetime problems. Eight fixes; **none of them was findable by building or by running**, which is the point worth keeping. The tree compiled clean, both hosts ran with no Diligent error on any stream, and every visible claim had already been verified on screen before this pass started.

**Two were mine, from this session, and both were breaches of a pattern the codebase already had right.**

- **`loadCubemap`'s decode job logged from a worker thread.** `Logger::Log` appends to a static vector and invokes `OnLog`, which the editor's console subscribes to; neither is synchronised. `createTexture`'s job does the identical decode and deliberately logs nothing — I had the precedent in front of me and broke it. The job now leaves a null loader behind and `finalizeCubemaps` reports from the render thread, which also gave the two later failure points (a texture that will not build, an image that is not 2D) the error message they never had.
- **A failed IBL shader compile took the process down inside `initialize()`.** `createIblPipelines` returned early on a compile failure, before creating the 1×1 fallback cube and before integrating the BRDF table — and `createScenePipeline` then dereferenced the null table. Since shaders are compiled at runtime, an ordinary edit under `bin/shaders` reached it, and the symptom was a log that stopped mid-initialisation because stdout had not flushed. **This is the third time the same shape has cost something this session** (the shader probe crash, and this twice over), which is why the rule about unchecked `GetStaticVariableByName` is now in Invariants alongside a second one about ordering. `createAmbientFallbacks` now builds both stand-ins before anything that can fail, and the two binds are guarded. Verified by renaming a symbol in `iblIrradiance.psh`: one legible error, the frame loop reached, a clean shutdown, and **exactly two** Diligent errors in the whole run — no per-draw validation spam, which is what proves the fallbacks were there.

**Two were decisions, both the owner's, and both went to the option that costs a little more code.**

- **Freeing a cube map while its image is still decoding no longer waits.** It could not simply recycle the slot — a job is writing into it — so `destroyCubemap` marks it abandoned and `finalizeCubemaps` reaps it on whichever frame the job lands. Without this, picking a second HDRI while the first was still loading put the four-second freeze straight back into the one gesture the asynchronous load exists for. `ResourcePool` gained `removeIf` for it, because `forEachAlive` gives a visitor no way to name the slot it is standing on. Verified with a temporary forced supersede one second into a decode: **"abandoned" is logged in the same second as the rebuild that caused it**, and the slot is reaped four seconds later when the job completes.
- **The image-based lighting now waits for the sun to settle; the sky does not.** Dragging the sun rebakes the dome, the shadows and the direct light together as before, and the two ambient cubes follow a quarter of a second after the inputs stop moving. The subtlety is that the previous ambient map has to *stay* meanwhile: clearing it per frame — which the first version did, because an invalid map was the signal that a precompute was owed — would flip the whole scene to the flat ambient colour and back on every frame of the drag. The signal is now `_ambientSource`, the sky handle the current ambient was built from, compared rather than flagged.

**Four were latent and quieter.**

- **The sky's branch condition was written twice**, in `describeSky` and again in `rebuildSky`. The snapshot's whole job is to record which sky will be built, and it was recording the inputs and re-deriving the branch. `BakedSky::isFlat` now carries it.
- **`BakedSky` compared the skybox asset by address.** Assets are rebuilt when a project reloads, so a freed pointer can compare equal to a different asset that lands at the same place — and the sky would silently never rebuild. It compares `getGuid()` now, which is what every other asset reference in the project is identified by.
- **`finalizeCubemaps` sat behind `endScene`'s early return**, so a decoded image could not land on a frame with no scene target. It runs before it now, behind a `_context` guard.
- **`EnvironmentBackgroudParameters::_skyboxTexture` was written twice and read nowhere** — a dead field that also serialised and drew an asset picker in the inspector, right next to the one that actually drives the sky. Deleted.

**Verified afterwards by repeating all four of this session's measurements** plus the new one: procedural sky unchanged (`bedbff` → `dbf4ff`), HDRI sky and its below-horizon band unchanged (`495762` → `a19d8b`), frames still produced mid-decode at eight seconds, exactly one sky rebuild and one ambient precompute over twenty seconds — the ambient arriving five seconds after the rebuild, which is the 4.2 s decode plus the settle — and both hosts clean.

**Left alone deliberately.** A material's binding keeps a destroyed ambient map's cubes alive until that material is next drawn, because a dynamic variable holds a reference; it is bounded to one map and self-healing. And `.hdr` is now accepted for material slots too, where `IsSRGB` is meaningless on linear float data — not reachable by accident, but worth knowing before someone tries it.

---

### After 7.c — the first real HDRI, and what it exposed

The project owner added `.hdr` to `File::isImage()` and imported a 4K sky, which closed both of the gaps 7.b and 7.c had left open — the equirectangular path and ambient from a loaded image both work. It also surfaced three defects, none of them in the code the HDRI was testing.

**The freeze was measured before anything was changed, and the measurement is the whole story.** Toggling any setting — SSAO, SSIL, bloom, none of which the renderer implements — froze the editor for about five seconds. Timers with `glFinish` on both sides of each stage:

| stage | ms |
| --- | --- |
| `loadCubemap`: decode the file | **4185** |
| `loadCubemap`: upload the source texture | 139 |
| `loadCubemap`: bake the cube at 1024² | 51 |
| `createAmbientMap`: irradiance | 2.4 |
| `createAmbientMap`: prefiltered | 19 |

All of 7.c costs 22 ms. The freeze was an 11 MB Radiance file being decoded by stb on the render thread, in a debug build — and being decoded again on every unrelated edit. **Guessing would have landed on the IBL bake**, which is the expensive-looking thing and is two orders of magnitude off.

**Defect one: one change flag for a whole config.** `GlobalLightSystem` rebuilt the sky whenever `GlobalLightSettings::isChangedFromEditor` was raised, and the inspector raises it for every field of every block it shows — post-effects included. `isChangedFromEditor` is no longer read at all; a `BakedSky` snapshot of exactly what a bake consumes is compared instead, which is the same re-derive-by-comparison idiom `MaterialAsset::getHandle()` and `isQuadStale()` already use. Only the fields the taken branch reads are filled in, so a mode nobody is in can never be the reason for a rebuild — the sun moving no longer rebuilds a cube map sky, which the old `trackProceduralSun` had to special-case. The comparison lives in `operator==` on the two parameter blocks, next to the fields it compares, with a comment naming the failure of forgetting to extend it: an inspector edit that does nothing.

Verified by logging every rebuild: **one** over eighteen seconds of running, where before it was one per edit.

**Defect two: the load was synchronous.** Now it is not — `loadCubemap` queues the decode on the existing `WorkerPool` and returns a handle to an empty slot, exactly as `createTexture` has always done for material textures; `finalizeCubemaps()` at the top of `endScene` polls with `wait_for(0)` and bakes the ones that have landed, before anything binds the scene target. `bakeCubemap` had to stop allocating its own slot and start filling one, because a loaded cube's slot exists before its image does. `IRenderer` gained `isCubemapReady`, and `GlobalLightSystem` generates the ambient map when an **invalid ambient map sits beside a ready cube** — no new flag, and it is `rebuildSky` clearing the map that says a precompute is owed.

Verified by capturing the game at eight seconds, mid-decode: the floor, the objects and the HUD are all rendering while the background is still the flat clear colour. Frames are being produced during the decode, which is the whole claim.

**Defect three, and the one worth remembering: the image's data stops above its own horizon.** A sky-only HDRI has nothing below the horizon and reads as pure black there — both an ugly lower half and, through IBL, an environment that lights nothing from below. The fix carries the horizon's colour down, dimmed by a ground albedo, which is what `skyProcedural.psh` already does for the analytic dome; it is opt-in per image (`SkyboxCubemapParameters::fillBelowHorizon`, default **off**) because a full spherical capture already has ground and silently discarding it is the worse failure.

The first attempt clamped the latitude to exactly `v = 0.5` and **changed nothing at all**. Decoding the file's scanlines directly found why: the first entirely black row is **1022 of 2048**, so the sky stops two rows *above* the midpoint and the clamp was sampling the very black it was replacing. The clamp now sits two degrees above the horizon, which is far enough inside the data for any such image and near enough that the colour is still the horizon's own — and because it is a `min`, it covers the empty rows just above the midpoint as well as everything below.

**Reading the file beat reading the code here.** "Black below the horizon" looks exactly like a wrong projection, and the projection was correct both times.

---

### Phase 7.c — image-based ambient lighting

Three decisions to the project owner up front, all taken as recommended, and the second is the one that shaped the shader.

- **Pooled ambient maps bound per material**, over one renderer-owned pair bound once. `IRenderer` is identical either way, so this was purely about the renderer's internals: each `AmbientMapHandle` owns its two cubes, the scene pipeline declares them DYNAMIC, and `submitDraws` re-points a material's binding only when the handle differs from what that binding already holds. The alternative — fixed textures rebaked in place, bound STATIC — would have been slightly cheaper and would have made `AmbientMapHandle` a lie, since only one map could ever exist.
- **The flat ambient colour stands in for a whole environment**, rather than staying a second formula on its own branch. This is exact rather than convenient, and the reason is that `ComputeIrradianceMap.psh` divides each sample by the cosine-hemisphere pdf, so what a cube holds is E/π and not E: a constant-radiance environment therefore convolves to *itself* in the irradiance map, and prefilters to itself at every roughness. Substituting a colour for both fetches is the same arithmetic, not an approximation of it. It cost a re-derivation of the ambient-only reference value Phase 6's shadow checks use — which turned out to be under 1/255 for a rough dielectric, see the verification below.
- **Rotation drives the ambient, energy does not.** `background.rotation` is applied to the lookup in the scene shader rather than re-baked, so it stays live and free and reflections line up with the sky as drawn; `background.energy` remains a background-only multiplier, so the two energy sliders never compound.

**DiligentFX supplied the mathematics and not the passes, and the split was deliberate.** `PrecomputeBRDF.psh` is used verbatim with its own `FullScreenTriangleVS.fx` — nothing about that integral is engine-specific and the table is read at exactly the two coordinates it is written at. `ComputeIrradianceMap.psh` and `PrefilterEnvMap.psh` were **not**: they come with `CubemapFace.vsh`, which orients a face with a rotation matrix and a `Pos.y *= -1.0` GL branch. That is a second cube-face convention standing next to the engine's own, and the plan's Invariants record that table being paid for twice already. So `iblIrradiance.psh` and `iblSpecular.psh` are engine shaders over the existing `fullscreen.vsh` and the existing face-axis table, and they `#include` `PBR_PrecomputeCommon.fxh` and `PBR_Common.fxh` for `Hammersley2D`, `ImportanceSampleGGX`, `SampleDirectionCosineHemisphere` and `SmithGGXSampleDirectionPDF`. The convolution loops are ours; none of the numerics are.

`bakeCubemap`'s face table moved to file scope on the way past, because the new bake needed it too and a second copy of those eighteen vectors is the failure this project has already had twice.

**Two things had to happen before the scene pipeline exists, and both are ordering, not logic.** The BRDF table is a STATIC variable of that pipeline, and a static variable can only be set while no binding has been created against it — so `createIblPipelines()` runs alongside `createShadowMaps()`, ahead of `createScenePipeline()`. And the 1×1 fallback cube has to exist before the first material, because a DYNAMIC variable starts out pointing at nothing and a draw validates every binding it has, including the ones a uniform branch will never read.

**Integrating the BRDF table is the first work in this project that draws before a frame is ever opened**, which is a new hazard rather than a new feature: raylib goes on to load its fonts and draw the editor's first frame through the same context, and it would do both into that table's framebuffer with the pass's own state still applied. `initialize()` now ends with `yieldToRaylib()`.

**One crash, and it was the diagnostic probe rather than the feature.** A temporary edit to the shipped `scene.psh` in the build directory replaced the final colour with a raw prefiltered fetch — which made `color` dead code, so the GLSL compiler eliminated the lights, the shadow arrays, the material textures and the BRDF table along with it. `createScenePipeline`'s chain of `GetStaticVariableByName(...)->Set(...)` then dereferenced null and the process died inside `initialize()`, with a log that ended mid-initialization because stdout had not flushed. Worth recording for two reasons: the pattern is pre-existing and used for every static resource, and a shader edit needs no rebuild — so this is reachable at any time by anyone experimenting in `bin/shaders`. The rule is in Invariants. A probe that keeps the other resources alive with a vanishing term (`probe + color * 1e-9`) has none of this problem.

**Verified in four parts, each isolating one link of the chain**, with both lights zeroed in the build directory's own copy of the scene so the frame was ambient and nothing else. `intensity: 0` on the sun is the right way to do that rather than deactivating the node — `trackProceduralSun` reads an active light only, and the sky's own sun elevation would have gone with it.

- **The irradiance cube, by the one signature a flat ambient cannot produce.** The floor, facing straight up, reads `75,90,113`; the cube's vertical side reads `66,78,96`. A vertical surface integrates half sky and half the brown ground albedo, so it must come out darker and less blue than a horizontal one — and under the old flat term the two were necessarily identical. Magnitudes check out too: undoing the encode and the 0.35 energy puts the floor's irradiance near 0.2 in red against 0.48 in blue, which is a clear sky.
- **The reflection cube, by probing its two ends in one frame** — mip 0 on one half of the floor and the last mip on the other. The mirror half reads sky colours that track the reflection direction (`d5f0ff` near the horizon through `b1c9ed` at steeper angles, the same family as the sky drawn above it), and where the reflection points below the horizon it reads the ground's brown (`34,28,8`). The rough half is a flat `78,89,164`-ish wash that barely varies with direction, which is what a fully convolved cube is supposed to look like.
- **The BRDF table and the Fresnel split, analytically, through the no-map branch.** With the camera on `SOLID_COLOR` the floor reads `85,92,101`; hand-computing the same path — ambient `66,78,96` taken as linear, `1 - F` at `f0 = 0.04`, the table's two terms at roughness 1, energy 0.35, then the gamma encode — gives `85.6, 92, 101`. That the arithmetic lands on the measurement to within a unit is what makes this a test of the table rather than of the plumbing.
- **The cost of the second decision, measured rather than assumed.** The same floor under the old formula would have read `85.6` in red against the new `85`. The Phase 6 shadow references survive intact.

Also confirmed: no Diligent error on any output stream in either host, in three full runs of the game and two of the editor; the sky, the shadows and the lighting are otherwise unchanged.

⚠️ **Untested, and for the same reason 7.b's equirectangular path is:** ambient generated from a loaded HDRI. It goes through the identical `createAmbientMap`, so only the source differs, but no image has been through it.

⚠️ **Not measured: what a rebake costs.** The maps are regenerated whenever the sun moves, which in the editor means every frame the light is being dragged, on top of the 1024² sky cube that was already being rebaked there. The reflection cube alone is roughly 17 million importance samples. Nothing has stuttered so far, but nothing has been timed either, and the knobs are the five constants at the top of `diligentRenderer.cpp`.
