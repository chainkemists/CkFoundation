# PHASE 2 — Pathfinding

> Freshness: authored 2026-08-04 at the 1→2 boundary. Status of record: PROGRESS.md. Spec detail:
> `research/phase1-port-map.md` §4 (AStar adapter) + `research/research-coupling.md` (raycaster
> port dispositions). Binding decisions: [C-D3] (CkAStar + refinement, NO Theta* port), [C-D14],
> [C-D19] (CkJolt segment query prerequisite), [C-D21] (below).

## Entry criteria

- [x] Phase 1 closed. Baseline = full suite 981/975/6 + Ck.VoxelNav 20/20 + Ck.Jolt 48/48.

## Decisions local to this phase

- **[C-D21]** Phase 2 search is SYNCHRONOUS inside the processor (CkPathNetwork precedent:
  `Search_RouteGraph(..., MaxIterations)` in-tick, iteration-capped). The budgeted/time-sliced
  async upgrade (`TFragment_AStar_SearchState` + `TProcessor_AStar_Execute`) is Phase 5 scope,
  alongside merging — additive, not a rewrite.

## Units

### Wave 1 (parallel)

**2A+2B — segment query + octree ray-marcher (one agent):**
- 2A (CkJolt, per [C-D19]): `FCk_Jolt_QuerySession::Get_IsSegmentBlocked(From, To) -> bool` —
  any-hit `CastRay`, static-domain + static-broadphase filters, no channel, no attribution;
  internal JPH-layer function + session method; CkJolt Claude.md updated; a case in the existing
  BoxOccupancy PIE test or a sibling (`Ck.Jolt.Query.*`).
- 2B (CkVoxelNav): grow `ICk_VoxelNav_GeometryBackend` + Jolt impl + stub with
  `Get_IsSegmentBlocked` (stub: segment-vs-box slab tests). Port the Revelles parametric octree
  ray-marcher (`Nav3DRaycaster.cpp`, ~825 lines — entirely octree-internal, plain class, kill the
  UObject/GEditor/NewObject sins per the coupling report) into `Octree/CkVoxelNav_Octree_Raycast.*`:
  `Raycast(FOctree, From, To) -> hit/clear` vs OCTREE occupancy (distinct from the backend's
  physics segment query — document when each is right). Hermetic tests: rays through known bakes
  (clear corridor, blocked wall, leaf-grazing, sub-node precision).

**2C — AStar adapter + path feature (one agent):**
- `Path/CkVoxelNav_Path_Graph.{h,cpp}`: `FCellId`-keyed `astar::AStarGraph` view over a published
  octree (raw ptr + shared query data, one-tick lifetime per `FRouteGraph`'s documented rule,
  `static_assert(astar::AStarGraph<...>)`), Neighbors = face neighbors + parent/child transitions
  (Brewer), Cost/Heuristic Euclidean with node-size compensation knob per upstream.
- `Path/` feature quartet: `FCk_Handle_VoxelNavPath` stamped ON the agent entity (follower
  precedent), `Request_FindPath(volume, from, to)` deferred + completion delegate; synchronous
  capped search in the handler ([C-D21]); result fragment: waypoints (cell centers), status enum,
  epoch-planned-against (staleness detectable). Signals OnPathReady/OnPathFailed.
- Hermetic tests over stub-baked octrees: reachable path found + every segment cell-free;
  unreachable reports Failed; epoch mismatch detectable.

### Wave 2 (after wave 1)

**2D+2E — refinement + integration tests (one agent):** visibility pruning over the raw cell path
(walk waypoints, drop any the octree raycast proves redundant) + the CatmullRom smoothing port
(subdivision-count knob); wire as an opt-in step of Request_FindPath. Tests: refinement strictly
shortens the reference zigzag path (recorded numbers into the test), smoothed path stays cell-free;
PIE: path across the baked boxes scene, spot-checked collision-free via the ray-marcher.

## Exit criteria

- [ ] Targeted: all `Ck.VoxelNav.*` green (20 + new), `Ck.Jolt` family green (48 + new).
- [ ] Full suite delta-zero vs the six known reds — ONE sample ([C-D18]).
- [ ] `rg --no-ignore -l "<Jolt/" Source/CkVoxelNav` → zero; CkAStar untouched (`git diff --stat`).
- [ ] Comment audit; PROGRESS updated; PHASE_3.md authored at the boundary.

## Fences

- No chunking/multi-volume (Phase 3), no crowd/InstallExternalPath wiring (Phase 4), no merging or
  async search (Phase 5), no Theta*/LazyTheta* under any circumstances ([C-D3]).
- CkAStar is read-only. CkNavigation untouched until Phase 4.
