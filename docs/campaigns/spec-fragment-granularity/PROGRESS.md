# PROGRESS — spec-fragment-granularity

Living doc. Newest entries on top within each section. See PROMPT.md for phases/rules.

## State

- **Phase:** P1–P4 + P6 audit COMPLETE and committed; P5 (monolith splits) STAGED with designs
  below — each split is its own gated unit for a fresh session.

## P5 — staged split designs (2026-08-06)

Priority order, each with its own build+suite gate; census data in the design spec §1.3:

1. **VatProxy** (13 members): extract `_PrevClipIndex/_PrevClipStartTime/_PrevPlayRate/
   _PrevLoopMode/_TransitionStartTime/_TransitionDuration` → `FFragment_VatProxy_Transition`
   (crossfade-source block; Transform `_Previous` precedent). Present iff a crossfade is in
   flight → its presence can replace any is-transitioning flag; check `_FinishedDispatched`.
2. **Minimap/Compass** (`_ScratchEntries/_ScratchPoiEntities/_ScratchParallelSlots` ×2 modules):
   scratch buffers are a DELIBERATE per-tick-reuse perf choice — do NOT naively move to locals;
   extract `FFragment_Minimap_Scratch` / `FFragment_Compass_Scratch` (keeps reuse, unclutters
   state) or measure and drop. Read the parallel-slot usage first (TParallelProcessor coupling).
3. **AudioTrack Current** (14 members): split the 6 delegate handles → `FFragment_AudioTrack_
   ComponentBindings` (bound/unbound with the component's lifetime); fade state
   (`_CurrentVolume/_TargetVolume/_FadeSpeed`) is a candidate `_Fade` fragment gated by
   `FTag_AudioTrack_IsFading`.
4. **Camera Current** (16): split read-cache config bools (7, set at compose) from per-frame
   intention/output (`_ViewInfo` etc.); the composed-profile cache is its own concern.
5. **Homing** (12): `_PreviousTargetLocation/_HasPreviousTargetLocation/_PreviouslyClosing/
   _MissNotified` → `_Tracking` fragment; the `_Has*` bool dissolves into fragment presence.
6. **VoiceTalker** (21 members, 5 concerns, 7 friend processors — LARGEST, do LAST, coordinate
   with the VoiceChat workstream): capture chain / loopback playout / fairness / clocks / tunables
   split per its own Current-comment inventory. The "runtime copies of tunable params" block is
   the doctrine-priority extraction (`FFragment_VoiceTalker_Tunables` — requests mutate ONLY it).

Rules for every split: keep the Has/Cast anchor stable (P6 rule), update friend lists, per-split
`[EDITOR-VERIFY]` if any debugger inspector reads the moved members (CkInspector_Audio reads
`FFragment_AudioTrack_Current.Get_State` — splitting `_State` out would break it; check each).
- **Baseline (2026-08-05):** superproject `3cf103f`; CkFoundation `7ebe720f2` (dev); CkTests
  `0ea0d6a` (dev); CkGameplayDebugger `77aff95` (DETACHED HEAD — resolve before committing there).
  Superproject shows pre-existing modified submodule pointers for CkFoundation+CkTests (not ours).
  CkFoundation untracked `Tools/`, `docs/digests/` — NOT ours, never stage.
  Toolbox config: **Development** (last-built; DebugGame refused as config-flip).
  Baseline build+`--test-pattern Timer` run: PENDING (fill in below).
- **Baseline test counts (2026-08-05):** `--test-pattern ck.timer` → **25/25 passed, 0 failed**,
  55s, no AS errors (Saved/Logs/BaselineTest2.log). Baseline build: green (BaselineBuildTest.log).
  NOTE: first baseline attempt hit `AS_COMPILE_FAILED` from a stale orphan
  `Script/Generated/utils_web_umg.as` (module no longer exists); the failing boot itself cleaned
  it. Re-run was clean. Bare pattern `Timer` also drags in engine `BuildPatchServices.*ProcessTimer`
  tests that never run and abort lanes — always use `ck.timer`.

## P1 — Timer pilot plan (frozen before edits)

Renames: `FCk_Fragment_Timer_ParamsData`→`FCk_Timer_Spec`,
`FCk_Fragment_MultipleTimer_ParamsData`→`FCk_MultipleTimer_Spec` (UFUNCTION param names and
UPROPERTY member names — e.g. `_TimerParams` — unchanged by rule).

Fragment restructure (CkTimer_Fragment.h): alias dies; `FFragment_Timer_Current`→`FFragment_Timer`
(chrono, primary state); NEW residue `FFragment_Timer_Params{ ECk_Timer_Behavior _Behavior }`.

Behavior changes (all deliberate, each needs a test eye):
1. Request handlers (Reset/Complete/Jump/Consume) branch on `FTag_Timer_Countdown` instead of
   stale Spec `_CountDirection` — FIXES live divergence bug (doc'd in design spec §1.2).
2. `Has()`/`Cast()` anchor: `{Current, Params}` → `FFragment_Timer` alone.
3. `MakeStatIdFromParams(Params)` → `MakeStatId(Handle)` reading GameplayLabel (STATS-only path).
4. `AddOrReplace` now re-unpacks the spec fully: re-adds `FTag_Timer_NeedsSetup` (countdown chrono
   gets its Setup Complete()), sets/clears `FTag_Timer_Countdown` and `FTag_Timer_NeedsUpdate` per
   spec. Old code left stale direction/run-state tags and skipped Setup — latent countdown bug.
5. Setup processor reads the countdown TAG (no Spec/Params dependency at all).

Consumer sweep list (C++): CkVfxCue_EntityScript.cpp, CkCue_EntityScript.cpp,
CkAudioCue_EntityScript.cpp, CkTween_Utils.cpp, CkSmTask_Delay.cpp, CkSmCondition_Timer.cpp,
CkTests/Net/CkAutoTest_NetSubject_EntityScript.cpp, CkTests/UnitTests/CkArchetypeTyped.spec.cpp
(direct fragment emplaces!), CkGameplayDebugger CkEcsDebugger_FeatureFlags.cpp:79
(RegisterFlag<FFragment_Timer_Params> → switch to FFragment_Timer), comment at
CkEcs/Handle/CkDebugCallstack_Macros.h:59.

AS sweep (textual): 1 site CkFoundation `Script/CkUtils_Timer.as`; ~90 files CkTests Script;
2 sites superproject Script. Pattern is plain `FCk_Fragment_Timer_ParamsData(` construction —
pure rename is safe. `Script/Generated/utils_timer.as` regenerates.

CoreRedirects (before any editor/test run):
`/Script/CkTimer.Ck_Fragment_Timer_ParamsData`→`Ck_Timer_Spec`,
`.Ck_Fragment_MultipleTimer_ParamsData`→`Ck_MultipleTimer_Spec`.
Asset carrying the FName: `Plugins/CkFoundation/Content/CkTimer/Utils_CkTimer_FL.uasset`
(BP function library) — `[EDITOR-VERIFY]` recompile+resave after campaign.

## P6 — identity-anchor audit (2026-08-06)

30 `CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE` sites key on a `_Params` fragment (list captured via
grep; includes Params-only anchors: 2dGridCell/Object, AnimAsset, AnimPlan, CameraLayer,
CameraShake, Camera, CrowdAgent, EntityCollection, EntityExtension, IsmProxy, MontagePlayer,
ObjectiveOwner, Spline, TransformInterpolation, VatProxy, Velocity). **All anchors remain VALID
after P1–P4:** no feature's Params fragment was removed — Timer's anchor was migrated to
`FFragment_Timer` in the same change that dissolved its Params wholesale storage; AudioTrack and
Probe keep residue `_Params` structs present on every feature entity. RULE going forward (also
design spec §4.5): any refactor that would remove or make conditional a feature's `_Params`
fragment MUST move that feature's Has/Cast anchor in the same change, with a membership test.

## Log

- 2026-08-06: P4 CODE COMPLETE (gate in flight — build + full suite vs the now-established
  999-passed reference incl. 2 known pre-existing PathNetwork reds):
  * AudioTrack: alias → 8-field residue (`_TrackName` stored RESOLVED); NEW
    `FFragment_AudioTrack_PendingSetup` (ScriptAsset + 3 library soft-ptrs) consumed+REMOVED by
    Setup on all three exits; Create unpacks; Setup view/signature updated.
  * Probe: alias → 7-field residue. **DESIGN REVISION vs frozen plan:** `_MotionQuality` RETAINED
    (the LinearCast tag is derived from quality AND non-static type — lossy for the
    Static+LinearCast authoring combo, and `Get_MotionQuality` is public API). Dissolved:
    `_StartingState` (already request-driven at Add), `_PersistContacts` (tag now set at Add,
    removed from Setup). Dead Params view members dropped: `FProcessor_Probe_UpdateTransform`,
    `FProcessor_Probe_EndPlay` (bodies verified Params-free; the `EnsureStaticNotMoved_DEBUG`
    processor KEEPS Params — it reads MotionType in its ensure).
  * Tween: `FProcessor_Tween_HandleYoyoDelays` dead Params view member dropped.
  * Transform: dead `using FFragment_Transform_Params` alias deleted (zero references verified).
  * Module docs: fragment-shape sections appended to CkAudio/CLAUDE.md + CkSpatialQuery/CLAUDE.md.
  * External-reader audits: no reads of any dropped getter in CkGameplayDebugger/CkTests; the
    debugger's `RegisterFlag<FFragment_AudioTrack_Params>` / `<FFragment_Probe_Params>` markers
    still valid (residue fragments remain on every feature entity).

- 2026-08-06: P3 RED CLASSIFICATION COMPLETE (A/B: all four repos stashed to P1 state, rebuilt,
  reran the failing patterns, popped, rebuilding at P3):
  * 2× `PathNetworkFollower_{ProjectsRibbonWaypoint,DesiredNavmeshClearance}` — **PRE-EXISTING**
    (CONFIRMED: fail identically on pre-P3 binaries). Both fixture-geometry assertions on
    `utils_nav::Try_ProjectOntoNavmesh`; last touched by sibling fix `4a48849` (08-04) for exactly
    this comparison logic. NOT ours to fix mid-campaign; left for the owning workstream.
  * `UsfOutline_VatShadowCustomData` — **ENVIRONMENT** (CONFIRMED mechanism: error-severity
    `LogZenServiceInstance: Unable to reach ... [::1]:8558` landing inside the test window fails
    the test; Zen answers IPv4 :8558 fine. The flap PREDATES the campaign — 1 hit in the pristine
    baseline log, 1 in green P1 gate (landed outside test windows), 0 in the three latest runs.
    The pre-P3 "pass" in the A/B coincided with the flap stopping, not with the code difference.)
  * `StateMachine_DivergenceFirstBranch` — **CONTENTION FLAKE** (passes solo; sibling session's
    BusterBlock editors shared the machine during the full gate).
  * `Angelscript.CppTests.AngelscriptCodeCoverage.IntegrationTest` — **OUT-OF-SCOPE** (engine-fork
    C++ test with zero Ck surface; the unfiltered `--test` run pulls in engine groups beyond the
    ~873-test house suite). INFERRED pre-existing, not A/B'd — accepted risk, zero rename overlap.
  Effective P3 house-suite result: **997/999 with both reds confirmed pre-existing.** Rebuild at
  P3 + UsfOutline/ck.timer confirmation runs in flight; commit P3 on green.

- 2026-08-06: P3 GATE (detached run, P3Gate3.log): **1004 total / 999 passed / 5 failed / 0
  contaminated / no AS errors / 4m31s.** The 5: PathNetworkFollower_ProjectsRibbonWaypoint...,
  PathNetworkFollower_DesiredNavmeshClearance... (both fixture/navmesh messages),
  UsfOutline_VatShadowCustomData (GPU), StateMachine_DivergenceFirstBranch (timing),
  Angelscript.CppTests.AngelscriptCodeCoverage.IntegrationTest (engine fork). None rename-shaped;
  all load-sensitive categories; the SIBLING session's BusterBlock editor competed for the machine
  during the whole run. Classification in progress: sequential detached re-runs of exactly those
  4 patterns on identical binaries. Flake-green → P3 accepted; still-red → stash-classify against
  pre-P3 `fec11c20e`.

- 2026-08-06: P3 gate attempt 2 POST-MORTEM: the harness background-command timeout (600000ms hard
  cap) killed the entire toolbox process tree ~10min in — 20s into the test phase (log froze
  22:31:14, no completion notification, editor lock free, wrapper gone). NOT a code failure:
  the renamed tree built clean, editors booted with AS compiling (no AS_COMPILE_FAILED), 3 lanes
  enumerated ~334 tests each and were streaming green. Meanwhile a SIBLING session is running
  BusterBlock tests on this machine (their toolbox, started 00:33). Re-run strategy: persistent
  monitor waits for the sibling to exit, then launches the gate DETACHED via Start-Process
  (immune to wrapper timeouts), `--test` only (binaries already current — no source edits since
  the completed build), and watches P3Gate3.log for summary/AS-fail/build-fail/died-silent.
  Lesson filed for the build-test skill.

- 2026-08-05: P3 gate attempt 1 failed in UHT (4s): engine-name collision — existing DataAsset
  `UCk_2dGridSystem_Spec` (CkGrid authoring layer) vs renamed `FCk_2dGridSystem_Spec` (UE strips
  U/F prefixes). ONLY collision in all trees (verified by class scan). Resolution: the fragment
  Spec rename stays uniform; the DataAsset became `UCk_2dGridSystem_AuthoringSpec` (11 files;
  the AS test already used "AuthoringSpec" vocabulary; zero serialized instances in any Content/
  — ClassRedirect added anyway as downstream insurance). Gate relaunched.

- 2026-08-05: P1 GATE GREEN: build ok, `ck.timer` 25/25 (baseline 25/25, zero delta), no AS
  errors. Committed: CkFoundation `fec11c20e` (code) + `75d17b349` (docs), CkTests `2df528c`
  (146 files), CkGameplayDebugger `618ee8c` (reattached from detached HEAD to `dev` first —
  branch pointed at same commit). NOT pushed. Superproject Script had no Timer refs (its 2
  ParamsData sites are other features → P3). `Config/DefaultGameplayTags.ini` gained 2 VoiceChat
  tags from the editor test boots ("Added via code") — NOT ours, left untouched.
- 2026-08-05: P3 APPLIED: apply-rename.sh swept 954 files (121 renames). Post-sweep stragglers
  all benign: 9 AttributeEditor files carry only `*ParamsDataCustomization` CLASS names
  (deliberately preserved, cosmetic); 1 stale Goap comment fixed to `FCk_Goap_Planner_Spec`.
  121 StructRedirects inserted into DefaultCkFoundation.ini (177 total now). Full gate
  (build + ENTIRE suite) launched. **Known gap: no full-suite baseline was captured** — if reds
  appear, classify by re-running the failing names against pre-P3 `fec11c20e` before blaming the
  sweep. Tooling lesson: `rg` is a harness shim absent in child shells — apply-rename.sh uses
  plain grep; scripts written by tooling need `sed -i 's/\r$//'` before bash runs them.

- 2026-08-05: P1 CODE COMPLETE (gate pending): CkTimer module converted (Fragment_Data/Fragment/
  Processor/Utils), 152 files swept for the two Spec renames (CkFoundation Source+Script, CkTests,
  superproject Script), fragment-name consumers updated (ArchetypeTyped.spec.cpp, GameplayDebugger
  FeatureFlags→`FFragment_Timer`, CkDebugCallstack_Macros.h comment), 2 StructRedirects appended
  to DefaultCkFoundation.ini:363-364, CkTimer/Claude.md updated + new "Fragment shape" section.
  Gate launched: build + `ck.timer` (baseline 25/25).
- 2026-08-05: P2 DONE: root CLAUDE.md (lingo row, two-tier table rewritten, canonical example →
  FCk_Timer_Spec, new "Spec unpacking" paragraph), Source/CLAUDE.md (composition ritual step 3,
  VfxCue example note), CkEcs/Claude.md (processor templates → FFragment_MyFeature, feature-flag
  marker wording), Script/CLAUDE.md (2 examples), skills (macros add-a-new-x §3.1 rewritten,
  8 other files token/phrase-updated, ckecs-architecture-contract tier table rewritten),
  DECISIONS.md §111 appended.
- 2026-08-05: Campaign opened. Design spec finalized (F1=Spec, F7=Params kept, F2–F6 = standing
  recs accepted via blanket mandate). Baseline build+test launched (background).

## P4 designs (frozen pre-gate, from first-hand reads)

**AudioTrack shrink** (`CkAudioTrack_Fragment.h:26` alias → residue):
- Residue `FFragment_AudioTrack_Params`: `{_TrackName, _Sound, _Priority, _OverrideBehavior,
  _LoopBehavior, _Volume, _DefaultFadeInTime, _DefaultFadeOutTime}` — all (b)/(c) per audit.
- NEW transient `FFragment_AudioTrack_PendingSetup`: `{_ScriptAsset, _LibraryAttenuationSettings,
  _LibraryConcurrencySettings, _LibrarySoundClassSettings}` — construction-only fields that must
  survive to the DEFERRED Setup processor; Setup consumes then REMOVES it (Dialog PendingQueries
  precedent). `FTag_AudioTrack_NeedsSetup` kept as the lifecycle marker (other views exclude it).
- `Create()` (`CkAudioTrack_Utils.cpp:18-43`) unpacks spec → residue + PendingSetup.
- Setup reads dropped fields from PendingSetup (`CkAudioTrack_Processor.cpp:78-83,116-120,135-136`).
- Has/Cast anchor unchanged `{Params, Current}` (`Utils.cpp:46-47`).

**Probe shrink** (`CkProbe_Fragment.h:38` alias → residue):
- Residue `FFragment_Probe_Params`: matching half `{_ProbeName, _ResponsePolicy, _Filter,
  _ContextOverlapPolicy, _SurfaceInfo}` + `_MotionType` (public 3-valued getter `Utils.cpp:156-160`
  + Setup's Static-tag derivation + ensure diagnostics — tag only encodes one bit).
- Dissolved: `_MotionQuality` → `Get_MotionQuality` reads `FTag_Probe_LinearCast` (set at Add,
  `Utils.cpp:60-61`); `_StartingState` → already consumed at Add via `Request_EnableDisable`
  (`Utils.cpp:72`); `_PersistContacts` → tag set at ADD (moves out of Setup `Processor.cpp:476`).
- Dead Params view members dropped: `FProcessor_Probe_UpdateTransform` (`Processor.h:192`),
  `FProcessor_Probe_EndPlay` (`Processor.h:436`).

**Also in P4:** `FProcessor_Tween_HandleYoyoDelays` dead Params view member (`CkTween_Processor.h:56`);
Transform's dead `FFragment_Transform_Params` alias + dead `FCk_Transform_Spec` struct? — NO:
struct stays (it's reflected API), only the never-used alias line dies (`CkTransform_Fragment.h:36`).

## Decisions / discards

- Historical campaign docs (voxelnav-port research, saveload PHASE_4B) that mention ParamsData are
  ARCHIVES — never sweep them.
- `FProcessor_Timer_Replicate` appears in FFragment_Timer_Current's friend list but no such
  processor exists — carried over as-is to FFragment_Timer (out of scope to prune friends).
