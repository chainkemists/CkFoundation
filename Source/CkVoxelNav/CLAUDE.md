# CkVoxelNav

**Boundary — read this before anything else.** CkVoxelNav owns *volumetric* (free-space) 3D
navigation: a Sparse Voxel Octree of the open air inside an authored volume, and pathfinding
through it for agents that are not stuck to a surface — flying, swimming, zero-g. It is **not** a
replacement for **`CkNavigation`**, which wraps engine Recast and stays the stack for walking
agents projected onto a navmesh; the two coexist and a game may run both. It is **not**
**`CkSpatialQuery`**, which answers *"which entities are inside/near this shape right now"* with
live Jolt probes — CkVoxelNav consumes world geometry once to bake a static structure and answers
*"is this space free"* / *"how do I fly from A to B"*; it registers no probes and holds no bodies.
It does **not** implement a search: **`CkAStar`** owns the budgeted, warm-startable A* core, and
CkVoxelNav supplies a graph adapter over its octree plus a post-search refinement pass so the
search runs on CkAStar unchanged.

All world-geometry queries go through **`CkJolt`**'s public, JPH-free query surface. CkVoxelNav
includes **no** Jolt headers — that is a hard invariant, not a preference (see Anti-patterns).

**Purpose:** bake and query a Sparse Voxel Octree of navigable free space over an authored volume,
and plan paths through it at horde scale.

**Depends on:** `CkAStar`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkJolt`, `CkLabel`, `CkLog`,
`CkNavigation`, `CkProfile`, `CkRecord`, `CkSettings`, `CkThirdParty`.
**Used by:** nothing yet.

---

## Key API

- `UCk_Utils_VoxelNavVolume_UE` (inherits `UCk_Utils_Ecs_Base_UE`) — `Add` composes the volume
  feature onto an owner as a child entity; `Has` tests for it; `Cast` / `CastChecked` convert a
  handle. `Add` stamps `ck::FTag_VoxelNavVolume_NeedsSetup`, which
  `ck::FProcessor_VoxelNavVolume_Setup` consumes; that processor validates the params and arms
  `FTag_VoxelNavVolume_NeedsBuild` unless `_AutoBuildOnSetup` opted out.
- `FCk_Fragment_VoxelNavVolume_ParamsData` — `_VolumeBounds` (world-space `FBox`),
  `_FinestCellSizeUu` (the finest navigable cell's **edge length** in uu, not a half-extent),
  `_ClearanceUu` (added to every probe half-extent — grows obstacles, shrinks free space),
  `_AutoBuildOnSetup`, and a per-volume budget override pair.
- `Request_Build` / `Request_CancelBuild` — deferred, completion delegate last. The build's
  completion delegate fires when the BUILD ends, not when the request is accepted: the delegate is
  carried on the build-state fragment across every frame the bake spans.
- Queries: `Get_IsBuilt`, `Get_BuildEpoch`, `Get_BuildStage`, `Get_BuildProgress`, `Get_BuildStats`,
  `Get_NumLayers`, `Get_IsPointFree`. Signal: `BindTo_OnBuildComplete` (fires on BOTH outcomes and
  carries an `ECk_SucceededFailed`).
- `ck::voxelnav::FBuildState` + `Request_AdvanceBuild` (`Octree/CkVoxelNav_Octree_Build.h`) — the
  resumable voxelizer, usable with no ECS at all. Everything it learns about geometry arrives
  through `ICk_VoxelNav_GeometryBackend`, so a whole bake runs against a hand-authored box list.
- `ck::voxelnav::Raycast` / `Get_IsSegmentBlocked` (`Octree/CkVoxelNav_Octree_Raycast.h`) — Revelles
  parametric traversal of a built octree, reporting the first occluded cell a segment enters
  (distance, impact point, entered cell face, cell address).

### Pathfinding

- `UCk_Utils_VoxelNavPath_UE` — `Add` stamps the feature **on the agent entity itself**, not on a
  child: a path belongs to the thing flying it, and every consumer that reads waypoints already
  holds the agent's handle. `FCk_Fragment_VoxelNavPath_ParamsData` carries what belongs to the
  agent (`_AgentRadiusUu`, `_HeuristicScale`, `_NodeSizeCompensation`); the query itself rides the
  request.
- `Request_FindPath(volume, from, to)` — deferred, completion delegate last. The search is
  **synchronous inside the drain** and bounded by an iteration cap (project setting
  `_MaxPathSearchIterations`), so a request made on one tick is answered on the next: the delegate
  reports `Succeeded` only when waypoints are readable, and every rejection reports `Failed` and
  fires `OnPathFailed` carrying an `ECk_VoxelNav_PathSearchOutcome` that names WHICH rejection
  (`NoBake`, `EndpointUnresolvable`, `Unreachable`, `IterationCapReached`, `InvalidRequest`) —
  because each one asks the caller for a different repair.
- **Refinement knobs ride the request** (`_VisibilityPruning` default ON, `_Smoothing` default OFF,
  `_SmoothingSubdivisions`). Pruning drops every waypoint the octree ray-marcher proves redundant;
  smoothing subdivides what survives with a centripetal Catmull-Rom, span by span, keeping a curve
  only where the octree agrees it stays in free space. A span that fails reverts to its straight
  segment, so the worst case of smoothing is the polyline it was handed. Full contract:
  `Path/CkVoxelNav_Path_Refine.h`.
- **The stored waypoints ARE the refined output** — the raw cell-centre path is not kept.
  `Get_RawWaypointCount` / `Get_RefinedWaypointCount` / `Get_PathLengthUu` are what a caller (or a
  test) reads to see what refinement bought.
- **`Stale` is DERIVED at the read boundary, never stored.** `Get_Status` compares the volume's
  current build epoch against `Get_PlannedAgainstEpoch`; a volume that is gone reads as stale too.
  Storing it would mean every rebuild had to walk every path that ever planned against it.
- `ck::voxelnav::Search_PathGraph` (`Path/CkVoxelNav_Path_Graph.h`) — the ECS-free search seam over
  a published octree, and `FPathGraph`, the `astar::AStarGraph` adapter it runs on (`static_assert`
  pinned). `FPathGraph` holds a RAW pointer to the octree and is valid for exactly one synchronous
  search inside one tick; its caller holds the `TSharedPtr` that keeps the structure whole.

### Dynamic occluders and local repair

- `UCk_Utils_VoxelNavOccluder_UE` — `Add` stamps the feature **on the moving entity itself** (it is a
  reading of that entity's transform, so the entity must already carry `Transform`).
  `FCk_Fragment_VoxelNavOccluder_ParamsData` carries authored `_HalfExtentsUu` plus an optional
  movement-threshold override; `Get_TrackedBounds` / `Get_TimesDirtied` are what a caller or a test
  reads. There is no component and nothing ticks per actor: one processor walks every occluder.
- **An occluder does not occlude.** Occupancy comes from the geometry backend, so a tracked entity
  only blocks navigation if it has geometry the bake can see — and CkJolt's occupancy surface is
  filtered to the **Static** body domain, so a **Kinematic** JoltBody is invisible to voxelization
  entirely. Moving a Static body is what "dynamic occluder" means here. The feature's job is to say
  WHEN and WHERE the volumes must look again.
- `UCk_Utils_VoxelNavVolume_UE::Request_MarkDirty` — the volume-side entry point. Regions arriving
  before a repair starts are UNIONED into one repair. A volume that has never baked reports `Failed`:
  a local repair carries the occupancy of everything OUTSIDE the dirty region over from the previous
  bake, and there is none to carry. `Get_RepairStage` / `Get_RepairStats` /
  `Get_PendingDirtyBounds`; signal `BindTo_OnRepairComplete` (distinct from the build signal, so a
  listener waiting for the FIRST bake is not woken by every obstacle that moves).
- `ck::voxelnav::FRepairState` + `Request_BeginRepair` / `Request_AdvanceRepair` /
  `Request_ReleaseRepairedOctree` (`Octree/CkVoxelNav_Octree_Repair.h`) — the resumable repair, ECS-free
  in the same way the voxelizer is. `Get_CellMortonsIntersectingBounds` is its dirty-box → cell math.
- **The local-repair contract**, test-pinned in `Ck.VoxelNav.Occluder.Repair.*`: *occupancy is
  re-probed only for cells whose inflated probe box intersects the dirty bounds — every other cell
  keeps the occupancy the last bake or repair recorded for it, bit for bit; a repair never mutates the
  octree it repairs, it assembles a whole new one and swaps it in; and structure is DERIVED from the
  updated blocked-cell set by the bake's own stages, never patched in place.* A repair therefore lands
  field-for-field where a full rebake of the moved scene would, for a fraction of the probes.

---

## Voxelization — the parts that look wrong until you know why

- **Occupancy is decided by COLLISION shapes, not render geometry.** Nav3D filtered candidates on
  simple collision but decided occupancy on LOD0 render triangles; Jolt unifies both onto the baked
  collision shape. Baked results therefore **differ from Nav3D's by design** — usually more correct,
  always different.
- **Probe count is the primary budget; wall-clock is only a guard.** A probe count is deterministic,
  so a test can assert a bake's exact cost and catch a regression in the hierarchy's pruning. A
  time-only budget would make that count machine-dependent and every such assertion flaky.
- **The build processor does not use `ck::RunPacedSteps`.** That primitive is built on the pump
  mechanism and refills its budget on `DeltaT > 0`, which would put probe batches in pump passes
  whose position relative to `FProcessor_JoltWorld_Step` is not pinned. A plain per-tick budget loop
  inside `ForEachEntity` is both simpler and provably inside the safe window — do not "fix" it.
- **The build and start processors sit in `FGroup_Transform`, after
  `FProcessor_JoltWorld_WaitForAsync` and before `FProcessor_JoltWorld_Step`.** That is the only
  window provably outside the async physics step. Anything later queries Jolt concurrently with the
  task-graph update whenever `jolt.EnableAsyncPhysicsUpdate` is on.
- **There are TWO segment tests and picking the wrong one is a silent mistake.** The octree
  ray-marcher answers "does the BAKE block this segment" — no physics query, agent clearance already
  baked in, resolution capped at a leaf sub-node — and it is the one pathfinding wants, because it
  asks the same structure the search planned through. `ICk_VoxelNav_GeometryBackend::Get_IsSegmentBlocked`
  answers "does PHYSICS block it" against the live world: exact shapes, finer than any cell, one
  narrowphase query each. Use it to validate a bake or for a tactical query, never to prune a path.
- **A repair rebuilds the octree's STRUCTURE rather than editing it.** Node identity here is
  `(layer, INDEX)`: parent, child and all six neighbour links store array positions, and every layer is
  Morton-sorted because lookups binary-search it. Upstream's dynamic path swap-removed and appended
  layer-0 nodes in place and never re-sorted layer 0, so the first obstacle movement invalidated every
  link and every search in the octree. Deriving structure from the blocked-cell set makes that class of
  corruption unrepresentable — at the cost of an O(existing nodes), probe-free pass per repair.
- **A repair's leaf occupancy is REPLACED, never accumulated.** `FLeafNode::Set_SubNodes` exists for
  exactly this: upstream's leaf writer was set-only, so an obstacle that vacated a still-occupied leaf
  left its bits behind forever — a permanent occupied trail behind anything that moved.
- **A volume over LANDSCAPE bakes as free space in a packaged build.** CkJolt extracts landscape
  heightfields only under `WITH_EDITOR`, so outside the editor there are no landscape bodies to
  find. The build ensures once when its whole-volume broadphase sweep returns zero bodies and then
  proceeds — free space is a legal answer, just a suspicious one.

---

## Anti-patterns

1. **Never include a JPH header here.** The geometry backend talks to CkJolt's opaque,
   JPH-free query types. A direct JPH include re-opens the allowlist this module was designed to
   keep closed and couples the octree to a physics backend it must not know.
2. **Don't route walking agents through this module.** A grounded agent belongs on
   `CkNavigation`/`CkCrowd`; a voxel path for a walker is both slower and worse.
3. **Don't re-implement A\*.** New search behavior belongs behind the CkAStar adapter or in the
   refinement pass, not in a bespoke solver.
4. **Don't key anything on actor pointers.** Volume and chunk identity are stable integer ids so a
   bake survives streaming and serialization.

---

## Attribution

Core algorithms are derived from **Nav3D 2.0**, © 2025 Darby Costello, MIT. The upstream LICENSE is
preserved at `docs/campaigns/voxelnav-port/` until the port matures, then alongside this module.

---

## Status

The octree core, the geometry backend, budgeted voxelization, pathfinding, and dynamic occluders with
local repair are implemented: a volume bakes an immutable Sparse Voxel Octree of its free space,
answers point and segment queries against it, an agent plans a refined route through it, and a moving
obstacle repairs only the cells it dirtied. Chunking, crowd consumption, and node merging are not
implemented yet; the plan of record is
`docs/campaigns/voxelnav-port/` (`PROMPT.md` for the mission and locked decisions, `PROGRESS.md`
for live status).
