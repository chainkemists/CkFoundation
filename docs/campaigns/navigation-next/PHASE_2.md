# PHASE 2 — The query surface

> Freshness: authored 2026-08-31 in planning session 1. Status of record is
> [PROGRESS.md](PROGRESS.md); the designs of record are [FEATURE_MATRIX.md](FEATURE_MATRIX.md)
> F1.13–F1.22, [MIGRATION_SEAM.md](MIGRATION_SEAM.md) Step 2, and
> [REPRESENTATION.md](REPRESENTATION.md) requirements 1–4 and 9. Where this file and a design doc
> disagree, the design doc wins and the disagreement is a **STOP**.
>
> **Every code block in this file is illustrative — the executor refines it mechanically, does not
> redesign it.**

---

## 1. Goal

Implement every non-search ground query against the published field — projection, is-navigable,
constrained surface walk, walkability raycast, boundary segments (with the off-game-thread
contract), closest boundary edge, reachability, surface attributes, point generation, and build
status — landing **F1.13–F1.22**, and register them as the CkGroundNav side of the PHASE_0 facade.

Two properties are non-negotiable in this phase and cheap to lose: **`Unbuilt` never collapses into
`NoSurface`**, and **the constrained walk never returns a position off the walkable set**.

---

## 2. Entry criteria

- [ ] **PHASE_1 closed in PROGRESS.md** with its numbers recorded (collapse ratio, merge tunables,
      probe counts, bake cost).
- [ ] Repo clean, on a `feature/` branch (root + CkFoundation + CkTests) — same commands as PHASE_1 §2.
- [ ] **The bake still produces what this phase queries**:
      ```powershell
      Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
        --test-pattern GroundNav --parallel 1 --output=Saved/Logs/BuildTest.log `
        --project="D:\Repos\CkPlugins_3"
      ```
      Pre-flight: the editor must be closed for this project (the toolbox exits **77** if it is
      open) — see the `build-test` skill's pre-flight table.
      → PHASE_1's `Ck.GroundNav.*` family green at its recorded Total. A lower Total than PHASE_1
      recorded means tests were lost ⇒ **STOP**.
- [ ] **The PHASE_0 facade is intact and provider-neutral**:
      ```powershell
      rg --no-ignore -n "Try_ProjectPoint|Try_MoveAlongSurface|Try_SurfaceRaycast|Get_BoundarySegments" `
         "Plugins\CkFoundation\Source\CkNavigation\Public"
      ```
- [ ] **Recast-agreement fixtures exist or are authored first.** Each capability below owes a
      hermetic correctness test **and** a Recast-agreement test on a shared fixture (VALIDATION.md
      A4 preamble). If the shared fixtures do not exist yet, authoring them is step 0 of this phase,
      not an afterthought.
- [ ] **Agreement budgets measured, not guessed.** Every `[MEASURE at phase entry]` budget in
      VALIDATION.md A4 is filled in **now**, by measuring the *Recast* path on the *same* fixture,
      and recorded in PROGRESS.md with the artifact it was measured on.
- [ ] **Phase-entry baseline captured this session** (full suite, `--parallel 1 --no-nullrhi`,
      totals + failing-test names + artifact identity in PROGRESS.md).
- [ ] Skills loaded: `ck-macros-and-codegen`, `ckecs-architecture-contract`,
      `ck-tests-authoring-and-running`, `build-test`, `ck-change-control`.

---

## 3. Standing decision gate (applies to EVERY `-> verify:` line in this file)

- Observation matches the stated expectation → continue.
- Compile/UHT/link failure → fix mechanically and re-verify. **Two failed attempts → STOP.**
- A hermetic test fails → the implementation is wrong; the assertion is the specification. Do not
  loosen a tolerance to pass. Two failed attempts → STOP.
- A **Recast-agreement** test fails outside the measured budget → **STOP** and record both numbers
  and the fixture. Widening an agreement budget is a promotion-gate change and belongs to the
  orchestrator. (Recast is the oracle for *agreement*, not for *correctness*: where the hermetic
  test and the agreement test disagree, record both and STOP — do not pick one.)
- A previously green test is now red → **STOP**, revert the offending step, diagnose, re-sequence.
- **Anything else → STOP, record it in PROGRESS.md § Blockers with verbatim evidence, end session.**

---

## 4. Sub-phase 2A — Projection, is-navigable, surface attributes, build status (F1.13, F1.14, F1.20, F1.22)

**2A entry:** §2 all green.
**2A exit:** projection's success and failure cases pinned, including `Unbuilt` ≠ `NoSurface`;
batch equals N singles element-for-element.

### Steps

1. Implement point projection (F1.13): quantize the query to tile + cell, walk candidate layers'
   spans within the ±Z extent, expand outward in rings within the XY extent, reject cells whose
   clearance is below the agent radius. Modes: **closest**, **down-only**, **up-only**. Tie-break is
   **horizontal-first** — a point 10 uu sideways beats one 50 uu below, which is the behaviour crowd
   containment depends on. Half-extents come from the provider-neutral setting ([NN-D8e]).
   Illustrative math-core signature (the facade UFUNCTION already exists from PHASE_0):

   ```cpp
   namespace ck::groundnav
   {
       auto
       Get_ProjectPoint(
           const FCk_GroundNav_Field&           InField,
           const FCk_GroundNav_ProjectionQuery& InQuery) -> FCk_GroundNav_ProjectionResult;

       auto
       Get_ProjectPoints_Batch(
           const FCk_GroundNav_Field&                    InField,
           TConstArrayView<FCk_GroundNav_ProjectionQuery> InQueries,
           TArrayView<FCk_GroundNav_ProjectionResult>     OutResults) -> void;
   }
   ```
   The batch variant amortizes tile lookup across sorted queries; it does not have its own semantics.
   → verify: Layer 1 — a point 200 uu above a floor projects to the floor with the correct normal; a
   point inside a wall with floors both sides projects to the horizontally nearer one; a point over a
   hole with the search box smaller than the hole returns `NoSurface`, larger returns the hole's rim;
   a query over an **unbuilt** tile returns `Unbuilt` and never `NoSurface`; batch results are
   element-wise identical to N single calls.

   **Gate 2A-1.**
   - All five cases pass and the batch is element-wise identical → continue.
   - Batch and single disagree on any element → the batch has acquired its own semantics. Fix so the
     batch is an amortization only. Two attempts → STOP.
   - `Unbuilt` and `NoSurface` are indistinguishable at any call path → **STOP**. Their conflation is
     the exact defect the neutral status enum exists to remove, and it is invisible until PHASE_3's
     deferral behaviour goes wrong.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

2. Implement is-navigable (F1.14): the tile→layer→cell lookup without ring expansion, plus the
   clearance predicate. Single + batch.
   → verify: Layer 1 — agreement with projection-at-zero-extent on 10k random points over a fixture,
   and a **measured** cost strictly below projection on the same fixture, recorded in VALIDATION.md.

3. Implement surface attributes at a point (F1.20): surface normal, area-tag set, cost multiplier,
   owning plate/layer id, single + batch, reading **plate-level** attributes (which is why the plate
   decomposition stores policy at plate level rather than per cell).
   → verify: Layer 1 — attributes inside a painted region match the paint, one cell outside do not,
   batch equals N singles. (Painting itself is PHASE_4; use a bake-time authored area tag here.)

4. Implement build status (F1.22): `Get_IsBuildInProgress(world)`, `Get_IsBuilt(point)`,
   `Get_IsBuilt(bounds)`, `Get_SurfaceBounds(world)`, `Get_ProviderHealth(world)`. Health returns a
   **status enum, never a bool**, and **failure is a status, never an empty answer** — the same rule
   the in-house volumetric debug snapshot enforces.
   → verify: Layer 1 — status transitions `Unbuilt → Building → Built` exactly once per tile per
   build; a point in an unbuilt tile reports `Unbuilt` **consistently across F1.13–F1.21**.
   Layer 2 — a PIE autotest waits on the named condition "surface built", never on a hop count.

5. Gate 2A: `--test-pattern GroundNav`.
   → verify: Total rose by the tests added; all green.

---

## 5. Sub-phase 2B — Constrained surface walk and walkability raycast (F1.15, F1.16)

**2B entry:** 2A exit green.
**2B exit:** **zero** end positions off the walkable set across 10k randomized moves. This is a hard
zero — a containment escape is a promotion blocker.

### Steps

1. Implement the constrained surface walk (F1.15). This is the **single Transform writer for grounded
   agents**: every grounded agent's final position each frame will pass through it. A DDA walk over
   the cell grid from start toward target, clipping at the first cell failing the walkable+clearance
   predicate, projecting the residual motion onto the blocking edge (slide), continuing under a
   **bounded iteration count**. Plate/portal structure gives the early-out: a move staying inside one
   plate's rectangle and above its clearance is admitted **without stepping cells at all** — the
   common case for a crowd agent's per-frame delta. Illustrative:

   ```cpp
   namespace ck::groundnav
   {
       auto
       Get_MoveAlongSurface(
           const FCk_GroundNav_Field&                InField,
           const FCk_GroundNav_SurfaceWalkQuery&     InQuery,
           FCk_GroundNav_SurfaceWalkDiagnostics&     OutDiagnostics) -> FCk_GroundNav_SurfaceWalkResult;
   }
   ```
   `OutDiagnostics` carries the instrumented counters the tests assert on (cells stepped, slide
   iterations, early-out taken) — values only.
   → verify: Layer 1 — 10k randomized moves over a fixture with walls, holes and a multi-storey
   overlap assert **zero** end positions off the walkable set; a move into a concave corner
   terminates within the iteration bound; a move fully inside one plate takes the early-out path
   (assert on the instrumented counter).

   **Gate 2B-1.**
   - Zero escapes over 10k moves, corner terminates, early-out counter fires → continue.
   - **One or more escapes** → **STOP** immediately. Do not add a post-hoc clamp, a re-projection
     "safety net", or an epsilon. A containment escape is a defect in the walk, and a clamp that
     hides it converts a visible failure into an invisible one. Record the escaping move's start,
     target, and result verbatim.
   - The concave-corner case hits the iteration bound instead of terminating → the slide is
     oscillating; the bound is the guard, not the fix. Two attempts → STOP.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

2. Implement the walkability raycast (F1.16): the same DDA walk **without the slide** — the first
   failing cell is the hit. The plate-rectangle early-out applies again: a segment contained in one
   plate is clear by construction. The optional cost cap accumulates per-cell area cost multipliers
   as it walks, which is what gives CkPathNetwork's "walkable *and* cheap enough" question a direct
   answer.
   → verify: Layer 1 — a segment across an open plane is clear; a segment through a wall is blocked
   with the hit within one cell of the analytic wall plane; a segment along a corridor narrower than
   the agent radius is blocked; results are symmetric in A/B for the clear case and hit positions
   agree within a cell for the blocked case; a ray along a layer boundary **does not leak between
   layers**.

3. Recast-agreement pass for both. Run the shared-fixture agreement tests against the budgets
   measured in §2.
   → verify: endpoint agreement for the walk and hit-point agreement for the raycast are inside the
   recorded budgets. The walk's budget is the tightest in the campaign — treat a marginal pass as a
   number to record, not a result to round.

4. Gate 2B: `--test-pattern GroundNav`.
   → verify: all green; the containment 10k assertion is in the run and did not skip.

---

## 6. Sub-phase 2C — Boundary segments, closest edge, and the thread contract (F1.17, F1.18)

**2C entry:** 2B exit green.
**2C exit:** the Layer-1 concurrency stress test below passes — every off-thread result is
self-consistent with the epoch it captured, with zero crashes and zero ensures.

### Steps

1. Precompute boundary segments **at bake time** as part of plate extraction: every plate edge not
   covered by a portal is a boundary edge, stored per tile in a flat segment array with a coarse
   spatial index, with **consistent winding established once at bake, not per query**. (This is a
   PHASE_1 artifact reached back into; it is in this phase because its consumer is the query. Adding
   it does **not** license any other bake change here.)
   → verify: `Ck.GroundNav.Bake.*` still bit-identical for field values; the new segment array is
   deterministic across 100 runs.

2. Implement the boundary-segment query (F1.17) as a bounded index scan against a
   `TSharedPtr<const>` field, writing into caller-provided storage — **no locks, no allocation from a
   shared pool**. Illustrative, with the contract stated in the header where a caller reads it:

   ```cpp
   namespace ck::groundnav
   {
       // THREAD CONTRACT: safe to call from any thread. Operates entirely on the immutable field
       // snapshot the caller already holds; writes only into OutSegments. Holding the snapshot is
       // the caller's guarantee of self-consistency — a concurrent republish cannot affect it.
       auto
       Get_BoundarySegments(
           const FCk_GroundNav_Field&                   InField,
           const FCk_GroundNav_BoundaryQuery&           InQuery,
           TArray<FCk_GroundNav_BoundarySegment>&       OutSegments) -> ECk_NavSurface_QueryStatus;
   }
   ```
   → verify: Layer 1 — segments around a point in an open room match the room's walls within a cell;
   winding is consistent (run an interior test on **every** returned segment); the cap truncates
   deterministically, nearest-first.

3. The threading pin — this is the requirement the whole representation was selected to make free.
   **There is no sanitizer target in the toolbox; do not reference one.** The mechanism is a
   **Layer-1 concurrency stress test** plus the existing Layer-1 **thread-contract test**:

   - the game thread publishes and repairs the field for **M = 1000** iterations;
   - **N = 8** worker tasks concurrently run boundary, projection, and raycast queries, each against
     a field snapshot it captured before querying;
   - every result is asserted **self-consistent with the epoch its snapshot carries** — no torn
     reads (a result mixing data from two publishes), no crashes, no ensures;
   - the thread-contract test separately asserts that no off-thread entry touches game-thread-only
     state.

   → verify: Layer 1 — the stress test above completes with every result epoch-consistent, and the
   thread-contract test green.

   **Gate 2C-3.**
   - Every result epoch-consistent, no crash, no ensure → continue.
   - A result is self-consistent with **no** epoch (mixed data from two publishes) or the run crashes
     → find what the query reads outside the snapshot it holds (a cached pointer, a shared scratch
     buffer, a lazily built index). Adding a lock is **not** the fix: the design's claim is that no
     lock discipline is needed, so a lock here means the query violated the immutable-snapshot
     contract. Two attempts → STOP.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

4. Implement closest boundary edge / closest point on boundary (F1.18) over the same precomputed
   array with an expanding-ring nearest search. No new bake stage.
   → verify: Layer 1 — agreement with a brute-force nearest-segment reference on 10k random points;
   the returned closest point always lies **on** the returned segment.

5. Migrate the parallel avoidance consumer. `CkCrowdAgent_AvoidanceSample_Processor` calls the facade
   from its `TParallelProcessor` **unchanged in structure**.
   → verify: Layer 2 — crowd avoidance autotests green with the parallel processor unchanged
   (VALIDATION.md A4 boundary bullet + A2 off-thread bullet).

6. Gate 2C: `--test-pattern Crowd` then `--test-pattern GroundNav`.
   → verify: both green; crowd totals unchanged from the §2 baseline.

---

## 7. Sub-phase 2D — Reachability and point generation (F1.19, F1.21)

**2D entry:** 2C exit green.
**2D exit:** cross-island reachability rejects with an expansion counter of **zero**.

### Steps

1. Implement `Get_IsReachable` (F1.19) as a component-label array compare — near-O(1), answering
   *definitely unreachable* vs *possibly reachable*, with the clearance limitation from F1.9
   restated in the API contract comment.
   → verify: Layer 1 — on a two-island fixture, cross-island reachability is false with an
   **asserted expansion counter of zero**. An assertion on the counter, not on timing.

2. Implement F1.19's flood-fill core: Dijkstra-style priority-queue expansion over the plate-portal
   graph where each expanded plate carries its funnel state, so the recovered distance is the **true
   string-pulled distance**, not a portal-centre sum. A pluggable early-exit predicate bounds it.
   One-to-many variants answer "which of these 50 destinations are reachable and how far".

   **The funnel machinery is shared verbatim with F1.24 (PHASE_3) — one implementation, two
   consumers.** Author it here so PHASE_3 consumes it; do not let PHASE_3 write a second one.
   → verify: Layer 1 — flood-fill distances agree with independently computed path lengths **within
   one finest cell** on 100 sampled destinations; the early-exit predicate provably bounds expansion.

   **Gate 2D-2.**
   - Distances agree within one finest cell and the early exit bounds expansion → continue.
   - Distances are systematically longer than the independent reference by roughly a portal-centre
     detour → the funnel state is not being carried through expansion; that is the difference between
     true and approximate distance, and F1.21's distance-range generator depends on it. Fix. Two
     attempts → STOP.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

3. Implement the three point generators (F1.21): (a) random points in radius, (b) random points by
   **path-distance range** — the query EQS actually wants, which a radius cannot express — and
   (c) grid of points in radius/bounds with an optional grid-phase alignment flag. (a) and (c) sample
   plate rectangles **weighted by walkable area**, rejecting cells under the clearance threshold;
   (b) runs the step-2 flood fill with the distance range as its early-exit predicate then samples
   within admitted plates. RNG is caller-seeded; results are reproducible for a given seed + field
   epoch.
   → verify: Layer 1 — 100k random points are all on walkable ground with a spatial distribution
   uniform across plates under a **chi-square uniformity test, rejecting at p < 0.01 over >= 10,000
   samples**; the same seed + epoch reproduces the same
   set; grid-phase alignment produces identical lattice positions for two overlapping query bounds;
   distance-range points all verify against an independent path query.

4. Register every CkGroundNav query behind the PHASE_0 facade, and add the facade-equivalence tests.
   → verify: Layer 1 — the facade returns identical results to direct provider calls for every
   capability (VALIDATION.md A2, first bullet, now satisfied for the second provider too).

5. Phase gate: full suite on the phase's **final** artifact.
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
     --parallel 1 --no-nullrhi --output=Saved/Logs/BuildTest.log --project="D:\Repos\CkPlugins_3"
   ```
   → verify: delta-zero vs the §2 baseline plus the phase's new tests green.

---

## 8. Exit criteria

- [ ] Every capability F1.13–F1.22 has **both** a hermetic correctness test and a Recast-agreement
      test on a shared fixture, each named in PROGRESS.md.
- [ ] **Containment: zero** end positions off the walkable set over 10k randomized moves.
- [ ] **Thread contract:** the Layer-1 concurrency stress test (8 worker tasks querying against
      captured snapshots across 1000 publish/repair iterations) reports **every result
      epoch-consistent**, with zero torn reads, zero crashes and zero ensures; the Layer-1
      thread-contract test is green; the parallel avoidance consumer is unchanged.
- [ ] `Unbuilt` never returned as `NoSurface` and vice versa, asserted at every query entry point.
- [ ] Reachability rejection asserted on an **expansion counter of zero**, not on elapsed time.
- [ ] One funnel implementation exists (grep evidence): the flood fill and PHASE_3's string-pull
      share it.
- [ ] All A4 `[MEASURE at phase entry]` budgets filled in with measured numbers plus the artifact
      they were measured on; the is-navigable-vs-projection cost delta recorded.
- [ ] Three environments exercised for every new public API: C++, Blueprint, AngelScript.
      **Carve-out:** `Get_BoundarySegments` — and any other function carrying the off-thread
      thread contract — is **C++-only by contract**. Blueprint and AngelScript get a
      game-thread `UFUNCTION` wrapper; the three-environments rule applies to that wrapper.
      The off-thread C++ entry is exercised by the Layer-1 thread-contract test instead.
- [ ] Full suite **delta-zero** vs the §2 baseline on the phase's final artifact; zero new ensures,
      zero new warnings; editor boots clean. **Deferred by [NN-D54]: no full-suite run until every
      phase is done; scoped `--test-pattern` gates stand in, and the one full suite runs at campaign
      end against the P2 entry baseline.**
- [ ] Provenance cells complete for F1.13–F1.22.
- [ ] Comment audit run; PROGRESS.md updated; PHASE_3 entry criteria re-verified.

---

## 9. Fences

- **No search.** A* over the plate-portal graph, string-pulled *paths*, partial paths, cost model,
  and install are PHASE_3. The flood fill here is a query, not a path service, and it must not grow
  a waypoint-returning API.
- **No markup, no repair.** PHASE_4. Area tags queried here are bake-time authored.
- **No clamp, epsilon, or re-projection band-aid on containment.** If the walk can leave the walkable
  set, fix the walk. A hidden escape is worse than a visible one and blocks promotion either way.
- **No locks in the query path.** The immutable snapshot is the concurrency design. A lock means the
  contract was violated somewhere upstream.
- **No second funnel implementation.** F1.19 and F1.24 share one.
- **No batch variant with its own semantics.** Batches amortize lookup; they never change answers.
- **Do not widen a measured agreement budget** to make a test pass. That is a promotion-gate change.
- **Do not change the bake** beyond adding the precomputed boundary-segment array (2C step 1). Any
  other bake change is PHASE_1 scope reopened → STOP.
- **`CK_REGISTER_PROCESSOR` on every processor added.**
- **Settle on named conditions, never hop counts**, and make every predicate name *this test's own*
  entities — field and build state are global to the shared PIE world.

---

## 10. [P2] Done means

VALIDATION.md **A4** (query surface) is green with evidence in PROGRESS.md, the off-thread and
per-world bullets of **A2** are green for the CkGroundNav provider as well as the Recast one, every
VALIDATION.md §0 standing gate passes on the phase's final artifact, and the §2.4 coverage row
"projection / surface walk / raycast / boundary / reachability" has its Layer-1 requirement
satisfied and its Layer-2 requirement started via the migrated crowd consumers.
