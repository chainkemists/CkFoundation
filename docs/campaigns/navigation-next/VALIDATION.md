# VALIDATION — acceptance protocol and definition of done for navigation-next

> The campaign closes only when every line here is checked **with evidence recorded in PROGRESS.md**
> — a test name, a log path, a recorded number, or a named human verifier. Extend this document per
> phase (each phase's exit criteria append here), never silently.
>
> **Every numeric budget in this document is written `[MEASURE at phase entry — no estimated
> numbers]`.** The number is filled in by measuring the *Recast* path on the *same fixture* at the
> owning phase's entry, and recorded in PROGRESS.md alongside the artifact it was measured on. A
> budget an executor invented, estimated, or carried over from another machine is not a budget.
>
> **Vocabulary.** *Delta-zero* = pass/fail/skip totals and failing-test names identical to the
> phase-entry baseline, **plus** the phase's new tests green. *Phase-entry baseline* = a full-suite
> toolbox run captured in the phase's first session, against the artifact that session started from.

---

## 0. Standing gates (every phase, no exceptions)

These are re-run at every phase exit. A phase whose own criteria are green but which fails one of
these has not exited.

- [ ] **Baseline captured at phase entry** — full toolbox `--build --test` (`--parallel 1`,
      `--no-nullrhi`), totals and failing-test **names** recorded in PROGRESS.md with the artifact
      identity. No baseline ⇒ no "no regressions" claim is admissible from that phase.
- [ ] **Phase-exit full suite is delta-zero vs that baseline**, run on the **final** artifact of the
      phase (not a mid-phase build). A green run predating the phase's last edit is stale-green and
      does not count.
- [ ] **Zero new ensures and zero new warnings** across the run. The AutoTest harness escalates a
      `ck::Warning` to a failure, so a warning is a red test, not a note. Inspect a fresh editor
      startup log for boot ensures and AS compile errors as well — process exit code alone is not
      the verdict.
- [ ] **Editor boots clean** with the new modules enabled; AngelScript bindings generate without
      error; `Script/Generated/*` churn is regenerated, not hand-edited.
- [ ] **Three environments** for every public API added in the phase: exercised from C++, from
      Blueprint, and from AngelScript. "Works in C++" is one third of done.
      **Carve-out:** `Get_BoundarySegments` — and any other function carrying the off-thread
      thread contract — is **C++-only by contract**. Blueprint and AngelScript get a
      game-thread `UFUNCTION` wrapper; the three-environments rule applies to that wrapper.
      The off-thread C++ entry is exercised by the Layer-1 thread-contract test instead.
- [ ] **Provenance rows complete** — every feature the phase added has a non-empty Provenance cell in
      `FEATURE_MATRIX.md` naming its public algorithm(s) and public reference(s); every PR in the
      phase named its references in the PR body. An empty Provenance cell blocks the gate.
- [ ] **Attribution obligations discharged** for any permissively licensed reference used: license
      file present, header attribution present, module doc row present.
- [ ] **No forbidden-source language** anywhere in the phase's output — no campaign doc, source
      comment, commit message, or PR body names or alludes to a forbidden reference codebase (see
      PROMPT.md § 0 for the policy that defines "forbidden").

---

## 1. Per-tier gates

The tiers mirror `MIGRATION_SEAM.md` Step 4. **Tier A** is a prerequisite for **Tier B**; **Tier B**
(promotion) and **Tier C** (retirement) are separate sign-offs and are never granted together.

### Tier A — Capability gates (per phase, prerequisite for promotion)

#### A1 — Contract neutralization (P0)

- [x] **No Unreal-Navigation type appears in the public request/result contract.** (evidence: P0
      CLOSE-OUT, PROGRESS.md:1048 — "`CkNav_Fragment_Data.h` Unreal-nav types: 0 hits ✓")
      Evidence: `rg --no-ignore -n 'UNavArea|UNavigationQueryFilter|TSubclassOf<UNav' Source/CkNavigation/Public/CkNavigation/Nav/CkNav_Fragment_Data.h`
      → **zero hits**. Excluded areas are `TArray<FGameplayTag>`; the per-query override is an
      `FGameplayTag`.
- [ ] **Filter definitions are provider-neutral assets** naming allowed/excluded area tags and
      per-area cost multipliers; the Recast adapter compiles them into engine filters. CkCrowd's
      strict and permissive filter classes exist as two filter-definition assets.
- [x] **No back-compat shim** — the old typed members are deleted, not deprecated. (evidence: P0
      CLOSE-OUT, PROGRESS.md:1049 — "Removed members repo-wide: 0 code hits ✓") Evidence: the
      call-site sweep landed in the same change; a repo-wide grep for the removed member names
      returns zero.
- [x] **Behaviour unchanged.** Full suite delta-zero. (evidence: P0 CLOSE-OUT, PROGRESS.md:1025-1027
      — final phase gate Total 1300 / Passed 1280 / Failed 20, fail set ⊆ the 22-name baseline set,
      zero new reds in any gate of the phase) This phase is a refactor: any test whose
      result *changes* is a defect, including one that goes from red to green unexplained.

#### A2 — Facade and adapter (P0/P2)

- [x] **All twelve capabilities are reachable through the facade**, and each has at least one
      hermetic or PIE test exercising it through the facade rather than through a provider directly.
      (evidence: P0 CLOSE-OUT, PROGRESS.md:1058-1060 — "Twelve capabilities tested through the
      facade: 8 AS PIE autotests + net isolation + thread-contract + RevisionRing +
      LatestWinsCoalescing C++ tests ✓")
- [x] **Every consumer site in the R3 dependency table is disposed of**: migrated to the facade, or
      deleted per its `[RETIRE]` bucket. (evidence: P0 CLOSE-OUT, PROGRESS.md:1061 — "R3 disposition
      checklist: 65 rows, zero unresolved ✓"; PROGRESS.md:1056 — "`DiagNavClip|DrawNavProjection`:
      0 code hits ✓") Evidence: a checklist in PROGRESS.md with one row per R3
      table row and its resolution. Sites bucketed `[RETIRE]` (`DiagNavClip`, `DrawNavProjection`,
      the debug-settings probe) are **deleted**, and their deletion is confirmed by grep.
- [x] **The off-thread contract is documented and enforced.** (evidence: P0 CLOSE-OUT,
      PROGRESS.md:936-938 — the `Get_BoundarySegments` off-thread contract test added in 0C batch C;
      PROGRESS.md:1066 — "`Get_BoundarySegments` C++-only per the §8 carve-out ✓") `Get_BoundarySegments` states in its
      contract comment that it is callable off the game thread against an immutable snapshot, and
      the parallel avoidance consumer calls it from its parallel processor unchanged.
      Evidence: a test that calls it concurrently from a worker while the field is republished, and
      completes with a self-consistent result and no ensure.
- [x] **The deferred-request queue is per-world.** (evidence: P0 CLOSE-OUT, PROGRESS.md:1062 —
      "Multi-PIE deferred-queue isolation: GREEN (per [NN-D18] accepted form) ✓") Evidence: a multi-PIE test in which two worlds
      each hold in-flight deferred requests and neither observes the other's; the 5s force-fail
      watchdog and latest-wins revision semantics still hold in each world independently.
- [x] **Revision-consolidation:** exactly one surface-revision observer remains; the CkQueue
      duplicate is gone. (evidence: P0 CLOSE-OUT, PROGRESS.md:1057 — "Revision observers: exactly
      one implementation site (`CkNavigationRevision_Subsystem`) ✓") Evidence: grep returns a single generation-observer implementation.

#### A3 — Bake core (P1)

- [x] **Hermetic, ECS-free bake.** The math core bakes a hand-authored geometry list with no world,
      no registry, and no editor. Evidence: the `Ck.GroundNav.Bake.*` family runs headless.
      **Met 2026-09-01** — 63 headless unit tests under `CkTests.UnitTests.CkGroundNav.Bake.*`, all
      driving the pipeline through the stub geometry backend. Fences re-verified on the final artifact:
      no Jolt header in the module, and no `UWorld`/`FCk_Handle`/`UObject` under `Bake/`.
- [x] **Deterministic probe budgeting.** Probe **count** is the asserted budget; wall-clock is only
      a guard. Evidence: a test asserting an exact probe count for a fixed fixture; re-running the
      bake produces byte-identical field values (determinism pin).
      **Met 2026-09-01** — `Budget_ProbeCountIsIndependentOfSliceSize` asserts the same total across
      four slice sizes and against the one-shot bake, and `Budget_SlicedBakeMatchesOneShotBake`
      compares the two fields byte for byte over a 3x3 field built in nine slices. Gate 61/61,
      `Saved/Logs/P1-1De-Budget-Gate2.log`. Per [NN-D34] the resumable unit is the tile, so a slice
      may overshoot its budget by one tile; the probe count cannot vary with slice size because a
      tile is never split.
- [x] **Overlapping floors resolve into distinct layers.** Fixture: a two-story box with a floor
      above a floor → two layers over one footprint, each independently walkable, no cross-talk.
      **Met 2026-08-31** — `Layers_TwoStoreysAreTwoDisjointLayers` (exactly two layers, disjoint
      per-column assignment), with `Layers_RampOverItselfSplitsWithoutSevering` pinning that a floor
      that climbs over itself splits into layers without losing the connection between them.
- [x] **Slopes, ramps, and stairs merge into plates that stay planar enough for the funnel.**
      Evidence: a fixture per case asserting plate count, per-plate plane-fit residual within the
      frozen tolerance, and normal-cone compliance.
      **Met 2026-08-31.** Frozen tunables, per [NN-D29]: `PlaneFitToleranceUu` **10.0**
      (usable window `[1.0, shallowest riser)`; the 1.0 floor is int8 normal-quantization drift,
      measured at 0.979 uu over 48 ramp cells), `NormalConeDegrees` **10.0** (measured inert above
      ~3 degrees on every fixture, including two built specifically to make it bite).
- [x] **Clearance is correct.** Evidence: a distance-transform test asserting exact clearance values
      on a hand-authored fixture, including at a concave corner and at a tile boundary (the
      cross-tile case is the one that silently breaks).
      **Met 2026-09-01** — `Clearance_MatchesABruteForceReference` (every cell against an O(n²)
      reference), `Clearance_CorridorSpineIsHalfItsWidth`, `Clearance_OpenSquareAndIsolatedCell`
      (the concave corner and the one-cell island), and the cross-tile case pinned by
      `Tile_SeamAgreesWithAWiderBake`: a tile baked with its halo agrees cell for cell with a bake of
      the wider region, which is exactly the property a truncated halo would break silently.
- [x] **Plate merge collapses as designed.** Evidence: measured cell → plate counts on both a
      known-layout fixture and a generated scene, recorded as numbers.
      **Measured 2026-09-01** on the reference fixture (`Test_GroundNav_ReferenceNumbers.cpp`, pinned
      in the assertion): 9,792 walkable cells → 22 plates = **445.09 cells/plate**, with 826,038 probes
      spent, identical across runs (re-pinned twice since first measured at 117,962: [NN-D35] made the
      probe an honest unit, and [NN-D38]'s closure check reads every body's mesh once). That beats the in-house volumetric precedent (91,752 → 359 =
      255.6). NOT a prediction for a shipped map: the fixture is synthetic and chosen to exercise every
      stage. Per PHASE_1 a poor ratio would be a PHASE_3 budget finding rather than a correctness
      failure, so a good one is not a performance claim either.
- [x] **Portals carry minimum clearance across the boundary they span.** Evidence: a fixture with a
      pinch point at a plate boundary where a per-plate clearance would wrongly admit an agent that
      the per-portal minimum correctly rejects.
      **Met 2026-09-01** — `Portals_PinchPointRejectsWhatPlateClearanceAdmits`: two 500 uu rooms
      joined by a 75 uu corridor; both portals offer 50.0 uu and reject a 100 uu radius, while both
      rooms' best-cell clearance of 250 uu would have admitted it. Note the aggregate the number
      actually is, per [NN-D30]: per crossing cell pair the tighter of the two sides, maximised along
      the interval, because the agent chooses where to cross. Gate 41/41,
      `Saved/Logs/P1-1C3-Portals-Gate3.log`. The cross-TILE portal clauses are owed by 1D.
- [x] **Stable integer identity only.** Evidence: `rg` over the field types finds no raw pointer, no
      `TObjectPtr`, no `TWeakObjectPtr`, no engine-object reference inside the field value types.
      **Met 2026-09-01 (session 6)** — `rg "TObjectPtr|TWeakObjectPtr|UObject|\*_"` over
      `Field/CkGroundNav_FieldTypes.h`, `Field/CkGroundNav_Field.h`, `Bake/CkGroundNav_BakeTypes.h`
      → 0 hits. The one non-integer the field carries is the FString description on an open-body
      diagnostic, captured by value at bake time so it outlives the body it names.
- [x] **Bake cost within budget** — bake wall time and peak memory for the reference scene:
      **Measured 2026-09-01 (session 6).** Two scenes, because the phase has two kinds of reference:
      (1) the synthetic reference fixture (`Test_GroundNav_ReferenceNumbers.cpp`, 3×3 tiles of 800 uu at
      25 uu cells, five closed boxes): published field **191,557 bytes** (exact `Get_AllocatedSize`,
      asserted identical across runs) and **10.23 ms** wall time headless on the maintainer's machine
      (`Saved/Logs/P1-Batch3-Gate3.log`); (2) the tuning-range gym over live Jolt geometry (the
      [EDITOR-VERIFY] scene, Test A): a 16-tile field of 17,677 walkable cells and 56 plates baked in
      **23.6 ms**, single regions of 120×120 columns in 8.3–8.9 ms
      (`Saved/Logs/CkPlugins-backup-2026.09.01-22.32.13.log`). Memory is the published product's heap
      footprint, not a process peak: the per-tile transients (span, layer, clearance fields over the halo
      lattice) are freed before the next tile bakes, and the debug summary now prints the same
      `memory` line for any bake so a real level's number is one console command away. No budget was
      set for the phase to be within; these are the recorded values a later phase budgets against.

#### A4 — Query surface (P2)

Each capability gets a hermetic correctness test **and** a Recast-agreement test on a shared fixture.

- [ ] **Projection** — nearest walkable surface with asymmetric extents; batch form agrees with the
      single form element-for-element; a point above a two-layer stack projects to the correct
      layer; a point outside every tile fails cleanly rather than returning a garbage surface.
      **Hermetic half met 2026-09-02** (`CkTests.UnitTests.CkGroundNav.Query.Projection_*`, 9 tests,
      `Saved/Logs/P2-2A-Gate1.log`): batch element-wise identical to singles over 500 seeded queries;
      the two-layer column answers by vertical step-band (deck vs ground); outside every tile is
      `NoSurface`, over an unbuilt tile `Unbuilt`, at every entry point. Recast-agreement half owed.
      **Is-navigable cost delta recorded** (PHASE_2 §4 step 2): over the same 10,000 points,
      is-navigable read 14,911 cells (0.43 ms) against projection's 755,010 cells at 100 uu extent
      (21.69 ms) — asserted on cells, reported on time; agreement with zero-extent projection on all
      10,000 points.
- [ ] **Constrained surface walk** — a walk that would leave the surface is clamped to it; the
      result never leaves walkable ground; the off-surface recovery path returns an agent nudged
      outside back onto the field. This is the single Transform writer for grounded agents: its
      agreement budget is the tightest in the campaign.
      Endpoint agreement budget vs Recast on the shared fixtures:
      **37 uu** (Recast's own walk ends 12.0 uu short of the analytic rim on the level floor in all
      eight directions, `Saved/Logs/P2-RecastBudgets.log`, 2026-09-02, + one 25 uu cell). Interior
      walls re-measured on the second nav-bounds band when it exists.
      **Hermetic half met 2026-09-02** (`Query.SurfaceWalk_*`, 6 tests, `Saved/Logs/P2-2B-Gate4.log`):
      zero escapes over 10,000 seeded moves at radius 0 and at radius 30.
- [ ] **Walkability raycast** — clear line reports clear; blocked line reports the correct hit point
      within **37 uu** (Recast's own ray stops 12.0 uu short of the analytic rim,
      `Saved/Logs/P2-RecastBudgets.log`, 2026-09-02, + one 25 uu cell); a ray along a layer boundary does not
      leak between layers. **Hermetic half met 2026-09-02** (`Query.Raycast_*`, 6 tests,
      `Saved/Logs/P2-2B-Gate4.log`): wall hit within one cell of its plane, layer isolation on the deck,
      200 seeded pairs symmetric, cost cap stops the ray.
- [ ] **Boundary segments** — wall segments near a point match the fixture's authored walls;
      segment count and ordering are stable across repeated calls on an unchanged field.
- [x] **Reachability** — an unreachable goal is rejected in near-constant time via component id, with
      no search performed. (evidence: PHASE_2 EXIT CRITERIA, PROGRESS.md:372 — "reachability
      rejection on expansion count zero — MET (2D, two-island and open-component tests)") Evidence: a test asserting the search iteration counter is zero on the
      rejection path.
- [x] **Flood-fill point generation** — random and grid point generation over a component returns
      only walkable points, honours the requested count, and is deterministic under a fixed seed.
      (evidence: Gate 2D-3, PROGRESS.md:317-321 — `P2-2D-Gate3.log` 153/153, "40k-point chi-square
      below the Wilson-Hilferty 0.99 quantile, every path-distance point in range against an
      independent flood and the reference paths, overlapping aligned grids agree exactly")

#### A5 — Search, install, and shadow harness (P3)

- [ ] **Plate/portal graph satisfies the `AStarGraph` concept.** Evidence: a `static_assert` in the
      graph header, as the existing graph adapters do.
- [ ] **Paths are correct.** Evidence: on authored fixtures — path exists where one exists, fails
      cleanly where none does, every consecutive waypoint pair is walkable by raycast, and the path
      stays within the field.
- [ ] **Funnel string-pulling shortens raw plate-sequence paths measurably** on the reference
      scenes; numbers recorded. Improvement floor:
      `[MEASURE at phase entry — no estimated numbers]`.
- [ ] **Partial paths** behave like the existing contract: `Status::Partial`, and the crowd strict
      phase still treats a short partial as a verdict that triggers permissive re-dispatch.
- [ ] **Per-area costs and per-query filters affect the route** as the filter definition specifies.
      Evidence: the same start/goal pair routes differently under two filter definitions.
- [ ] **Install through the existing seam only.** Evidence: CkGroundNav reaches consumers via
      `MarkPathPending` / `InstallExternalPath` / `AbandonPath` / `FailPath` with the caller
      revision retained; grep confirms no second install path was invented.
- [ ] **Episode lifecycle preserved verbatim.** The provider fork gains a branch and changes nothing
      else. Evidence: the crowd episode tests are delta-zero, and a test asserts that exactly one
      provider is dispatched per episode and that a stale result never installs.
- [x] **Shadow mode operational** — see §3.
      **Met 2026-09-03 (session 8, 3D)** — `ECk_NavSurface_ShadowMode {Off, GroundNavShadowsRecast}`
      (per-world, `FRWLock`-guarded, project default Off) dispatches the shadow query after the
      authoritative Recast `Request_FindPath` at every re-dispatch site, with `_ActiveProvider`
      unchanged. The ignore gate is the **first** statement of
      `FProcessor_CrowdAgent_OnGroundNavPathResolved` (Gate 3D-1): the shadow result is compared and
      discarded, never installed — `Shadow_InstalledPathIsByteIdenticalToRecast` is green and the
      `Crowd` + `Nav` patterns are identical shadow-on vs shadow-off. §3.3's diagnostics fragment is
      satisfied by construction: `FFragment_GroundNav_ShadowDiagnostics` on the world transient
      entity carries counters, six 12-bucket histograms with interpolated p95, a 5×9 status-pair
      matrix and diverging query ids as plain values — the value-only grep over the fragment and
      report files is 0 hits — and the report is a 29-column `schema=1` table pinned by
      `Shadow_ReportSchemaIsStable` (`ck.GroundNav.ShadowReport`), with `ck.Crowd.DrawShadowRoutes`
      drawing both providers' routes. **Evidence:** Gate 5a `Saved/Logs/P3-3D-Gate5a.log` — 198/198
      (three AS `Shadow_*` rows plus three C++ `Shadow.*` rows in
      `Test_GroundNav_ShadowDiagnostics.cpp`); final gates on the ship artifact
      `P3-3D-K-GroundNav.log` 198/198, `P3-3D-K-Nav.log` 335/337 (baseline pair), Gate 9 `Crowd`
      115/118 (baseline pair plus a known flake); scoped sweeps `P3-3D-S2-Crowd.log` and
      `P3-3D-S2-Nav.log`, numbers in the 3D block below. **Scope:** this ticks the harness only.
      §3.1's full-suite shadow runs and §3.3's full-suite shadow report — the B1 promotion evidence
      — are deferred to campaign end per [NN-D54] and stay unticked.
- [ ] **Search cost within budget** — per-query time at the reference agent count, and queries per
      frame against the existing budget: `[MEASURE at phase entry — no estimated numbers]`.


**A5 tracked regression numbers (3A, measured 2026-09-02, `Saved/Logs/P3-3A-Gate2b.log`, pinned in
`Test_GroundNav_ReferenceNumbers.cpp` `Reference_SearchBudgetsAreStableAndRecorded`):**

| Reference route | expansions | cells read | crossings | pulled length = oracle |
|---|---|---|---|---|
| query scene (150,150)→(150,1450) | 2 | 16 | 2 | 1300.000 |
| doorway (200,500)→(200,1400) | 3 | 21 | 3 | 926.130 |

All nine reference pairs (5 query-scene, 4 doorway) string-pull to the visibility-graph oracle exactly
(delta 0.000); expansions 2 on every query-scene pair, 3–5 on the doorway pairs; sliced == one-shot on
status, corridor, crossings, search cost and expansion count.

**Plan repair (3C, measured 2026-09-02, `Saved/Logs/P3-3C-Gate4d.log`, `Repair.*` rows):** two-route
scene, seal at door 2 of 3 → verdict Repaired, **warm 5 expansions vs cold 11**; same epoch → StillValid
with 0 expansions; seal at door 0 → FullReplan with the cold count. Crowd Layer 2: walk, one provider
per episode, NoRoute and Stall twins green on the CkGroundNav provider (`P3-3C-Stall.log` 4/4).

**Shadow / A-B (3D, 2026-09-03):** Gate 3D-1 — installed paths byte-identical with shadow on
(`Shadow_InstalledPathIsByteIdenticalToRecast` green; `Crowd` + `Nav` patterns identical shadow-on vs
shadow-off). First scoped divergence report (see PROGRESS "3D divergence report"): fielded fixtures
6/6 both succeed, length Δ ≤ 28.3 uu, endpoint Δ 10 uu, GroundNav 0.010–0.015 ms vs Recast 0.011–0.025
ms per query; the crowd suite is Recast-only for lack of a GroundNav field over its fixtures.

#### A6 — Dynamics, markup, and repair (P4)

- [x] **Markup paints.** A runtime request makes a region impassable or expensive; a path that
      previously crossed it no longer does. Evidence (2026-09-03, `P4-4A-Final-GroundNav.log` 226/226):
      `Bake.Markup_ImpassableBoxMakesExactlyTheCoveredCellsImpassable`, `Markup_OverlappingCostRecordsTakeTheMaxAndUnionTheTags`,
      `Volume.MarkupLive_AttributesReportThePlateCostAndTags`, and the PIE pin
      `Ck_AutoTest_GroundNav_Markup_PaintThenRepathDoesNotCross` (settled route clears the paint by 243 uu).
- [x] **Markup-live probe is ground truth.** (2026-09-03: `Volume.MarkupLive_IsFalseUntilTheCoveringTilesRepublish` + `_TwoTileRecordIsLiveOnlyWhenBothRepublish` pin the strict rule; the PIE pin fails under `ck.GroundNav.Debug.MarkupLiveGate 0` (`P4-4A-PinBypass2.log`) and passes with the wait, `revisionAtPaint=46 revisionAtLive=54 framesWaited=17`.) Evidence: a test that requests markup, polls
      `Get_IsMarkupLive`, and asserts that the field reflects the markup at the exact moment the
      probe first returns true — never before, never long after. Markup-live latency budget:
      **Recast, measured 2026-09-03 (`P4-A6-Recast2-recovered.log`, `MarkupLiveLatency`): 2 frames / 0.0167 s for a
      `Nav.Area.Restricted` paint to read live (min = max = 2 over 5 paints); an impassable paint carves
      its hole in 3 frames / 0.025 s; an unpaint repairs in 2 frames.** (measured on Recast, same fixture).
- [~] **The `UNavArea_Null` fixture vocabulary is fully served.** (2026-09-03, `P4-4D-Cross2-{Crowd,Queue}.log` under a temporary GroundNav default: 8 of the 12 automated sites green on CkGroundNav — 2, 5, 6, 7, 9, 13 through the crossover and 11, 12 native — 3 red on the crowd's Recast-only painter (1, 3, 4) and 1 red on a false arrival past a sealing wall (8) — the fourteen sites are tabled in PROGRESS's 4D block; the crowd-painter sites 1/2/4/5 and the two gyms are the named B2 seam gap for P5/P8, so this banks B2 PARTIALLY.) Every AS test and gym that punches
      a navigation hole today does so through the neutral markup request and is green on
      CkGroundNav. Evidence: the enumerated list of 10+ fixture sites, each with its migrated test
      name and result. This is a promotion prerequisite, not a nice-to-have.
- [x] **Local repair equals partial re-derivation.** (2026-09-03, `P4-4C-Gate3.log` 255/255: `Repair.RepairedFieldMatchesAFullRebake` byte-identical over every array with tile epochs excluded; `Repair.MovedObstacleChangesOnlyWhereItMoved` 3 of 9 tiles, untouched tiles byte-equal incl. epoch; `Repair.SlicedRepairMatchesOneShotRepair` budget 1 vs unlimited identical; PIE `Repair_MovedMarkupBoxChangesOnlyWhereItMoved` columns 0 and 7 never in a changed box.) Evidence, mirroring the in-house volumetric
      pins: repaired field **==** full rebake of the same scene; a moved obstacle changes the field
      **only** where it moved; a sliced (budget-split) repair **==** a one-shot repair.
- [x] **Repair never mutates a published field.** (2026-09-03: `Repair.HeldPreRepairFieldIsUnchanged` — pointer and value copy unchanged across the repair, state spent on release.) Evidence: a test holding a `TSharedPtr<const>` to
      the pre-repair field across a repair and asserting it is unchanged and still self-consistent.
- [x] **Epoch semantics hold.** (2026-09-03, `P4-4B-Final-GroundNav.log` 242/242: `Revision.TileEpochSumRisesOnEveryInterleaving` + `TileCountIsInvariantAcrossRebuilds` + `WorldRevisionDoesNotFallWhenAVolumeIsTornDown`; `NavSurface.RebuiltFiresExactlyOncePerQueuedPublish`; `Invalidation.OnePublishFlagsEachPathAtMostOnce` + `CorridorPlannedAgainstTheCurrentEpochIsNotFlagged`; PIE `Rebuild_InvalidatesWalkingRouteExactlyOnce` = one re-plan per drift, never per frame, fails under the bypass.) Epoch bumps once per completed rebuild; staleness is derived at the
      read boundary; the chunked fingerprint is the epoch sum; a consumer holding a stale epoch
      re-plans exactly once per drift, not once per frame.
- [~] **Moved-obstacle repair latency** vs the Recast baseline — recorded, not claimed as ≤: GroundNav's PIE moved-box pin lands both halves in 19 frames with the volume sliced to ONE tile a tick (18 tiles + the finishing pass); at the default budget the whole repair is a single slice, so the comparable number is probes (134,757 vs 397,539 for a full bake of the 3x3 fixture, `[REPAIR-BUDGET]`). Recast's 2 + 2 frames is a same-frame modifier repaint on a tiny navmesh. ≤ the Recast-measured baseline on the same fixture:
      **Recast, measured 2026-09-03 (`P4-A6-Recast3.log`, `MovedObstacleRepairLatency`): an impassable
      markup box dropped and repainted 600 uu away in one frame — the old spot projects again AND the
      new spot stops projecting in 2 frames / 0.0167 s (min = max = 2 over 5 moves).** Caveat per
      [NN-D67]: the obstacle is a markup box, the fixture suite's vocabulary; a runtime Movable mesh never
      enters Recast's dynamic tile rebuild in this project, so there is no Recast geometry-move number.

#### A7 — Links, PathNetwork, authoring (P5)

- [x] **Nav links traverse** — an agent crosses an authored link and the path accounts for its cost.
      (evidence: [NN-D79], PROGRESS.md:2593 — "A7 evidence — links traverse and are costed (5A/5B
      pins) MET")
- [ ] **CkPathNetwork off-network legs** resolve through the facade; route quality and failure
      reasons unchanged. Evidence: the PathNetwork suite delta-zero.
- [ ] **The segment safety oracle is migrated** — every compiled ribbon segment is proven walkable
      through the facade before install, with the same reject/accept verdicts as the Recast oracle
      on the existing authored networks. Evidence: a verdict-agreement test over the shipped
      networks; disagreements enumerated and individually adjudicated, not summarized.
- [ ] **Editor authoring snap** places nodes on the surface, in-editor, without PIE.
      `[EDITOR-VERIFY]` per §4.3.

#### A8 — Debugger and gym (P6)

- [x] **Value-only snapshot contract.** The snapshot type carries boxes, counts, ids, epochs, and
      status only — never handles, UObjects, worlds, actors, or shared field structures. (evidence:
      [NN-D85], PROGRESS.md:2482-2484 — "value-only asserted at compile time
      (`CkGroundNav_DebugSnapshotTraits.h`, one `static_assert` per captured type + the diagnostics
      fragment) and by the torn-down-producer pins (`Snapshot_OutlivesItsProducer`,
      `Viewport3d.GroundNavFieldCopyOutlivesProducer`) — MET") Evidence:
      a grep of the snapshot header plus a test constructing a snapshot with the producer already
      torn down.
- [x] **Failure is a status, never an empty scene.** Every failure mode renders as an explicit
      status (missing/stale/building/current/failed/runtime-only), and a test asserts each maps to
      its status rather than to an empty draw. (evidence: [NN-D85], PROGRESS.md:2485 — "every
      failure mode a status (the four per-status pins,
      `ACaptureThatIsNotCurrentDrawsOnlyItsStatus`) — MET")
- [x] **Whole-snapshot atomic replace** with cache-identity checked before enumeration. (evidence:
      [NN-D85], PROGRESS.md:2485-2487 — "whole-snapshot atomic replace with cache identity checked
      before enumeration (`FCk_GroundNav_DebugSnapshotCache`, the collector's key-gated copy,
      `Snapshot_CacheReplacesTheWholeValue`) — MET")
- [x] **Draw module is runtime-tier**; the debugger window is DeveloperTool-tier; the editor half is
      Editor-tier. (evidence: [NN-D85], PROGRESS.md:2487-2488 — "tiers — no module added, draw in
      Runtime CkGroundNav, debugger in DeveloperTool CkCrowdDebugger, the gameplay-debugger surface
      in CkTests AS content") Evidence: the `.uplugin` module rows.
- [ ] **Gym registered and functional** — see §2.3 and `[EDITOR-VERIFY]` §4.1.

#### A9 — Persistence and cook (P7)

- [x] **Baked tiles serialize as values** and round-trip byte-identically; stable integer ids survive
      the round trip with no pointer identity anywhere. (evidence: [NN-D92], PROGRESS.md:2384 —
      "`Ck.GroundNav.Serialization.*` green — MET (10 pins, `P7-B2-Final1.log`)"; PROGRESS.md:2390-2391
      — "A9's evidence is complete on the runtime side (serialized values, version gate, cooked
      assets, state-selected fallback)")
- [ ] **Cooked build loads baked tiles** and produces paths identical to the editor bake on the same
      scene. The local DryRun driver and cooked-loader paths are green through Gate82, including
      pure-bake byte equality and atomic profile publication. Actual loaded-from-cook packaged
      Development/Test evidence remains open on the build machine.
- [x] **A stale or missing cook is a status, not a crash** — the field reports its state and the
      provider fails paths cleanly, exactly as an unbuilt region does. (evidence: [NN-D90],
      PROGRESS.md:2409-2419 — `ECk_GroundNav_CookStatus {RuntimeOnly, MissingCook, StaleCook, Cooked}`
      + the five headless load pins `CookedAssets.{LoadingACookedFieldComposesItFromItsTiles,
      AStaleFingerprintIsStaleCook,AnIncompatibleFormatIsStaleCook,AMissingTileIsStaleCook,
      ATruncatedTileBlobIsStaleCook}`; [NN-D92], PROGRESS.md:2390-2391 — runtime side complete)
- [x] Nothing process-relative is persisted. Evidence: Gate107 explicitly ran and passed
      `CkTests.UnitTests.CkGroundNav.Serialization.NothingProcessRelativeIsPersisted`; the row checks
      that pending-clock and wall-time fields remain outside serialized value blobs.

### Tier B — Promotion gate (CkGroundNav becomes the default provider; Recast remains present)

All Tier A gates green, **and**:

- [ ] **B1 — Shadow parity on the full crowd / queue / path-network suites.**
      Success/failure **agreement on every test** (not a percentage — every test). Zero containment
      escapes. Path-length delta within the per-map budget:
      `[MEASURE at phase entry — no estimated numbers]` per map. Evidence: the shadow report of §3,
      one row per map, produced on a full-suite run with shadow mode enabled.
- [ ] **B2 — Fixture vocabulary fully served.** Every crowd obstacle test green on CkGroundNav via
      neutral markup (A6 restated as a promotion condition, because it is the one most likely to be
      partially satisfied).
- [ ] **B3 — Dynamic gates.** Markup-live latency and moved-obstacle repair latency ≤ the
      Recast-measured baseline on the same fixtures:
      `[MEASURE at phase entry — no estimated numbers]`.
- [ ] **B4 — Performance.** Query-per-frame budget adherence at the reference agent count; bake and
      repair cost within budget; no frame-time spike beyond the processor budget. All numbers
      measured on a stated artifact and recorded:
      `[MEASURE at phase entry — no estimated numbers]`.
- [ ] **B5 — Both debugger surfaces at parity** (runtime in-world draw + the crowd-debugger adapter),
      in PIE **and** in a packaged Development build **and** in a packaged Test build. `[EDITOR-VERIFY]`
      §4.2 and §4.4.
- [ ] **B6 — Multi-PIE and teardown clean.** No cross-world leaks, no ensures on world death,
      per-world state proven isolated. §5.
- [ ] **B7 — Rollback proven, not assumed.** Gate97 compiled Recast as the default and ran its broad
      AutoTest set at 939/952. Both previously red PathNetworkFollower fixtures passed after their
      provider-neutral fixture corrections. Timestamp reconciliation showed Gate101's 600 fast polls
      represented only 3.76 wall seconds, before Recast's 5 s deferral and Crowd's 10 s watchdog owe
      a terminal; Gate110 now passes the provider-neutral current-revision Pending contract 1/1 under
      Recast. Gate102 advances Queue past its old no-move failure but does not settle the front member
      within 6.08 s; no third timeout increase was accepted. Gate111 restored GroundNav after the
      unload-tombstone and sublevel manifest-identity fixes, rebuilt, and passed 474/474 GroundNav
      rows. Gate112 then kept all GroundNav rows green in the 1867/1877 full suite; its ten failures
      exactly match established non-GroundNav baseline rows. Recast remains selectable for A/B, with
      the Queue runtime compatibility debt explicit.
- [x] **B8 — CTO sign-off for promotion**, recorded as [S12-D1] in PROGRESS.md.

### Tier C — Retirement gate (Recast deleted) — STRUCK by [S12-D1]

This checklist is retained as historical planning context only. Recast remains selectable for A/B
and shadow comparison; these rows are no longer campaign requirements.

- [ ] **C1 — CkPathNetwork fully migrated** off the Recast safety oracle and the connector-path
      builder; zero remaining calls.
- [ ] **C2 — Editor authoring snap migrated**; the editor half no longer reaches Recast.
- [ ] **C3 — Every `[ADAPTER]` site from R3 is deleted**, one row at a time, with the R3 table
      annotated to zero remaining adapter rows.
- [ ] **C4 — `NavigationSystem` and `AIModule` removed** from every affected `Build.cs`. Evidence:
      `rg -n 'NavigationSystem|AIModule' Plugins/CkFoundation/Source/**/*.Build.cs` → zero hits in
      the affected modules, and the build is green without them.
- [ ] **C5 — The Recast tile-draw debug module is deleted** (retired, not rewritten); CkGroundNav's
      own draw module is its replacement and is the only navigation draw module remaining.
- [ ] **C6 — Repo-wide symbol sweep clean.** `UNavigationSystemV1`, `ARecastNavMesh`,
      `UNavigationQueryFilter`, `UNavArea`, `INavRelevantInterface`, `FPathFindingQuery`, and
      `UNavArea_Null` return **zero** hits across CkFoundation, CkTests, and CkGameplayDebugger
      (`.h/.cpp/.cs/.as`), searched with `--no-ignore`.
- [ ] **C7 — Full-suite delta-zero on the final artifact** after all deletions, with the packaged
      Development and Test builds re-verified (deletion regressions surface in packaged builds
      first).
- [ ] **C8 — Documentation reconciled.** `Source/CkGroundNav/Claude.md` current with its boundary
      paragraph, API, and anti-patterns; `Source/CLAUDE.md` decision-tree and tier rows landed;
      every consumer module's `Claude.md` updated; every campaign doc updated or tombstoned;
      `FEATURE_MATRIX.md` Provenance complete for every shipped feature.
- [ ] **C9 — CTO sign-off for retirement**, recorded as its own numbered decision, distinct from
      B8. Retirement is never granted in the same review as promotion.

---

## 2. Test-layer requirements per feature category

Every feature category below must be covered at **all** of its listed layers. A feature green at one
layer only is not covered.

### 2.1 Layer 1 — Hermetic C++ automation (the math core)

Applies to: rasterization, layer extraction, distance transform / clearance, plate merge, portal
extraction, graph adjacency, funnel string-pull, flood fill, DDA traversal, serialization
round-trip, repair equivalence, epoch arithmetic.

- Location: `Source/CkTests/Private/UnitTests/CkGroundNav/Test_<Subject>_<Scenario>.cpp`.
- **`IMPLEMENT_*AUTOMATION_TEST` macros only** — zero `DEFINE_SPEC`; `.spec.cpp` is a naming
  convention in this repo, not a spec framework.
- **Naming family: `Ck.GroundNav.<Area>.<Scenario>`.** `<Area>` ∈ {`Bake`, `Layers`, `Clearance`,
  `Merge`, `Portals`, `Graph`, `Search`, `Funnel`, `Query`, `Markup`, `Repair`, `Epoch`,
  `Serialization`, `Facade`, `Shadow`}. `<Scenario>` states what is **verified**, never what the
  code does — `RepairedFieldMatchesAFullRebake`, not `TestRepair2`.
- **No ECS, no world, no editor.** The math core is exercised against hand-authored geometry lists,
  following the in-house precedent where a voxelizer is usable with no ECS at all. A math test that
  needs a `UWorld` is a design defect in the math core, not a test that needs a world.
- **Assert counts, not vibes** — probe counts, cell counts, plate counts, portal counts, iteration
  counts, exact clearance values. These are the deterministic budgets.
- **PIE-requiring C++ tests live in separate `*Pie.cpp` files**, following the existing convention.
- A new C++ automation test requires a **touch + relink** to appear; `--generate` alone does not.

### 2.2 Layer 2 — Production-path PIE AngelScript autotests

Applies to: every capability a consumer reaches through the real request/signal seam — path
request → result install, markup request → live probe, projection, containment, provider-episode
lifecycle, revision/cancellation, failure signals.

- One `UCk_AutoTest_Base` subclass per `.as` file, in `Script/CkGroundNav/`. **One class per file** —
  both generators key on that.
- **The test drives the real production API** — the utils facade and the request/signal seam — never
  an internal helper. A test that reaches past the public surface proves nothing about the contract
  the consumers use.
- Timeout via `default _TimeoutSeconds` on the **entity script** (base default 5.0s); the generator
  propagates it to the wrapper CDO. Any instruction to set it on the actor wrapper is stale.
- Bind `OnPathReady` / `OnPathFailed` and assert on the signal, not on a polled snapshot, wherever
  the seam offers a signal.
- **Net-mode tests** (`UCk_AutoTest_NetBase`, or `ServerAndClientsIndependent`) are required for any
  behaviour whose correctness depends on authority. Remember these need a **C++ rebuild** to appear
  at all — an AS recompile is not enough.
- New AS autotests need `--discover-fresh`; a green run whose Total matches the old count is
  stale-green and has not run the new tests.
- **Never rename a test class casually** — a rename orphans the placed wrapper actor in the map.
- Never edit `.as` or source while a run is in flight.

**Settling — named conditions, never fixed hops.** Declare tests as steps
(`Add_Step` / `Add_Step_WaitUntil` / `Run_Steps`, or standalone `WaitUntil(n"Predicate", n"Continue")`).
Before writing any wait, answer the five questions in order:

1. **Would the predicate still be true if the system did nothing?** If yes it is a *negative*
   assertion ("exactly once", "does not fire", "survives X") and cannot become a condition — it
   would pass vacuously. Keep the settle and put a positive assertion before it proving the
   machinery ran.
2. **Does the predicate name MY entities?** All autotests share one PIE world. `Get_Entries().Num() >= 4`
   cannot distinguish this test's four from anyone's four. Scan for the test's own handles or a tag
   private to it. This bites navigation tests hardest, because markup and field state are global.
3. **Which stage does the ASSERTION read?** Gate on that stage. A bake can be live while the plate
   graph the assertion reads is still empty.
4. **Is the value monotonic in the direction needed?** Epochs are; clearance under repair is not;
   a path result can be overwritten by a newer revision. If it is not monotonic, wait on the EVENT.
5. **Has this file been converted before, and how long is the window?** `WaitOneFrame` is **0.05s of
   wall clock, not one frame**. Re-pointing a hop at an already-true predicate silently deletes a
   real wait.

A settle you cannot justify is not a defect to fix — record why you kept it, in the file.

### 2.3 Layer 3 — Gym (interactive, human-verified)

- [ ] **Six CkGroundNav gyms** exist in `Script/CkGroundNav/` — **GroundNav Tuning Range**,
      **GroundNav Walk**, **GroundNav Links**, **GroundNav Dynamic Obstacle**, **GroundNav Markup** and
      **GroundNav vs Recast** — each **registered in
      `Script/Common/CkTests_GymRegistry.as`** via `RegisterProjectGym`.
- [ ] Steps are **CkStateMachine graphs** — one `UCk_Gym_StepState` subclass per step, dwell gated by
      `UCk_Gym_Dwell` — so the HUD highlights the live state and the displayed sequence cannot drift
      from what is running. The superseded `AutoStep % TotalSteps` if-else shape is not used.
- [ ] The gym adopts the **shared control panel**: `Get_ControlRows()` declares rows,
      `Request_ControlActivated()` acts on the index, the HUD draws and polls. Rows are rebuilt each
      frame, so every value column reads back **live state** from whatever owns it — mirrored in a
      member only where there is genuinely no readback, and said so in a comment there.
- [ ] **Reserved keys Tab and H are not bound** by any row. Disabled rows stay visible and muted, so
      readiness can change without reordering activation indices.
- [ ] Minimum control rows: provider select (Recast / CkGroundNav / shadow); field overlay toggle
      (plates, portals, clearance, layers, boundary segments); paint-markup action + clear;
      rebake/repair action; spawn-agents cycle; path-draw toggle; a status row showing epoch,
      tile count, plate count, and provider health.
- [ ] Scenario stations cover, at minimum: flat open ground; a two-story overlap; a ramp; a
      staircase; a narrow gap near the clearance threshold; a moved obstacle; a painted markup
      region; a multi-tile crossing; a no-route failure.
- [ ] `[EDITOR-VERIFY]` walkthrough per §4.1.

### 2.4 Coverage matrix — which layers each category owes

| Feature category | L1 hermetic C++ | L2 PIE AS | L3 gym | `[EDITOR-VERIFY]` |
|---|---|---|---|---|
| Rasterization / layers / clearance / merge / portals | **required** | — | required | §4.1 |
| Search + funnel string-pull | **required** | **required** | required | §4.1 |
| Projection / surface walk / raycast / boundary / reachability | **required** | **required** | required | §4.1 |
| Markup + markup-live probe | **required** | **required** | **required** | §4.1 |
| Repair + epoch observability | **required** | **required** | required | §4.1 |
| Provider episode / revision / cancellation | — | **required** | — | — |
| Links + PathNetwork migration | required | **required** | required | §4.3 |
| Debugger snapshot + draw | **required** | — | — | **§4.2, §4.4** |
| Persistence / cook | **required** | — | — | §4.4 |
| Multi-PIE / teardown | — | **required** | — | §4.5 |

---

## 3. Shadow / A-B parity protocol

Shadow mode dispatches the **same** request to both the Recast adapter and CkGroundNav. The
authoritative provider's result installs; the shadow result is compared, counted, and discarded.
Parity evidence is produced by running suites in shadow mode — **never** by demonstrating a working
scene.

### 3.1 Which suites run in shadow mode

- [ ] The **full CkCrowd suite** — agent movement, containment, steering, block detection, path
      refresh, avoidance sampling, avoidance volumes, obstacle fixtures.
- [ ] The **full CkQueue suite** — formation slot placement and navigation-change retries.
- [ ] The **full CkPathNetwork suite** — off-network legs, segment safety verdicts, corridor compile.
- [ ] The **CkEqs** projection post-pass tests.
- [ ] The **CkGroundNav** L2 autotest family itself (self-consistency under shadow dispatch).
- [ ] The crowd, queue, and path-network **gyms** and existing test maps, run manually under shadow
      mode for the `[EDITOR-VERIFY]` legs.

### 3.2 Metrics recorded per comparison

Recorded for **every** shadow dispatch, aggregated per map and per suite:

| Metric | Recorded as | Promotion condition |
|---|---|---|
| Success/failure agreement | count agree / count disagree, with the **name** of every disagreeing test | **Zero disagreements.** Not a percentage. |
| Fail-reason agreement (when both fail) | count agree / disagree, disagreements enumerated | Disagreements individually adjudicated, not summarized away |
| Path length delta | absolute and relative, per query; per-map mean, p95, max | Within per-map budget `[MEASURE at phase entry — no estimated numbers]` |
| Endpoint delta | distance between the two paths' final points | Within budget `[MEASURE at phase entry — no estimated numbers]` |
| Waypoint count delta | absolute, per query | Recorded; a systematic skew is investigated before promotion |
| Containment escapes | count of frames in which a grounded agent left walkable ground | **Zero.** Any escape blocks promotion. |
| Query time | per query, both providers; per-map mean, p95, max | Within budget `[MEASURE at phase entry — no estimated numbers]` |
| Partial-path agreement | count of queries where one reports Partial and the other does not | Enumerated and adjudicated |

### 3.3 Where the metrics are rendered

- [ ] Counters and per-map aggregates accumulate into a **value-only diagnostics fragment** — plain
      values only, no handles, no UObjects, no shared field structures, so it is snapshot- and
      debugger-safe by construction.
- [ ] The **debugger** renders that fragment: agreement counters, delta distributions, the list of
      diverging test/query identities, and a per-query overlay drawing both providers' paths in
      distinguishable colours so a divergence is visually locatable, not just counted.
- [ ] The full-suite shadow run emits a **shadow report** into PROGRESS.md: one row per map with
      every metric above, plus the artifact identity it ran against. This report **is** the B1
      promotion evidence.

---

## 4. `[EDITOR-VERIFY]` — human-eyes-only checks

Agents cannot launch the editor, PIE, or a packaged build. Each check below is performed by a human
and its result — including the verifier and the artifact — recorded in PROGRESS.md.

### 4.1 `[EDITOR-VERIFY]` — CkGroundNav gym walkthrough

1. Open the gym level, PIE, press **Tab**, and pick the relevant one of the four CkGroundNav
   gyms — **Tuning Range**, **Links**, **Repair**, **Routing** — for the station named in the step below.

   Each gym's first row after its header is a live **Verdict**: a green Verdict is the pass signal for the steps below; a red one names the failing criterion.
2. On the flat-ground station: enable the field overlay. **Expect** plates drawn as merged convex
   regions (not per-cell squares), portals drawn on shared edges, and a plate count in the status
   row far below the cell count. (Tuning Range — the staircase/platform/pillar scene)
3. Cycle the overlay to **clearance**. **Expect** values decreasing toward walls, and visibly
   symmetric around a free-standing pillar. (Tuning Range — the staircase/platform/pillar scene)
4. Cycle the overlay to **layers** on the two-story station. **Expect** two distinct layers over one
   footprint, and that selecting each draws only its own plates. (Tuning Range — the staircase/platform/pillar scene)
5. Spawn agents and issue a move across the ramp station. **Expect** agents to follow the ramp
   surface, height-tracking continuously, with no popping between plate heights. (Routing)
6. On the staircase station: **expect** agents to ascend without stalling at a step edge and without
   the path cutting through the stair riser. (Tuning Range)
7. On the narrow-gap station: cycle the agent radius above and below the gap's clearance. **Expect**
   the wide agent to route around and the narrow agent to pass — the transition happening exactly at
   the clearance threshold, not one cell early or late. (no gym station today — check in the Tuning Range scene)
8. Paint markup over the agents' corridor. **Expect** the status row's epoch to bump, the overlay to
   show the region excluded, agents to re-plan around it **once** (not repeatedly), and the
   markup-live indicator to turn true before the re-plan, never after. (Repair)
9. Clear the markup. **Expect** the epoch to bump again and the corridor to reopen. (Repair)
10. Move the obstacle on the moved-obstacle station. **Expect** the overlay to change **only** near
    the obstacle — surrounding plates visually unchanged — and no frame-time hitch. (Repair)
11. Cross the multi-tile station. **Expect** a continuous path across the tile boundary with no seam,
    no duplicated waypoint at the boundary, and no stall on the crossing frame. (Routing)
12. On the no-route station: **expect** a clean failure with the correct fail reason surfaced in the
    status row, agents holding their previous path rather than teleporting or freezing mid-air. (Routing)
13. **Fail signatures to watch for at every station:** agents sinking below or floating above the
    surface (containment defect); paths hugging walls (clearance not biasing cost); a path crossing a
    portal narrower than the agent (portal minimum-clearance defect); the overlay going empty rather
    than showing a status (failure-as-empty-scene defect); the epoch bumping every frame (repair
    thrashing). (applies across all four gyms)

### 4.2 `[EDITOR-VERIFY]` — debugger visual parity in PIE

1. PIE on a crowd test map with shadow mode enabled.
2. Open the crowd debugger. **Expect** the navigation status panel to report the CkGroundNav
   provider and its health, with surface bounds populated and the viewport fit correct.
3. Enable both providers' path overlays. **Expect** both drawn in distinguishable colours; where
   they agree they should be visually coincident.
4. Open the shadow-parity panel. **Expect** live agreement counters, delta distributions, and a
   list of diverging query identities. Select a divergence. **Expect** the viewport to focus it and
   draw both paths.
5. Enable the field overlay from the debugger (not the gym). **Expect** identical geometry to the
   gym's overlay — the two must not disagree.
6. Kill the field mid-session (trigger a rebuild). **Expect** the debugger to show a *building*
   status, then *current*, and **never** an empty scene at any point.
7. **Fail signatures:** an empty viewport with no status; a stale overlay after an epoch bump; the
   debugger holding a reference that keeps a torn-down world alive; a crash or ensure on closing the
   debugger window while PIE is running.

### 4.3 `[EDITOR-VERIFY]` — editor authoring snap (no PIE)

1. In the editor, with a level containing a baked CkGroundNav field and **not** in PIE, place a path
   network actor and add nodes.
2. **Expect** each node to snap onto the walkable surface at placement, matching the surface height
   within the snap tolerance, on flat ground, on a ramp, and on the upper of two overlapping floors.
3. Drag a node off the surface entirely. **Expect** a clear un-snapped indication, not a silent snap
   to an arbitrary layer.
4. **Fail signatures:** nodes snapping to the wrong layer over an overlap; snap requiring PIE to
   work; an editor hitch on each placement.

### 4.4 `[EDITOR-VERIFY]` — packaged Development and Test builds

Run **twice**: once against a packaged **Development** build, once against a packaged **Test** build.

1. Launch the packaged build on the crowd test map.
2. **Expect** agents to navigate identically to PIE — this is where cook, module tier, and
   editor-only-dependency mistakes surface.
3. **Development build:** confirm the runtime draw module is present and the field overlay console
   toggle draws. Confirm DeveloperTool-tier debugger windows behave per their tier — present where
   the tier permits, cleanly absent where it does not, and **never** a crash or an ensure from a
   missing module.
4. **Test build:** confirm the same navigation behaviour and confirm the tier gating again — a
   module that should be stripped is stripped, and nothing references it.
5. Confirm baked tiles loaded from the cook: paths match the editor bake on the same scene, and no
   runtime bake occurred (status row / log shows loaded-from-cook, not built-at-runtime).
6. Trigger a runtime markup paint. **Expect** it to work in the packaged build exactly as in PIE.
7. **Fail signatures:** navigation working in PIE but not packaged (editor-only dependency); a
   missing-module ensure at boot; a runtime bake in a cooked build; debug draw present in a tier
   that should have stripped it — **Shipping only** ([NN-D80] F8): a packaged Development or Test build MUST draw; drawing absent from a Test build is itself a failure.

### 4.5 `[EDITOR-VERIFY]` — multi-PIE session check

1. PIE with **two or more** clients (or two independent worlds) on a navigation map.
2. **Expect** each world to bake, hold, and query its own field. Paint markup in world A. **Expect**
   world B's field and agents to be entirely unaffected.
3. Trigger a rebuild in world A. **Expect** world B's epoch not to move.
4. End PIE. **Expect** a clean teardown: no ensures, no warnings, no lingering log activity from a
   dead world, and no editor-session leak (repeat PIE start/stop five times and confirm memory and
   log behaviour are steady).
5. **Fail signatures:** shared/global field state (world B changing when A is painted); a deferred
   request from a dead world completing into a live one; an ensure on world death; growth across
   repeated PIE cycles.

---

## 5. Packaging, teardown, and isolation gates

- [ ] **Packaged Development build**: navigation behaviourally identical to PIE; the runtime-tier
      draw module present and functional; DeveloperTool-tier modules behave per their tier. §4.4.
- [ ] **Packaged Test build**: same, with tier stripping confirmed and no reference to a stripped
      module. §4.4.
- [ ] **Module tiers correct in the `.uplugin`**: field/draw = Runtime; debugger window =
      DeveloperTool; editor halves = Editor. Runtime code never depends on an editor-tier module.
      Evidence: the module rows plus a green packaged build (the build is the real proof).
- [ ] **World teardown is ensure-free.** Evidence: an automated teardown test destroying a world
      holding a baked field, in-flight deferred requests, live markup, and an open shadow comparison,
      asserting no ensures and no leaked bindings — plus the §4.5 human check.
- [ ] **No process-wide navigation state.** Evidence: grep for global/static navigation state returns
      only per-world-keyed containers; the deferred queue is per-world (A2); the revision observer is
      per-world.
- [ ] **Multi-PIE isolation** proven both by the automated net/multi-world test and by §4.5.
- [ ] **Repeated PIE cycles are steady** — five start/stop cycles with no growth in tracked field
      allocations and no accumulating warnings.

---

## 6. Definition of done

The campaign is done when:

1. Every Tier A gate is green, with evidence recorded in PROGRESS.md.
2. Every Tier B gate is green and **promotion has its own CTO sign-off**, recorded as a numbered
   decision.
3. Every Tier C gate is green and **retirement has its own, separate, later CTO sign-off**, recorded
   as its own numbered decision. **Promotion and retirement are never signed off in the same
   review.** Promotion says "the new provider is good enough to lead"; retirement says "the old one
   is no longer needed". They are different claims resting on different evidence, and conflating
   them removes the rollback path that makes the whole migration safe.
4. Every feature in `FEATURE_MATRIX.md` has a complete Provenance cell, and every PR that added a
   navigation algorithm named its public references.
5. Every `[EDITOR-VERIFY]` section has a recorded human result naming the verifier and the artifact.

**A green demo path is never parity evidence.** A gym that looks right, a scene where agents walk
nicely, a video, or a hand-picked query proves that one path worked once. Parity is the shadow
report of §3 over the full suites, with agreement on **every** test and deltas inside **measured**
budgets. Anyone offering a working demo in place of that report has not met this gate, and the
reviewer's correct response is to ask for the report.

---

## 7. Closeout order — showcase, matched performance, external acceptance, then BusterBlock

This order supersedes any older text that treats the current `GroundNav vs Recast` gym or isolated
provider measurements as performance acceptance.

### 7.1 Professional visual showcase

Gate119 implements Baseline/Hero/Diagnostic controls and a shared presentation candidate. Final
runtime gates passed GroundNav 523/523 (historical network Iris diagnostic retained) and
PathNetwork 108/108. The seven gyms were run through the Gate119 runbook and the human/showcase
gate was closed by user attestation on 2026-09-10. No image, clip, or log artifact was archived or
is claimed by that attestation; the pre-change baseline was prepared, not rendered. Frozen file/DLL identity is in
`GATE119_PRESENTATION_ARTIFACTS.md`. Its
pre-edit identity and contract are in `GATE119_SHOWCASE_BASELINE_AND_CONTRACT.md`; exact capture
commands, matrix, and historical sign-off criteria are in `GATE119_CAPTURE_RUNBOOK.md`. The
attestation closes the human/showcase decision only; it does not create a rendered baseline or an
archived capture matrix.

- [ ] Pre-change baseline rendered and archived for the six registered GroundNav gyms and the
      PathNetwork gym. **Not claimed:** preparation occurred, but no rendered baseline was archived.
- [x] Give every gym a fixed hero view, readable feature/cause/result, concise caption, consistent
      route/agent/field colors, and a capture-safe presentation state without changing its verdict.
- [ ] Fresh 16:9 hero/diagnostic captures and state-changing clips archived with revision, provider,
      settings, resolution, action, verdict, and log. **Not claimed:** no capture archive was supplied or verified in this session; this is not a request to repeat the approved gyms.
- [x] Human visual review closed by user attestation on 2026-09-10. This is not a substitute for the
      unarchived capture evidence above.

### 7.2 Matched GroundNav-versus-Recast benchmark

Gate123 final passed 5/5 in 57 s with zero skips/contamination; four real reports pass `--require-eligible`, schema is 21/21, and no ensure failures, script errors, Pending Timeout or never-answered warnings occurred. The local authored map, MapContract and native harness are accepted: both `QueryBurst128` rows are 128 Ready; crowd has 240 ready/replanned, zero goal failures/off-surface, provider/debug restored and explicit sample-boundary `Stop240`. GroundNav ends `completions=1, Walking=0, PathPending=239, Idle=1, None=0` in 136 frames; Recast ends `completions=0, Walking=233, PathPending=7, Idle=0, None=0` in 174 frames. This is not convergence, behavioral-equivalence or performance-win evidence. Gate121d's two post-terminal Pending Timeout warnings are historical; Gate123 removed the post-sample live-simulation path. Gate122 PathDiagnostics remains 7/7 with zero ensure/script errors. Provenance: `Saved/Logs/S14-Gate123-MatchedBenchmarkFinal.log`, SHA-256 `4649f895c26e07c940c6298b2a6b83769515ad52823a7322747f142d52f79157`. Build-machine matched pairs, build artifact/run-id provenance and traces remain deferred until BusterBlock integration. Recast remains retained as the A/B provider and rollback path.

- [x] Use one shared authored geometry fixture that supplies valid Recast nav data and a settled
      GroundNav field for the same agent capsule. Dynamic parity-gym geometry alone is insufficient.
- [x] `QueryBurst128`: identical deterministic endpoints; both providers report 128 Ready and zero
      Partial/Failed. Observed latency is end-to-end issue-to-observed-terminal, not pure queue wait or search duration.
- [x] `CrowdConvergence240`: local harness/schema acceptance is complete with explicit `Stop240` at the
      sample boundary and no post-terminal warnings. Its terminal distributions do not imply convergence or equivalence.
- [ ] Every record includes run id, provider, map/fixture hash, build/config, machine/settings, workload,
      behavior eligibility, and raw sample/export location. Missing or non-finite fields reject a run.
- [ ] Collect at least three eligible paired runs on the same Development-game artifact, alternating
      provider order. Debug draw is off. Preserve one Insights trace per provider. Report all runs and
      spread; an ineligible or behaviorally different workload is inconclusive, never a winner.

### 7.3 Remaining editor, real-map, and package acceptance

- [x] Complete §4.1 gym walkthroughs using the current six gym names, plus the PathNetwork six-scenario
      walkthrough. Closed by user attestation on 2026-09-10 after all seven gyms were run; no capture
      matrix or fresh diagnostic-free log archive is claimed.
- [ ] Complete §4.3 PathNetwork snap on flat, ramp, and overlapping floors, including Ctrl+Z/Ctrl+Y,
      no stale connection, and measured editor responsiveness.
- [ ] Measure LiveExtract on representative large static geometry and record before/after timing,
      affected field identity/epoch, undo/redo behavior, and log evidence.
- [ ] On an authored World Partition map, exercise real cell and data-layer load, deactivate,
      reactivate, replacement, unload, and reload while routes are idle, installed, and in flight.
- [ ] Run commandlet DryRun, then the real build-machine write/cook; inspect source manifest, index,
      all profile variants, tile identity, selector, fingerprint/hash, and loaded-from-cook evidence.
- [ ] Exercise the same map and matched benchmark in packaged Development and Test. Compile/package
      Shipping and confirm basic provider selection/load. Local editor evidence does not close these.

### 7.4 BusterBlock migration is the final integration gate

- [ ] Before updating plugins, explicitly pin BusterBlock to Recast and preserve its Recast nav data.
- [ ] Migrate or bridge all eight legacy `Nav.Filter.*` class mappings to provider-neutral definitions.
- [ ] Update plugins, compile, and run the existing BusterBlock Recast regression before GroundNav opt-in.
- [ ] Author one isolated BusterBlock GroundNav map and complete real cook, streaming, pathing, and
      packaged matched A/B acceptance without changing production-world defaults.
- [ ] Migrate one real map only after all prior gates pass. Retain and exercise the Recast rollback
      switch. A missing field, filter mismatch, behavior-ineligible benchmark, ensure, stale callback,
      or package-only divergence is a stop condition.

The step-by-step operator version of this checklist is
`docs/campaigns/navigation-next/GROUNDNAV_ACCEPTANCE_GUIDE.html`.


## 8. Gate124 compatibility and A/B readiness - 2026-09-10

The authoritative BusterBlock continuation prompt supersedes older priority ordering, not acceptance requirements. See `GATE124_COMPATIBILITY_AND_REBASE_PLAN.md` for the concrete scope and preserved backlog.

- [x] Verify both selected roots and actual Foundation/Tests/Toolbox pins, original dirty identities, candidate ancestry and protected hashes before the game baseline. Evidence: `GATE124_ENTRY_EVIDENCE.json`; 60 SHA256 matches plus fixture MD5.
- [x] Capture an unchanged-game focused Recast baseline with named failures. Evidence: `GATE124_RECAST_BASELINE_EVIDENCE.json`, 22/27, exit 1, one boot; this does not satisfy the campaign full-suite/clean-startup gate.
- [ ] Obtain the explicit scoped dirty-work preservation/commit decision and safely rebase feature branches onto refreshed dev without advancing any dev ref.
- [ ] Preserve all eight actual game filters, underlying area costs (including crowd cost 64), Recast area registration and wrong/missing mapping rejection with no unrestricted fallback/partial publication.
- [ ] Preserve exact AccessZone/Entryway policy geometry and owned lifecycle for the selected provider; legacy Recast markup alone is not GroundNav policy.
- [ ] Prove game Recast routes and queue/sidewalk behavior after integration. Carry the fresh LivenessWatchdogAccrual assertion and source-control diagnostics separately from historical Gate102/Gate112 debt.
- [ ] Implement per-run startup choice and requested/effective provider, field/filter/markup readiness, scenario/seed/settings/source/artifact reporting, with bounded explicit ineligibility.
- [ ] Exercise the isolated authored game scenario in Recast -> GroundNav -> Recast runs. Explicitly label retained Recast projections and sidewalk backing; no whole-game provider-equivalence claim.
- [ ] Keep controls-ready, behavior-accepted, package-accepted and performance-measured statuses separate. No startup control or game A/B behavior has been implemented at this entry checkpoint.
- [ ] Retain deferred editor snap/undo/LiveExtract, authored WP/data layers, manifests/DryRun/build-machine cook, Development/Test/Shipping package checks, matched performance, later production-map adoption/rollback, named Recast queue debt, inherited suite failures and unrelated CPU/voice/dirty work. The seven approved gyms remain closed; no local cook/package.
