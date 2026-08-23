# Migration log: r3d → DiligentEngine

An index, not a document. Each phase's record is its own file under [`docs/migration-log/`](docs/migration-log/) — **open the one file for the area you are about to work in, and no others.** It was one file of 168 KB until it became clear that one careless read would cost a third of a context window.

History, not instructions. The working document — current status, the rules that must not be broken, and what is still planned — is [DILIGENT_MIGRATION.md](DILIGENT_MIGRATION.md). If you find something here that is load-bearing and missing there, move it.

| phase | what it records |
| --- | --- |
| [`8a-editor-grid`](docs/migration-log/8a-editor-grid.md) | The editor's analytic grid and the overlay channel under it: why the engine ships no grid, why DiligentFX's shader header lost too, and the two pieces of GL state it left behind |
| [`7g-depth-of-field`](docs/migration-log/7g-depth-of-field.md) | Depth of field, landed after the phase closed: the CoC-weighted spiral gather, why it can't share reflections' same-texture trick, and the units gotcha the test scene's own default ran into |
| [`7f-screen-space`](docs/migration-log/7f-screen-space.md) | Ambient occlusion and reflections: the ambient target, the packed surface target, and why DiligentFX lost again |
| [`7-renderer-decomposition`](docs/migration-log/7-renderer-decomposition.md) | The renderer split into four objects: what each holds, what it borrows, and the order that was forced |
| [`7e-bloom`](docs/migration-log/7e-bloom.md) | Bloom: the chain, why DiligentFX was turned down, and the rlgl blend-function bug |
| [`7d-fog`](docs/migration-log/7d-fog.md) | Fog: the standalone depth-driven pass, height falloff, and the closed-form check |
| [`7c-image-based-ambient`](docs/migration-log/7c-image-based-ambient.md) | Image-based ambient, the first real HDRI, and the stabilisation pass after it |
| [`7b-skybox`](docs/migration-log/7b-skybox.md) | The skybox: Hosek-Wilkie and the equirectangular unwrap |
| [`7a-hdr-tonemapping`](docs/migration-log/7a-hdr-tonemapping.md) | HDR, the tone mapping operators, and colour grading |
| [`6d-omni-shadows`](docs/migration-log/6d-omni-shadows.md) | Omni shadows, and the raylib 6.0 / OpenGL 4.3 upgrade that landed beside them |
| [`6-lights-and-shadows`](docs/migration-log/6-lights-and-shadows.md) | All three light types, cascaded and spot shadows |
| [`materials-as-assets`](docs/migration-log/materials-as-assets.md) | Materials became project assets - `.mat`, and what that fixed |
| [`5-meshes-and-importer`](docs/migration-log/5-meshes-and-importer.md) | The six primitive generators and the assimp importer |
| [`4-materials-textures`](docs/migration-log/4-materials-textures.md) | `MaterialHandle`, texture refcounting, and colour space by hand |
| [`3-diligent-mvp`](docs/migration-log/3-diligent-mvp.md) | The Diligent MVP: sub-phases, the `PBR_Renderer` spike that was rejected, R3D removed |
| [`2-irenderer-seam`](docs/migration-log/2-irenderer-seam.md) | The `IRenderer` seam: the shape that was agreed, and the calls taken in 2.d |

## Writing a new entry

Keep it to what a later session cannot re-derive from the code: **the decisions taken and why the alternatives lost**, **what broke and how it was found**, and **the numbers that were measured**. Everything else is in the diff. An entry is worth what it saves someone from re-deriving, and prose costs tokens to write as well as to read.
