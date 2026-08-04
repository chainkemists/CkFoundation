# VALIDATION — definition of done for voxelnav-port

> The campaign closes only when every line here is checked with evidence recorded in PROGRESS.md.
> Authored 2026-08-03; extend per phase (each phase's exit criteria append here), never silently.

## Build & regression gate

- [ ] Full toolbox `--build --test` (per `build-test` skill; `--parallel 1`; `--no-nullrhi` for the
      full suite) green on the final artifact of the final phase.
- [ ] Totals delta-zero vs the campaign-open baseline (PROGRESS.md § Baseline) **plus** all new
      CkVoxelNav tests green — every new test named in PROGRESS.md.
- [ ] No new ensures/warnings during the suite (AutoTest harness escalates ck::Warning to failure).
- [ ] Editor boots clean with the module enabled; AS bindings generate without error.

## Functional acceptance (autotests unless marked)

- [ ] A gym scene with authored 3D geometry bakes a voxel octree from the Jolt static world;
      occupancy spot-checks agree with Jolt geometry (parity-sampler pattern).
- [ ] Path found between two free-space points through a 3D obstacle field; path is collision-free
      (octree raycast along every segment reports clear).
- [ ] Refinement pass measurably shortens raw A* paths on the reference scene (recorded numbers).
- [ ] Multi-chunk: path crosses a chunk boundary seamlessly; adjacency survives save/load with
      stable ids (no actor-pointer identity anywhere).
- [ ] Dynamic occluder: moving an occluder dirties + rebuilds affected leaves; a previously valid
      path invalidates and replans.
- [ ] Crowd integration: an agent with the VoxelNav follower feature receives its polyline through
      `FFragment_Nav_PathResult` (InstallExternalPath seam) and walks it via existing steering.
      Covered headlessly by `Ck.VoxelNav.Crowd.Pie.AgentReceivesItsRouteThroughTheNavPathSeam`.
- [ ] Stale-epoch auto-replan: a repair that bumps the volume's epoch under a WALKING voxel agent
      makes the resolver re-issue `Request_FindPath` toward its active goal, exactly once per drift.
      Covered headlessly by `Ck.VoxelNav.Crowd.Pie.StaleVolumeEpochReplansTheWalkingAgentExactlyOnce`.
- [ ] Flying agent: exclusion tags keep ConstrainToNavmesh/planar processors off; the agent tracks a
      climbing 3D route instead of being floor-clamped. Covered headlessly by
      `Ck.VoxelNav.Crowd.Pie.FlyingAgentFliesItsRouteInThreeDimensions` — with one gap that PIE
      automation cannot close: that headless map has **no nav data**, so `ConstrainToNavmesh` passes
      displacement through for grounded agents too and the test can only prove the FACING split. That
      a grounded agent is still surface-clamped **where a navmesh exists**, while a flying one beside
      it is not, is what the manual step below adds.

      `[EDITOR-VERIFY]` — flying vs grounded over real navmesh:
      1. Open any level that has a baked navmesh and open floor space (a CkTests gym level works;
         if the level has no `NavMeshBoundsVolume`, drop one over the floor and let it build —
         the green navmesh overlay under `P` is the confirmation).
      2. Place a `Ck Entity Spawner` (or run from an entity script) that composes, at a spot ~1000uu
         above the floor: `Transform` → `CrowdAgent` with `_AgentMode = Flying` → `Velocity` →
         `Acceleration` → `EulerIntegrator Request Start` → `VoxelNavPath` (`_AgentRadiusUu` 42).
      3. Compose a second agent identically at the same height but with `_AgentMode = Grounded`,
         offset ~300uu sideways so the two do not overlap.
      4. Add a `VoxelNavVolume` whose `_VolumeBounds` covers the floor AND the airspace above it
         (`_FinestCellSizeUu` 50), then `Request Build` and wait for `Get Is Built`.
      5. `Request Set Volume` on BOTH agents' `VoxelNavPath`, pointing at that volume.
      6. PIE. Issue `Request MoveTo` on both agents toward a point on the far side of the volume that
         is ALSO ~1000uu up (over the floor, not on it).
      7. **Expected — flying agent:** leaves the spawn height and holds it, crossing above any
         obstacle between it and the goal; visibly PITCHES nose-up/nose-down through any climb or
         descent in the route; passes over the obstacle rather than around it.
      8. **Expected — grounded agent:** drops to the navmesh surface within the first second (the
         constraint projects it down) and walks the floor for the rest of the run; its pitch stays
         flat throughout. This is the difference the headless test cannot show.
      9. **Fail conditions to watch for:** the flying agent frozen at its spawn point (its
         displacement drain is missing), sinking to the floor (it is still in ConstrainToNavmesh's
         view), or rolling/banking (rotation is being composed as a delta rather than written
         absolutely). Any of the three is a regression in the Flying tag's exclusions.
      10. Console `ck.Crowd.Debug 1` + `ck.Crowd.DrawPlannedPaths 1` draws both agents' installed
          polylines — the flying one's should be visibly airborne along its whole length.
- [ ] Three environments: at least one end-to-end exercise each from C++, Blueprint, and
      AngelScript for the public API (Add volume / request build / request path / read result).

## Perf gate (Phase 5)

- [ ] Merged-vs-plain cell A/B on the reference scene recorded (bake time, cell count, query time).
      No functional test regressions with merging enabled.
- [ ] Bake of the reference gym completes within its budgeted frames without frame-time spikes
      beyond the processor budget (numbers recorded, not vibes).

## Hygiene

- [ ] `rg --no-ignore -l "Jolt/" Source/CkVoxelNav` → zero. CkThirdParty allowlist names CkVoxelNav
      for libmorton only.
- [ ] MIT attribution for Nav3D (Darby Costello) and libmorton present and accurate.
- [ ] `Source/CkVoxelNav/Claude.md` current (boundary paragraph, API, anti-patterns);
      Source/CLAUDE.md tier + decision rows landed; every campaign doc updated-or-tombstoned.
- [ ] All commits on `feature/ckvoxelnav-port` branches (CkFoundation, CkTests); pushes/pointer
      bumps only on explicit maintainer go (recorded decision [C-D6]).
