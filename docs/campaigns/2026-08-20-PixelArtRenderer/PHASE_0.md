# PHASE 0 — De-risk spike + baseline (CkPixelArtRender skeleton, hardcoded)

> Executor: read [PROMPT.md](PROMPT.md) fully first. Load skills BEFORE starting:
> `ck-change-control`, `ck-debugging-playbook`, `ck-build-and-env`. Build/test ONLY via the
> `/build-test` skill (toolbox). This phase writes the REAL module skeleton with hardcoded
> values — Phase 1 productionizes in place; nothing here is a throwaway branch.

## Entry criteria

1. CkFoundation is on a new branch: from `Plugins/CkFoundation`, `git checkout -b feature/pixel-art-renderer dev`.
   CkTests: `git -C Plugins/CkTests checkout -b feature/pixel-art-renderer` from its current branch
   (record the start commit of BOTH repos in PROGRESS.md).
2. **Baseline capture (before ANY edit):** run the full gate per `/build-test`
   (`--test --no-live`), record in PROGRESS.md: total/pass/fail counts AND the names of every
   failing test. This is the delta-zero reference for the whole campaign. If the toolbox reports
   contamination (exit 78) or refuses, STOP and record a blocker.
3. Editor is CLOSED (this phase compiles C++).

## Executable spec

No red test can exist before the module does. The spec for this phase is a **repro command with
expected output**:

- Command (after step 6): launch standalone —
  `<Editor>Editor.exe <BB.uproject> <SpikeMap> -game -windowed -resx=1920 -resy=1080 -log`
  then in console: `ck.PixelArt.Spike 1`
- Expected log lines (exact breadcrumbs this phase adds):
  `[CkPixelArt] Spike active: viewport=1920x1080 internal=644x364 fraction=0.335417`
  and per-frame-once `[CkPixelArt] Upscaler AddPasses: in=644x364 out=1920x1080 stage=PrimaryToOutput`
- Expected image: visibly chunky pixels, sharp (not blurry) edges. Screenshot via console `Shot`,
  path recorded in PROGRESS.md (this is the D1 evidence image for the maintainer).

## Steps

1. **Create the module skeleton** `Source/CkPixelArtRender/` (mimic, do not invent):
   - `CkPixelArtRender.Build.cs` — `class CkPixelArtRender : CkModuleRules`. Deps: Core,
     CoreUObject, Engine, RenderCore, Renderer, RHI, Projects, CkCore, CkLog, CkSettings.
     Copy the Renderer-private include lines VERBATIM-adapted from
     `Plugins/ScreenSpaceFogScattering/Source/ScreenSpaceFogScattering/ScreenSpaceFogScattering.Build.cs:12-21`
     (`Runtime/Renderer/Private`, and the 5.6+ `Renderer/Internal` line).
   - `CkPixelArtRender_Module.{h,cpp}` — StartupModule:
     `AddShaderSourceDirectoryMapping(TEXT("/CkPixelArt"), <plugin>/Source/CkPixelArtRender/Shaders/CkPixelArt)`
     (mimic `CkUsf_Module.cpp:9-18`).
   - `CkPixelArtRender_Log.{h,cpp}` — `CK_DEFINE_LOG_FUNCTIONS` per house pattern
     (namespace `ck::pixel_art`).
   - Register in `CkFoundation.uplugin`: `"Type": "Runtime", "LoadingPhase": "PostConfigInit"`,
     standard Win64/Mac/Linux allowlist (mimic the `CkIskmRendererVF` entry at uplugin:650-652).
2. **The upscale global shader** `Shaders/CkPixelArt/PixelArtUpscale.usf` + `CkPixelArtRender_UpscaleShader.{h,cpp}`:
   - `class FCk_PixelArt_UpscalePS : public FGlobalShader` — `DECLARE_GLOBAL_SHADER`,
     `SHADER_USE_PARAMETER_STRUCT`. Parameters:
     ```
     SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
     SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)     // bilinear clamp
     SHADER_PARAMETER(FVector2f, InputExtentInv)              // 1/texture extent
     SHADER_PARAMETER(FVector2f, InnerRectMinTexels)          // margin offset (Phase 0: 0,0)
     SHADER_PARAMETER(FVector2f, InnerRectSizeTexels)         // displayed window in texels
     SHADER_PARAMETER(FVector2f, SubTexelOffsetTexels)        // Phase 0: 0,0
     RENDER_TARGET_BINDING_SLOTS()
     ```
     `IMPLEMENT_GLOBAL_SHADER(FCk_PixelArt_UpscalePS, "/CkPixelArt/PixelArtUpscale.usf", "MainPS", SF_Pixel);`
   - Shader body: the t3ssel8r box filter from
     [RESEARCH_Technique.md](RESEARCH_Technique.md) §B, operating in texel space of the inner
     window: `tx = InnerRectMinTexels + SubTexelOffsetTexels + UV * InnerRectSizeTexels`, then
     boxSize/txOffset/floor+0.5 math, sample with `InputTexture.SampleGrad(InputSampler, uv, ddx, ddy)`
     where uv = texel-space result × `InputExtentInv`. Phase 0 may hardcode boxSize from a
     uniform ratio instead of fwidth if derivative plumbing fights back — note it if so.
3. **The spatial upscaler** `CkPixelArtRender_Upscaler.{h,cpp}`:
   - `class FCk_PixelArt_SpatialUpscaler final : public ISpatialUpscaler` (include
     `PostProcess/PostProcessUpscale.h` — renderer-private). Ctor takes a by-value
     `FCk_PixelArt_UpscaleFrame { FIntPoint _InternalSize; FVector2f _SubTexelOffsetTexels; int32 _MarginTexels; }`.
   - `GetDebugName()` → `TEXT("CkPixelArt")`. `Fork_GameThread` → `new` copy of itself.
   - `AddPasses`: replicate the output-geometry rules of
     `ISpatialUpscaler::AddDefaultUpscalePass` (`PostProcessUpscale.cpp:243-288`): honor
     `PassInputs.OverrideOutput` when valid, else create the output texture
     (`PrimaryToSecondary` → rect `[0,0]..View.GetSecondaryViewRectSize()`; else
     `View.UnscaledViewRect`). One fullscreen PS pass with the shader above. Input rect =
     `PassInputs.SceneColor.ViewRect`.
4. **The view extension** `CkPixelArtRender_ViewExtension.{h,cpp}`:
   - `class FCk_PixelArt_ViewExtension : public FSceneViewExtensionBase`. Phase 0 activation:
     a single dev CVar `ck.PixelArt.Spike` (int, default 0, `ECVF_Cheat`) — the IsActive functor
     returns true only when it is non-zero AND the context has a game world AND
     `NOT IsRunningDedicatedServer()`.
   - `BeginRenderViewFamily`: if active and `Views[0].State` is a game view —
     `InViewFamily.SetPrimarySpatialUpscalerInterface(new FCk_PixelArt_SpatialUpscaler(Frame));`
     with Phase-0 Frame = hardcoded (InternalHeight 360+2·margin? — Phase 0: margin 2, so
     internal 644×364 at 16:9, offsets zero). Log the breadcrumb from the executable spec once
     per state change, not per frame.
   - Owner: `UCk_PixelArtRender_Subsystem_UE : UEngineSubsystem` — creates via
     `FSceneViewExtensions::NewExtension<>` in `Initialize`; `Deinitialize` pushes a false
     `IsActiveFunctor` then resets (mimic `SSFSSubsystem.cpp` teardown exactly).
5. **Screen-percentage delivery — the D8 decision gate.** Implement mechanism (a) first:
   when the spike is active, each frame (game thread, e.g. from the SVE's `SetupViewFamily`)
   compute `Fraction = (InternalW − 0.5) / ViewportW` and set `r.ScreenPercentage` =
   `Fraction × 100` via `IConsoleVariable::Set(..., ECVF_SetByCode)`; restore the prior value on
   disable (store it; zero residue).
   - **Gate 5.G — verify internal size is pixel-exact**: run the executable-spec command at
     1920×1080, 2560×1440, and a resized 1367-wide window. Expected: the `AddPasses` breadcrumb
     reports EXACTLY `internal = round-target` sizes each time (644×364 at 16:9 outputs;
     compute expected for 1367 and assert).
     - If exact at all three → **D8 = (a)**. Record in PROGRESS.md, continue.
     - If off-by-one anywhere → switch to mechanism (b):
       `InViewFamily.SetScreenPercentageInterface_Unchecked` replacing the viewport driver with
       our own `ISceneViewFamilyScreenPercentage` impl returning the exact fraction (Fork +
       UpperBound per `SceneView.h:2163-2203`); re-run the sweep. If (b) exact → **D8 = (b)**.
     - Anything else (upscaler never called, sizes wildly wrong, asserts) → STOP; paste the log
       into PROGRESS.md Blockers; end session.
6. **Spike map + preconditions**: prefer REUSING an existing map (any BB gym/test map with
   visible geometry works — reading it is fine; do not edit BB content). If a dedicated map is
   genuinely needed, create it inside the **CkTests plugin's Content** (never under BB
   `/Content/`), with a few engine basic shapes + a directional light. Console setup for the
   run: `r.AntiAliasingMethod 0`, `r.Ortho.Debug.ForceAllCamerasToOrtho 1` (debug cvar —
   Phase 3 replaces this with real CkCamera ortho), dynamic res off.
7. **Empirical checks** (record each in PROGRESS.md as VERIFIED/FAILED + evidence):
   - a. PP-materials-at-internal-res: enable an existing Stylize look (e.g.
     `ck.Usf.CelShade.Enabled 1`) with the spike active — its bands/patterns must be chunky
     (internal-res), not native-res fine.
   - b. Auto-ortho-planes at low res: with `r.Ortho.AutoPlanes 1` vs explicit planes, log
     near/far (breadcrumb) at 100% vs spike screen percentage — confirm the documented
     resolution-dependence (justifies D4's explicit-planes default).
   - c. PIE vs standalone: run the same scene in PIE at >100% OS DPI and standalone; note the
     visual difference (DPI secondary upscale). Record; no fix — this is documentation evidence.
   - d. (Opportunistic, non-blocking) directional light function in the ortho view if a light
     function material is already available in BB content; otherwise record UNTESTED.
8. **Commit** (per `/commit` rules — no co-author line, commit only, never push):
   one commit for the module skeleton, one for the spike findings doc updates.

## Exit criteria

- Executable-spec command produces the expected breadcrumbs and a recorded screenshot.
- Gate 5.G passed with D8 recorded (a or b) + the three-size evidence table in PROGRESS.md.
- Checks 7a–7c recorded with evidence.
- `--build` clean; full suite delta-zero vs the Phase-0-entry baseline (names, not just counts).
- PROGRESS.md updated (phase status, D8, blockers empty or explicit).

## Fences

- Do NOT modify CkUsf, CkCamera, or any Stylize subsystem in this phase.
- Do NOT use `TemporalUpscale`/TSR anywhere; if the upscaler silently stops running, check
  `r.AntiAliasingMethod` FIRST (`PostProcessing.cpp:600` gate) before debugging anything else.
- Do NOT register the upscaler anywhere except `BeginRenderViewFamily` (engine `check`s fire —
  `SceneRendering.cpp:5140-5146`).
- Do NOT delete the upscaler you `new` — the family owns it. Do not cache it across frames.
- Do NOT call `SetScreenPercentageInterface` (checked variant) — the viewport client already
  installed one; only `_Unchecked` may replace (mechanism b).
- `r.Ortho.Debug.ForceAllCamerasToOrtho` is non-shipping debug-only — it must never appear
  outside Phase 0/2 test setup steps.
