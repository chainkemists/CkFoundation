# CkPixelArtRenderer

**Purpose:** The t3ssel8r-style pixel-art renderer, render-side. Drives the scene to a low internal
resolution, snaps the camera onto that resolution's texel lattice, and replaces the engine's primary
spatial upscale with a box filter that puts the sub-texel snap remainder back as a UV shift. The
result is a chunky-but-sharp image whose pixels do not crawl when the camera moves.

**Depends on:** nothing Ck. Engine only — Core, CoreUObject, Engine, RenderCore, RHI (public);
Renderer, Projects (private).

**Used by:** `CkPixelArt`, which is the only thing that should configure it. Nothing else in the
suite depends on this module, and nothing here knows `CkPixelArt` exists.

---

## Why this module can use no Ck code at all

It loads at **PostConfigInit**, which is the last loading phase before the engine builds its global
shader map (`AppInit` loads PostConfigInit modules; `PreInitPreStartupScreen` then calls
`CompileGlobalShaderMap`; PreDefault/Default modules load only afterwards). A module that declares a
global shader type has to be loaded by then, and its shader-directory mapping has to exist by then.

`CkModuleRules` publicly adds `AngelscriptCode` (whose plugin module is `PostDefault`) and
`ApplicationCore`, neither of which can load that early — so this module uses plain `ModuleRules`,
exactly like the sibling `CkIskmRendererVF`. The consequences are visible throughout and are not
style drift:

- `ensureMsgf`, not `CK_ENSURE_IF_NOT` (the macro lives in CkCore). It is still an ensure and never a
  log-and-continue; see root `CLAUDE.md` non-negotiable #3 for what that rule is protecting.
- `!`, not the `NOT` macro. Plain structs with public members, not `CK_PROPERTY` accessors.
- `UE_LOG` with a plain category, not `CK_DEFINE_LOG_FUNCTIONS`.

The reflected, accessor-bearing configuration surface belongs in `CkPixelArt`, which is a normal
Default-phase `CkModuleRules` module.

**It also includes `Renderer/Private`.** `ISpatialUpscaler` is a renderer-private interface that the
engine explicitly does not keep source-compatible across versions. Budget for this module in every
engine upgrade.

## Key API

- `FCk_PixelArtRenderer_StateRegistry::Set/Clear/TryGet(World, Config)` — the ONLY channel in. Keyed
  per world, weakly, game thread only.
- `FCk_PixelArtRenderer_StateRegistry::Set_FrameReport/TryGet_FrameReport(World)` — what the renderer
  did to the frame currently being assembled: render size, displayed window, texel world size, the
  snapped origin and the remainder. `TryGet` reports ABSENCE for a report from an earlier frame,
  because a stale snap describes a camera that has already moved.
- `ck::pixel_art::Get_SnappedViewOrigin / Get_ViewBasis / Get_OrthoWidthFromProjection /
  Get_HorizontalMarginTexels` — the pure arithmetic, unit-tested in CkTests
  (`CkTests.UnitTests.CkPixelArtRenderer.*`).
- `UCk_Utils_PixelArtRenderer_UE` — the same functions as a Blueprint surface, plus
  `Get_ExactFraction`.
- `UCk_PixelArtRenderer_Subsystem_UE` — an engine subsystem whose only job is owning the scene view
  extension's lifetime.

Debug surface. All of it exists because the properties it checks cannot be checked any other way — a
batch of console commands has no frames between its lines, and a screenshot of `stat gpu` is not a
number:

| Command | What it answers |
|---|---|
| `ck.PixelArt.Debug.ToggleLoop <N>` | does enabling and disabling N times leave `r.ScreenPercentage` where it started? One toggle per frame, then a PASS/FAIL verdict |
| `ck.PixelArt.Debug.Pan <TexelsPerFrame>` | drifts the view target diagonally so pixel creep, whole-texel stepping and compensated motion can be told apart. Run it again with no argument to stop |
| `ck.PixelArt.Debug.PerfSweep <Seconds>` | mean GPU ms/frame with the renderer off, at 360 and at 180, measured and printed |
| `ck.PixelArt.Debug.SnapOnly` / `FreezeSnap` / `CompSign` | the three states of the snap, and the escape if the compensation sign is ever wrong |

## How one frame works

Three game-thread hooks in a fixed order inside `UGameViewportClient::Draw`:

| Hook | Site | What it does |
|---|---|---|
| `SetupViewFamily` | `GameViewportClient.cpp:1486` | Resolves the world's config, computes the geometry, drives `r.ScreenPercentage`, publishes the frame report |
| `SetupViewProjectionMatrix` | `:1687` via `LocalPlayer.cpp:1266` | Folds the render margin into the projection, snaps the view origin, records the remainder |
| `BeginRenderViewFamily` | `:1971` | `new`s the upscaler for this frame with the frame's geometry and remainder baked in |

The view family owns the upscaler instance and deletes it — allocate one per frame, never cache it,
never delete it. That instance IS the whole game-thread-to-render-thread transport, which is why
there is no lock and no shared mutable state on the render side.

## Contracts worth knowing before changing anything here

**Disabling is a lease, not an event.** Every frame the renderer should run, `SetupViewFamily` renews
it; a frame that ends without a renewal restores `r.ScreenPercentage` and forgets the saved value.
That converges from every path that can stop the renderer — console override, config cleared, world
torn down, activation functor false — instead of only from the one path someone remembered to hook.
There is deliberately no "on disabled" callback.

**Only one axis can be pixel-exact.** The renderer applies ONE resolution fraction to both axes and
takes `CeilToInt` of each. Width is the exact axis (`Get_ExactFraction` subtracts half a pixel so the
ceil lands on the target); height follows within a texel.

**The render margin cannot be spent evenly on both axes.** Two texels of extra width buys about 1.1
extra rows at 16:9 and fewer on an ultrawide, so `Get_HorizontalMarginTexels` solves for the width
whose vertical fallout still reaches the requested margin. `DoApply_ScreenPercentage` logs an Error
naming all three computed insets if any drops below one texel.

**The projection's vertical term is derived, not scaled.** `M[0][0]` is scaled by
`Inner/Rendered`; `M[1][1]` is then set from `M[0][0]` and the RENDERED aspect. Scaling both by the
same factor would leave texels a fraction of a percent non-square, because the rendered height is a
`CeilToInt`. Square texels are the premise of the look.

**The snap is orthographic-only; the margin fold is not.** One world-space snap displaces near and far
geometry by different screen amounts, so no snap corrects all depths under perspective — that is why
the style is orthographic. The fold applies either way, because the scene rasterizes into more texels
than are displayed regardless of projection type, and skipping it would CROP by the margin instead of
gaining it.

## Supported-feature matrix

| Thing | Status |
|---|---|
| Anti-aliasing | None or FXAA only. TSR and TAA switch the view to temporal upscaling, which structurally disables the spatial upscale slot this renderer occupies. Logged as a Warning on the transition |
| Dynamic resolution | Must be off — it fights the screen percentage the renderer drives |
| PIE | Preview only. A DPI-derived secondary fraction below 1 inserts a second, engine-owned resample after ours, which softens the image; the drive divides the settled fraction out from the next frame, so the internal geometry (and the snap) stays exact. Take visual verdicts standalone |
| Scene / reflection captures | Never pixelated themselves — they bypass the projection hook the snap lives in. A capture EXISTING in the world is fully supported: deferred captures render between the viewport's SetupViewFamily and BeginRenderViewFamily, so BOTH hooks identity-check the family against the game viewport (the gym's judge scene keeps a per-frame capture alive as the tripwire for this) |
| Split screen, stereo | Untested. The frame report is per world; the real granularity is per viewport |
| Perspective cameras | The margin fold applies; the snap does not, so pixels creep. Not a defect — see above |
| Zoom (changing ortho width) | Full-frame shimmer while it changes, stable once settled. The documented limit of the technique |

## Anti-patterns

- Registering the upscaler anywhere but `BeginRenderViewFamily`. The renderer checks all three
  upscaler slots are null before calling extensions and `checkf`s a double-assign.
- Caching or deleting the upscaler instance. The view family owns it.
- Restoring `GFastVRamConfig` by copying `AddDefaultUpscalePass` verbatim — it is `extern` WITHOUT
  `RENDERER_API` and a plugin module cannot link it. The VRAM hint is omitted deliberately.
- Reading `GFrameCounter` on the render thread. Both ends of the frame handshake are game-thread
  hooks.
- Adding a Ck dependency "just for one helper". It cannot load at PostConfigInit; put the code in
  `CkPixelArt` instead.
