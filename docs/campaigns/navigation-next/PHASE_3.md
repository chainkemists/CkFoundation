# PHASE 3 — Path search, install through the existing seam, and shadow mode

> Freshness: authored 2026-08-31 in planning session 1. Status of record is
> [PROGRESS.md](PROGRESS.md); the designs of record are [FEATURE_MATRIX.md](FEATURE_MATRIX.md)
> F1.23–F1.30 + F1.40 + F1.41, [MIGRATION_SEAM.md](MIGRATION_SEAM.md) Steps 3–4, and
> `research/R4-ck-native-assets.md` §2 (CkAStar). Where this file and a design doc disagree, the
> design doc wins and the disagreement is a **STOP**.
>
> **Every code block in this file is illustrative — the executor refines it mechanically, does not
> redesign it.**

---

## 1. Goal

Make CkGroundNav an actual path provider: A* over the plate-portal graph on **CkAStar**, funnel
string-pulling over portal intervals, partial paths, the cost model, the neutral post-processing
chain, plan repair, install through the **existing** `MarkPathPending` / `InstallExternalPath` seam,
a fourth branch in CkCrowd's provider fork, failure parity, and shadow/A-B mode with divergence
metrics — landing **F1.23–F1.30**, **F1.40**, **F1.41**.

The install seam already exists and already carries two non-Recast providers. **Do not invent a
second one.**

---

## 2. Entry criteria

- [ ] **PHASE_2 closed in PROGRESS.md** with its A4 budgets filled in as measured numbers.
- [ ] Repo clean, on a `feature/` branch (root + CkFoundation + CkTests) — commands as PHASE_1 §2.
- [ ] **CkAStar still offers exactly what this phase builds on** (R4 §2 spot-checks; re-verify, do
      not trust the line numbers):
      ```powershell
      rg --no-ignore -n "AStarGraph|Neighbors|Heuristic|IsGoal" `
         "Plugins\CkFoundation\Source\CkAStar\Public\CkAStar\Algorithm\CkAStar_GraphConcept.h"
      rg --no-ignore -n "ContinueSearch|InWarmStartFromIndex|ValidateExistingPath" `
         "Plugins\CkFoundation\Source\CkAStar\Public\CkAStar\Algorithm\CkAStar_Search.h"
      ```
      Missing warm-start ctor or `ValidateExistingPath` ⇒ F1.30's stated seam is gone ⇒ **STOP**.
- [ ] **The install seam is untouched since PHASE_0**:
      ```powershell
      rg --no-ignore -n "InstallExternalPath|MarkPathPending|AbandonPath|FailPath" `
         "Plugins\CkFoundation\Source\CkNavigation\Public\CkNavigation\Nav\CkNav_Algorithm.h"
      ```
- [ ] **The provider fork and episode lifecycle are where R3 says they are**:
      ```powershell
      rg --no-ignore -n "RequestPathForActiveGoal" `
         "Plugins\CkFoundation\Source\CkCrowd\Public\CkCrowd\Agent\CkCrowdAgent_HandleRequests_Processor.cpp"
      ```
      Three provider branches expected (VoxelNav → PathNetwork → CkNavigation). A different count ⇒
      **STOP**, reconcile PROGRESS.md first.
- [ ] **One funnel implementation exists** from PHASE_2 (F1.19's flood-fill funnel). This phase
      **consumes** it; it does not author a second.
- [ ] **A5 `[MEASURE at phase entry]` budgets filled in now**, by measuring the Recast path on the
      same fixtures: funnel improvement floor, per-query time at the reference agent count, queries
      per frame against the existing budget. Recorded in PROGRESS.md with the artifact.
- [ ] **Phase-entry baseline captured this session** (full suite, `--parallel 1 --no-nullrhi`) —
      **read per [NN-D54]: NO full-suite run; the P2 entry baseline is the standing reference and
      every gate is a scoped `--test-pattern` run until every phase is done** —
      command shape as PHASE_1 §2. Pre-flight: the editor must be closed for this project (the
      toolbox exits **77** if it is open) — see the `build-test` skill's pre-flight table.
- [ ] Skills loaded: `ck-macros-and-codegen`, `ckecs-architecture-contract`,
      `ck-tests-authoring-and-running`, `ck-performance-and-analysis`, `build-test`,
      `ck-change-control`.

---

## 3. Standing decision gate (applies to EVERY `-> verify:` line in this file)

- Observation matches the stated expectation → continue.
- Compile/UHT/link failure → fix mechanically and re-verify. **Two failed attempts → STOP.**
- A hermetic test fails → the implementation is wrong; the assertion is the specification. Two
  failed attempts → STOP.
- A **crowd** test changes result → **STOP** before anything else. The episode lifecycle is preserved
  verbatim, so a crowd behaviour change means the fork was modified rather than extended.
- A shadow-mode divergence appears → that is **data, not a failure**. Record it in the shadow report.
  Divergences are adjudicated at the promotion gate (PHASE_8), never silently tuned away here.
- A previously green test is now red → **STOP**, revert the offending step, diagnose, re-sequence.
- **Anything else → STOP, record it in PROGRESS.md § Blockers with verbatim evidence, end session.**

---

## 4. Sub-phase 3A — The plate-portal graph and A* (F1.23)

**3A entry:** §2 all green.
**3A exit:** a `static_assert` pins the graph to the `AStarGraph` concept; a sliced search matches a
one-shot search exactly.

### Steps

1. Author the graph model. It must satisfy the **existing** concept — `Neighbors`, `Cost`,
   `Heuristic`, `IsGoal`, nothing geometric — over plate ids. Per-query immutable seed data follows
   `FPathGraphSharedData`'s precedent, **including its rule that agent fit is expressed as a
   clearance minimum, not as a layer/plate index: "search must stay blind to how a cell is
   addressed."** Illustrative:

   ```cpp
   namespace ck::groundnav
   {
       struct CKGROUNDNAV_API FPathGraphSharedData
       {
           CK_GENERATED_BODY(FPathGraphSharedData);

       private:
           TSharedPtr<const FCk_GroundNav_Field> _Field;
           FCk_GroundNav_CompiledFilter          _Filter;
           float                                 _MinClearanceUu = 0.0f;

       public:
           CK_PROPERTY_GET(_Field);
           CK_PROPERTY_GET(_Filter);
           CK_PROPERTY_GET(_MinClearanceUu);
       };

       struct CKGROUNDNAV_API FPlatePortalGraph
       {
       public:
           auto Neighbors(FCk_GroundNav_PlateId InPlate) const -> TArray<FCk_GroundNav_PlateId>;
           auto Cost(FCk_GroundNav_PlateId InFrom, FCk_GroundNav_PlateId InTo) const -> float;
           auto Heuristic(FCk_GroundNav_PlateId InFrom, FCk_GroundNav_PlateId InGoal) const -> float;
           auto IsGoal(FCk_GroundNav_PlateId InPlate) const -> bool;

       private:
           FPathGraphSharedData _Shared;
           FCk_GroundNav_PlateId _Goal{};
       };
   }

   static_assert(ck::astar::AStarGraph<ck::groundnav::FPlatePortalGraph, ck::groundnav::FCk_GroundNav_PlateId>);
   ```
   → verify: the `static_assert` compiles, as the existing graph adapters do (VALIDATION.md A5,
   first bullet).

2. Implement the **portal transition-point math** — the one genuinely new piece R4 flags: the point
   on a portal interval used by `Cost` and `Heuristic` (the analogue of CkVoxelNav's
   `Get_CellTransitionPoints`).
   → verify: Layer 1 — transition points lie on their portal interval, are deterministic, and are
   inset by the agent radius where the interval permits.

3. Drive the search with `TSearchState::ContinueSearch(FSearchParams)` — budget is **iterations AND
   microseconds**, and state is preserved across a frame boundary on `InProgress`. Failure modes must
   all stay distinguishable: `Unbuilt`, `NoStartSurface`, `NoGoalSurface`, `Unreachable`,
   `BudgetExceeded`, `Blocked`.

   **Heuristic contract.** The baseline heuristic is **Euclidean distance** between the current
   transition point and the goal — admissible, inadmissibility bound **0**. An optional greedy
   weight `w >= 1.0` is a tunable that **defaults to 1.0** (admissible at its default); the
   inadmissibility bound at a given `w` is `(w - 1)`. Any `w > 1.0` shipped or measured must be
   recorded with its measured path-quality impact against the analytic-optimum fixture.
   → verify: Layer 1 — optimal path length on a fixture with a known analytic optimum, within the
   heuristic's stated inadmissibility bound; a search across two disconnected components rejects via
   the F1.9 component labels with **zero expansions**; a sliced search matches a one-shot search
   exactly; node cap and length cap each terminate with the correct distinguishable status.

   **Gate 3A-3.**
   - All four cases pass and expansion counts are recorded → continue.
   - The cross-component search performs expansions → the component reject is not wired ahead of the
     search. Fix; this is the near-O(1) rejection the whole feature exists for. Two attempts → STOP.
   - Sliced ≠ one-shot → search state is not fully preserved across the slice boundary. Two attempts
     → STOP.
   - Path length exceeds the analytic optimum by more than the bound implied by `w`'s **configured
     value** (0 at the default `w = 1.0`) → **STOP**: raising `w` is a design decision, and its
     path-quality cost must be measured and recorded before it is raised.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

4. Record expansion counts for the reference fixtures in VALIDATION.md as **tracked regression
   numbers**.
   → verify: the numbers are in VALIDATION.md and PROGRESS.md, with the artifact they came from.

5. Gate 3A: `--test-pattern GroundNav`.

---

## 5. Sub-phase 3B — Funnel, partial paths, cost model, post-processing (F1.24, F1.25, F1.26, F1.28)

**3B entry:** 3A exit green.
**3B exit:** L-shaped corridor funnels to exactly 3 waypoints; every waypoint is ≥ R from the nearest
boundary.

### Steps

1. Consume PHASE_2's funnel over the portal interval sequence (F1.24). Radius inset is applied to
   portal intervals **before** funnelling, clamping to the portal midpoint when the interval is
   narrower than 2R — which cannot happen for an admitted path, because the portal's minimum
   clearance already gated it.
   → verify: Layer 1 — an L-shaped corridor emits exactly 3 waypoints (start, inner corner, goal); a
   straight corridor exactly 2; string-pulled length ≤ corridor centre-line length on **every**
   fixture; every waypoint is ≥ R from the nearest boundary (via F1.18); degenerate portals produce
   neither duplicate nor NaN waypoints.

   **Gate 3B-1.**
   - All five properties hold → continue.
   - A waypoint is closer than R to a boundary → the inset is applied after funnelling instead of
     before. Order is the spec. Fix. Two attempts → STOP.
   - A second funnel implementation was written for this phase → delete it and consume PHASE_2's.
     Two implementations will drift and F1.19's distances will silently diverge from paths.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

2. Implement the cost model (F1.26): base Euclidean distance through the portal transition point,
   multiplied by (a) the destination plate's **area cost multiplier**, **max-wins** when multiple
   painted regions overlap, (b) a slope/gradient penalty scaling with height change over horizontal
   distance, and (c) a **clearance bias** mildly penalising wall-hugging. Accumulation is
   **integrate over the segment, not max-of-cells** — that is fixed, not chosen. Cost lives entirely
   in the graph model's `Cost(A,B)`; the field stores an area-tag key and multiplier per plate, and
   the per-query compiled filter supplies the tag→multiplier table.

   **Frozen formula shapes — implement these exactly; the constants are measured, not invented:**
   - slope penalty: `cost = cost * (1 + k_slope * rise / run)`.
     `k_slope` is a tunable; default `[MEASURE at phase entry]` on the ramp-vs-level fixture.
   - clearance bias: `cost = cost * (1 + k_wall / (clearance_cells + 1))`.
     `k_wall` is a tunable; default `[MEASURE at phase entry]` on the corridor fixture.
   - corner offset distance: `agent_radius * k_corner`, `k_corner` default **1.0**;
     `[MEASURE at phase entry]` on the corner fixture.
   - crossover tolerance for the analytic cost-crossover assertion: **one finest cell**.

   Executors implement the formulas as written and record the measured defaults in PROGRESS.md at
   phase entry. Changing a **formula shape** is an orchestrator decision; changing a constant is a
   measurement.
   → verify: Layer 1 — a path around a 3× cost region is chosen when the detour is <3× longer and
   through it when longer, at the **exact analytic crossover ± one finest cell**; two overlapping painted
   regions yield the max, not the product or sum; the slope penalty makes a ramp route lose to a
   level route at the configured threshold; clearance bias shifts a corridor path toward the centre
   line measurably **without changing its plate corridor**.

3. Implement partial paths and search bounds (F1.25): best-node tracking (lowest h-value expanded)
   returned on cap or exhaustion when `_AllowPartialPath` is set; the funnel then runs over the
   truncated corridor normally. The status mapping into the existing
   `{None, Pending, Ready, Failed, Partial}` enum stays byte-compatible.
   → verify: Layer 1 — a goal in a sealed room with partials enabled returns `Partial` with a path
   ending adjacent to the seal; with partials disabled returns `Failed` with no waypoints; the node
   cap produces `Partial` (flag on) or `BudgetExceeded` (flag off) and **never** a silent truncation
   reported as `Ready`. Layer 2 — the crowd strict→permissive re-dispatch autotests green, because
   the partial/complete distinction is load-bearing gameplay logic, not a nicety.

4. Implement the post-processing chain (F1.28) as pure functions over a waypoint array + the field,
   in order: funnel → **corner offset** → the existing Ck skip-first-waypoint pass → per-waypoint
   fill (direction to next, integrated distance, integrated cost, surface normal, area tags).
   → verify: Layer 1 — corner offset moves exactly the inside-corner waypoints and by
   `agent_radius * k_corner`; **every offset waypoint remains on walkable ground with clearance ≥ R** (offsetting off
   the surface is a real failure mode); integrated distance equals the polyline length; a failed
   query leaves the previous `_Waypoints` intact.

5. Gate 3B: `--test-pattern GroundNav` then `--test-pattern Crowd`.

---

## 6. Sub-phase 3C — Install, provider branch, failure parity, plan repair (F1.29, F1.41, F1.30)

**3C entry:** 3B exit green.
**3C exit:** an agent walks a route end-to-end via the new provider in a Layer-2 PIE autotest.

### Steps

1. Add the CkGroundNav branch to `RequestPathForActiveGoal`, **beside** VoxelNav → PathNetwork →
   CkNavigation. Each branch already does `MarkPathPending(handle, revision)` then dispatches; the
   new branch does exactly the same. Extend `ECk_NavSurface_Provider` with its second entry now that
   the provider exists.
   → verify: `rg --no-ignore -n "MarkPathPending" Plugins/CkFoundation/Source/CkCrowd` shows one call
   per branch and no new install mechanism anywhere:
   ```powershell
   rg --no-ignore -n "InstallExternalPath" Plugins/CkFoundation/Source
   ```
   → hits only in `CkNav_Algorithm` and in each provider's dispatch site.

2. Preserve the episode lifecycle **verbatim** — advance revision → abandon previous provider → mark
   the shared slot pending → dispatch **exactly one** provider → install only a fresh result →
   release terminal episodes. The fork gains a branch and changes nothing else.
   → verify: Layer 2 — the crowd episode tests are delta-zero, **and** a test asserts that exactly
   one provider is dispatched per episode and that a stale result never installs.

   **Gate 3C-2.**
   - Episode tests delta-zero and the exactly-one-dispatch assertion holds → continue.
   - Two providers dispatch in one episode → **STOP**. This is the failure mode the episode design
     exists to prevent and it will not reproduce reliably later.
   - A stale result installs → the caller revision was not retained through the new branch. Fix; the
     retention is what makes external install the writer authority. Two attempts → STOP.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

3. Implement failure parity (F1.41): a **total** mapping from CkGroundNav statuses onto the existing
   `FCk_Nav_PathResult` fail-reason enum, with the same signals and the same
   waypoints-preserved-on-failure behaviour.
   → verify: Layer 1 — an **exhaustiveness** test: every status maps to exactly one existing reason,
   and adding a status without extending the map fails to compile or fails the test. Layer 2 —
   `NoRouteFailsClean` and `Stall_UnreachableGoalFailsBounded` green on CkGroundNav with the same
   signal sequence.

4. Implement plan repair (F1.30) on the **existing** CkAStar seam: the free
   `ValidateExistingPath(Graph, Path) -> int32` returns the first disconnected step; the warm-start
   constructor `(Graph, Start, Goal, ExistingPath, WarmStartFromIndex, Capacity)` re-searches from
   there. Validation runs against the **current published field epoch**, so the check is a pure
   function of two values.
   → verify: Layer 1 — a path invalidated at step k re-searches with expansions bounded well below a
   cold search (record the number); a path invalidated at step 0 correctly reports full replan; a
   still-valid path validates in O(path length) with **zero** expansions.

5. The end-to-end Layer-2 pin — this phase's headline. A PIE AngelScript autotest in which an agent
   requests a route on the CkGroundNav provider, receives it through the real signal seam
   (`OnPathReady` / `OnPathFailed` — assert on the **signal**, not a polled snapshot), and walks it.
   One `UCk_AutoTest_Base` subclass per `.as` file; timeout via `default _TimeoutSeconds` on the
   **entity script**; every wait a named condition on this test's own entities.
   → verify:
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
     --test-pattern GroundNav --parallel 1 --discover-fresh --output=Saved/Logs/BuildTest.log `
     --project="D:\Repos\CkPlugins_3"
   ```
   → the new autotest appears (Total rose) and is green. A green run at the old Total is
   **stale-green** and has not run it.

6. Gate 3C: `--test-pattern Crowd` + `--test-pattern GroundNav`.

---

## 7. Sub-phase 3D — Shadow / A-B mode (F1.40)

**3D entry:** 3C exit green.
**3D exit:** a full-suite shadow run completes with zero crashes and emits a divergence report whose
schema is stable enough to diff run over run.

### Steps

1. Implement shadow dispatch: a setting sends the **same** request to both providers; the Recast
   result **installs** (authoritative); the CkGroundNav result is compared and discarded.
   → verify: Layer 2 — with shadow on, installed paths are byte-identical to shadow-off Recast runs
   (the shadow result must never leak into the install path).

   **Gate 3D-1.**
   - Installed paths identical to shadow-off → continue.
   - Installed paths differ → the shadow result is reaching the install seam. **STOP** immediately;
     until this is false, every parity number collected is meaningless.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

2. Author the diagnostics fragment — **values only**: counts, deltas, histograms, ids. Never handles,
   never UObjects, never field shares. This is the value-only debug-snapshot rule, and it is what
   makes the fragment snapshot- and debugger-safe by construction. Illustrative:

   ```cpp
   namespace ck
   {
       struct CKGROUNDNAV_API FFragment_GroundNav_ShadowDiagnostics
       {
           CK_GENERATED_BODY(FFragment_GroundNav_ShadowDiagnostics);

           friend class FProcessor_GroundNav_ShadowCompare;

       private:
           int32   _ComparisonCount = 0;
           int32   _OutcomeAgreeCount = 0;
           int32   _OutcomeDisagreeCount = 0;
           int32   _FailReasonDisagreeCount = 0;
           int32   _PartialDisagreeCount = 0;
           int32   _ContainmentEscapeCount = 0;
           double  _PathLengthDeltaSum = 0.0;
           double  _PathLengthDeltaMax = 0.0;
           double  _EndpointDeltaMax = 0.0;
           int32   _WaypointCountDeltaSum = 0;
           TArray<FName> _DivergingQueryIds;

       public:
           CK_PROPERTY_GET(_ComparisonCount);
           CK_PROPERTY_GET(_OutcomeAgreeCount);
           CK_PROPERTY_GET(_OutcomeDisagreeCount);
           CK_PROPERTY_GET(_FailReasonDisagreeCount);
           CK_PROPERTY_GET(_PartialDisagreeCount);
           CK_PROPERTY_GET(_ContainmentEscapeCount);
           CK_PROPERTY_GET(_PathLengthDeltaSum);
           CK_PROPERTY_GET(_PathLengthDeltaMax);
           CK_PROPERTY_GET(_EndpointDeltaMax);
           CK_PROPERTY_GET(_WaypointCountDeltaSum);
           CK_PROPERTY_GET(_DivergingQueryIds);
       };
   }
   ```
   Every metric of VALIDATION.md §3.2 must be representable here: success/failure agreement,
   fail-reason agreement, path-length delta (absolute and relative; mean, p95, max per map), endpoint
   delta, waypoint-count delta, containment escapes, query time both providers, partial-path
   agreement. Counters are **per fixture** so VALIDATION.md can table them.
   → verify:
   ```powershell
   rg --no-ignore -n "FCk_Handle|TSharedPtr|UObject|TWeakObjectPtr" `
      "Plugins\CkFoundation\Source\CkGroundNav\Public\CkGroundNav\Shadow"
   ```
   → **zero hits**, and a Layer-1 test constructs the fragment with the producer already torn down.

3. Author the comparison processor (`CK_REGISTER_PROCESSOR` — it is a processor, so it registers).
   → verify: the processor is registered and the counters move on a shadow run.

4. Run the full suites in shadow mode and emit the report.
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
     --parallel 1 --no-nullrhi --output=Saved/Logs/BuildTest.log --project="D:\Repos\CkPlugins_3"
   ```
   → verify: zero crashes; the report emits one row per map with every §3.2 metric plus the artifact
   identity, pasted into PROGRESS.md.

   **Gate 3D-4.**
   - The run completes and the report is schema-complete → continue. **Divergences are expected at
     this phase and are data, not failures** — they are adjudicated at PHASE_8's promotion gate.
   - The run crashes or hangs under shadow dispatch → **STOP**; a shadow harness that cannot survive
     the suite cannot produce promotion evidence.
   - A §3.2 metric has no field to hold it → add the field; a metric missing from the report is a
     metric that will be missing at the promotion gate.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

5. Phase gate: full suite with shadow **off**, on the phase's final artifact.
   → verify: delta-zero vs the §2 baseline plus the phase's new tests green.

---

## 8. Exit criteria

- [ ] `static_assert` pins `FPlatePortalGraph` to the `AStarGraph` concept.
- [ ] Path correctness on authored fixtures: a path exists where one exists; clean failure where none
      does; **every consecutive waypoint pair walkable by raycast**; the path stays within the field.
- [ ] Funnel improvement over raw plate-sequence paths measured and ≥ the recorded floor.
- [ ] Partial paths behave as the existing contract, with the crowd strict→permissive re-dispatch
      autotests green.
- [ ] Per-area costs and per-query filters change the route: the same start/goal pair routes
      differently under two filter definitions.
- [ ] Install goes through the existing seam only — grep confirms no second install path.
- [ ] Episode lifecycle preserved verbatim; exactly-one-dispatch and no-stale-install asserted.
- [ ] Exhaustive status→fail-reason mapping test green.
- [ ] Plan repair: expansions on a warm-started repair recorded and well below a cold search.
- [ ] **Layer-2 PIE autotest: an agent walks a route end-to-end via the CkGroundNav provider**, green,
      appearing at a risen Total under `--discover-fresh`.
- [ ] Shadow mode operational, its diagnostics fragment value-only (grep evidence), a full-suite
      shadow report pasted into PROGRESS.md.
- [ ] A5 budgets filled in with measured numbers; expansion counts recorded as tracked regressions.
- [ ] Three environments for every new public API: C++, Blueprint, AngelScript.
- [ ] Full suite **delta-zero** vs the §2 baseline on the phase's final artifact; zero new ensures,
      zero new warnings; editor boots clean.
- [ ] Provenance cells complete for F1.23–F1.30, F1.40, F1.41.
- [ ] Comment audit run; PROGRESS.md updated; PHASE_4 entry criteria re-verified.

---

## 9. Fences

- **No second install path.** `MarkPathPending` / `InstallExternalPath` / `AbandonPath` / `FailPath`
  plus the revision ring is the seam, and CkVoxelNav and CkPathNetwork already prove it works.
- **Do not restructure the provider fork.** It gains a branch. Refactoring it "while you're in there"
  destroys the delta-zero claim for the entire crowd suite.
- **No second funnel.** Consume PHASE_2's.
- **Shadow results never install.** The authoritative provider is the one the setting names; the
  shadow result is compared and discarded.
- **Do not tune away a divergence.** Divergences are the phase's product. Adjudication is PHASE_8's
  job, per-divergence, enumerated — never summarized into a percentage.
- **The diagnostics fragment holds values only** — no handles, no UObjects, no field shares. Also:
  `_PendingSinceSeconds` and any wall-time value stay process-relative and are never persisted or
  replicated.
- **Do not change the default provider.** Promotion is PHASE_8 and has its own CTO sign-off. Every
  phase leaves the Recast path selectable and green; rollback stays a one-setting revert.
- **No markup, no repair-on-geometry-change, no dynamics.** PHASE_4. Plan repair (F1.30) is in scope
  as a *search* capability; the change *detection* that triggers it (F1.34) is not.
- **`CK_REGISTER_PROCESSOR` on every processor**; deferred `Request_*` APIs end with the completion
  delegate as the **last** parameter, `AutoCreateRefTerm`, no C++ default.
- **`--discover-fresh` for the new AS autotests**; a net AS autotest needs a full C++ rebuild to
  exist at all.

---

## 10. [P3] Done means

VALIDATION.md **A5** (search, install, and shadow harness) is green with evidence in PROGRESS.md, the
§3 shadow protocol is operational and has produced its first full-suite report, every VALIDATION.md
§0 standing gate passes on the phase's final artifact, and the §2.4 coverage rows "search + funnel
string-pull" and "provider episode / revision / cancellation" have their Layer-1 and Layer-2
requirements satisfied.
