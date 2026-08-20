# CkPixelArt — 3D pixel-art renderer (t3ssel8r-style) — mission brief (PROMPT.md)

> **Written:** 2026-08-20. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** the campaign ships and `Source/CkPixelArt*/Claude.md` carries the
> permanent contract. On death: delete or tombstone.
>
> **Status: LOCKED 2026-08-20.** The maintainer approved execution ("offload the campaign to an
> Opus executor"), which locks D1–D8 as written — including D1's scoped re-open of the
> SceneViewExtension ruling (the Stylize-scope ruling stays in force for its own effect class).
> D8 remains deliberately empirical: it is a bounded decision GATE inside PHASE_0.md with an
> enumerated branch table, not a design choice — the executor follows the gate, never invents.
> Phase docs: [PHASE_0](PHASE_0.md) · [PHASE_1](PHASE_1.md) · [PHASE_2](PHASE_2.md) ·
> [PHASE_3](PHASE_3.md) · [PHASE_4](PHASE_4.md) · [PHASE_5](PHASE_5.md) · [PHASE_6](PHASE_6.md) ·
> [VALIDATION](VALIDATION.md).
> Research basis: [RESEARCH_Technique.md](RESEARCH_Technique.md) (the technique corpus, verified
> math + shaders), [RESEARCH_UeApis.md](RESEARCH_UeApis.md) (engine integration, verified at
> file:line against the 5.7.4 fork), [RESEARCH_Codebase.md](RESEARCH_Codebase.md) (CkUsf/CkCamera
> state).

---

## Goal

A production-quality t3ssel8r-style 3D pixel-art renderer in CkFoundation: the scene renders at a
low internal resolution through an orthographic camera whose position is snapped to the texel
grid, gets its outlines and quantized toon look at that resolution, and is upscaled to native
output with a sharp box-filter that re-applies the sub-texel camera remainder — crisp stable
pixels, no pixel creep during translation, smooth scrolling, native-resolution UI. Enable/disable
at runtime with zero residue; every public API usable from C++, Blueprint, and AngelScript.

The technique in one line (full math in RESEARCH_Technique):

```
ortho camera, position snapped to view-aligned texel lattice
  → scene + PP (outline/toon/palette) rendered at e.g. 640×360 (+margin)
  → custom primary spatial upscaler: box-filter upscale, UV-shifted by the snap remainder
  → Slate/UMG composites after, at native res (engine guarantees this)
```

## Why this architecture (the one-paragraph version)

The engine has a purpose-built slot for exactly this: a **custom primary spatial upscaler**
(`ISpatialUpscaler`, registered per-frame from a SceneViewExtension's `BeginRenderViewFamily`)
fully replaces the default upscale pass (`PostProcessing.cpp:1949-1983`), while a low screen
percentage (floor 1%; 640×360 from any output is legal) makes the ENTIRE pipeline — lighting,
shadows, and every post-process material including CkUsf looks — genuinely render at texel
resolution. The two-days-old commercial plugin doing this ("Stable 3D Pixel Art", Fab) has the
exact limitation fingerprint of this design. The alternatives are structurally worse: a
SceneCapture doubles view cost and diverges features; PP-material-only pixelation has no true
texel grid, no perf win, and shimmers (confirmed dead end). CkUsf's blendable vehicle cannot
change render resolution — which is why this campaign needs D1.

## Success criteria (observations, not activities)

1. In the PixelArt gym at 640×360 internal on a 1440p output: slow diagonal camera translation
   shows **no pixel creep** (static geometry's pixels do not crawl) AND **no stutter** (motion is
   sub-pixel smooth), verified by eye and by a slow-motion capture. `[EDITOR-VERIFY]` — and
   because PIE's DPI-driven secondary upscale distorts the pipeline, the sign-off run is
   **standalone**, at two output resolutions including one non-integer texel ratio.
2. Outlines are exactly 1 texel wide at internal resolution: silhouettes darken, creases
   brighten, no doubled crease lines, no staircase false-positives on the gym's ground plane at
   grazing angles. Stable under translation. `[EDITOR-VERIFY]`
3. UMG test widget renders at native resolution, crisp, above the pixelated scene.
4. Disabling the subsystem at runtime restores stock rendering with zero residue — no active
   view extension work, no upscaler, no lingering cvar overrides; toggling 100 times leaks
   nothing (memory report delta-zero).
5. AutoTests cover: snap math (view-aligned lattice + remainder, property-tested), pixel-exact
   fraction computation (CeilToInt lands on the target for a sweep of viewport sizes), ortho
   attribute flow on the camera entity, and subsystem enable/disable request contract.
6. Every public API exercised from C++, Blueprint, AND AngelScript (non-negotiable #4).
7. Full CkTests suite delta-zero against the baseline captured at Phase 0 entry (counts + names).
8. GPU frame time with the renderer ON (640×360 internal + upscale) is measured against native
   baseline in the gym scene and recorded — no estimates (non-negotiable #7).

## Constraints & proposed decisions

| # | Decision (PROPOSED) | Why |
|---|---|---|
| D1 | **Scoped re-open of the "no SceneViewExtension" ruling.** The pixel-art renderer's vehicle is a SceneViewExtension + custom primary spatial upscaler + plugin global shaders. The prior ruling (`Source/CkUsf/PROMPT.md:40,78`, `PROGRESS.md:56`, maintainer-approved 2026-08-06) **stays in force for Stylize-class effects** — its rationale ("the look pipeline already does the job; SVE loses validator/MID/AS integration") is correct for color-grading-shaped passes and does not cover changing the resolution the scene renders at, which no blendable can do. | A blendable cannot occupy the upscale slot or change screen percentage. The engine slot for this exists since 4.27 (`ISpatialUpscaler`), the repo already contains a working UE 5.7 SVE+global-shader reference (`Plugins/ScreenSpaceFogScattering`), and the renderer-private include is pin-safe on our fork. **REQUIRES EXPLICIT MAINTAINER SIGN-OFF.** |
| D2 | **Two new modules, mirroring the CkIskmRenderer / CkIskmRendererVF split:** `CkPixelArtRender` (Runtime, **PostConfigInit** — shader dir mapping `/CkPixelArt`, global shaders, the SVE, the upscaler, screen-percentage handling; deps Core/Engine/RenderCore/**Renderer + Renderer-Private include**/RHI/Projects/CkCore/CkLog/CkSettings — NO CkEcs/CkUsf) and `CkPixelArt` (Runtime, Default, T4 — the world subsystem, presets, project settings, CVars, AS/BP surface, CkUsf look application; deps CkPixelArtRender, CkUsf, CkCore, CkEcs, CkLog, CkSettings). | Global shaders against a mapped path need PostConfigInit (precedent: CkIskmRendererVF, engine's CompositeCore/LensDistortion/OpenColorIO). A single PostConfigInit module depending on CkUsf would drag CkUsf→CkEcs→… into the PostConfigInit load order — an untested ripple the VF-style split avoids. PostConfigInit gotcha to copy from CompositeCore: config-driven cvars aren't applied yet in StartupModule. |
| D3 | **The camera snap lives render-side, in `ISceneViewExtension::SetupViewProjectionMatrix`** — snap `ViewOrigin` to the view-aligned texel lattice (texel = OrthoWidth / InternalW, in the view's right/up basis), stash the sub-texel remainder, and hand it to the upscaler via the per-frame `new FUpscaler(remainder, …)` in `BeginRenderViewFamily` (family-owned lifetime = race-free transport). CkCamera is NOT modified for snapping. | This hook runs after PlayerCameraManager modifiers (shakes) — it is the last word on the view origin, so shakes cannot un-snap the frame (an ECS-side snap would be perturbed: `ApplyCameraModifiers` runs after `GetCameraView`). It also works for any camera, not just CkCamera-driven ones — framework-general. Accepted consequence: gameplay reads the unsnapped camera (divergence ≤ 1 texel); world-space-widget projection may diverge by ≤ 1 output pixel — a `Get_SnappedView` query utility is provided for consumers that care. |
| D4 | **CkCamera gains orthographic support as its own deliverable** (useful independent of pixel-art): `_OrthoWidth` as a per-camera Sensor attribute (the mechanical five-site pattern, `CkCamera_Utils.cpp:370-709`, + `Acquire_CameraModifier_OrthoWidth` for layer blending), projection mode as a non-blending plain field + `Request_Set_*` (template: `Request_Set_ConstrainAspectRatio`), **explicit ortho near/far planes by default** — auto-planes are OFF because the engine's auto far plane is a function of the view rect (`CameraStackTypes.cpp:386-473`), which our low-res rendering changes. Written into `_ViewInfo` at the single assembly site (`CkCamera_Processor.cpp:293-304`). | The ECS `_ViewInfo` is the sole render authority (`CkCamera_Component.cpp:16` whole-struct assign); nothing else can set ProjectionMode/OrthoWidth. Resolution-dependent auto planes would silently change scene depth range when the pixel-art renderer toggles. |
| D5 | **Upscale filter = t3ssel8r's box filter** (single bilinear tap at a derivative-shifted UV, box clamped to ≤1 texel, `SampleGrad` for stable mips), with `uv += SubTexelOffset` prepended. **Render margin: +2 texels per side** — internal size = target + 2×margin, ortho width scaled by (target+2·margin)/target, the upscaler samples the inset window shifted by the remainder. Internal resolution is authored as either an explicit WxH or an integer texels-per-pixel scale; pixel-exactness via `Fraction = (InternalW − 0.5) / ViewportW` so the engine's CeilToInt lands exactly. | The box filter is the correct-minification superset of the "sharp bilinear" family (survey in RESEARCH_Technique §B). Margin is the standard fix for the shift-back reading unrendered edges (±½ texel bound; 2 gives outline-kernel headroom). |
| D6 | **Outline + toon = ONE new CkUsf PostProcess-domain look** (`PixelArt.ush` + LookDefinition + AS asset, riding the existing generator/validator/MID pipeline unchanged): 4-tap depth silhouette with grazing-angle-scaled threshold (darken), opposed-pair normal-contrast crease detection with de-doubling bias (brighten), optional palette-band-shift edge coloring (the t3ssel8r signature: convex→next-brighter band, concave→next-darker, decided geometrically pre-quantization), band-quantized lighting + palette using `StylizeCommon.ush`. CelShade/ScreenDither are NOT modified. Placement `SceneColorAfterDOF`; it runs at internal resolution automatically because every PP pass precedes PrimaryUpscale. | Two pre-TAA looks do not compose (`CkUsf/Claude.md:453`) — the pixel-art look must be self-contained. The house look pipeline gives validator + MID + AS authoring for free, and all the quantize/palette/dither library code already exists. Shader recipes are fully specified with verbatim reference code in RESEARCH_Technique §C/§D. |
| D7 | **Preconditions are validated loudly, never silently forced.** Enabling requires: AA method None/FXAA (TSR/TAAU structurally disable the upscale slot — `SceneView.cpp:1012-1019` + `PostProcessing.cpp:600`), dynamic resolution off, `ScreenPercentage` show flag on. `Request_SetEnabled` fails with a diagnostic naming the offending state (CK_ENSURE + `Failed`), and a separate explicit `Request_Apply_RecommendedCVars` opts into having the subsystem set them. The supported-feature matrix (VSM-not-CSM for ortho shadows, SSAO off, no light functions, no split-screen/captures/stereo) ships in the module Claude.md. | Non-negotiable #3 (no silent handling) + Resilience tenet 7 (fail-closed with a bounded escape). A renderer silently flipping global cvars is a split-brain waiting to happen. |
| D8 | **The screen-percentage delivery mechanism is decided empirically in Phase 0**, choosing among: (a) drive `r.ScreenPercentage` (survives — `MinResolutionScale` is 0.0; least invasive; per-frame updatable), (b) `SetScreenPercentageInterface_Unchecked` replace in `SetupViewFamily`, (c) custom GameViewportClient. Phase 0's spike report records the choice and why. | `UGameViewportClient::Draw` installs its own driver (`GameViewportClient.cpp:1626/1910`) and double-assign asserts; which override point is cleanest is an empirical question not worth deciding on paper. |

## Non-goals

| Out of scope | Why |
|---|---|
| Split screen, stereo/VR, SceneCapture support, multiple simultaneous game views | Per-view snap bookkeeping + the local-player-only projection hook; same exclusions as the commercial plugin. Captures bypass `SetupViewProjectionMatrix` by construction. |
| Perspective-camera stabilization | Mathematically unsolvable by snapping ("no single grid snap compensates for all depths"). Ortho-only is the technique. |
| Texel splatting (dylanebert) as the renderer | A fundamentally different architecture (cubemap G-buffers + quad splatting). Recorded in RESEARCH_Technique §A as the rotation-stable alternative; its outline rule + OKLab posterization ARE borrowed by D6. |
| Coexistence with TSR/DLSS/FSR/XeSS/dynamic res | Structurally impossible (the temporal path disables the spatial upscale slot) and aesthetically wrong. |
| Per-material toon ramp authoring for BusterBlock content | Content-side adoption, separate effort. The framework provides the post-quantization path; the per-material highlight/midtone/shadow ramp model (t3ssel8r's actual material system) is a content/material-library follow-up. |
| God rays, cloud shadows, grass/water/firefly exemplars | Phase 7 stretch / backlog — content-side recipes are documented in RESEARCH_Technique §E; the god-ray raymarch needs shadow-map access from a custom pass (engine-coupled) and must not gate the core renderer. |
| Migrating Stylize subsystems to the SVE vehicle | D1 explicitly keeps the prior ruling for them. |
| Rotation-stable orbiting | Rotation re-rasterizes by construction; fixed/stepped camera angles are the design (Fab plugin has the same limitation). Zoom = continuous re-snap with accepted transition shimmer (bababuyyy model). |

## Phase map

Details land in `PHASE_N.md` files as each phase is authored; this is the stable skeleton.

| Phase | Deliverable | Exit gate |
|---|---|---|
| 0 | **De-risk spike** (scratch branch, throwaway allowed): minimal SVE + hardcoded global-shader upscaler + screen-percentage route on the fork; prove 640×360→native sharp upscale in PIE and standalone. Resolves D8; empirically checks: PP-materials-at-internal-res, ortho light functions, auto-plane behavior at low res, PIE DPI distortion. **Baseline capture:** full CkTests suite counts + failing names. | Spike renders; PHASE_0 findings doc written; D8 chosen; maintainer go/no-go on D1 confirmed against a real image. |
| 1 | `CkPixelArtRender` module: mapping, box-filter global shader (+ remainder offset), driver per D8, SVE lifecycle (engine-subsystem owner, teardown via false IsActive functor), `Fork_GameThread`, `OverrideOutput` contract, CVars (`ck.PixelArt.*`). | Upscaler runs gated + toggleable, zero residue off; unit tests for fraction exactness green. |
| 2 | Snap + margin + remainder: `SetupViewProjectionMatrix` view-aligned lattice snap, margin fold (fraction + ortho width scale), remainder→upscaler, zoom re-snap policy. | No-creep + no-stutter demonstrable with a debug fly camera; snap math property tests green. |
| 3 | CkCamera ortho support per D4 (parallelizable with 1–2). | Ortho attribute flow autotests green; a CkCamera-driven ortho view renders through the stack. |
| 4 | `CkPixelArt` module: subsystem + preset asset + project settings + CVar fold-in (mirror the Stylize three-layer flow), AS/BP/C++ surface, precondition validation per D7. | Request contract autotests green; 3-environment exposure verified. |
| 5 | The `PixelArt` look per D6: outline (silhouette/crease/band-shift) + band quantization + palette. | Success criteria 1–2 pass in the gym `[EDITOR-VERIFY]`. |
| 6 | PixelArt gym in CkTests + full test pass + docs (both module Claude.md files, Source/CLAUDE.md tier rows, feature matrix) + perf measurement (criterion 8) + fix the two stale docs noted in RESEARCH_Codebase (CkEcs group roster, CkGraphics claim). | All success criteria; full suite delta-zero vs Phase 0 baseline. |
| 7 | Stretch/backlog (separate sign-off): god-ray raymarch pass, cloud-shadow material exemplar, per-object snap helper, stencil point-light look, BB content adoption. | — |

## Reading list (before authoring anything — non-negotiable #1)

| Read | Why |
|---|---|
| The three RESEARCH docs in this folder | The verified technique math, engine APIs at file:line, and codebase state. Do not re-derive. |
| `Plugins/ScreenSpaceFogScattering/` — `SSFSViewExtension.{h,cpp}`, `SSFSSubsystem.{h,cpp}`, Build.cs, uplugin | The in-repo working reference for SVE + global shaders + RDG + PostConfigInit + teardown. The scaffold to mimic. |
| Engine: `Renderer/Private/PostProcess/PostProcessUpscale.h`, `PostProcessing.cpp:441-483,600,1949-1983`, `SceneViewExtension.h`, `CameraStackTypes.cpp:183-473`, `LegacyScreenPercentageDriver.*` | The exact contracts being implemented against (5.7.4 fork — verified no deviation). |
| `Source/CkUsf/Claude.md` (whole), esp. the Stylize settings-flow + limitations table | The subsystem/preset/CVar shape `CkPixelArt` mirrors, and the look-authoring contract D6 rides. |
| `Source/CkUsf/Shaders/CkUsf/{StylizeCommon.ush, Looks/CelShade.ush, Looks/EdgeOutline.ush, Looks/ScreenDither.ush}` | The quantize/palette/dither library + existing edge detectors to reuse, not reinvent. |
| `Source/CkCamera/` — `CkCamera_Processor.cpp:263-313`, `CkCamera_Component.cpp:9-21`, `CkCamera_Utils.cpp:370-709`, `CkCamera/CLAUDE.md` | The view authority, the ViewInfo assembly site, and the attribute-materialization pattern D4 follows. |
| `Source/CkIskmRendererVF/` (module shape + uplugin entry) | The house PostConfigInit sibling-module precedent D2 mirrors. |
| Engine plugin `CompositeCore` — `CompositeCoreModule.cpp`, `Passes/CompositeCorePassDilate.cpp` | The PostConfigInit + global-shader plugin idiom, incl. the early-cvar gotcha. |

## Standing risks (name them before they bite)

1. **PIE lies.** The DPI-driven secondary upscale resamples our pixels in-editor; every visual
   verdict that matters is taken standalone (success criterion 1). The gym must print a visible
   "PIE preview is approximate" placard when it detects a non-100 secondary fraction.
2. **Renderer-private include** = recompile-coupled to engine bumps. Acceptable on the pinned
   fork; note it in the module Claude.md so the next engine upgrade budget includes it.
3. **The AA/upscale interlock is silent by design upstream** — if something re-enables TSR, the
   upscaler simply stops being called (no error). D7's validation must also run per-frame-cheap
   in the SVE's IsActive gate and log state transitions, so the failure is visible.
4. **Two sessions/branches discipline**: CkFoundation dev currently carries unpushed sibling
   work (see memory). Campaign work goes on a feature branch; no submodule pointer bumps without
   the containment guard.
5. **Shadow policy**: CSM+ortho is broken at engine level (hardcoded perspective cascade build).
   The gym scene ships with VSM configured; the feature matrix documents it.
