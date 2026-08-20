# Render verification tools

**"It compiles" and "it launches" prove nothing here.** Every real bug in the Diligent migration was silent at build time and visible only on screen. These scripts exist so the acceptance criterion can be a measurement instead of a look, and so they are not rewritten from scratch every session — which happened twice before they were committed.

Everything rests on one measured property: **a capture of a given config is bit-identical between runs.** Two separate launches of the same scene diffed to zero pixels. That is what makes `diff.ps1` a gate rather than an approximation.

## The scripts

| script | answers |
| --- | --- |
| `capture.ps1` | run the build, wait for it to settle, save the client area |
| `runlog.ps1` | what did it print? The renderer reports failures it survives instead of dying |
| `diff.ps1` | did this change alter the image at all? **The refactor gate** |
| `flatratio.ps1` | by how much did a post-effect scale the image, where its kernel cannot matter? |
| `grid.ps1` | what is roughly where, without looking at the image |
| `sample.ps1` | named texels, or one scanline across several captures |

`pixels.ps1` is dot-sourced by the others and is not run on its own.

## The two questions they split

**"This should change nothing"** — a refactor, a file split, a rename. Capture before, capture after, `diff.ps1`. Anything but `IDENTICAL` is a regression until explained.

**"This should change the image in a way I can predict"** — a new post-effect. Predict a number, then measure it. `flatratio.ps1` does this for anything whose strength is a scalar: over regions flat enough that every tap of the kernel reads the same value, the kernel drops out and only the scale factor is left.

## Typical use

```powershell
$bin  = 'D:\Egor\BreadEngine\cmake-build-debug\bin'
$tool = 'D:\Egor\BreadEngine\tools\render'

# Refactor gate
& $tool\capture.ps1 -Exe $bin\ExampleGame.exe -Out before.png
#   ... change the code, rebuild BOTH targets ...
& $tool\capture.ps1 -Exe $bin\ExampleGame.exe -Out after.png
& $tool\diff.ps1 -A before.png -B after.png

# A post-effect at a predicted strength: bloom, additive, one level, intensity 0.25
& $tool\flatratio.ps1 -Base noBloom.png -Effect bloom.png -Expected 1.1068
```

## The frame probe — "which pass made it wrong?"

A screenshot only shows the end of the chain, so a pass that is correct and a pass that is corrupted downstream of it look identical. `BREAD_FRAME_PROBE` reads one texel of whatever colour target is bound, after every pass of one frame, and logs it:

```powershell
$env:BREAD_FRAME_PROBE = '640,1000'   # x,y from the top left, as these tools count
& $tool\runlog.ps1 -Exe $bin\ExampleGame.exe -Pattern 'Frame probe'
Remove-Item Env:\BREAD_FRAME_PROBE
```

```
Frame probe after clear:      0.051117, 0.073792, 0.116516, 1.000000
Frame probe after geometry:   0.794434, 0.780273, 0.721680, 1.000000
Frame probe after background: 0.794434, 0.780273, 0.721680, 1.000000
Frame probe after fog:        0.727539, 0.710449, 0.655762, 1.000000
Frame probe after bloom:      0.909180, 0.887695, 0.819336, 1.000000
Frame probe after composite:  0.956863, 0.949020, 0.913726, 1.000000
```

Every line is checkable in closed form, which is the point: `clear` is the background colour through `toSceneLinear`; `bloom` is `0.7275 x 1.25` for additive bloom at intensity 0.25; `composite` is `0.9092^(1/2.2)` quantized to 8 bits. A line that does not match its own arithmetic names the pass to look at.

It is compiled into release builds too — the cost when the variable is unset is one branch per pass, and a frame that is only wrong in release is exactly when it is wanted.

## The GL state guard

Debug builds check, at every hand-off to raylib, that the GL state matches what rlgl believes it left behind — blending and its function, depth test and write mask, culling and winding, sRGB conversion, pixel-unpack stride, bound samplers, colour write mask. Each disagreement is named once:

```
WARNING: GL state left for raylib is wrong: blend source factor is 1, raylib expects 770.
         Restore it in yieldToRaylib - rlgl will not.
```

rlgl tracks that state in software and only issues a call when its own copy changes, so anything Diligent sets and does not put back stays invisible until it produces a wrong frame somewhere unrelated. Five of the migration's Invariants are bugs of exactly that shape. Nothing to enable — if it is silent, the boundary is clean.

## Reading the numbers

- `diff.ps1` — `IDENTICAL`, or a differing-pixel count plus the largest per-channel delta. A handful of pixels at delta 1 is still a regression: this capture path has no noise in it.
- `flatratio.ps1` — the **mean** carries the verdict. The spread is always wider, because 8-bit quantization alone is ±5% where the base value is 20.
- `-Half` must exceed the effect's reach. A bloom chain of eight levels blurs further than one of one, and a flatness window smaller than the blur leaves the kernel in the answer.

## Watch out

- **Build every target you are about to run.** `BreadEditor` and `ExampleGame` share one `bin/`, and the shader copy is a post-build step on each — so building only one refreshes the shaders the *other* will load.
- **Shaders compile at runtime**, so a shader-only experiment needs no rebuild. But a rebuild re-copies `modules/engine/rendering/diligent/shaders/` over `bin/shaders/` and silently discards edits made there.
- **A probe that multiplies a suspect value by `1e-9` does not neutralise it** — `Inf * 1e-9` is `Inf`. Write a literal, and keep the resource alive with an added vanishing term instead; a plain `* 0.0` gets the texture dead-code-eliminated, and `GetVariableByName` then returns null into an unchecked dereference.
- The wait in `capture.ps1` is not padding. Environment images decode on a worker thread and the ambient precompute waits for the sky to settle; a short wait captures a frame that is correct but unfinished.
