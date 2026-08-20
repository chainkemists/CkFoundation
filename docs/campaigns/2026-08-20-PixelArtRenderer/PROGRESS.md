# PROGRESS — CkPixelArt (t3ssel8r-style 3D pixel-art renderer)

> Living doc — the ONLY place current state lives. PROMPT.md is the locked charter.
> Executor: update this file at every phase boundary, every gate verdict, and every blocker.
> Never improvise past a failed gate — record it under Blockers and stop the phase.

## Status: PHASES 2 + 3 IMPLEMENTED, gate pending. Next: PHASE_4.md (CkPixelArt module).

Phase 1 exit gate: **1193 / 1190 pass / 3 fail** (1188 baseline + the 5 new spec tests).
**No new failures.** One PRE-EXISTING failure disappeared —
`Angelscript.CppTests.AngelscriptCodeCoverage.IntegrationTest`, an ENGINE test asserting that
coverage reports exist under `Saved/Automation/Tmp/TestOutput/`. It ran and passed this time,
almost certainly because earlier runs in this session populated that directory. It was not
touched and is not claimed as fixed; if it reappears in a later baseline that is noise, not a
regression.

Nothing pushed. CkFoundation and CkTests both carry commits on `feature/pixel-art-renderer`.

Decisions D1–D8 LOCKED 2026-08-20 (maintainer directive to execute; F1 scoped-SVE-re-open
approved; F2 module split approved; F3 render-side snap; F4 content exemplars → Phase 7 backlog;
F5 routing = Opus executes, Fable audits gates).

## Host project (divergence D-1 below)

The campaign docs were authored expecting **BusterBlock** as the host (`BB.uproject`, "BB root").
This executor session's root is **CkPlugins_Other** (`D:\Repositories\CkRepos\CkPlugins_Other`,
`CkPlugins.uproject`) — the plugin-isolation dev host, and the checkout where the CkFoundation
`feature/pixel-art-renderer` branch already lives. All build / test / standalone runs use
CkPlugins_Other. The SSFS reference plugin is not vendored here; it was read read-only from
`D:\Repositories\CkRepos\BusterBlock\Plugins\ScreenSpaceFogScattering`.

## Baseline (Phase 0 entry)

- CkFoundation start commit: `ae93c8c287ee8dc3d50d450e58b47c73dd00b9a3` ("feat: Pixel ArtRenderer
  campaign docs" = `dev` @ `96b5cad0e` + the campaign folder) · branch `feature/pixel-art-renderer`
  (pre-existing; tree clean at entry)
- CkTests start commit: `35510633c912acd81b73001559c674cd6f60f18f` · branch
  `feature/pixel-art-renderer` created this session from `dev` (tree clean at entry)
- Superproject CkPlugins_Other @ `133f8f9` (dev). Pre-existing local dirt NOT authored by this
  session and left untouched for its owning session: `CkPlugins.uproject` (local
  EngineAssociation GUID), `Config/DefaultGameplayTags.ini` (+5 lines), and submodule pointer
  drift for CkFoundation / CkTests / CkGameplayDebugger.
- Full suite, gate of record: `./CkAuto/UnrealToolbox.exe --test --no-live --discover-fresh`
  (log `Saved/Logs/BuildTest-Baseline3.log`, run 2026-08-20, binaries from the `--build` in
  attempt 1 — no source edits between the build and this run):
  **total 1188 · pass 1184 · fail 4 · skipped 0 · contaminated 0 · 5m 20s.**
- **Failing test NAMES — the delta-zero reference for every later gate:**
  1. `Angelscript.CppTests.AngelscriptCodeCoverage.IntegrationTest` (ENGINE AngelScript-plugin test;
     fails asserting coverage reports were written under `Saved/Automation/Tmp/TestOutput/...`)
  2. `Project.Functional Tests.CkTests.AutoTests.AutoTests_CkTests_Level.Ck_AutoTest_Crowd_NavQueryFilter_ForceReplan`
  3. `Project.Functional Tests.CkTests.AutoTests.AutoTests_CkTests_Level.Ck_AutoTest_PathNetworkFollower_ProjectsRibbonWaypointWithinNavQueryExtent`
     ("fixture must require a 25-50cm ribbon projection (tight=true, broad=true, delta=2.0cm)")
  4. `Project.Functional Tests.CkTests.AutoTests.AutoTests_CkTests_Level.Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward`
     ("the ribbon must have navmesh room inward from the discovered boundary")

  All four are PRE-EXISTING: captured before any source edit in this campaign.

### Baseline capture — what it took (keep for the next executor)

Two earlier attempts were discarded; neither was a code problem and neither is a blocker:

1. `--build --target=Editor --test --no-live` → **exit 76** (`AS_COMPILE_FAILED`: an editor boot
   failed to compile AngelScript and ran stale bytecode, so its 334/333/1 counts are meaningless).
   The build itself SUCCEEDED. Cause: this worktree's gitignored `Plugins/CkFoundation/Script/Generated/`
   was left over from the `livetune/phase-0` branch (most files dated 2026-08-04) and the run's four
   parallel editor lanes regenerated it *while compiling it* — errors named `UCk_Utils_LiveTune_UE`
   (exists only on `livetune/phase-0`), `FCk_Request_Probe_Reconfigure` (deleted), and crowd-agent
   getters missing from a wrapper whose C++ does have them. Verified afterwards that the on-disk
   generated scripts had settled correct for this branch (orphan `utils_live_tune.as` deleted,
   `utils_probe.as` clean, `utils_crowd_agent.as` carrying `Get_HasReachedActiveGoal`).
2. `--test --no-live` → exit 1, AngelScript clean, but 1005 total with **12 phantom LiveTune tests
   "never run"** — a stale cached test list from that other branch, which also HID ~183 real tests.
   `--discover-fresh` ("force a fresh discovery editor launch … or to reset a stale cache") is the
   documented remedy and produced the 1188-test run recorded above.

Lesson for later phases: after any branch switch in this worktree, the first toolbox run may burn
on stale `Script/Generated/` + a stale test cache. Prefer `--discover-fresh` on the first run of a
session, and never trust an exit-76 run's counts.

## Phase checklist

| Phase | Deliverable | Status | Exit evidence |
|---|---|---|---|
| 0 | Spike: module skeleton, hardcoded upscale proven, D8 gate, empirical checks 7a–7d, baseline | **DONE** | `CkPixelArtRender` module (14 files) + uplugin entry; gate 5.G passed at 3 viewport sizes (D8 = a); box filter verified by zoom; full suite delta-zero on the final binary (`Saved/Logs/BuildTest-Phase0-Final.log`) |
| 1 | CkPixelArtRender productionized: registry, CVars, lifecycle, fraction/config tests | **DONE** | 5/5 spec tests green; `PixelArt.Spike` gone (0 hits); 100x toggle PASS with zero residue; PrimaryToSecondary/OverrideOutput path verified against the engine's rect assertion; suite 1193/1190/3 with no new failures |
| 2 | Snap + margin + remainder; creep/stutter A/B verified | NOT STARTED | |
| 3 | CkCamera ortho (attribute + requests + ViewInfo) | NOT STARTED | |
| 4 | CkPixelArt module: subsystem/preset/settings/CVars, D7 preconditions | NOT STARTED | |
| 5 | PixelArt look (outline + banding + palette + band-shift edges) | NOT STARTED | |
| 6 | Gym, gate of record, perf table, docs, VALIDATION executed | NOT STARTED | |
| 7 | BACKLOG (separate sign-off): god rays, cloud shadows, per-object snap, stencil point-light, BB adoption | BACKLOG | |

## Decision gates — verdicts

### D8 — screen-percentage mechanism: **(a) drive `r.ScreenPercentage`.** PASSED gate 5.G.

Mechanism (b) (`SetScreenPercentageInterface_Unchecked`) was never needed. Evidence — standalone
`-game -windowed`, spike on, breadcrumbs from `LogCkPixelArt` (logs under `Saved/Logs/PixelArtSpike_*.log`):

| Viewport | Fraction requested | Fraction applied | Rendered internal | Displayed | Secondary | Stage |
|---|---|---|---|---|---|---|
| 1920x1080 | 0.335156 | 0.335156 | **644**x362 | 640x358 | 1.0000 | PrimaryToOutput |
| 2560x1440 | 0.251367 | 0.251367 | **644**x362 | 640x358 | 1.0000 | PrimaryToOutput |
| 1367x768  | 0.470739 | 0.470739 | **644**x362 | 640x358 | 1.0000 | PrimaryToOutput |

The driven axis (width) lands on exactly 644 = 640 + 2x2 margin at all three, including the
non-integer-ratio 1367 case — the `(W - 0.5)/ViewportW` formula does what RESEARCH_UeApis §2 said.
`applied == requested` means nothing outranked `ECVF_SetByCode`. Height is 362 at all three because
all three viewports are ~16:9; per divergence **D-3** the height is derived, not driven, so the
displayed window is 640x358 rather than the phase doc's 640x360.

**Filter quality verified, not just "the pass ran":** a 4x nearest-neighbour zoom of the capture
shows flat uniform texel blocks with a razor-thin antialiased seam at texel boundaries, no interior
blur and no ghosting — i.e. the t3ssel8r box filter behaving correctly (D5).

**Success criterion 3 (native-res UI) already holds**, for free: in the same capture the Slate/UMG
gym-cycler menu is crisp at 1920x1080 over a visibly chunky scene, exactly as the frame ordering in
RESEARCH_UeApis §5 predicted.

### Phase 0 empirical checks

- **7a PP-materials-at-internal-res — VERIFIED (with a caveat).** Two standalone runs of the same
  scene with `ck.Usf.CelShade.Enabled 1`, one with the spike and one without. The look is genuinely
  active in both (it visibly desaturates versus a non-CelShade capture). At an identical 300x60 crop,
  spike-off fits a whole line of scene text; spike-on shows the same region ~3x magnified in square
  texel blocks. So scene-colour content is produced at 644x362 while a Stylize blendable is running,
  which matches the mechanism (every hookable PP pass precedes PrimaryUpscale —
  `PostProcessing.cpp:441-483`). *Caveat:* the crop has no geometry edges, so this does not
  separately measure a CelShade ink line / halftone at texel scale. Queued for the human below.
- **7b auto-ortho-planes resolution-dependence — SOURCE-VERIFIED, RUNTIME-UNVERIFIED.**
  `FMinimalViewInfo::AutoCalculateOrthoPlanes` (`CameraStackTypes.cpp:386-473`) derives a
  unit-per-pixel ratio from the view rect, so the auto far plane is a function of render resolution.
  That is sufficient to justify D4's explicit-planes default. A runtime near/far breadcrumb would
  need a probe this phase does not otherwise want; queued below rather than invented here.
- **7c PIE-vs-standalone DPI — HALF-MEASURED.** Standalone reports `secondary=1.0000` at all three
  sizes, i.e. no DPI-driven secondary resample, so the standalone numbers above are exact. The PIE
  half needs the editor and is queued for the human. The module already logs the settled
  `SecondaryViewFraction` from `BeginRenderViewFamily`, so the PIE run will name its own distortion.
- **7d ortho light functions — UNTESTED.** No light-function material was to hand in this host
  project (allowed by the phase doc as opportunistic).

### Not yet established in Phase 0

- **Zero-residue on disable (criterion 4) — NOT VERIFIED.** The restore path exists
  (`Request_RestoreScreenPercentage`, called from both the cvar's on-changed callback and the
  subsystem's `Deinitialize`), but the evidence is missing: a graceful `CloseMainWindow` reached
  `LogCore: Engine exit requested (reason: ConsoleCtrl RequestExit)` and the abslog ended there
  without flushing the shutdown lines, so no `Spike inactive: restored ...` line was captured.
  Do NOT record this as working. Phase 1 owns lifecycle/CVars and must prove it with a runtime
  toggle (spike on -> frame rendered -> spike off -> read `r.ScreenPercentage` back).

### Phase 1 exit criteria — verdicts

| Criterion | Result |
|---|---|
| Both spec tests green (`--test-pattern PixelArtRender --discover-fresh`) | **5/5** — the fraction test alone asserts 8 viewport widths x 4 targets |
| `rg "PixelArt.Spike" Source` -> 0 hits | **0** |
| 100x toggle leaves `r.ScreenPercentage` at its pre-enable value | **PASS** — 100 Active/Released pairs, every restore exactly `0.0000`, verdict logged by `ck.PixelArt.Debug.ToggleLoop` |
| Full suite vs baseline | **1193 / 1190 / 3 — no new failures** |
| `OverrideOutput` / not-last-pass path (step 4) | **Verified** at `r.SecondaryScreenPercentage.GameViewport 50`: `stage=PrimaryToSecondary`, `out=640x360` matching `GetSecondaryViewRectSize()`, no engine assertion |

Zero-residue (Phase 0 left this NOT VERIFIED) is now established, by the runtime toggle loop rather
than by argument.

### Phase 2 sign gate 6.G — HUMAN-QUEUED (see H-5)

The gate is `[EDITOR-VERIFY, standalone]` by its own heading and no agent can score it: every verdict
in it is "does this look like it crawls / steps / moves smoothly". The compensation SIGN was therefore
DERIVED rather than discovered, and `ck.PixelArt.Debug.CompSign -1` exists so a human can overturn the
derivation in one console command instead of a rebuild.

The derivation, so the maintainer can check the reasoning and not only the picture: the shader samples
at `UV * Extent + SubTexelOffsetTexels`. Sampling further RIGHT in the source puts content that was
further right at the current screen position, which moves the picture LEFT — the same direction the
camera moving right would have moved it. So the horizontal remainder goes in with its own sign. The
vertical one is negated, because the remainder is expressed in the WORLD's sense of up while V grows
downward. Code: `CkPixelArtRender_ViewExtension.cpp`, `BeginRenderViewFamily`.

### Fable adjudication — render margin across axes (2026-08-20)

**Question.** D5 locks "+2 texels per side", but its formula is one-dimensional (width only) and the
renderer applies ONE resolution fraction to both axes. At 16:9 a 2-texel horizontal margin buys only
~1.1 rows vertically; on a 32:9 ultrawide it degrades to ~0.5 rows per side, below the functional
minimum. Three options went to a Fable agent: (A) charter-literal, (B) aspect-compensated horizontal
margin, (C) symmetric-integer margin with a per-axis projection scale.

**Ruling: B, floor-bias on an odd surplus, with the real insets asserted at runtime.** A fails the
no-sampling-outside-the-render invariant at shipping aspect ratios. C buys symmetric edges by making
every texel ~0.3% rectangular, trading a correctness guarantee for cosmetics — and square texels are
the premise of the look. B holds the invariant at any aspect for ~3% more rendered pixels.

Implemented as `ck::pixel_art::Get_HorizontalMarginTexels`, which solves for the margin directly
rather than using the ruling's `ceil(M * Aspect)` approximation: the rendered height is
`ceil((RenderedWidth - 0.5) / Aspect)`, so the width that guarantees `M` rows on both sides is
`(InnerHeight + 2M) * Aspect + 0.5`. At 1280x720 / 360p / M=2 that gives a 4-texel horizontal margin,
rendering 648x365 for a 640x360 window — insets 4/4 horizontal, 2/3 vertical.

Fable's flagged risk was that the guarantee is proven with clean arithmetic and could fail on odd
viewport heights or DPI-scaled windows. Taken: `DoApply_ScreenPercentage` logs an Error naming the
three computed insets whenever any drops below one texel, and the spec test sweeps six viewport sizes
(incl. 2560x1080, 3440x1440, 1024x768) x four internal heights x three margins.

**One correction to the framing given to Fable**, recorded because it changes nothing but was
overstated: options A and B do not produce *exactly* square texels either. The engine takes CeilToInt
of the rendered height, so the rendered rect's aspect differs from the viewport's by a fraction of a
percent no matter what. The implementation therefore does NOT scale `M[1][1]` by the same factor as
`M[0][0]` — it DERIVES `M[1][1]` from the actual rendered aspect, which makes texels exactly square
and moves the rounding error into the vertical framing (~0.1% of view height) where it is invisible.
That is strictly better than any of the three options as described.

## Divergence log (phase doc vs repo — repo is authority for mechanics, PROMPT.md for intent)

- **D-1 — Host project is CkPlugins_Other, not BusterBlock.** See above. The step's intent (a host
  project that compiles CkFoundation and can be launched standalone) is unaffected. Spike map for
  step 6: `/CkTests/TestGyms/TestGyms_CkTests_Level`, already this project's `GameDefaultMap`
  (`Config/DefaultEngine.ini:13`). `r.AntiAliasingMethod=0` is already project-wide
  (`Config/DefaultEngine.ini:59`), so the D7 AA precondition holds by default here.

- **D-2 — `CkPixelArtRender` must NOT inherit `CkModuleRules`, and must NOT depend on
  CkCore/CkLog/CkSettings.** PHASE_0 step 1 prescribes `class CkPixelArtRender : CkModuleRules`
  with those deps at `PostConfigInit`. `CkModuleRules` publicly adds **`AngelscriptCode`**
  (`Source/CkBuildConfig/CkBuildConfig.Build.cs:55`), whose module loading phase is
  **`PostDefault`** (`<Engine>/Plugins/Angelscript/Angelscript.uplugin:20-22`), plus
  **`ApplicationCore`** (`CkBuildConfig.Build.cs:215`). The in-repo PostConfigInit precedent the
  same step names as the thing to mimic says so verbatim: *"Engine-only module (NO Ck deps, NOT
  CkModuleRules — CkModuleRules pulls AngelscriptCode/ApplicationCore which cannot load at
  PostConfigInit)"* (`Source/CkIskmRendererVF/CkIskmRendererVF.Build.cs:1-3`). PROMPT.md D2's own
  rationale ("would drag CkUsf→CkEcs→… into the PostConfigInit load order — an untested ripple
  the VF-style split avoids") is the same argument applied one module deeper.
  → **Resolution:** plain `ModuleRules`, `CppStandardVersion.Cpp20`, engine-only deps (Core,
  CoreUObject, Engine, RenderCore, RHI + private Renderer, Projects) and the Renderer-private
  include paths. Consequences: no `CK_DEFINE_LOG_FUNCTIONS` (it lives in CkLog) — a plain
  `LogCkPixelArt` category instead; no `CK_GENERATED_BODY` on the subsystem. House naming,
  formatting and validation rules still apply.

- **D-3 — `internal=644x364` is unreachable with a single resolution fraction.** The engine applies
  ONE fraction to both axes: `ViewSize = CeilToInt(UnscaledViewSize * Primary * SecondaryViewFraction
  * LensDistortion)` (`SceneRendering.cpp:3352` calling `:3116-3122`). At 1920x1080 a width-driven
  fraction of 644/1920 gives height `CeilToInt(1080 * f) = 363`, not 364 — and the doc's own
  `fraction=0.335417` (= 644/1920) contradicts step 5's pixel-exact formula
  `(InternalW − 0.5)/ViewportW` (= 0.335156).
  → **Resolution:** drive the fraction from the target WIDTH using the `−0.5` formula (the mechanism
  RESEARCH_UeApis §2 verified), assert exactness on the driven axis, and log/record the derived
  height. Gate 5.G is evaluated on the driven axis. Note the fraction must also divide out
  `FSceneViewFamily::SecondaryViewFraction`, which multiplies in at the same site.

- **D-4 — the upscale shader's window geometry rides `FScreenPassTextureViewport`, not two extra
  uniforms.** PHASE_0 step 2 lists `InnerRectMinTexels` / `InnerRectSizeTexels` as shader
  parameters. `AddDrawScreenPass` already plumbs the input rect to the VS via
  `DrawScreenPass_PostSetup` → `DrawPostProcessPass` (`Renderer/Private/ScreenPass.cpp`), so the
  VS-provided UV spans exactly the rect handed in as `InputViewport`. Passing the inner rect there
  (the SceneColor view rect inset by the margin) makes the two representations unable to disagree.
  The shader keeps `InputExtent`, `InputExtentInv`, `SubTexelOffsetTexels`. Same math, same intent.

- **D-5 — Phase 1's config/registry types carry NO CkCore macros.** PHASE_1 step 1 specifies
  `CK_GENERATED_BODY` + private `_Members` + `CK_PROPERTY` + `CK_DEFINE_CONSTRUCTORS` on
  `FCk_PixelArt_RenderConfig`, and `CK_DEFINE_CUSTOM_FORMATTER_ENUM` on the filter enum. All of
  those live in CkCore, which this module cannot link (divergence D-2). Confirmed against the
  engine rather than assumed: `FEngineLoop::AppInit` loads PostConfigInit modules
  (`LaunchEngineLoop.cpp:6756`), `PreInitPreStartupScreen` then calls `CompileGlobalShaderMap`
  (`:3241`), and `Default`-phase modules only load later in `LoadStartupModules` (`:4611`) — so
  PostConfigInit is the LAST phase before the global shader map is built and the module cannot
  move to Default either. → Resolution: plain public-member struct, exactly like
  `FCk_Iskm_BoneMatrix3x4` in the sibling PostConfigInit module. The enum IS a `UENUM` (no CkCore
  needed) so Phase 4 can expose it. The reflected, accessor-bearing, BP/AS-facing configuration
  surface belongs in Phase 4's `CkPixelArt`, which CAN link CkCore.
  **Open for the maintainer:** validation in this module uses `ensureMsgf`, which house doctrine
  normally forbids. The doctrine's stated reason ("logs get ignored, ensures do not") is served by
  it, and the thing it actually forbids — `Warning`/`Error` log-and-continue — is avoided. If the
  ruling is that no ensure at all may live in a CkCore-less module, the validation has to move to
  Phase 4's module and this one becomes contract-by-construction.

- **D-6 — the driven axis is WIDTH, the authored knob is HEIGHT.** `FCk_PixelArt_RenderConfig`
  carries `InternalHeight` (PHASE_1's field), but D5 locks the exactness formula as
  `(InternalW - 0.5)/ViewportW`. The engine applies ONE resolution fraction to both axes
  (`SceneRendering.cpp:3352`), so only one axis can be exact. Resolution: the author sets a
  vertical texel count, the extension derives `TargetWidth = round(InternalHeight * aspect)`, and
  the fraction is driven on width per D5. At 16:9 that reproduces Phase 0's verified 640 -> 644 exactly.
  The vertical texel count therefore lands within a texel of the authored value, which is
  harmless: texel squareness comes from the ortho projection matching the view aspect, not from
  the internal height being a round number.

- **D-7 — `ck.PixelArt.Debug.ToggleLoop` is instrumentation added to satisfy an exit criterion.**
  PHASE_1 requires proving zero residue by toggling 100x "via console". No batch of console
  commands can express it: `-CkDeferredCmdsFile` drains every line in ONE pump, so there are no
  frames between the toggles and therefore no lease renewals to release. The debug command flips
  the enable CVar once per frame via `FTSTicker` and logs a PASS/FAIL verdict against the
  `r.ScreenPercentage` value captured before the first enable. It is debug-only surface in a
  runtime module — flagged so it can be vetoed; removing it would return zero-residue to
  "unverified", which is where Phase 0 had to leave it.

- **D-8 — the snap/remainder handshake is an extension member, not a per-world transient slot.**
  PHASE_2 step 2 prescribes writing the remainder into `FCk_PixelArt_FrameTransients` on the state
  registry and asserting `_FrameNumber == GFrameCounter` when consumed. The three hooks that must
  agree all run on the game thread, in a fixed order, inside one `UGameViewportClient::Draw`:
  `SetupViewFamily` (`GameViewportClient.cpp:1486`), then `SetupViewProjectionMatrix` via
  `LocalPlayer::CalcSceneView` (`:1687` -> `LocalPlayer.cpp:1266`), then `BeginRenderViewFamily`
  (`:1971`). One `TOptional` member on the extension therefore carries the frame's state with no lock
  and no map lookup, and is strictly more correct for multiple viewports (the registry is keyed per
  WORLD; the real granularity is per viewport). The frame-number guard is kept — it is the part that
  earns its keep — and the report IS still published to the registry, because Phase 4's snapped-view
  consumers and the debug pan read it from there.

- **D-9 — the reflected snap wrapper takes `FVector2D`, not `FVector2f`.** PHASE_2's spec names
  `FVector2f& OutRemainderTexels` on `UCk_Utils_PixelArtRender_UE`. `FVector2f` is declared
  `BlueprintInternalUseOnly` by the engine (`NoExportTypes.h:654`) and cannot appear on a Blueprint
  node. The free function `ck::pixel_art::Get_SnappedViewOrigin` keeps the specced `FVector2f`
  signature and is what the spec tests exercise; the BPFL wrapper converts.

- **D-10 — "red first" is not literally reachable for a C++ unit test of a new native API.** The
  phase docs require the spec test to be run RED before the implementation. A test calling
  `ck::pixel_art::Get_ViewBasis` before that function exists does not fail, it fails to COMPILE, and
  the only way to produce a genuine red would be to stub the API with deliberately wrong returns —
  theatre that proves nothing. What was actually done, and what the rule protects: the spec was
  authored from the phase doc before the implementation and was never weakened to make it pass. The
  one bound the tests do relax is the idempotence tolerance, which scales with texel size because the
  residue is a floating-point artefact of the lattice arithmetic — stated in the test itself.

- **D-11 — the margin fold applies to perspective projections too; only the SNAP is
  orthographic-only.** PHASE_2's fence says the snap never applies to a perspective projection, and it
  does not. The fold is a different thing: the scene rasterizes into more texels than are displayed
  regardless of projection type, so without folding, a perspective view would silently CROP by the
  margin instead of gaining it. Folding widens the FOV by the same ratio and costs nothing.

- **D-12 — `FCk_Request_Camera_SetProjectionMode` uses `TOptional<float>` in a reflected struct.**
  Root CLAUDE.md prefers enum-mode + value pairs and flags `TOptional` as open adjudication A1.
  PHASE_3 step 4 explicitly asks for "optional plane overrides", and there is in-repo precedent for
  the reflected form (`CkPmg_Fragment_Data_Donut.h:80,83`), so this follows the newer-modules branch
  of A1. If A1 is ruled the other way, the fix is local to this one struct.

## Blockers

_(none — executor: paste exact commands + full error text here, then END the phase. Do not
work around a failed gate.)_

## Human verification queue (maintainer)

Phase 6 populates the rest from VALIDATION.md §C. Queued so far:

- **[EDITOR-VERIFY] H-1 — D1 sign-off image on real geometry.** The Phase 0 captures prove the
  mechanism but were taken on the gym-cycler menu over sky, because the spike map shows a menu until
  a gym is chosen (passing `?game=Ck_AggroGym_GameMode` on the URL did NOT take — gym GameModes are
  AngelScript classes and need their full class path, which was not worth chasing in Phase 0).
  Steps: launch
  `Binaries\Win64\CkPluginsEditor.exe CkPlugins.uproject /CkTests/TestGyms/TestGyms_CkTests_Level -game -windowed -resx=1920 -resy=1080 -log`,
  press `Tab`, pick a gym with visible 3D geometry (e.g. *Crowd Foundation* or *IskmRenderer*), then
  in the console: `r.AntiAliasingMethod 0`, `r.Ortho.Debug.ForceAllCamerasToOrtho 1`,
  `r.Ortho.Debug.ForceOrthoWidth 2000`, `ck.PixelArt.Spike 1`, `Shot`.
  Expected: chunky-but-sharp pixels on the geometry, crisp UI above it. This is the image the D1
  ruling should be judged on.
- **[EDITOR-VERIFY] H-2 — 7a on geometry.** Same run as H-1, plus `ck.Usf.CelShade.Enabled 1`.
  Expected: CelShade's ink lines / halftone are chunky at texel scale, not native-res fine.
- **[EDITOR-VERIFY] H-3 — 7c PIE half.** Run the same scene in PIE at >100% OS DPI with the spike on.
  Expected: `LogCkPixelArt` logs a `Secondary view fraction is <1` line, and the image is softer than
  the standalone capture. No fix wanted — this is the evidence for "PIE lies" (standing risk 1).
- **[EDITOR-VERIFY] H-4 — 7b runtime.** With the spike on, compare `r.Ortho.AutoPlanes 1` against
  explicit near/far and confirm the auto far plane moves with the render resolution.

- **[EDITOR-VERIFY] H-5 — Phase 2 gate 6.G, the creep/stutter/smooth A/B.** Standalone, gym map with
  visible geometry, `r.AntiAliasingMethod 0`, `r.Ortho.Debug.ForceAllCamerasToOrtho 1`,
  `r.Ortho.Debug.ForceOrthoWidth 2000`, `ck.PixelArt.Enabled 1`, `ck.PixelArt.Debug.LogState 1`, then
  `Ck_PixelArt_DebugPan 0.2`. Score three states, in this order:
  1. `ck.PixelArt.Snap 0` — expected: pixels visibly CRAWL along edges. Without this the other two
     verdicts mean nothing, because it is what proves the problem exists in this scene.
  2. `ck.PixelArt.Snap 1` + `ck.PixelArt.Debug.SnapOnly 1` — expected: motion advances in whole
     texels, visibly STEPPING (proves the snap is live).
  3. `ck.PixelArt.Debug.SnapOnly 0` — expected: smooth motion, no crawl.

  If state 3 is smooth but still creeping, the compensation sign is inverted: set
  `ck.PixelArt.Debug.CompSign -1`, confirm, and say so — the default then flips in code and the knob
  goes. Anything else (jitter worse than state 1, smearing at a screen border) is a blocker, not a
  tweak. `Ck_PixelArt_DebugPan` with no argument stops the pan.
- **[EDITOR-VERIFY] H-6 — Phase 2 margin check.** During H-5 state 3, raise the pan to
  `Ck_PixelArt_DebugPan 0.49` (worst-case remainder) and watch all four screen borders. Expected: no
  smear, no garbage, no repeated edge column. Then `ck.PixelArt.Margin 0` — expected: the smear
  APPEARS, which is what proves the margin is the thing preventing it.
- **[EDITOR-VERIFY] H-7 — Phase 2 zoom policy.** With the pan running, sweep
  `r.Ortho.Debug.ForceOrthoWidth` from 2000 to 4000 over a few seconds. Expected: full-frame shimmer
  WHILE the width changes, stability once it settles. That is the documented limit of the technique
  (RESEARCH_Technique §A "Zoom"), not a defect — the check is that it settles.

### Evidence on disk (gitignored, host project — copy out before cleaning `Saved/`)

| What | Path |
|---|---|
| Gate 5.G logs (3 viewport sizes) | `Saved/Logs/PixelArtSpike_{1920x1080,2560x1440,1367x768}.log` |
| 7a runs (spike+CelShade, CelShade only) | `Saved/Logs/PixelArtCheck_{geometry,celshade,celshade_nospike}.log` |
| Spike captures (menu-over-sky) | `Saved/Screenshots/WindowsEditor/ScreenShot0000{0,1,2,3}.png` |
| 7a A/B captures | `ScreenShot00004.png` (spike+CelShade) vs `ScreenShot00005.png` (CelShade only) |

## Session log

- 2026-08-20 (Fable, session 1): research fan-out (5 sweeps) → RESEARCH_*.md; PROMPT.md
  authored; decisions locked per maintainer directive; full executor package written
  (PHASE_0–6, VALIDATION.md, this file). No source-code changes anywhere; the campaign folder
  is the only writing. Repo ground at authoring time: CkFoundation dev @ `96b5cad0e`, clean.
- 2026-08-20 (Opus executor, session 2): Phase 0 entry. CkTests `feature/pixel-art-renderer`
  created; the CkFoundation branch already existed. Reading list worked and re-verified against
  the 5.7.4 fork at `D:\Repositories\UnrealEngine-Angelscript`: `ISpatialUpscaler`
  (`PostProcessUpscale.h`), the call site + gate (`PostProcessing.cpp:600,1949-1983`), the
  `DrawRectangle` UV contract (`ScreenPass.cpp` `DrawScreenPass_PostSetup`), the screen-percentage
  clamps (`LegacyScreenPercentageDriver.cpp` — `r.ScreenPercentage.MinResolution` defaults to
  `0.0`, so a ~33% fraction survives), and `UGameViewportClient::Draw` ordering
  (`:1486` SetupViewFamily → `:1563` checkf-null → `:1687` CalcSceneView → `:1895` legacy driver
  install → `:1971` BeginRenderingViewFamily) — which confirms the fence against the checked
  `SetScreenPercentageInterface`. Divergences D-1…D-4 recorded above. Full-suite baseline launched.
- 2026-08-20 (Opus executor, session 2, cont.): **PHASE 0 COMPLETE.** Wrote `Source/CkPixelArtRender`
  (Build.cs, module, log category, `PixelArtUpscale.usf`, upscale global shader, spatial upscaler,
  scene view extension, engine subsystem) + the uplugin entry. First compile was clean — the
  renderer-private `PostProcess/PostProcessUpscale.h` include and the `Renderer` link work from a
  plugin module, which was the campaign's biggest structural unknown. Ran the standalone sweep,
  passed gate 5.G, ran checks 7a–7d, did the comment audit (stripped Phase/campaign breadcrumbs from
  shipped code), rebuilt, and re-ran the full suite on the FINAL binary: delta-zero.

  Notes for the next session:
  - `--generate` does NOT work on this machine (`Visual Studio 2022 x64 must be installed in order to
    build this target`). Build WITHOUT `--generate`; UBT picks up new modules from the `.uplugin`
    on its own. A `--build --generate` invocation also left the toolbox hung and had to be killed.
  - `GFastVRamConfig` (used by `AddDefaultUpscalePass`) is `extern` WITHOUT `RENDERER_API`, so a
    plugin module cannot link it. The upscaler omits that VRAM hint deliberately — do not "restore"
    it by copying the engine function verbatim.
  - Standalone runs are drivable head-lessly via `-CkDeferredCmdsFile=<file>` (CkCore, queues each
    line into `GEngine->DeferredCommands` at `OnFEngineLoopInitComplete`) — that is how the gate
    sweep set cvars and took screenshots without a human at the keyboard. All commands in the file
    drain in ONE pump, so it cannot express "do X, wait a frame, then do Y" — which is exactly why
    the runtime toggle-off (zero-residue) check could not be completed here.
  - `?game=Ck_AggroGym_GameMode` on the map URL does NOT select a gym (AngelScript GameMode classes
    need their full class path); the gym map shows the cycler menu until a human presses Tab.
