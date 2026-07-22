# CkPoi v2 refactor — PROGRESS.md (living log)

## Current state

**As of 2026-07-22:** Gates 1–3 ✅. Gate 3 (`CkPoi` meta-feature rewrite) complete: CkPoi owns no
fragment machinery (FTag_Poi + EntityTag/Label composition), all consumers rewired
(Compass/Minimap/EcsDebugger inspector/MapDebugger), 25 AS test/gym files rewritten, full sweep
green vs baseline (Poi 44/44, Compass 13/13, Minimap 14/15 same pre-existing red, VisibleRange 4/4).
Committed locally; NOTHING pushed. Gate 4 (Compass+Minimap semantic rewire) not started.
**Baseline for Gate 4:** the Gate 3 exit numbers above (`Exit_Gate3_*.log`).
**Next action:** Start Gate 4 — write `Plan/Gate_04_CompassMinimap.md` first. Outstanding human
items: BP-node `[EDITOR-VERIFY]` checklists from Gates 2 AND 3 (see each gate's exit criteria).
**Blocked on:** nothing.

## Decision log

| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-07-21 | Parent→child VisibleRange cascade lives in `CkPoiDisplayDefinition`, not `CkVisibleRange` | Keeps `CkVisibleRange` fully standalone/reusable outside Poi; see PROMPT.md decision #5 | If a future consumer other than Poi ever needs the same cascade pattern — extract it then, don't pre-build |
| 2026-07-21 | Cadence retrofit of existing hand-rolled accumulators (Compass/Minimap/Crowd/Goap) is a non-goal of this campaign | Scope control — this campaign is about the Poi shape, not a codebase-wide cadence sweep | Separate follow-up task, tracked in REFACTOR_MultiProjectorPoi.md's Follow-ups section |
| 2026-07-21 | AutoTests for CkFoundation framework features live under `Plugins/CkTests/Script/<Feature>/`, NOT `Plugins/CkFoundation/Script/<Feature>/` | House convention (matches `CkAttribute`'s own tests) — got this wrong once mid-Gate-1, causing a real AS duplicate-class stall; recorded here so it isn't relearned the hard way again | n/a — settled |
| 2026-07-21 | Forks executing a build/test cycle must run the toolbox invocation SYNCHRONOUSLY (no `run_in_background` inside the fork's own tool call) | A fork's background process isn't reliably tracked once the fork's own turn ends — twice in Gate 1 a fork reported "waiting for the background build" and then its task showed `completed` with nothing actually verified. The toolbox process itself does keep running (confirmed via `--build-status`), but the fork can't observe or report on it after its turn ends, and the dispatcher gets no notification for it either | Apply to every future gate's fork prompts |

## Dated entries (append-only, newest first)

### 2026-07-22 — Gate 3 closed: CkPoi is a meta-feature; consumers rewired; zero regressions

- **Session shape:** Fable wrote the gate contract + delegation prompts, line-audited all agent
  output, ran the build/test pipeline, fixed the two pipeline incidents below, and closed the docs.
  Three Opus agents implemented: (1) CkPoi rewrite + Compass/Minimap rewire, (2) debugger rewires,
  (3) 25 AS test/gym rewrites. All three outputs were high quality; Fable's audit fixes were
  cosmetic (CRLF, em-dash literals, map-row name fallback, one dangling comment).
- **Confirmed (fresh toolbox runs, `Exit_Gate3_*.log`):** build green (attempt 3); Poi 44/44 vs
  baseline 44/44 — the suite reshaped 1:1 (7 CkPoi tests: 5 rewritten in place, 1 renamed
  `SetStateTags_ReplacesAll`→`StateTags_ViaEntityTag` (no replace-all verb on EntityTag; re-scoped
  to multi-tag independence), 1 untouched pair already new-API-clean); Compass 13/13 (=);
  Minimap 14/15 (= same pre-existing `Minimap_Add_CreatesChild` red — Gate 4 fix);
  VisibleRange 4/4 (=). Residue + stock-ensure greps zero.
- **FogOfWar open item RESOLVED:** NOT a CkPoi consumer (comment-only reference, now reworded) —
  confirmed by accessor-level sweep; the Open-items row below is closed.
- **LNK lesson (extends the Gate 2 accessor-sweep lesson):** consumers of a module whose PUBLIC
  headers embed record-of-children machinery need a direct `CkRecord` dep —
  `FCk_Handle_EntityExtension` is DECLARED in CkRecord (`CkRecord_Fragment_Data.h:14`), not in
  CkEntityExtension (first fix attempt targeted the wrong module). CkGoap's Build.cs comment
  documents the same trap from the other side (keep CkRecord private).
- **AS test-rename wedge (new failure mode, recorded in Gate_03 doc):** renaming an AutoTest class
  deadlocks the AS compile via the committed generated `CkTestsAssets.as` + the orphaned placed map
  actor (populator can't remove an actor whose class won't load; the row then passes VACUOUSLY from
  cached discovery). Recovery: prune stale generated blocks + `git rm` the orphan external-actor
  package + `--discover-fresh`. This is the sharp edge behind CkTests CLAUDE's "prefer stable names".
- **Inferred (unconfirmed, human-only):** BP node rendering for the reshaped `[Ck][Poi]` surface —
  `[EDITOR-VERIFY]` checklist in Gate_03 exit criteria.

### 2026-07-21 (evening) — Gate 2 closed: CkPoiDisplayDefinition complete; Gate 1 Fable-audited

- **Session shape (per the campaign's tiering directive):** Fable did the Gate-1 audit, gate
  plan, delegation prompts, all audits, and the build/test/commit pipeline; two Opus agents did
  the C++ implementation and the AS test authoring. Both agent outputs were line-audited by
  Fable before use; both were high quality (one spec-vs-code fork each, both resolved correctly
  in favor of real code).
- **Gate 1 audit verdict: no blockers.** All Gate-1 files read in full against the design
  contract; `FCk_Chrono` semantics verified in `CkChrono.cpp:34-41` (Tick latches Done →
  Complete-seed + `ShouldRun` Reset behavior correct); green evidence re-read from the actual
  log. Three non-blocking notes: `Update_Distance` plain-setter deviation is deliberate+documented;
  `ck::cadence::ShouldRun` discards completion-tick overshoot (matches the hand-rolled precedent
  it replaced); `CadenceGatesUpdates` has a theoretical flake window if a single frame hitches
  >0.5s inside its 0.1s short-wait.
- **Confirmed (fresh toolbox runs, this session):** build green; new suite
  `Total: 4, Passed: 4, Failed: 0` (`BuildTest_Gate2_PoiDisplayDefinition.log`); exit sweep
  Poi 44/44 / Compass 13/13 / Minimap 14/15 vs baseline 40/40 / 13/13 / 14/15 — zero
  regressions (`Baseline_Gate2_*.log`, `Exit_Gate2_*.log`).
- **Pre-existing red found while baselining (NOT this campaign's):**
  `Ck_AutoTest_Minimap_Add_CreatesChild` — reproducible in isolation on the unmodified test env;
  AS "Specified function is not compatible with delegate function" at its
  `FCk_Lambda_InHandle(this, n"OnEachMinimap")` bind (the const-ref-signature trap) + a
  `[Server] ! Has(InHandle)` ensure; kills the editor (toolbox respawns). File untouched since
  `f87dbb1`. **Follow-up for Gate 4** (Minimap rewire) — fix the delegate signature there.
- **Two LNK2019 rounds during the gate — the pre-plan "zero consumer edits" claim was wrong.**
  Name-grep missed two consumers of the moved types because both reach them through `auto`/
  template deduction (type name never literal): `CkCompassUI_MarkerWidget.cpp::DoResolveDisplay`
  (PDA `Get_Icon`/`Get_Tint`/`Get_SizeHint`/`LoadSynchronous`) and
  `CkInspector_Poi.cpp:90` (enum `StaticEnum` via `ck::Format_UE`). Fixed by direct deps in
  `CkCompass.Build.cs` and `CkEcsDebugger.Build.cs` (CkGameplayDebugger repo now in this
  campaign's diff). **Process lesson: for type moves, sweep by ACCESSOR names
  (`Get_OffscreenPolicy`/`Get_DisplayAsset`), not type names.**
- **Design refinement recorded in the gate doc:** the parent→child cascade is a native
  cross-module signal bind (`ck::UUtils_Signal_OnVisibleRange_HiddenChanged::Bind<&...>` — the
  CkTween↔CkTimer precedent, `CkTween_Utils.cpp:769`), NOT a polling processor; the module ships
  zero processors. Bind-once guarded by `FTag_PoiDisplayDefinition_CascadeBound`; Create seeds
  `FTag_PoiDisplayDefinition_ParentHidden` on children born under an already-hidden owner (the
  design-doc gotcha, proven by the CreateUnderHiddenParentSeedsVote discriminator test).
  Empirically confirmed: binding on an owner that composes VisibleRange LATER (or never) is safe.
- **Inferred (unconfirmed, human-only):** BP node rendering for the new surface —
  `[EDITOR-VERIFY]` checklist in Gate_02 exit criteria.

### 2026-07-21 — Gate 1 closed: CkVisibleRange complete, all tests green
- **Two real bugs found and fixed** (both by direct inspection after forks produced incomplete/buggy work, not by the forks themselves):
  1. `CkVisibleRange_Processor.cpp` — `Broadcast` calls passed the payload unwrapped instead of via `ck::MakePayload(...)` (compile error, C2672). Fixed by matching `CkPoi_Processor.cpp:96-97`'s pattern.
  2. `CkVisibleRange_Utils.cpp::Add` — seeded a fresh/zero `FCk_Chrono` instead of an already-`.Complete()`d one, meaning a newly-composed entity would show the default (visible) state for up to one full `_UpdateInterval` before ever being evaluated. Matches the exact problem Compass/Minimap solved via their Setup processor's `_TimeSinceUpdate = TNumericLimits<double>::Max()` priming — `CkVisibleRange` had no equivalent until this fix.
- **Process lesson (see decision log):** two forks in a row kicked off the toolbox build/test via their own backgrounded bash call, then ended their turn believing they'd be notified later — they weren't, and both runs were left orphaned/unverified. Recovered both times by checking `--build-status` and the raw logs directly rather than trusting the fork's self-report.
- **A third, self-inflicted issue:** while writing tests myself (after the two fork attempts), placed 3 duplicate-named test files under the WRONG plugin (`CkFoundation/Script/CkVisibleRange/` instead of `CkTests/Script/CkVisibleRange/`) on top of a set the second fork had actually already written correctly there — caused a genuine AngelScript duplicate-class compile error and a stuck infinite hot-reload retry loop (~40 min). Diagnosed via `Saved/Logs/CkPlugins.log`'s live AS error stream (user flagged the stall directly, which prompted the investigation). Fixed by deleting the wrong-location duplicates; the already-running stalled editor auto-recovered via hot-reload once the collision was gone — no restart needed.
- **The second fork's actual test authorship was good** — kept all three of its test files (`CkAutoTest_VisibleRange_{OwnRangeBoundaryCrossing,ExplicitOverrideIsIndependentVote,CadenceGatesUpdates}.as`) after one precision fix to `CadenceGatesUpdates.as`: its first assertion didn't actually discriminate the `.Complete()`-seed bug it claimed to test (a buggy fresh-Chrono and a correctly-evaluated in-range default both read as "visible" — the assertion would pass either way). Fixed by moving the distance out-of-range to before the first tick, so the correct answer (hidden) diverges from the buggy default (visible).
- **Confirmed (fresh toolbox run, this session, `Saved/Logs/BuildTest_VisibleRange.log`):** `=== Test summary === Total: 4, Passed: 4, Failed: 0` — the 3 new `Ck_AutoTest_VisibleRange_*` tests plus `Ck_AutoTest_Minimap_MaxVisibleRange_Culls` (a pre-existing unrelated test matching the `VisibleRange` filter substring, confirming no incidental regression elsewhere). `CadenceGatesUpdates`'s ~1.0s real duration matches its ~0.1s+0.6s Timer-wait budget, confirming the timing assertions are actually exercised, not vacuously passing.
- **Inferred, not directly reconfirmed:** the final green run's binary reflects both C++ fixes (the Broadcast fix was built and verified separately before the Chrono fix; the Chrono fix's own dedicated verification is this same green run, since AS test execution requires the up-to-date compiled module).

### 2026-07-21 — campaign chartered
- Design finalized across conversation: `REFACTOR_MultiProjectorPoi.md` (all open calls resolved).
- ck-methodology invoked; PROMPT.md/PLAN.md/PROGRESS.md/Gate_01 written before any code.
- Confirmed (this session, prior research): `CkProjectile_Utils.cpp:16-32` meta-feature pattern,
  `CkEntityTag_Fragment.cpp:55` persistence handler, `TProcessorBase::_TickRate`
  (`CkProcessor.h:81,186-199`), `FCk_Chrono` API (`CkChrono.h`).
- Not yet started: any code in `CkVisibleRange`, `CkPoiDisplayDefinition`, or `CkPoi`.

## Open items

| Item | Status | Next step |
|---|---|---|
| Baseline test counts for CkPoi/CkCompass/CkMinimap | Not captured | Capture via toolbox at Gate 3 entry |
| Whether `CkFogOfWar` touches `CkPoi`'s old fragment directly | RESOLVED 2026-07-22: NO (comment-only reference) | n/a — closed at Gate 3 entry sweep |
