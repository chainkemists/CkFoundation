# PROGRESS — CkPixelArt (t3ssel8r-style 3D pixel-art renderer)

> Living doc — the ONLY place current state lives. PROMPT.md is the locked charter.
> Executor: update this file at every phase boundary, every gate verdict, and every blocker.
> Never improvise past a failed gate — record it under Blockers and stop the phase.

## Status: CAMPAIGN COMPLETE through Phase 6. Machine lines green; human queue H-1 … H-11 open (H-4's runtime half now mechanized — see the third review). Three adversarial reviews run and dispositioned.

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
| 2 | Snap + margin + remainder; creep/stutter A/B verified | **DONE** (gate 6.G human-queued) | 8 snap spec tests green; suite 1202/1199/3, same 3 pre-existing failures BY NAME; CkFoundation `54b4bfe5b`, CkTests `365db8f2` |
| 3 | CkCamera ortho (attribute + requests + ViewInfo) | **DONE** | `Ck_AutoTest_Camera_OrthoProjection` green in a fresh-discovery run; `rg -in pixelart Source/CkCamera` = 0; CkFoundation `43746c7ee`, CkTests `b5869fa0` |
| 4 | CkPixelArt module: subsystem/preset/settings/CVars, D7 preconditions | **DONE** | `Ck_AutoTest_PixelArt_SubsystemContract` green; module + uplugin entry + tier rows landed |
| 5 | PixelArt look (outline + banding + palette + band-shift edges) | **DONE** (visual rubric human-queued) | `Ck_Usf_GenerateLooks PixelArt` clean, `M_CkUsf_Look_PixelArt.uasset` written + validated; positional contract test green |
| 6 | Gym, gate of record, perf table, docs, VALIDATION executed | **DONE** (gym extended post-gate) | Gate of record delta-zero (1204/1201/3, same failing names); perf table measured; gym + both module `Claude.md`s + tier rows + two stale-doc fixes landed |
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

### Phase 4 verdicts

| Criterion | Result |
|---|---|
| Subsystem contract AutoTest | **GREEN** — default-off, full settings round-trip (renderer AND look halves), refusal under forced TSR, enable after `Request_Apply_RecommendedCVars`, reset to defaults |
| Module skeleton + uplugin entry + tier rows | Landed (`CkPixelArt`, Runtime/Default) |
| Precondition report populated, enable refused under TSR | **GREEN** — exercised deliberately by the test, which forces `r.AntiAliasingMethod 4` and restores it |

**One real bug the spec test caught, worth recording because the code contradicted its own doc.**
`Get_PreconditionReport()` cached the report from the last refusal and returned it in preference to a
fresh one, while its header comment claimed it recomputed. So a caller that refused, fixed the
setting, and asked again still got the stale complaint — the exact moment someone is reading that
report and trusting it. The cache is gone; it recomputes every call, and the refusal path relies on
the ensure's own log line for the "what did the refusal see" record.

**A consequence worth knowing:** `Request_Apply_RecommendedCVars` writes at `ECVF_SetByConsole`, not
`ECVF_SetByCode`. The situation it exists to rescue is usually one where somebody typed the offending
value into the console, and a `SetByCode` write is silently dropped underneath a console one — the
caller would see an unchanged report and no explanation.

### Phase 5 verdicts

| Criterion | Result |
|---|---|
| `Ck_Usf_GenerateLooks PixelArt` | **GREEN, zero errors.** `LogSavePackage: Moving ... to '../../Plugins/CkFoundation/Content/CkUsf/GeneratedLooks/M_CkUsf_Look_PixelArt.uasset'` -> `CkUsfEditor: Trace: Generated master for look [PixelArt]` -> `AssetCheck: ... Validating asset` with no validator output. Log: `Saved/Logs/PixelArtGenerateLooks.log:3129-3150` |
| Look contract test | **GREEN** — parses the real `PixelArt.ush` entry signature off disk and checks it against `_Parameters` name-for-name in order, plus domain / blendable location / scene textures / every parameter grouped |
| `rg -n "_Group"` on the look asset | Every parameter grouped, via the existing `CkUsf::Usf_ScalarIn` / `Usf_VectorIn` helpers |
| Visual rubric (1-texel silhouettes, brightened creases, no doubled lines, no ground staircase, palette-adjacent band shift) | **HUMAN-QUEUED (H-8)** — `[EDITOR-VERIFY]` by its own definition |

The generation run was driven headlessly the same way Phase 0's standalone sweeps were: a full editor
boot with `-CkDeferredCmdsFile` carrying `Ck_Usf_GenerateLooks PixelArt`, then the log read for the
generator's own lines. Nothing about it needed a human at the keyboard, so it is a machine line rather
than a queued one.

### Gym additions after the gate (2026-08-20)

Two gaps against PHASE_6's station list, both found by re-reading the phase doc against what was actually
built rather than by any test:

- **The judge scene had no lighting of its own.** PHASE_6 asked for a directional light with VSM and the scene
  was inheriting whatever the shared gym map provided. That matters more here than usual: every banding and
  crease verdict IS a verdict about lighting, so an inherited setup makes two runs incomparable. It now builds
  a low-angle key (shadows on, so VSM — `r.Shadow.Virtual.Enable=1` is already project-wide) plus a sky-light
  fill. The low angle is deliberate: it throws a long shadow across the ground plane, which is what gives the
  grazing-angle test something to be wrong about.

- **No UMG overlay for the native-res UI criterion.** Replaced by something better rather than reproduced:
  `ACk_PixelArtGym_HUD` subclasses the shared cycler HUD and draws an always-on station panel. Crisp text
  sitting directly on a chunky frame IS the criterion, and a panel that is always there judges it continuously
  instead of at one station the observer has to walk to.

The panel also carries **number-key station selection** (1-9 and 0, both number row and numpad — a laptop has
no numpad, and binding one but not the other makes the feature depend on hardware). The non-obvious part is
that the keys would have been inert without suspending the PROXIMITY selection: the gym picks a station by how
close the pawn is standing, so the next tick would have snapped straight back. A key press latches until the
player moves 400uu — past shuffling on the spot, well short of the 1200uu station spacing — and the panel
footer names which mode is live, because "I pressed 5 and it went back to 2" is otherwise silent.

Verified: AngelScript compiles clean, the gym loads via
`?game=/Script/Angelscript.Ck_PixelArtGym_GameMode`, `Game class is 'Ck_PixelArtGym_GameMode'` and
`Client Set HUD` both appear in the boot log. **NOT verified: how any of it looks.** The panel's layout,
whether it collides with the Tab hint, and whether the lighting flatters or ruins the banding verdicts are all
human observations — folded into H-8 and H-9.

### Post-campaign — a real defect the margin tripwire caught (2026-08-20)

Found while adding the gym's own lighting, which is what made it reachable: a real-time sky light triggers a
cubemap capture, and that capture arrives at the scene view extension as its OWN view family with its OWN
render target.

```
LogRenderer: Forcing update for all mesh draw commands: SkyLight real-time capture change
LogCkPixelArt: Active: viewport=128x128 rendered=128x128 displayed=360x128 at (3,0) fraction=1.000000
LogCkPixelArt: Active: viewport=1920x1080 rendered=648x365 displayed=640x360 fraction=0.337240
```

**What was wrong.** `BeginRenderViewFamily` already skipped captures (no view state), but `SetupViewFamily` —
the hook that drives `r.ScreenPercentage` — had no equivalent guard. So the renderer computed geometry from a
128x128 texture nobody sees and, because the screen percentage is a GLOBAL console variable, stamped that
frame's value over the real viewport's. Every capture frame, forever. Any world with a scene capture, a
reflection capture, a planar reflection or a real-time sky light hits it, which is most games — the campaign
simply never had one in the scene until the gym gained a fill light.

A second defect fell out of the same line: an internal height LARGER than the viewport degenerated silently
(displayed 360x128 out of a 128x128 render, zero margin on every side). This renderer only ever downscales, so
that never supersamples — it just breaks. Reachable on any window smaller than the configured internal height.

**Fixes.** `SetupViewFamily` now bails unless the family's render target IS the game viewport's. Identity, not
a list of capture kinds: the flags (`bIsSceneCapture` and friends) live on `FSceneView` and not on the family,
the family's views are not built yet at that point, and the question is strictly "is this the thing on screen".
The internal height is separately clamped to what the viewport holds with margin, warning that the grid is
coarser than configured — a small window is a legitimate state, not a misconfiguration.

**Verified at runtime**, same gym boot before and after: the `viewport=128x128` line and the margin error both
go from 1 occurrence to 0, while `viewport=1920x1080 rendered=648x365 displayed=640x360 margin=4/2/3` is
unchanged. Log: `Saved/Logs/PixelArtGymBoot.log`.

**Worth recording about the method, not just the bug.** This was caught by the runtime tripwire the Fable
margin ruling asked for — its flagged risk was that the margin guarantee was proven with clean arithmetic and
might not survive real viewport sizes. It did not, for a reason neither of us predicted, and the assertion is
the only thing that said so. The main viewport looked perfect throughout and the bad frames were interleaved
with good ones; no screenshot would have shown it.

### Gate of record — the delta, in full

| | Baseline (Phase 0) | Gate of record (Phase 6) |
|---|---|---|
| Total | 1188 | 1204 (+16, all added by this campaign) |
| Passed | 1184 | 1201 |
| Failed | 4 | 3 |
| Failing NAMES | `Angelscript.CppTests.AngelscriptCodeCoverage.IntegrationTest`, `Crowd_NavQueryFilter_ForceReplan`, `PathNetworkFollower_ProjectsRibbonWaypointWithinNavQueryExtent`, `PathNetworkFollower_DesiredNavmeshClearanceMovesInward` | the same three, minus the Angelscript coverage one |

The Angelscript coverage test is an ENGINE test asserting that coverage reports exist under
`Saved/Automation/Tmp/TestOutput/`; it passes once earlier runs in a session have populated that
directory, which is what happened here. It was never touched and is not claimed as fixed.

**The 16 tests this campaign added**, all green: 2 fraction · 3 config registry · 8 snap math ·
1 camera ortho AutoTest · 1 subsystem contract AutoTest · 1 look positional contract.

**One honest gap in what the gate covers.** Nothing automated exercises the look's SHADER BODY — the
contract test checks its signature and the generator checks it compiles. Two real defects in that body were
found by reading it back rather than by any test (D-20, D-21), which is a fair estimate of how much is
still resting on the H-8 rubric. If one claim here is most likely to be wrong, it is that the shader does
what its comments say; the arithmetic around it is pinned, the arithmetic inside it is not.

### Phase 6 — perf measurement (success criterion 8)

Measured on THIS machine and THIS scene, by the renderer's own `ck.PixelArt.Debug.PerfSweep 5`: three
stages of 300 frames each, the first 30 of every stage discarded while render targets settle, mean GPU
milliseconds per frame from `RHIGetGPUFrameCycles()`. Standalone `-game -windowed -resx=2560 -resy=1440`,
gym map, VSync and the frame cap off. Log: `Saved/Logs/PixelArtPerfSweep.log:2061-2107`.

| Configuration | Mean GPU ms/frame | vs native |
|---|---|---|
| OFF (native 2560x1440) | **1.214** | — |
| ON @ 360 internal | **0.854** | 70.3% |
| ON @ 180 internal | **0.802** | 66.0% |

ON is cheaper than OFF, which is the direction the design predicted: the scene rasterizes into about 6% of
the pixels and the upscale is one fullscreen pixel shader. No STOP condition.

**Read these as a direction, not as a budget.** The gym judge scene is deliberately geometry-light (engine
basic shapes), so native is only 1.2 ms to begin with and the fixed costs — the upscale pass, the
post-process chain that runs at internal resolution either way — are a large share of what is left. On a
scene where native GPU time is dominated by pixel work the ratio would improve; on one dominated by draw
call count or vertex work it would not, because the renderer changes neither. Re-measure on the real scene
before quoting a number to anyone.

**What the same run also confirmed at runtime, on a viewport size no test covers:** at 2560x1440 with a
360-texel internal height the renderer chose `rendered=648x365 displayed=640x360 at (4,2) margin=4/2/3`.
That is exactly what `Get_HorizontalMarginTexels` predicts, so the aspect-compensated margin ruling holds
outside the unit tests as well as inside them.

### Phase 6 — VALIDATION.md execution

| # | Check | Verdict |
|---|---|---|
| A1 | Fraction exactness unit tests | **GREEN** — 8x4 viewport/target sweep |
| A2 | Snap math property tests | **GREEN** — 8 tests, incl. the margin fold and the basis extraction |
| A3 | Camera ortho AutoTest | **GREEN** — present in fresh discovery, not a zero-match |
| A4 | Subsystem contract AutoTest | **GREEN** |
| A5 | Look contract test | **GREEN** |
| A6 | Look generation validator | **GREEN** — zero errors, master written and validated |
| A7 | Full suite, gate of record | **GREEN — delta-zero.** 1204 total / 1201 pass / 3 fail, `--test --no-live --discover-fresh`, 5m 07s, log `Saved/Logs/Test-GateOfRecord.log`. The three failures are the SAME NAMES as the Phase-0 baseline (`Crowd_NavQueryFilter_ForceReplan`, both `PathNetworkFollower_*`). 1204 = the 1188 baseline + 16 tests this campaign added |
| A8 | Zero-residue greps | **GREEN** — `rg -n "PixelArt.Spike" Source` = 0; `rg -in "pixelart" Source/CkCamera` = 0 |
| A9 | Shipping-config compile | **UNRUN** — the toolbox lane used here is `--config=Development --target=Editor`; a Shipping target was never built in this campaign, so this line is honestly unrun rather than assumed |
| B1 | C++ surface | **GREEN** — the unit tests call the utils directly |
| B2 | Blueprint surface | **HUMAN-QUEUED (H-11)** — the nodes exist by construction (UFUNCTION + DisplayName), but "a BP graph can call them" is an editor observation |
| B3 | AngelScript surface | **GREEN** — the AutoTests and the gym drive the subsystem, the camera requests and the presets entirely from AngelScript, and they compile and run |
| C1-C9 | Visual / human | **QUEUED** — H-1 … H-11 below |
| D | Change-control class 3+ | Docs landed, comment audit done, everything committed on the feature branches, nothing pushed, no submodule pointer bumps |

## Adversarial audit (Fable, 2026-08-20) and what was done about it

A full adversarial pass over both modules, the CkCamera diff, the shader against its sibling looks, all five
test files and the gym, with the engine-fork claims re-verified from source. 17 findings + 3 questions.
Every one is dispositioned below; the two CRITICALs were confirmed against real code before anything moved.

### Fixed

| # | Finding | Disposition |
|---|---|---|
| 1 | **The shader double-decoded the world normal** (`*2.0-1.0` on an already-decoded `PPI_WorldNormal` tap) | CONFIRMED against `CelShade.ush:104`, `EdgeOutline.ush:48`, `CrossHatch.ush:98` - all three use the tap raw. Removed, and the sibling `+ float3(0,0,1e-6)` epsilon added: the bad remap was accidentally masking a NaN on sky pixels, where the normal is zero. This corrupted crease contrast AND made `Facing` vary with camera yaw on a flat floor, so the grazing-angle guard - the very thing D-21 was fixed for - was misbehaving silently |
| 2 | **`r.ScreenPercentage` restored at the wrong PRIORITY**, killing the resolution-quality slider for the process | CONFIRMED: `Scalability.cpp:551` writes at `ECVF_SetByScalability`, weaker than the `SetByCode` the restore pinned. Now saves the priority beside the value, writes the value at the CURRENT priority so it cannot be dropped, then restores the original priority bits |
| 3 | **Per-world CVar priors over process-global CVars** - one PIE world's teardown restored values out from under another that was still rendering, and the second world held no prior to fix it | Replaced with a process-wide REFCOUNTED lease. Last holder out restores; a world that finds the value already moved takes a reference rather than recording a bogus prior |
| 4 | The recommended CVars were pinned at `SetByConsole` after restore, so they would ignore project settings and device profiles forever | Same priority-restoring mechanism as 2 |
| 6 | **Split-brain look effect**: `DoEnsure_LookEffect` keyed only on the MID, which outlives the world-spawned actor carrying the blendable - once the actor died the look silently never rendered again | Now a reconciler rather than a cache: validates MID, component AND actor, rebuilds when any is gone, and resets `_WrittenLook` so the next write is full rather than a diff against a dead material |
| 7 | `Request_SetProjectionMode` had **zero coverage in any environment** - the AS test reached ortho only through the profile | Extended `CkAutoTest_Camera_OrthoProjection`: switches mode both ways through the request, carries clip planes with it, and asserts both reach `ViewInfo`. This also answers Q3 (whether the generator binds the reflected `TOptional<float>` members) by exercising them |
| 8 | **The margin-fold assertions were tautologies** - algebra of the test's own arithmetic, passing whether or not the implementation existed | The fold is now `ck::pixel_art::Apply_MarginFold`, called by the view extension and DRIVEN by the test, which reads the result back off the matrix. Plus a degenerate-input case |
| 9 | The "screen percentage was rejected" Error sat behind the geometry early-out, so the common case never reported | Moved above it |
| 10 | The margin error blamed the margin when the cause was internal resolution >= viewport | Message now distinguishes the two |
| 11 | `!` instead of `NOT` in a CkCore-linked module | Fixed; the D-2 exemption covers only the CkCore-less render module |
| 12 | The gym's pan "toggle" could never stop the pan (always passed a speed, which the command treats as restart) | Tracks its own state and sends the bare form to stop |
| 13 | ToggleLoop's "run with the renderer OFF" was documentation, not a guard - started while enabled it produces a false FAIL | Refuses with an explanation |
| 15 | Restore stamped a remembered value over one the user had since typed into the console | Restores only when the current value is still the one we wrote |
| 14 | `Request_Apply_RecommendedCVars` could not clear `ShowFlag.ScreenPercentage`, a precondition it reports - apply, read an unchanged report, nothing to try next | The escape now sets the show flag too. `r.SecondaryScreenPercentage` stays applied-but-not-gated, now with a comment saying why: it costs sharpness, it does not prevent rendering, and gating on it would refuse to enable on every DPI-scaled display |

### Disagreed with, and why

**Finding 5 - "there is no crease de-doubling gate; every crease renders as a 2-texel pair".** The behaviour
is confirmed. The conclusion is not: this look FOLLOWS section C.3, whose reference implementation returns
highlight-or-darken per texel and in which both sides of a fold do fire. A bright/dark pair reads as a lit
bevel and is literally "highlight the outward-facing edges, and darken the outlines" with both sides visible.
The defect section C.1's gate prevents is a different shape - two texels given the SAME treatment, which
reads as one fat line. So the code is documented and kept, and **H-8's wording is what changed**, because the
rubric as written described the other defect. This is a visual judgment that cannot be settled headlessly; the
H-8 note now says so explicitly and names the fix if the maintainer disagrees on sight.

### Recorded, not actioned

- **16 - naming drift.** `UCkPixelArt_Subsystem` (no `_UE`) follows the CelShade exemplar PHASE_4 said to
  mirror exactly; `FCk_PixelArtRender_StateRegistry` follows house form. Both defensible in isolation, but the
  pair is inconsistent. Not renaming a committed public type over it - noted for the maintainer.
- **17 - `DoOn_CVarChanged` is a near-no-op** by the module's own design. Kept because it becomes load-bearing
  the moment the look half gains any CVar, and its absence would then be a silent missing re-sync.
- **Q1 - scene captures with view state.** Already closed before the audit reported, by the render-target
  identity guard: a `bCaptureEveryFrame` capture renders into its own target and never reaches the drive. The
  audit was reading pre-fix code.

### What this says about the method

Two of the three worst findings were in the shader BODY - the exact gap called out above as the one thing
nothing automated covers. That is now three shader defects (D-20, D-21, finding 1) found by reading against
the sibling looks and zero found by any test. The shader is the part of this feature least protected by the
gate, and a reviewer's eyes are currently the only thing protecting it.

## Post-campaign — the gym was perspective, so the snap never ran (2026-08-20)

Reported by the maintainer on sight: "The camera in the gym is a perspective camera and not orthographic I
think, which is why I see flickering." Correct, and it is the most consequential gym defect found so far.

**What was wrong.** The gym had no camera of its own. `ACk_Gym_Base_Pawn` derives from `ADefaultPawn`, which
is perspective, and none of the four gym scripts touched a camera. The view extension deliberately returns
before the snap on a perspective projection (`CkPixelArtRender_ViewExtension.cpp`, `SetupViewProjectionMatrix`
— one world-space snap displaces near and far geometry by different screen amounts, so the technique is
orthographic-only by construction). The margin fold and the box upscale still ran, so the image still
pixelated and nothing looked broken.

The consequence is that every station had been rendering low-resolution and UNSNAPPED — which is exactly the
CREEP station. Stations 5/6/7 are the A/B rig for pixel creep and they were three copies of the same image.
Every creep, stutter and smoothness verdict the gym could produce was worthless, and it produced them
confidently.

**Why it survived the whole campaign.** `r.Ortho.Debug.ForceAllCamerasToOrtho` appears five times in this
campaign's own docs and zero times in gym code. The manual verification instructions opened by telling the
operator to type it. A gym whose central demonstration depends on the operator remembering a debug console
variable is not a gym that demonstrates anything — and it is exactly the shape of defect that reads as
working, which is why no automated line caught it either.

**Decision (made, not delegated — low-blast and reversible).** The gym composes the camera it needs, using
CkCamera rather than the debug CVar or a bare `UCameraComponent`. Three reasons, in order: this campaign added
`Request_SetProjectionMode` to CkCamera and had no gym exercising it; `ACk_CameraGym_Pawn` is an existing
precedent for a gym pawn that composes a CkCamera, so this is mimicry rather than invention; and forcing a
global debug CVar from a gym is the same "hide the problem behind a fallback" shape the subsystem's
precondition gate was built to refuse.

**What changed.**

| File | Change |
|---|---|
| `CkTests/Script/CkPixelArt/CkPixelArtGym_Pawn.as` (new) | `ACk_PixelArtGym_Pawn` composing a CkCamera with an orthographic sensor (OrthoWidth 2200 → ~3.4uu/texel at 360p, explicit 10/50000 clip planes), plus `UCk_PixelArtGym_CameraLayer_Fixed` holding a fixed boom (2500uu, pitch -35) with orbit, auto-reorient and collision all off — each of those would move the camera for reasons unrelated to the pan, and the creep verdicts are read off camera motion |
| `CkPixelArtGym_GameMode.as` | `DefaultPawnClass` repointed, with the reason on the line |
| `CkPixelArtGym_PlayerController.as` | `Get_ProjectionIsOrthographic()` / `Request_ToggleProjection()` forwarders, read off the pawn rather than mirrored; header updated |
| `CkPixelArtGym_HUD.as` | `[P]` flips the projection; a footer row reports the mode, in warning colour on perspective |

**Correction found on the maintainer's first run.** The first version passed a default-constructed
`FCk_Fragment_Camera_ParamsData()`, copying `ACk_CameraGym_Pawn`, whose comments state that the director
"auto-creates a `UCk_CameraComponent` on this pawn". It does not. `_OutputComponent` is the single ESSENTIAL
constructor parameter of the params struct and is `CK_PROPERTY_GET` only, so there is no path that supplies it
after construction — the ensure fired immediately:

```
Camera Add on entity [25|0(25)(Ck_PixelArtGym_Pawn_0)] requires a valid output UCk_CameraComponent
supplied in FCk_Fragment_Camera_ParamsData - none was provided.
```

The pawn now declares `UPROPERTY(DefaultComponent) UCk_CameraComponent CameraComponent` and passes it to the
constructor, which is what `ACkAutoTest_GameplayCamera_Helper` has been doing all along — the AutoTest fixture
was the correct exemplar and the gym pawn was not. Mimicking a sibling is only as good as the sibling, and the
comment was more confident than the code.

**Adjacent defect, flagged not fixed:** `CkTests/Script/CkCamera/CkCameraGym_Pawn.as:90` has exactly this bug
and declares no camera component anywhere — the CkCamera gym should be hitting the same ensure on entry. Left
for its own change rather than folded in here.

**The clip planes are explicit on purpose**, and this is the second time that mattered: auto-calculated ortho
planes are derived from the view rect, so the 180 / 360 / 540 stations would each have been judging a
different depth range while appearing to differ only in resolution.

**`[P]` is part of the exercise, not a debug affordance.** Perspective IS what the defect looks like, and the
gym could not previously show it. Being able to flip the same station between the two is the fastest way to
learn to recognise it — which is the thing that would have caught this on day one.

**Correction to the manual verification instructions.** The step "run `r.Ortho.Debug.ForceAllCamerasToOrtho 1`
and `r.Ortho.Debug.ForceOrthoWidth 2200` before judging anything" is now obsolete: the gym is orthographic on
entry. H-8's rubric assumes an orthographic view, and previously did not say so.

## Module rename: `CkPixelArtRender` -> `CkPixelArtRenderer` (2026-08-20)

Maintainer call, for consistency with the `CkIsmRenderer` / `CkIskmRenderer` siblings. Mechanical, but with
three consequences worth recording because each would have been a silent breakage:

- **The generated AngelScript wrapper had to be deleted.** `Script/Generated/utils_pixel_art_render.as` is
  gitignored and regenerated at editor startup, but it is compiled BEFORE it is regenerated - and it referenced
  `UCk_Utils_PixelArtRender_UE`, a class that no longer exists under that name. Left in place it would have
  failed the AngelScript compile on the next boot with an error pointing at a file nobody edited. It now
  regenerates as `utils_pixel_art_renderer.as`.
- **The log category was renamed too**, `LogCkPixelArt` -> `CkPixelArtRenderer` (dropping the engine-style `Log`
  prefix, which no other category in the plugin carries - all ~90 siblings are bare `Ck<Module>`). The old name was derived
  from a module name that no longer matched, and it collided in the reader's eye with the SEPARATE `CkPixelArt`
  category that the sibling module declares through `CK_DEFINE_LOG_FUNCTIONS`. Two categories one character
  apart, on two different modules, is a debugging trap. Quoted log evidence earlier in this document predates
  the rename and is left verbatim; instructions that tell a human what to grep were updated.
- **The `/CkPixelArt` shader virtual mapping was NOT renamed.** It namespaces the feature, not the module, and
  the look shader in CkUsf sits under the same feature name. Renaming it would have churned shader paths for no
  reader benefit.

Also added while here: the two rows this feature never had in `Source/CLAUDE.md`'s "I need to..." decision
tree. A module with no row there is a module nobody finds.

### Operational trap: renaming a test leaves a stale toolbox plan that ABORTS LANES

The first post-rename gate reported `1204/1201/3` - exactly the baseline - and still could not be trusted. Buried
in the tail:

```
[utb --test] [lane 1/3]: no progress across 2 consecutive spawns - aborting with 4 tests never run
             (first: 'CkTests.UnitTests.CkPixelArtRender.ConfigRegistry.RejectsNullWorld')
```

All three lanes aborted that way (4 + 5 + 4 = 13 never run, which is exactly the pixel-art test count), and every
name they cite is the PRE-rename one. `Saved/UnrealToolbox/TestBatchCmds__lane_*.txt` - regenerated by that very
run - still listed the old names, so each lane spawned an editor looking for tests that no longer exist, made no
progress, and killed itself.

Two lessons, both about not trusting a number:
- **A pass/fail count matching the baseline is not evidence the suite ran.** Here it matched precisely BECAUSE
  the never-run tests were counted out of the total rather than as failures. The abort line is in the tail, far
  below the summary block, and nothing else flags it.
- **Renaming any automation test invalidates the toolbox plan.** Delete `Saved/UnrealToolbox/TestBatchCmds*.txt`
  after a test rename and re-run; they regenerate. Otherwise the gate silently skips exactly the tests whose
  names changed - the ones the rename most needed to exercise.

## Second Fable review - house-idiom conformance (2026-08-20)

Requested by the maintainer alongside four direct instructions (rename the module, use the strict utilities,
cut the comments, and "the code should be very similar to how it is written elsewhere in CkFoundation").
No critical bugs. 5 MAJOR, 14 MINOR, 4 uncertain. The reviewer verified the Stylize exemplar shapes before
reporting and correctly did NOT report the T0 module's `!` / `ensureMsgf` / plain-struct usage.

### The one that mattered: an un-restored show flag

`Request_Apply_RecommendedCVars` set `EngineShowFlags.SetScreenPercentage(true)` and nothing ever put it back,
while the three console variables beside it got the full refcounted, priority-preserving lease. The subsystem
header promises the opposite ("Disabling ... restores any console variables ... moved"), so a project that
deliberately runs with that flag off had it silently latched on for the world's life after a single
Apply/Disable cycle. Fixed with `DoRestore_ScreenPercentageShowFlag`, which follows the same only-undo-what-is-
still-ours rule as the CVar leases: if somebody turned it on themselves in the meantime, it is left alone.

That is a genuine miss from the FIRST audit, which added the show-flag write specifically so the escape could
clear every row it reports (finding 14 there) and did not ask what puts it back.

### Comment density, measured rather than asserted

The maintainer's "avoid extensive comments" was actionable once measured against neighbours instead of taste.
Baselines: CkTimer `.cpp` 1.9%, the CkUsf Stylize `.cpp` files 5.7-6.3%, their headers 19-22%, complex
`Looks/*.ush` 28-43%.

| File | Before | After |
|---|---|---|
| `CkPixelArtRenderer_ViewExtension.cpp` | 17.4% | 9.2% |
| `CkPixelArt_Subsystem.cpp` | 9.7% | 6.1% |
| `CkPixelArt_Subsystem.h` | 19.4% | 12.2% |
| `CkPixelArt_Params.h` | 16.2% | 14.0% |
| `CkPixelArtRenderer_SnapMath.h` | 58.4% | rewritten; prose replaced by `/** contract */` blocks |
| `PixelArt.ush` | 28.8% | unchanged - already at CelShade (29.2%) and ScreenDither (30.0%) |

**A correction worth recording:** the first pass applied a 6% target to HEADERS as well, which was wrong - it
was measured on `.cpp` files only. `CkUsf_CelShadeSubsystem.h`, the exemplar this feature mirrors, is 22.5%.
Headers carry contract docs and legitimately run three times denser than implementation. `PixelArt.ush` was
left alone for the same reason: measured against its actual siblings it was never an outlier.

### Two defects found by looking rather than by testing

- **A doc block attached to the wrong function.** The first audit's extraction of `Apply_MarginFold` inserted
  the new declaration BETWEEN `Get_HorizontalMarginTexels`' doc block and its declaration, so one public
  function was documented by the other's contract and the second had none. Nothing catches this but reading.
- **Campaign breadcrumbs in a shipped shader.** `PixelArt.ush` cited `RESEARCH_Technique.md` sections C.1/C.2/
  C.3/D in two places - a direct violation of the comment rule ("a comment naming a Gate, Phase, PROMPT,
  campaign ... is always noise"), and the only `Looks/*.ush` in the suite that referenced a campaign doc.

### Idiom fixes

Log category `LogCkPixelArt` -> `CkPixelArtRenderer` (the `Log` prefix was the only one in ~90 categories);
BPFL category -> `Ck|Utils|PixelArtRenderer`; `Ck_PixelArt_DebugPan` -> `ck.PixelArt.Debug.Pan` (the module's
own banner declares everything lives under `ck.PixelArt.*`); the missing `CK_DEFINE_CUSTOM_FORMATTER_ENUM` for
the two T0-defined enums, hosted in CkPixelArt since the macro cannot live in a CkCore-less module;
`ck::Is_NOT_Valid` for the two UObject pointers that used `== nullptr` (pending-kill reads as valid there);
`ck::algo::Transform` + `ck::Format_UE` for the one hand-rolled transform loop; `NOT Test*` in the unit tests
(needing an explicit `CkCore/Macros/CkMacros.h` include, because the module under test pulls in no Ck header);
`Make_Config` and named `constexpr` bool arguments; engine includes normalised to angle brackets (1518 vs 178
suite-wide).

### Declined or documented rather than fixed

- **Restoring the AutoTest's prior `r.AntiAliasingMethod`.** Correct in principle, but `UCk_Utils_CVar_UE`
  exposes no value getter to AngelScript, and adding one to CkCVar for a test is scope creep into a module
  nobody asked me to touch. The literal `0` stays, now under a KNOWN LIMITATION comment that says so.
- **Type-prefix split** (`FCk_PixelArtRenderer_StateRegistry` vs `FCk_PixelArt_RenderConfig`) and the
  `ck::pixel_art` / `ck::pixelart` namespace pair. Both are real inconsistencies. Renaming committed public
  types and a namespace reached from tests and AngelScript is churn disproportionate to a MINOR, and the
  reviewer offered "commit to PixelArt = the feature name" as an equally valid resolution. Left for the
  maintainer.

### Uncertain items, recorded not chased

The reviewer flagged four it could not confirm. The one worth a second pair of eyes: `SetupViewFamily` resets
the per-frame state for EVERY active family, so if any capture or planar-reflection family's `SetupViewFamily`
could interleave between the game viewport's `SetupViewFamily` and its `BeginRenderViewFamily`, that frame
would lose its upscaler and render blurry. The reviewer's reading of the 5.7 flow says captures update during
the world tick, before `UGameViewportClient::Draw`'s contiguous hook run, so it cannot interleave - but that
is a reading, not a test. The other three: a blocking `LoadObject` on the explicit (non-deferred) settings
path, `DoCheck_ResetToDefaults` assuming the host project configures no default preset, and the two
edge-detector members being per-extension rather than per-world (log-only impact).

## Third adversarial review (Fable, 2026-08-20) — the rendered result, end to end

Scope per the maintainer's brief: the shader against RESEARCH_Technique §C/§D line by line, the four
uncertainties left open by review #2 (the first chased to ground against the pinned engine), the
supported-but-unexercised conditions (portrait, DPI, resize, multi-world, roll, zoom), and moving human
verification into the suite where the lane environment allows it. Four findings fixed, three recorded, one
review-#2 uncertainty refuted with engine evidence.

### Fixed

**1 — CRITICAL: review #2's open uncertainty #1 is REFUTED — the capture interleave is real.** The chain,
read from the 5.7.4 fork rather than argued: `FRendererModule::BeginRenderingViewFamilies` updates deferred
scene captures (`SceneCaptureUpdateDeferredCapturesInternal`, `SceneRendering.cpp:5173`) BEFORE
`CreateLinkedSceneRenderers` (`:5176`), and the main family's `BeginRenderViewFamily` extension callbacks
fire inside `CreateSceneRenderers` (`SceneRenderBuilder.cpp:507`). The capture path gathers ALL active
extensions (`SceneCaptureRendering.cpp:886`) and calls `SetupViewFamily` on each (`:812`). So every frame, a
`bCaptureEveryFrame` scene capture's `SetupViewFamily` lands BETWEEN the game viewport's `SetupViewFamily`
and its `BeginRenderViewFamily` — and the old reset-first shape wiped `_FrameConfig`/`_FrameReport` there,
so the viewport silently lost its upscaler on every frame a capture was alive: engine CatmullRom on the
low-res render, the margin displayed instead of inset, no snap compensation. Any game with a minimap or
mirror hits it permanently. Review #2's reading ("captures update during the world tick") is true only for
sky-light and reflection captures (`UWorld::Tick`); it does not hold for SceneCapture2D/Cube deferred
captures. Fix: BOTH hooks now identity-check the family against the game viewport before touching or
consuming per-frame state (`Get_IsPrimaryViewportFamily` in `SetupViewFamily` AND `BeginRenderViewFamily` —
the Begin check also matters because a `bCaptureEveryFrame` capture HAS view state, so the old
`State != nullptr` guard would not have kept it from consuming the viewport's geometry once the reset
stopped protecting it by accident). The gym's judge scene now keeps a 128x128 per-frame capture alive as a
permanent tripwire: a regression of this guard makes the whole gym go soft and mis-framed on sight.

**2 — Shader: the crease detector fired across occlusion boundaries** (`PixelArt.ush`). The far side of
every silhouette carried a large normal contrast with no crease in it, so it rendered a bright or dark
fringe hugging each outline — worst against the sky, where the zero-normal's `(0,0,1)` stand-in guarantees
maximum contrast against any side face and the convex rule classifies it BRIGHT: a 1-texel halo in the sky
around every object, reading as a doubled silhouette (H-8 rubric item 3). This is exactly what
RESEARCH_Technique §C.2's `saturate(normalDifference - invDepthDifference)` suppression and §C.3's
silhouette `continue` exist to prevent, and this shader had neither. Fix: a per-neighbour continuity gate —
a neighbour's normal contrast is zeroed when the depth step to it exceeds the silhouette cutoff computed
from the NEARER of the two depths. The min-depth basis is the load-bearing part: the sky's own enormous
depth would otherwise inflate its gate right back open, and for the silhouette direction (neighbour deeper)
min == center, so the silhouette test itself is numerically unchanged. At the gym's framing the lit-bevel
pair at a cube's ground contact survives the gate (step ~174uu against a ~2400uu-depth cutoff of ~360uu+),
so the intended §C.3 look is intact — only the fringe on the far side of true occlusion boundaries dies.
Also removed while in the block: a dead first assignment to `MostDifferentFacing`, and the classification
loop now reuses the gated contrasts instead of recomputing raw ones.

**3 — The DPI divide-out was structurally inert** (`DoApply_ScreenPercentage`). It divided by
`InViewFamily.SecondaryViewFraction`, which at `SetupViewFamily` time still holds the construction default
1.0 — the engine assigns the DPI-derived value at `GameViewportClient.cpp:1566-1584`, between the hook at
`:1486` and the projection hook at `:1687`. So in PIE above 100% OS DPI, or under an explicit
`r.SecondaryScreenPercentage.GameViewport` (including the Phase-1 verification run at 50), the internal
geometry was wrong by the secondary fraction: width exactness broke, the upscaler warned "the camera snap
was computed for", and pixels crept. Fix: the drive divides by the LAST SETTLED fraction, read in
`BeginRenderViewFamily` the frame before — exact whenever DPI is stable, self-healing one frame after a
change (the upscaler's existing warning names the single mismatched frame). PIE remains "preview only" for
sharpness — the engine's SmoothStep secondary resample still runs after ours — but the geometry and the
snap compensation are now exact in it, which retires the geometry half of standing risk 1.

**4 — The shader re-implemented three StylizeCommon helpers** (non-negotiable #9). `SnapToPalette` was a
verbatim copy of `CkUsf_Stylize_NearestPaletteColor`; palette modes 0/2 re-implemented
`CkUsf_Stylize_QuantizeSteps` / `CkUsf_Stylize_QuantizeLuminance` minus their top-band `min()` guard (an
input of exactly 1.0 overshot to `N/(N-1)` before the final saturate). Replaced with the library calls;
behaviour identical except the guard now holds.

**5 — Both new module `Claude.md`s were never actually committed.** This repo gitignores `*.md`
(`.gitignore:49`, "md files, they should be force added instead"); every tracked module doc
(`CkTimer/Claude.md`, `CkCamera/Claude.md`, `CkUsf/Claude.md`) was force-added at some point —
`Source/CkPixelArt/Claude.md` and `Source/CkPixelArtRenderer/Claude.md` never were. They existed only on
this machine's disk: the branch as committed shipped the feature with no module documentation, the Phase-6
exit evidence ("both module Claude.md's ... landed") was wrong, and all three adversarial reviews read the
files from disk without noticing — a reviewer diffing the BRANCH would have seen a feature with no docs.
Fixed by force-adding both in this review's docs commit. Lesson: after authoring any new `.md` outside
`docs/`/`.claude/`, check `git ls-files` for it — "the file exists" and "the file is tracked" diverge
silently here by design.

Plus two new pinning unit tests, `CkTests.UnitTests.CkPixelArtRenderer.OrthoPlanes.*` — see the H-queue
verdict below.

### The unexercised-conditions sweep — verified clean, with the evidence

- **Portrait and non-16:9.** The arithmetic holds at aspect < 1 (worked example 1080x1920 @ 360p: inner
  203x360, horizontal margin 2, rendered 207x368, insets 2/4/4 — all ≥ the requested 2).
  `Get_OrthoWidthFromProjection` reads the SPAN (`2/M[0][0]`), not the camera's OrthoWidth, so the
  landscape/portrait axis-multiplier flip its doc block warns about never reaches the texel math, and
  `Apply_MarginFold`'s derived `M[1][1]` keeps texels exactly square at any aspect (proven by the existing
  fold test's square-texel assertion across 6 aspects).
- **Camera roll and rotated yaw.** Already covered by tests, not merely claimed:
  `Snap.BasisMatchesCameraAxes` pins the column extraction against `FRotationMatrix` axes at yaw -120 /
  roll 22.5, and the four 1000-sample property sweeps draw roll from the full -180..180 range. A transpose
  error fails those today. The remainder lives in the view basis, which is also what the upscaler's UV
  shift is expressed in, so compensation under roll is correct by construction.
- **Window resize mid-run, both resolution modes.** Geometry is recomputed every frame from
  `RenderTarget->GetSizeXY()`; `GeometryIsUnchanged` gates only the logging. TexelsPerPixel re-derives the
  inner height from the live viewport each frame, which is the mode's definition.
- **Two PIE worlds / one enabled one not.** Per-frame state is written and consumed inside one viewport's
  `Draw`; a second world's family resets state only in its OWN Draw and (post-fix) only for its own primary
  family. The CVar leases were already process-wide and refcounted (audit finding 3). Residual:
  `_LastViewportSize`/`_LastFraction`/`_LastSecondaryViewFraction` are per-extension, so two viewports with
  DIFFERENT sizes or DPI alternate the change-log every frame and each mispredicts the other's secondary
  fraction — log noise plus a one-frame-class geometry error, only in multi-viewport PIE, recorded not fixed.
- **Zoom.** The shimmer-while-changing / stable-once-settled behaviour is the technique's documented limit
  (RESEARCH §A) and no cheap improvement exists inside this architecture: the alternative is QUANTIZED zoom
  (integer texel-per-pixel steps, scaling the upscale window between steps), which is a Phase-7 feature —
  it needs its own upscale-quad scaling, not a tweak to the current path.

### Recorded, not actioned

- **Ortho depth-threshold units are boom-relative.** The silhouette cutoff multiplies
  `max(CenterDepth, 1.0)` — the CelShade/EdgeOutline sibling shape, correct under perspective where depth
  deltas scale with distance. Under ortho they do not: the cutoff varies with the boom length (which has no
  other visual effect in ortho — boom 2500 vs 250 gives 10x different edge verdicts on an identical image)
  and across the screen (~2x between near and far ground at the gym's pitch), so identical props at the top
  and bottom of the frame can get different edges. The research-faithful ortho form is a fixed view-space
  threshold (§C.2), but that changes the knob's units and demands re-tuning with eyes on the image — so it
  is folded into H-8: if the rubric shows edge verdicts varying across the ground plane, the fix is to gate
  the `max(CenterDepth, 1.0)` factor on the projection type (`View.ViewToClip[3][3]` is 1 under ortho) and
  re-calibrate `DepthThreshold` in absolute units for the ortho branch. Not changed blind: with the default
  0.15 relative threshold the gym's silhouettes near the ground are actually drawn by the crease pair, not
  the depth test (a 100uu cube's step is ~174uu against a ~375uu+ cutoff), and re-tuning that balance
  without an image would be guessing.
- **The lit-bevel crease pair** stands per the settled disposition, and the new continuity gate was checked
  against it: the pair still fires at cube-ground contact (the step is well under the gate), so H-8 item 2's
  intended look is unchanged.
- **`bConstrainAspectRatio` (letterboxed cameras).** The drive predicts from the render-target size; a
  constrained camera's view rect is a sub-rect, so the prediction would mismatch, the upscaler would warn,
  and pixels would creep. Nobody has asked for letterboxed pixel-art; a matrix row can be added when someone
  does.

### H-queue mechanization (the "convert human verification into machine verification" task)

- **Moved into the suite:** H-4's runtime half. Two new unit tests pin the engine behaviour D4 rests on:
  `OrthoPlanes.AutoFarPlaneTracksViewRect` proves `FMinimalViewInfo::AutoCalculateOrthoPlanes` derives a
  DIFFERENT far plane for a 1920x1080 rect than for the 648x365 internal rect under the default
  `r.Ortho.AutoPlanes` configuration (defaults verified in `CameraStackTypes.cpp`), and
  `ExplicitPlanesIgnoreViewRect` proves authored planes survive untouched at both rects. If the engine ever
  changes that derivation, the first test fails and D4 can be revisited. H-4 itself can drop to a footnote.
- **Cannot move, with the reason:** the toolbox lanes run `-nullrhi` (verified in the lane logs'
  `LogInit: Command Line`), so `UGameViewportClient::Draw` never assembles a real view family in the suite —
  nothing that needs SetupViewFamily, a frame report, or an executed shader body can be asserted there.
  That covers H-5/6/7 (also inherently visual verdicts), H-8 (the contract test pins the signature; the
  BODY only ever executes under a real RHI), H-9, and the imagery halves of H-1/2/3. H-10's
  restore-the-prior-value half still needs an AngelScript CVar value getter — declined in review #2 as
  scope creep into CkCVar; unchanged. H-11 is editor-only by definition.
- **Made observable rather than provable:** the interleave fix. The judge scene's new capture tripwire
  means any human gym run (H-5 through H-9) now exercises the capture path continuously — a regression
  reads as the whole gym going soft — and a headless standalone boot greps as
  `Upscaler AddPasses` still appearing with the capture alive.

### Gate

Full suite on the rebuilt binary (`--build --config=Development --target=Editor --test --no-live
--discover-fresh`, log `Saved/Logs/BuildTest-AdversarialReview3.log`):
**1206 total / 1203 passed / 3 failed / 0 skipped / 0 contaminated / 5m 30s.**
1206 = the 1204 gate-of-record plus the two new OrthoPlanes tests, both green by name. The three failures
are the SAME pre-existing names as every prior gate (`Ck_AutoTest_Crowd_NavQueryFilter_ForceReplan`, both
`Ck_AutoTest_PathNetworkFollower_*`). Zero `never run`, zero lane aborts (grepped, not assumed).
`Ck_AutoTest_PixelArt_SubsystemContract` and `Ck_AutoTest_Camera_OrthoProjection` both green; the gym
scripts (including the new capture tripwire) compiled clean.

Runtime confirmation of the interleave fix, real RHI, standalone gym boots
(logs `Saved/Logs/PixelArtCaptureTripwire{,3,4}.log`):

- Run 3 (`ck.PixelArt.Enabled 1`): tripwire alive (`🟪 Capture tripwire alive` at frame 19), drive line
  `Active: viewport=1280x720 rendered=648x365 displayed=640x360 at (4,2) margin=4/2/3` and
  `Upscaler AddPasses ... stage=PrimaryToOutput` both present, ZERO `viewport=128x128` lines.
- Run 4 (`ck.PixelArt.Debug.ToggleLoop 60`): capture alive from frame 21; **61 `Active:` drive lines
  spanning frames 14→393** — ~59 full enable cycles executed WHILE the capture interleaved every frame,
  all with identical correct geometry and `applied == fraction`; final verdict
  `ToggleLoop PASS: r.ScreenPercentage returned to its pre-enable value — no residue`.

What this proves vs infers: every game-thread half (the drive, the lease, the geometry, zero
capture-driven writes) is runtime-proven across 60 cycles beside the live capture. The per-frame
UPSCALER registration during capture-alive frames is proven by code-reading plus the engine ordering
(the `AddPasses` breadcrumb is geometry-change-gated, so it logs once) — the one residual that only an
eyes-on gym run can fully close, and the tripwire makes any regression of it read as the whole gym going
soft.

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
  `CkPixelArtRenderer` category instead; no `CK_GENERATED_BODY` on the subsystem. House naming,
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

- **D-13 - `Get_IsEnabled()` returns `ECk_EnableDisable`, not `bool`.** PHASE_4's spec text says `bool`.
  The exemplar it also names (`UCkUsf_CelShadeSubsystem`, which the same phase says to mirror EXACTLY)
  returns the enum, and root CLAUDE.md prefers enums over bools. The enum wins on both counts.

- **D-14 - there is no `DoGet_EffectiveSettings` on the subsystem.** PHASE_4 step 7 asks for the
  CelShade-style CVar fold. The fold already exists, in the right place: `ck::pixel_art::Fold_Overrides`
  is applied by the RENDER module on the way OUT of the state registry (Phase 1). Folding again on the
  way in would make an override indistinguishable from a setting and leave it behind when the CVar goes
  back to -1 - which is the exact property the CelShade shape exists to preserve. The look half has no
  CVars, so a second fold would have nothing to do.

- **D-15 - `UCkPixelArt_Preset` mirrors the params FLAT rather than nesting one `_Params`.** PHASE_4
  step 3 asks for one nested field. An AngelScript `asset` block assigns reflected properties by name
  and cannot reach into a nested struct (no precedent for it anywhere in `Script/`), so a nested preset
  would have been authorable only in the editor - and the phase's own exemplar, `UCkUsf_CelShadePreset`,
  is flat for the same reason. `Get_AsParams()` packs them.

- **D-16 - `CK_DEFINE_CONSTRUCTORS(T)` with no essentials does not compile.** The macro expands to
  `T() = default;` plus `CK_DEFINE_CONSTRUCTOR(T, )`, and the zero-vararg form is a syntax error
  (C2760/C2351 at the macro line). Structs whose fields are all optional simply omit it - which is what
  `FCk_Usf_CelShade_Params` does. PHASE_4 step 2's "CK_DEFINE_CONSTRUCTORS with no essentials" is not a
  reachable shape.

- **D-17 - the presets live in `Script/CkPixelArt/`, not `Script/CkUsf/`.** PHASE_5 step 1 puts them
  under CkUsf. They are `UCkPixelArt_Preset` assets, so they belong with the module that owns the type;
  the LOOK asset (a `UCkUsf_LookDefinition`) does live in `Script/CkUsf/` as the phase doc says.

- **D-18 - `CkPixelArt_Log.h` lives under `Public/CkPixelArt/`, not at the module root.** The CkVisibleRange
  shape puts it at the root, which only resolves for modules that add `ModuleDirectory` to their public
  include paths (CkUsf does). Following the sibling `CkPixelArtRender` instead keeps both campaign modules
  on one convention and needs no build-file special case.

- **D-19 — the gym's PIE placard is unconditional, not conditional.** PHASE_6 step 1 asks the gym to print
  it "when it detects PIE + non-100 DPI-derived secondary fraction". AngelScript exposes no PIE predicate
  (`Gameplay::IsInEditor` does not exist — the compiler said so), and more to the point the condition that
  actually matters is not "is this PIE" but "did the engine settle on a secondary view fraction below 1",
  which only the renderer can see. It already logs that value the frame it changes. The gym therefore states
  the caveat unconditionally and points at that log line as the evidence, rather than guessing at the
  condition from the wrong side.

- **D-20 — the look normalizes luminance through Reinhard rather than saturating it.** Not in any phase
  doc; caught while reading the shader back. The look sits pre-tonemap, where scene colour is unbounded, so
  the first version's `saturate` before banding would have collapsed everything above 1 into the top band —
  a bright scene renders flat, and the failure would have been read as a palette problem in the H-8 rubric.
  Now normalized (`L/(1+L)`), banded there, and inverted on the way out with a max-tone cap, which is
  `HandDrawn.ush`'s approach and its stated reason. The palette stage still bounds its input, and that one
  IS a property of the stage: once a pixel is snapped to an authored palette there is no dynamic range left
  to preserve.

- **D-21 — the shader's view vector comes from `ScreenVectorFromScreenRect`, not `In.CameraVector`.** Also
  caught by reading rather than by a test: `CameraVector` is wired for SURFACE looks only and reads zero in
  a post-process one, which would have made every surface test as fully face-on and disabled the
  grazing-angle threshold scaling entirely — i.e. the ground-plane staircase this look is specifically
  built to avoid, silently, with the knobs for it present and inert. `CelShade.ush:464` uses the same helper
  for the same reason.

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

  **The Tab step is no longer necessary — the URL form was the problem, and it is solved.** A gym GameMode
  DOES take on the map URL, using the full AngelScript class path rather than the bare name:
  `?game=/Script/Angelscript.Ck_PixelArtGym_GameMode`. Verified in the perf run, which loaded straight into
  the pixel-art gym with no keyboard involved (`Saved/Logs/PixelArtPerfSweep.log:2085`). Every remaining
  visual item below can therefore be launched directly into its gym.

  **The two `r.Ortho.Debug.*` lines are only for spiking a DIFFERENT gym.** As of the camera fix above, the
  pixel-art gym enters orthographic on its own and `[P]` flips it — so anywhere a step below reads "launch the
  pixel-art gym, then force ortho", drop the forcing. The forcing is still required for H-1/H-2 precisely
  because those spike a foreign gym (Crowd, IskmRenderer) whose camera is perspective, and H-4 forces it
  deliberately to test the auto-planes behaviour.
- **[EDITOR-VERIFY] H-2 — 7a on geometry.** Same run as H-1, plus `ck.Usf.CelShade.Enabled 1`.
  Expected: CelShade's ink lines / halftone are chunky at texel scale, not native-res fine.
- **[EDITOR-VERIFY] H-3 — 7c PIE half.** Run the same scene in PIE at >100% OS DPI with the spike on.
  Expected: `CkPixelArtRenderer` logs a `Secondary view fraction is <1` line, and the image is softer than
  the standalone capture. No fix wanted — this is the evidence for "PIE lies" (standing risk 1).
- **[EDITOR-VERIFY] H-4 — 7b runtime.** With the spike on, compare `r.Ortho.AutoPlanes 1` against
  explicit near/far and confirm the auto far plane moves with the render resolution.
  **Mechanized (third review):** `CkTests.UnitTests.CkPixelArtRenderer.OrthoPlanes.*` now proves both
  halves against the live engine in every suite run. The editor comparison is optional corroboration only.

- **[EDITOR-VERIFY] H-5 — Phase 2 gate 6.G, the creep/stutter/smooth A/B.** **Prefer the pixel-art gym's own
  stations 5/6/7 now** — they are this A/B, pre-configured, on a camera that is orthographic on entry, and
  `Ck_GymPixelArt_TogglePan` drives the motion. Nothing needs forcing. The manual form below remains valid for
  scoring the A/B inside some other gym: standalone, gym map with visible geometry, `r.AntiAliasingMethod 0`,
  `r.Ortho.Debug.ForceAllCamerasToOrtho 1`, `r.Ortho.Debug.ForceOrthoWidth 2000`, `ck.PixelArt.Enabled 1`,
  `ck.PixelArt.Debug.LogState 1`, then `Ck_PixelArt_DebugPan 0.2`. Score three states, in this order:
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

- **[EDITOR-VERIFY] H-8 - Phase 5 outline rubric.** Gym map, standalone, station **PRESET: CRISP 16**.
  Score five things:
  1. Silhouettes are darkened and exactly ONE texel wide (use station **FILTER: NEAREST** to count).
  2. Creases render as a PAIR: the outward-facing texel brightened, the inward one darkened, one texel each.
     That is intended (RESEARCH_Technique section C.3 - it reads as a lit bevel, and is what "highlight the
     outward-facing edges, and darken the outlines" means when both sides of a fold are visible). The DEFECT
     to look for is two texels given the SAME treatment, which reads as one fat line; that is what section
     C.1's de-doubling gate exists to prevent, and this look relies on the pair being opposite instead.
     If the pair reads as mush rather than as a bevel at 360p, say so - the gate is a small change and this
     is the call that needs eyes on it.
  3. No doubled SILHOUETTES anywhere on the cube stack - an object's outer edge is one-sided by
     construction (only the nearer texel draws it), so two there is a real defect.
  4. No staircase of false outlines across the ground plane at grazing angle. This is the most likely
     defect and the reason `AngleZCutoff` / `AngleZScale` exist; if it appears, raise `AngleZScale`.
  5. In band-shift mode every outline colour is one the palette already contains. A grey or black line
     means the edge is being applied AFTER the palette snap instead of before.
  The **PRESET: SOFT RAMP** station is the A/B - flat edge colours and per-channel steps, which should
  read as toon shading rather than pixel art.
- **[EDITOR-VERIFY] H-9 - UMG overlay at native resolution.** Any gym station with the renderer on:
  the station placards and the cycler menu must stay crisp at native resolution above the chunky scene.
  Blurry UI means the upscale is happening after UI composition rather than before.
- **[EDITOR-VERIFY] H-10 - toggle residue, ten times.** From the gym, walk between **OFF (NATIVE)** and
  **RENDERER ONLY** ten times, then check `r.ScreenPercentage`, `r.AntiAliasingMethod` and
  `r.DynamicRes.OperationMode` are back at their pre-gym values. The machine half of this is already
  covered (`ck.PixelArt.Debug.ToggleLoop` 100x, Phase 1); this is the half that also covers the
  subsystem's CVar restore.

- **[EDITOR-VERIFY] H-11 — Blueprint surface (VALIDATION B2).** In the editor, make a throwaway Blueprint
  and place `[Ck][PixelArt] Get Pixel Art Subsystem` -> `Apply Preset` -> `Request Set Enabled`. Expected:
  all three nodes exist under the `[Ck][PixelArt]` display names and wire up. This is the only line in
  VALIDATION §B that cannot be answered from a headless run.

### Evidence on disk (gitignored, host project — copy out before cleaning `Saved/`)

| What | Path |
|---|---|
| Gate 5.G logs (3 viewport sizes) | `Saved/Logs/PixelArtSpike_{1920x1080,2560x1440,1367x768}.log` |
| 7a runs (spike+CelShade, CelShade only) | `Saved/Logs/PixelArtCheck_{geometry,celshade,celshade_nospike}.log` |
| Spike captures (menu-over-sky) | `Saved/Screenshots/WindowsEditor/ScreenShot0000{0,1,2,3}.png` |
| 7a A/B captures | `ScreenShot00004.png` (spike+CelShade) vs `ScreenShot00005.png` (CelShade only) |
| Look generation (A6) | `Saved/Logs/PixelArtGenerateLooks.log` (generator + validator lines at :3129-3150) |
| Perf sweep (three GPU numbers) | `Saved/Logs/PixelArtPerfSweep.log` (:2061-2107) |
| Gate of record | `Saved/Logs/Test-GateOfRecord.log` |

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
- 2026-08-20 (Opus executor, session 3): **PHASES 2-6.** Phase 2 (snap/margin/remainder) and Phase 3
  (CkCamera ortho) implemented and committed after a delta-zero full suite; Phase 4 (`CkPixelArt` module),
  Phase 5 (the `PixelArt` look, generated and validated) and Phase 6 (gym, docs, perf table) landed on top.

  One decision went to a Fable agent — the render margin across axes — and its ruling is recorded above
  with the one correction I had to make to the framing I gave it.

  Notes for whoever picks this up:
  - **A gym GameMode DOES take on the map URL**, with the full AngelScript class path:
    `?game=/Script/Angelscript.Ck_PixelArtGym_GameMode`. Phase 0 concluded the opposite from the bare-name
    form and left four visual items behind a keyboard step. They are all launchable now.
  - **Two real defects were caught by reading rather than by a test**, and both would have presented as
    "the look is wrong" rather than as a crash: the surface-only `In.CameraVector` (D-21) and the
    unbounded-input `saturate` (D-20). Nothing automated covers the shader BODY — the contract test checks
    its signature. That gap is what the H-8 rubric is for.
  - **A third was caught by the spec test**, which is the one that reads best: `Get_PreconditionReport()`
    cached a refusal while its own header comment promised it recomputed.
  - The still-open editor from the look-generation run hot-compiled the gym scripts and reported two
    AngelScript errors for free. Leaving an editor up while writing `.as` is a cheap feedback loop.

- 2026-08-20 (Fable, session 4): **third adversarial review** — the rendered result end to end. Findings
  and dispositions in the "Third adversarial review" section above. Fixed: the capture-family interleave
  (review #2's open uncertainty, REFUTED against the engine source), the crease-across-occlusion fringe in
  `PixelArt.ush`, the inert DPI divide-out, the stylize-helper duplication, and the never-force-added
  module `Claude.md`s. Added: two OrthoPlanes pinning unit tests (mechanizing H-4) and the gym's capture
  tripwire. Gate: 1206/1203/3, same three pre-existing names, zero never-run. Runtime: 60 toggle cycles
  beside a live capture, zero residue, zero capture-driven writes. Operational note for the next session:
  AngelScript does NOT concatenate adjacent string literals (a C++ habit) — the compiler error names only
  the line/column, and a failed gym-script compile keeps OLD bytecode for the whole boot, so the run looks
  mysteriously stale rather than broken.

## Final state — what is committed where, and what is not

**Nothing is pushed. No submodule pointer was bumped.** The superproject is still at `133f8f9` with
nothing staged; its three dirty entries (`CkPlugins.uproject`, `Config/DefaultGameplayTags.ini`, and the
gitlink drift) are the pre-existing dirt recorded in the baseline, untouched. Publishing is the
maintainer's `/ck-ship-dev`.

### CkFoundation — branch `feature/pixel-art-renderer`

| SHA | What |
|---|---|
| _(docs commit)_ | docs(pixelart): third adversarial review + the two module Claude.mds force-added |
| `f68c60886` | fix(CkPixelArtRenderer): capture-interleave guard; settled secondary-fraction prediction |
| `8bfbe0124` | fix(CkUsf): depth-gate the PixelArt crease detector; reuse the stylize helpers |
| `1f47b9e8f` | docs(pixelart): campaign complete through Phase 6 |
| `1fab414a6` | docs: tier-table rows + two stale claims corrected |
| `71582ac5a` | feat(CkUsf): the PixelArt look |
| `7c5a76da2` | feat(CkPixelArt): the game-facing half |
| `0d518cd0a` | docs(pixelart): Phase 2/3 verdicts + the margin adjudication |
| `43746c7ee` | feat(CkCamera): orthographic projection support |
| `54b4bfe5b` | feat(CkPixelArtRender): camera texel snap + remainder |
| `5a152a19e` `1760226ae` `c4b7463e0` `c5857bcc9` | Phases 0-1 (earlier session) |

### CkTests — branch `feature/pixel-art-renderer` (local only, no upstream)

| SHA | What |
|---|---|
| `ca3400e2` | feat(CkTests): capture tripwire in the pixel-art gym judge scene |
| `92327f29` | test(CkPixelArtRenderer): pin the ortho auto-plane resolution dependence |
| `ea4dd870` | feat(CkTests): the Pixel Art gym |
| `650c60cf` | test(CkUsf): the look's asset-to-HLSL contract |
| `bbc54ee2` | test(CkPixelArt): the subsystem contract |
| `b5869fa0` | test(CkCamera): orthographic projection end to end |
| `365db8f2` | test(CkPixelArtRender): the camera snap arithmetic |
| `3c08fdd1` | Phase 1 specs (earlier session) |

### Known limitations (= the supported-feature matrix)

Stated in full in [../../../Source/CkPixelArtRender/Claude.md](../../../Source/CkPixelArtRender/Claude.md);
the short version, because these are the ones that will generate bug reports:

- **Anti-aliasing must be None or FXAA.** TSR and TAA disable the upscale slot this renderer occupies.
- **Dynamic resolution must be off.**
- **PIE is a preview.** Above 100% OS DPI the engine adds a second resample after ours.
- **Orthographic only** for the snap. Under perspective the margin fold still applies but pixels creep,
  and no snap can fix that — one world-space snap displaces near and far geometry differently.
- **Zoom shimmers while it changes** and settles when it stops. Documented technique limit.
- **Split screen, stereo, scene captures: untested or out of scope.**
- **`Renderer/Private` include** — recompile-coupled to the pinned engine fork; budget for it on upgrade.

### What is the maintainer's to decide

- **The human queue H-1 … H-11.** Everything visual, plus the Blueprint-surface line. All of them are now
  launchable headlessly into the right gym — the URL form that Phase 0 could not make work is
  `?game=/Script/Angelscript.Ck_PixelArtGym_GameMode`.
- **Three things flagged for cheap veto:** the debug-only surface in a runtime module (D-7 ToggleLoop,
  plus DebugPan and PerfSweep), `ensureMsgf` in the CkCore-less module (D-5), and `TOptional` in a
  reflected request struct (D-12, open adjudication A1).
- **VALIDATION A9 (Shipping-config compile) is UNRUN**, not assumed. Only Development/Editor was built.
- **Phase 7 backlog** is untouched: god rays, cloud shadows, per-object snap, stencil point-light,
  BusterBlock adoption.
