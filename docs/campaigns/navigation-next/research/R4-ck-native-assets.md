# R4 — Ck-native asset inventory (research input, session 1, 2026-08-31)

> Status: accepted by orchestrator after spot-checks (`CkAStar_Search.h` warm-start ctor :37-44,
> `ValidateExistingPath` :18, `ContinueSearch` :54; `CkAStar_GraphConcept.h` concepts :16/:23 —
> match). Snapshot @ CkFoundation `a25ec9539`.

## 1. CkVoxelNav proven patterns (module CLAUDE.md + headers)

- **Immutable publish** — `FOctree` is a plain value published as `TSharedPtr<const FOctree>`
  (`Octree/CkVoxelNav_Octree_Types.h:7-10`); fragment slot
  `FFragment_VoxelNavVolume_BuiltOctree::_Octree` (`Volume/CkVoxelNavVolume_Fragment.h:64`).
  Rebuild assembles a new octree, swaps atomically; readers holding the shared ptr keep a
  consistent structure.
- **Epochs** — `_Epoch` bumps per completed (re)build (`Fragment.h:44-46,66`); staleness DERIVED
  at read boundary, never stored (CLAUDE.md:74-76); chunked volumes use the SUM of chunk epochs
  as monotone fingerprint (`_AggregatedChunkEpochSum`, `Fragment.h:178`). Test-pinned.
- **Chunked field** — stable integer ids (`FChunkId` = `FVolumeId` + lattice index,
  `Chunk/CkVoxelNav_Chunk_Types.h:28`), portals + adjacency table (:131,:156); partitioning
  decided at composition, not in a processor (CLAUDE.md:118-121); chunk = ordinary volume entity
  with `FFragment_VoxelNavVolume_ChunkIdentity`.
- **Local repair** — dirty bounds fragment + NeedsRepair/RepairInProgress tags; re-probe only
  cells whose inflated probe box intersects dirty bounds; repair NEVER mutates the published
  structure — assembles a new one (CLAUDE.md:176-180). Test-pinned
  (`RepairedOctreeMatchesAFullRebake`, `MovedObstacleFlipsOccupancyOnlyWhereItMoved`,
  `SlicedRepairMatchesOneShotRepair`).
- **Value-only debug snapshots** — `Debug/CkVoxelNav_DebugSnapshot.h`: boxes/counts/ids/epochs/
  status only; never handles/UObjects/octree shares. **Failure is a status, never an empty
  scene** (`EDebugSnapshotStatus` MissingCook/StaleCook/Building/Current/Failed/RuntimeOnly);
  source enum distinguishes LivePie/RetainedSnapshot/EditorPreview; layer bitmask with
  deterministic caps. Cache identity checked before enumeration; whole-snapshot atomic replace.
- **Budget discipline** — probe COUNT is the primary budget (deterministic, test-assertable);
  wall-clock only a guard (CLAUDE.md:214-217). Build processor scheduled in the only window
  provably outside the async physics step.
- **Geometry backend seam** — 3-function JPH-free backend (`Backend/CkVoxelNav_GeometryBackend.h`
  + `_Jolt.h`/`_Stub.h`); hermetic bakes against hand-authored box lists; **Static body domain
  only — Kinematic bodies invisible** (CLAUDE.md:161-165).

Test footprint (confirmed): 18 files (16 .cpp + 2 headers) under
`Plugins/CkTests/Source/CkTests/Private/UnitTests/CkVoxelNav`, 68 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros, naming
`Ck.VoxelNav.<Area>.<Scenario>`, PIE tests isolated in `*Pie.cpp` files. AS: 1 autotest + 2 gyms
(Stress, FlyingVsGrounded) in `CkTests/Script/CkVoxelNav/`, registered in
`Script/Common/CkTests_GymRegistry.as`.

## 2. CkAStar — generic search infrastructure (SPOT-CHECKED)

- `CkAStar_GraphConcept.h:16-31`: `AStarNodeId` = copyable + equality-comparable;
  `AStarGraph` requires exactly `Neighbors(A)`, `Cost(A,B)->float`, `Heuristic(A,B)->float`,
  `IsGoal(A)->bool`. Nothing geometric.
- `CkAStar_Search.h`: `TSearchState` — time-sliced `ContinueSearch(FSearchParams)` (:54,
  preserves state on InProgress; budget = iterations AND microseconds); **warm-start ctor**
  `(Graph, Start, Goal, ExistingPath, WarmStartFromIndex, Capacity)` (:37-44) = the plan-repair
  seam; default-constructible for fragment storage; debugger introspection (open/closed sets,
  g-scores, came-from, iteration/time counters). Free `ValidateExistingPath(Graph, Path)->int32`
  (:15-20) returns first disconnected step.
- Also: `CkAStar_PathCache.h` (generation-counter invalidation), `CkAStar_PrecomputedTable.h`
  (bake-time full A* table, serializable — doc-sourced, not code-verified), base template
  processors `TProcessor_AStar_Execute/_EndPlay`.
- Consumers: CkGoap (`CkGoap_Graph.h`), CkVoxelNav (`FPathGraph` over `FCellId` — a merged-BOX
  graph), CkPathNetwork (`FRouteGraph` over `FRouteNodeId` — an edge graph). **Concept proven
  over both box-decomposition and graph-edge search → will drive a polygon/portal graph.**
  Caveat: a polygon-portal graph needs its own transition-point math (analogue of
  `Get_CellTransitionPoints`).
- `FPathGraphSharedData` (`CkVoxelNav_Path_Graph.h:33-47`): per-query immutable seed; agent-fit
  expressed as MIN EXTENT not layer index ("search must stay blind to how a cell is addressed").
- NOTE: `CkAStar/Claude.md` is stale (describes CkGrid use case only).

## 3. Geometry/spatial services

- **CkSpatialQuery** — LIVE entity overlap probes via CkJolt-owned world; no BVH of its own, no
  tri-mesh access. A nav builder should follow CkVoxelNav's precedent: go through CkJolt's
  JPH-free geometry backend seam, not CkSpatialQuery.
- **CkShapes** — pure shape data layer (Box/Sphere/Capsule/Cylinder quartets + shared typesafe
  handle). Authored extents/clearance belong here, not bare floats.
- **CkPmg** — procedural mesh generation for debug visuals. Three tiers:
  `ck::pmg::Create_DebugLineSet` + `Append_Debug*_World` (retained wireframe — right for paths/
  corridors/outlines; chunk indefinitely-growing streams), `BasicShapes Add_*/Create_*` (filled
  procmesh), `DrawFilled*` (fire-and-forget). CkCrowdDebugger already depends on it.

## 4. CkPathNetwork capability summary

Four layers: authored ribbon layer (`FCk_PathNetwork_Ribbon` of points, Generated-vs-Authored
source with edit-promotion); detection+vectorization (mask rasterize with pre-admission bounds
validation → pluggable vectorizer → ribbons); compiled network (`CkPathNetwork_Build.h` "pure
math, runtime-callable, no ECS/world dependency" → `FBuiltNetwork` of nodes/edges/projections/
samples, derived-only); routing+corridors (`FRouteGraph` per-query A* space with virtual
Start/Goal + `FRouteCostPolicy`; world-free `RoutePlan` seam shared by runtime and editor
preview; corridor compile + path simplify; follower owns corridor, invalidated on rebuild by
dedicated processor; enumerated failure reasons). Authoring via `ACk_PathNetwork_UE : AInfo`.
Ribbons are polyline+width, not splines.

## 5. CkNavigation module surface (9 public headers)

`Nav/CkNav_Algorithm.h` (9 statics incl. the external-provider seam), `Nav/CkNav_Fragment.h`
(request queue), `Nav/CkNav_Fragment_Data.h` (reflected contract), `Nav/CkNav_Processor.h`
(HandleRequests + CancelPendingRequests), `NavAreaMarkup/CkNavAreaMarkup_Utils.h`,
`NavAreaMarkup/CkNavArea_Restricted.h`, `Revision/CkNavigationRevision_Subsystem.h`,
`Settings/CkNav_ProjectSettings.h`, `Utils/CkNav_Utils.h` (public API + 3 `_ForTesting` seams).
Load-bearing: `FCk_Nav_PathResult` + `InstallExternalPath` is the proven provider-agnostic seam —
VoxelNav and PathNetwork already reach the crowd through it with no downstream knowledge of which
provider ran. NOTE: `CkNavigation/Claude.md` flagged stale by Source/CLAUDE.md; `Plan/` holds 11
historical gate docs + PLAN.md (history, not live).

## 6. Debugger patterns

No CkVoxelNavDebugger module — VoxelNav visualization is hosted in **CkCrowdDebugger**
(DeveloperTool) which deps CkNavigation/CkVoxelNav/CkPathNetwork/CkCrowd/CkQueue/CkPmg/
CkDebugScene/CkDebuggerCommon (+editor-only CkVoxelNavEditor). Module tiers from
`CkDebugger.uplugin` (31 modules): Runtime = CkGameplayDebugger, CkDebuggerCommon,
CkEntityDebugOverlay, CkInputHudOverlay, **CkNavmeshDebugDraw**; DeveloperTool = CkCrowdDebugger,
CkAStarDebugger, and the other debugger windows; Editor = *DebuggerEditor halves.

Three-tier data flow (both CLAUDE.mds agree):
1. **Runtime feature module owns the snapshot type** (e.g. `CkVoxelNav_DebugSnapshot.h`).
2. **DeveloperTool debugger collects + adapts** (feature adapter translates; shared mechanics in
   CkDebugScene; snapshot boundary copies values — never UWorld/actor/handle/navmesh/producer).
3. **Thin viewport facade** over `SCkDebug_3dPreviewViewport`; capability-driven common controls.

Verification family `Ck.CrowdDebugger.Viewport3d.*` (copied lifetimes, stable identity, pick
mapping, atomic failure, 240-agent instancing). Known baseline failure: inherited
`Nav.Filter.Customer` mapping failure in current CkPlugins user config — do not hide new
failures behind it (CkCrowdDebugger/CLAUDE.md:29-30).

## 7. Test/gym discipline for a nav feature (CkTests/CLAUDE.md)

- **Layer 1 low-level C++ automation**: `UnitTests/<Module>/`, `IMPLEMENT_*AUTOMATION_TEST` only
  (zero DEFINE_SPEC), `Ck.<Feature>.*` naming. ECS-free math tested hermetically (VoxelNav
  precedent: voxelizer usable with no ECS against hand-authored box lists).
- **Layer 2 production-path PIE AS autotests**: one `UCk_AutoTest_Base` subclass per .as;
  wrapper generation on AS recompile; timeout via `default _TimeoutSeconds` on the entity
  script; net tests additionally need a C++ rebuild to appear.
- **Layer 3 gym**: station in `Script/<FeatureModule>/`, registered in
  `CkTests_GymRegistry.as`; steps are CkStateMachine graphs (one `UCk_Gym_StepState` per step,
  `UCk_Gym_Dwell` gating); shared control panel (`CkGym_ControlPanel.as`, rows rebuilt per
  frame); reserved keys Tab and H; exec `Ck_Gym_Restart/_Next/_Prev/_GoTo/_List`.
- **Settling discipline**: wait on NAMED CONDITIONS, never fixed hop counts; `WaitOneFrame` is
  0.05s wall-clock legacy; predicates must name the test's OWN entities (shared PIE world); one
  class per .as file; don't rename test classes (orphans placed wrapper actors).

## Undetermined (carried)

- CkAStar PathCache/PrecomputedTable claims are doc-sourced.
- CkVoxelNavEditor headers not enumerated.
- CkTests/CLAUDE.md:176-245 (trust levels, run mechanics, Warnings) unread by R4 — orchestrator
  or executor must consult when authoring VALIDATION.md.
