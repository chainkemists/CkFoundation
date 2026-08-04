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
- [ ] Flying agent: exclusion tags keep ConstrainToNavmesh/planar processors off; agent traverses
      a 3D gym course in PIE — `[EDITOR-VERIFY: open VoxelNav gym, PIE, observe agent fly the
      course without floor-snapping]`.
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
