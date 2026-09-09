# R3 — Recast/Unreal-Navigation coupling inventory (research input, session 1, 2026-08-31)

> Status: accepted by orchestrator after spot-checks (`CkNav_Fragment_Data.h` leak sites,
> `CkNav_Algorithm.h` external-path seam). Line numbers are a snapshot @ CkFoundation
> `a25ec9539`. Buckets: [NEUTRAL] already provider-neutral · [ADAPTER] Recast-specific, movable
> behind an adapter · [REIMPLEMENT] capability the new provider must supply · [RETIRE] deletable
> when Recast goes · [AMBIGUOUS] needs an orchestrator ruling.

Symbol counts across CkFoundation/CkTests/CkGameplayDebugger (`.h/.cpp/.cs/.as`):
UNavigationSystemV1 60 · ARecastNavMesh 48 · UNavigationQueryFilter 25 · UNavArea 45 (incl.
`UNavArea_Null` in AS tests) · INavRelevantInterface 1 · FPathFindingQuery 1.

## (A) Dependency table

### CkNavigation — the provider seam itself

| File | Lines | What | Bucket |
|---|---|---|---|
| `Nav/CkNav_Fragment_Data.h` | 11 include NavArea.h, 19-20 fwd, 183 `TArray<TSubclassOf<UNavArea>> _ExcludedAreaClasses`, 215 `TSubclassOf<UNavigationQueryFilter> _QueryFilterClassOverride` | The only two Unreal-Navigation leaks in the public request/result contract; everything else (waypoints, status, fail reason, revision, pending clock, diagnostics) is engine-neutral | [ADAPTER] — replace with opaque filter/area tokens |
| `Nav/CkNav_Fragment_Data.h` | 28-58 status/fail enums, 64-168 diagnostics/result, 254-274 abandon, 278-285 delegates | Status/result/abandon/diagnostics | [NEUTRAL] |
| `Nav/CkNav_Algorithm.h` | 11-13, 17-18, 26-34, 42 | Static API takes `ARecastNavMesh&` + `UNavigationSystemV1&` + filter class | [ADAPTER] — natural provider-interface boundary |
| `Nav/CkNav_Algorithm.h` | 55-95 `InstallExternalPath`, `PrependWaypoints`, `MarkPathPending`, `AbandonPath`, `FailPath`, `AgePathPending` | **The existing external-provider seam** — takes only `FCk_Handle` + waypoints + destination + revision | [NEUTRAL] — a new provider plugs in here today |
| `Nav/CkNav_Algorithm.cpp` | 30-97 `ResolveQueryFilter` (GetDefaultQueryFilter, GetQueryFilter, GetCopy, GetAreaID, SetExcludedArea) | Filter resolution + value-only exclusion overlay on a private filter copy; malformed overlay fails closed | [REIMPLEMENT] — cost/exclusion filtering is a capability |
| `Nav/CkNav_Algorithm.cpp` | 149-150 ProjectPoint, 165 GetNavMeshBounds, 170/177 diagnostic re-probes, 195 GetConfig agent dims | Projection + failure diagnostics | [REIMPLEMENT] (projection); 163-196 diagnostic re-probe block [RETIRE] |
| `Nav/CkNav_Algorithm.cpp` | 216-229 FPathFindingQuery + FindPath; 234-239 `FNavMeshPath::OffsetFromCorners` | Path query + corner offset | [ADAPTER] (dispatch), [REIMPLEMENT] (corner offset / string-pull) |
| `Nav/CkNav_Algorithm.cpp` | 250-313 `ExtractWaypoints` (IsPartial etc.) | Recast result → neutral waypoints | [ADAPTER] |
| `Nav/CkNav_Processor.cpp` | 209-226, 343-373 | Provider resolve (`GetCurrent`→`GetDefaultNavDataInstance`) + "is provider ready here" projection probe | [ADAPTER] |
| `Nav/CkNav_Processor.cpp` | 33-122 `GDeferredNavRequests`, `IsNewerRevision`, `AddDeferredLatest`, 5s `CVarMaxDeferralSeconds` | Process-wide deferred queue, revision-ring latest-wins. **Explicitly not keyed on world → not multi-PIE safe** (comment :35, echoed in module CLAUDE.md) | [NEUTRAL] mechanism, [AMBIGUOUS] placement — exists because Recast bakes async |
| `Nav/CkNav_Processor.cpp` | 183 budget, 299-331 revision supersession + authority, 386-398 defer/park, 403-488 drain + signals | Budget / revision / authority / signal lifecycle | [NEUTRAL] |
| `Settings/CkNav_ProjectSettings.h` | 43 `TMap<FGameplayTag, TSoftClassPtr<UNavigationQueryFilter>> _QueryFilters`, 81 | Tag→filter table — tag indirection already neutral, value type leaks | [ADAPTER] |
| `Settings/CkNav_ProjectSettings.h` | 25-38, 76 budget + projection half-extents | | [NEUTRAL] budget / [ADAPTER] projection extents ([AMBIGUOUS] whether neutral) |
| `NavAreaMarkup/CkNavAreaMarkup_Utils.h` | 21 `UCk_NavAreaMarkup_UE : UObject, INavRelevantInterface` (sole INavRelevant site), 41 `_AreaClass` | Actor-free dynamic nav-octree painter | [REIMPLEMENT] — dynamic cost/exclusion authoring; CkCrowd/CkQueue depend on it |
| `NavAreaMarkup/CkNavAreaMarkup_Utils.cpp` | 50, 116, 132 OnNavRelevantObjectRegistered/Unregistered | Register/unregister → tile rebuild | [REIMPLEMENT] |
| `NavAreaMarkup/CkNavArea_Restricted.h` | 15 `UCk_NavArea_Restricted : UNavArea` | Framework area class | [ADAPTER] |
| `Revision/CkNavigationRevision_Subsystem.cpp` | 21-66 `OnNavigationGenerationFinishedDelegate` observer | World-scoped "generation N finished" revision | [REIMPLEMENT] — PathRefresh + avoidance-volume retirement build on it |
| `Utils/CkNav_Utils.cpp` | 191-198 `NavSys->Build()` test hook | | [AMBIGUOUS] |
| `Utils/CkNav_Utils.cpp` | 227-241 `Try_ProjectOntoNavmesh` | Public projection UFUNCTION | [REIMPLEMENT] |
| `Utils/CkNav_Utils.cpp` | 261-263 OnActorRegistered/Unregistered | Actor nav-registration | [AMBIGUOUS] |
| `CkNavigation.Build.cs` | 17-18 NavigationSystem, AIModule | | [ADAPTER] |

### CkCrowd

| File | Lines | What | Bucket |
|---|---|---|---|
| `Agent/CkCrowdAgent_HandleRequests_Processor.h/.cpp` | h:122, cpp:117-134 | Strict/permissive filter-class selection | [ADAPTER] |
| `…HandleRequests_Processor.cpp` | 488-557 `RequestPathForActiveGoal` | **Provider fork: VoxelNav (503-528) → PathNetwork (530-554) → CkNavigation (556)**; each branch `MarkPathPending(handle, revision)` then dispatches | [NEUTRAL] |
| `…HandleRequests_Processor.cpp` | 405-484 revision advance / abandon / per-provider release switch | Episode acquire/release | [NEUTRAL] |
| `…HandleRequests_Processor.cpp` | 251-312 `Request_NavigationPath` | | [NEUTRAL] |
| `Agent/CkCrowdAgent_NavQueryFilter.h` | 17 `UCk_NavQueryFilter_AvoidStandingCrowds` | Strict filter | [ADAPTER] |
| `Agent/CkCrowdAgent_NavArea.h` | 14 `UCk_NavArea_CrowdAgent` | Stationary-markup cost area | [ADAPTER] |
| `Agent/CkCrowdAgent_ConstrainToNavmesh_Processor.cpp` | 108-143 project + 4x-radius recovery, 241 `FindMoveAlongSurface` | **Grounded containment + off-mesh recovery — the single Transform writer for grounded agents** | [REIMPLEMENT] |
| `Agent/CkCrowdAgent_PathRefresh_Processor.cpp` | 65-95 `GetAreaID`/`GetPolyAreaID` markup-landed probe; 140-152 `IsNavigationBuildInProgress` + revision subsystem; 461-486, 799-825 direct `FindPathSync` (escape path, corridor splice) | Rebuild observability + markup ground-truth + sync replans | [REIMPLEMENT] (probes/observability), [ADAPTER] (sync replans) |
| `Agent/CkCrowdAgent_AvoidanceSample_Processor.cpp` | 47, 171-189 `ARecastNavMesh::FindEdges` | dtLocalBoundary-style wall segments, **queried off game thread from TParallelProcessor** (h:28 documents thread-safety contract) | [REIMPLEMENT] — incl. the threading contract |
| `Agent/CkCrowdAgent_Steering_Processor.cpp` | 93-120, 185 `NavData->Raycast` | LOS waypoint retirement | [REIMPLEMENT] |
| `Agent/CkCrowdAgent_OnPathResolved_Processor.cpp` | 294 Raycast | LOS at install | [REIMPLEMENT] |
| `Agent/CkCrowdAgent_OnRouteResolved_Processor.cpp` | 276 Raycast | LOS at route install | [REIMPLEMENT] |
| `Agent/CkCrowdAgent_BlockDetect_Processor.cpp` | 333 FindMoveAlongSurface | Block/stall geometry | [REIMPLEMENT] |
| `Agent/CkCrowdAgent_DiagNavClip_Processor.cpp` | 213-329 | Read-only replica of clamp math | [RETIRE] (rewrite later if wanted) |
| `Agent/CkCrowdAgent_DrawNavProjection_Processor.cpp` | 53-64 | Debug draw of projection | [RETIRE] |
| `AvoidanceVolume/CkCrowdAvoidanceVolume_NavArea.h/.cpp` | three UNavArea subclasses + policy→class map | | [ADAPTER] |
| `AvoidanceVolume/CkCrowdAvoidanceVolume_Utils.cpp` | 222 excluded-area overlay build | | [ADAPTER] |
| `AvoidanceVolume/CkCrowdAvoidanceVolume_Processor.cpp` | 202-219 GetAreaID/GetPolyAreaID | Painted-area confirmation | [REIMPLEMENT] |
| `Settings/CkCrowd_DebugSettings.cpp` | 124 projection | Debug probe | [RETIRE] |
| `CkCrowd.Build.cs` | 21 NavigationSystem — **comment stale** (claims DrawNavProjection-only; six processors use it) | | [ADAPTER] |

### CkPathNetwork (+ Editor)

| File | Lines | What | Bucket |
|---|---|---|---|
| `Network/CkPathNetwork_Processor.cpp` | 165-200 `Resolve_OffPathLeg` (FindPathSync) | **Recast as connector-path builder** for off-network legs | [REIMPLEMENT] |
| same | 349-410 `Get_DefaultRecastNavmesh`, `Is_NavmeshSegmentDirectlyWalkable` (`NavMeshRaycast`), `Try_ResolveNavmeshSegment` (raycast→FindPathSync fallback) | **Recast as safety oracle** — every compiled ribbon segment proven walkable before install | [REIMPLEMENT] |
| same | 461-797 filter class threaded through route compiler; 556-650 ribbon containment via `ResolveQueryFilter` | | [ADAPTER] |
| `CkPathNetworkEditor/CkPathNetwork_EditorUtils.cpp` | 27-35, 176-186, 356-366 | Authoring-time node snap onto mesh | [REIMPLEMENT] (editor snap equivalent) |

### Other CkFoundation modules

| File | Lines | What | Bucket |
|---|---|---|---|
| `CkEqs/Query/CkEqs_Algorithm.cpp` | 266-280 `_ProjectOntoNav` post-pass (deliberately inlined per module CLAUDE.md, "keep the two in sync") | Candidate projection | [REIMPLEMENT]; [AMBIGUOUS] whether perf argument survives adapter indirection |
| `CkQueue/Queue/CkQueue_Formation_Processor.cpp` | 159-209 project + Raycast | Slot placement | [REIMPLEMENT] |
| `CkQueue/Navigation/CkQueue_NavigationRevisionSubsystem.cpp` | 30-64 | **Duplicate** nav-generation revision observer | [ADAPTER] + [AMBIGUOUS] consolidation |
| `CkVoxelNav/Path/CkVoxelNavPath_Utils` | h:60/cpp:56 | Existing non-Recast provider behind the same seam | [NEUTRAL] — reference implementation |

### CkGameplayDebugger

| File | What | Bucket |
|---|---|---|
| `CkNavmeshDebugDraw_Subsystem.cpp` (11 sites: weak ptrs 216-217, resolves, IsNavigationBeingBuilt ×4, GetAreaID, generation-finished binds) | Whole module draws Recast tile geometry | [RETIRE] ([AMBIGUOUS]: retire vs rewrite — orchestrator call) |
| `CkCrowdDebugger_DataCollector.cpp` 169-482, `Types.h:133` GetBounds viewport fit, NavmeshStatusPanel "UNavigationSystemV1 OK" | Needs neutral "nav world bounds + provider health" query | [ADAPTER] |
| `CkPerfLab_WorldSurvey_Builder.cpp` 57-250 | Navmesh-seeded spawn positions | [ADAPTER] |

### CkTests (AngelScript)

`UNavArea_Null` used as literal class to punch navmesh holes in 10+ AutoTests/gyms
(NarrowGap_TraverseCalm/NoRouteFailsClean/BlockedDetours, Stall_RepathsAroundLateObstacle,
Stall_UnreachableGoalFailsBounded, Steering_CornerRetirementKeepsAgentOnMesh,
OffPath_TeleportRepaths, Facing_CalmWhilePressingBlockedGap, CkQueueGym_PlayerController:1377,
Queue_NavigationChangeRetriesImpossibleFormation:89) + `CkTestsAssets.as:794,7955`
`TSoftObjectPtr<ARecastNavMesh>`. → [REIMPLEMENT]: the fixture vocabulary ("paint an impassable
box at runtime") must exist on the new provider or the crowd suite loses its obstacles.

## (B) Current request/result/revision/cancellation contract (exact)

**Request — `FCk_Request_Nav_FindPath : FCk_Request_Base`** (`CkNav_Fragment_Data.h:191-246`):
`FVector _TargetLocation` (ctor essential); `bool _AllowPartialPath = true`;
`FGameplayTag _QueryFilter` (settings tag→class, empty→NavData default);
`TSubclassOf<UNavigationQueryFilter> _QueryFilterClassOverride` (outranks tag, per-query — exists
for CkCrowd strict/permissive phase swap); `FCk_Nav_QueryFilterOverlay` (value-only excluded-area
list applied to a private filter copy; malformed → fails closed `NoDefaultFilter`);
`_StartOverride` + `_StartOverrideLocation`; `int32 _RequestRevision = 0` (opaque caller-owned; 0
opts out of stale protection).

**Result — `FCk_Nav_PathResult`** (aliased as `ck::FFragment_Nav_PathResult`):
`_Waypoints` (**preserved on failure** so consumers keep walking the old path);
`_DestinationLocation`; `_Status` ∈ {None, Pending, Ready, Failed, Partial}; `_Diagnostics`
(fail reason, target/agent/projected locations, projected flags, raw/extracted counts, query wall
time + duration); `_RequestRevision` echoed; `double _PendingSinceSeconds` (process-relative,
**MUST NOT be persisted or replicated**).

**Revision semantics** (`CkNav_Processor.cpp:50-113`): ring over [1, MAX_int32], shorter forward
distance = newer. Enforced at (1) `AddDeferredLatest` (newer evicts older deferred →
Failed_Cancelled; older arrival cancelled), (2) batch pre-drain (result-slot authoritatively newer
→ drain all as cancelled), (3) in-drain (≠ batch latest → cancelled). `InstallExternalPath`
retains caller revision to make external install the writer authority over later-draining Recast.

**Lifecycle**: `Request_FindPath` acquires. `Request_AbandonPath(revision)` releases —
immediate: status None, revision stamped, waypoints/destination reset, fail reason cleared,
pending clock zeroed; `PurgeInFlightQueriesFor` + `PurgeDeferredRequestsFor` remove entries
**before** completing any (completion delegates are caller AS code, may re-enter abandon).
`MarkPathPending(handle, revision)` is the provider-neutral acquire used by VoxelNav +
PathNetwork. `FailPath` is the watchdog terminal. Every Pending write restamps
`_PendingSinceSeconds` (:379-385).

**Budget/deferral**: `Get_MaxPathQueriesPerFrame()` default 8 per tick. Unprojectable start →
park Pending in `GDeferredNavRequests`; reprobe per tick; force-fail `NoNavData` past
`ck.Nav.MaxDeferralSeconds` (5s). Drain actions counted into `_LastVisitedCount` for the
scheduler pump.

**Signals**: `Nav_OnPathReady(FCk_Handle, FCk_Nav_PathResult)` / `Nav_OnPathFailed(FCk_Handle)` +
generic completion guard.

## (C) Capabilities Unreal Navigation supplies beyond FindPath

1. **Projection** (point → nearest walkable surface) — most widespread; configurable asymmetric
   box extents. Sites: Algorithm 149-177, Processor 226/373, Utils 237, ConstrainToNavmesh
   127/143, Eqs 280, Queue_Formation 187, PathNetwork_EditorUtils 35, PerfLab 90, + debug.
2. **Filtering / cost areas** — filter resolution + copies, GetAreaID, SetExcludedArea; tag→class
   table; 5 framework area/filter classes.
3. **Containment / constrained surface walk** — `FindMoveAlongSurface` (ConstrainToNavmesh 241 =
   single Transform writer; BlockDetect 333) + 4x-radius recovery projection.
4. **Walkability raycast (LOS)** — `ANavigationData::Raycast` ×5 sites + `NavMeshRaycast` in
   PathNetwork.
5. **Boundary geometry** — `FindEdges` wall segments near a point, **off game thread** from
   TParallelProcessor (thread-safety contract documented at AvoidanceSample h:28).
6. **Dynamic registration + rebuild** — INavRelevantInterface markup painter + register hooks +
   forced Build() test hook.
7. **Rebuild observability** — generation-finished delegate (×2 duplicate observers),
   IsNavigationBuildInProgress, GetPolyAreaID "did my markup land" ground-truth probe.
8. **Partial paths** — SetAllowPartialPaths + IsPartial → Status::Partial; CkCrowd strict phase
   treats short-partial as verdict triggering permissive re-dispatch.
9. **Path post-processing** — `FNavMeshPath::OffsetFromCorners` + Ck skip-first-waypoint pass.
10. **Authoring** — editor-time node snap in PathNetworkEditor.
11. **Persistence** — none today (nav data is baked level content; pending clock explicitly
    excluded from persist/replicate).
12. **Debug geometry** — NavmeshDebugDraw module (tile geometry), GetBounds viewport fit,
    bounds in projection-failure log.

## (E) Ambiguous items reserved for orchestrator ruling

1. `GDeferredNavRequests` — keep/move/fix (not multi-PIE-safe; exists because Recast bakes async).
2. `Request_SetActorNavigationRegistered` — keep iff new provider derives geometry from actors.
3. `Request_NavigationRebuild_ForTesting` — retire iff no bake; else tests need the hook.
4. Duplicate revision subsystems (CkNavigation + CkQueue) — consolidate?
5. Projection half-extents — provider-neutral setting vs adapter-private.
6. `CkNavmeshDebugDraw` — retire vs rewrite against new provider.
7. EQS inline projection duplication — does the perf argument survive adapter indirection?
8. `CkCrowd.Build.cs:21` comment stale (six processors use NavigationSystem, not one).
