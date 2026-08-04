# HEADLINE

CkFoundation's nav/AI stack is four separable pieces: CkNavigation is a thin, deliberately Recast/UNavigationSystemV1-bound synchronous path service with NO path-provider polymorphism but WITH a concrete external-path injection seam (`FCk_Nav_Algorithm::InstallExternalPath` / `MarkPathPending`) that a second provider already exploits; CkAStar is a fully generic, engine-agnostic C++ template A* (concept-constrained `AStarGraph`, time-sliced `TSearchState`, budgeted parallel processor templates) with no reflected public API of its own — a voxel-octree graph plugs in cleanly, but Theta*'s any-angle parent-rewiring does NOT fit its hardcoded relaxation loop; CkCrowd consumes paths exclusively through `FFragment_Nav_PathResult` (a provider-agnostic waypoint polyline) and its steering/braking math is already 3D-safe, but four downstream stages are hard-planar or hard-Recast — `ConstrainToNavmesh` (single transform writer, snaps Z to the navmesh surface, gated only by a GLOBAL project setting with no per-agent opt-out), `AvoidanceSample` (2D velocity obstacles), `PushApart`/`Separation` (Z zeroed), `FaceAngle` (yaw only). The exemplar to mimic for a volumetric-3D module is CkPathNetwork verbatim: it is a non-Recast path provider at T4, depends on both CkAStar and CkNavigation, ships a "volume" typesafe handle (`FCk_Handle_PathNetwork`, child entity + built-graph fragment + epoch) paired with an "agent" typesafe handle (`FCk_Handle_PathNetworkFollower`, fragments stamped directly on the agent), implements `astar::AStarGraph` with a `static_assert`, and installs its compiled waypoints through `InstallExternalPath`. Zero prior 3D/volumetric/octree/SVO/flying-nav ambition exists anywhere in the codebase — greps return nothing; the only adjacent notes are "Future work: HPA*" and repeated "no off-mesh links / jumps" limitations, plus a LOCKED scope decision "Recast/UE Nav: Keep".

## Conventions & doc context (read first)

- `Plugins/CkFoundation/CLAUDE.md` — doctrine of record. Engine is UnrealEngine-Angelscript 5.7.x, EnTT 3.16.0, ~101 modules. Non-negotiables: research-by-mimicry of a neighbouring feature quartet before authoring; `CK_ENSURE_IF_NOT` + a SEPARATE ordinary early-out branch (never validate only inside the macro); requests are deferred through `_Requests` fragments; every public API must work in C++/BP/AngelScript; every deferred `Request_*` ends with a trailing `const FCk_Delegate_Request_OnCompleted& InDelegate` with `meta=(AutoCreateRefTerm="InDelegate")` and no C++ default.
- ECS two-tier naming (root CLAUDE.md:181-199): reflected `FCk_Fragment_[Feature]_ParamsData` in `X_Fragment_Data.h`; runtime `ck::FFragment_[Feature]_[Type]` in `X_Fragment.h`; bridge alias `using FFragment_X_Params = FCk_Fragment_X_ParamsData;`; typesafe handle `FCk_Handle_[Feature]` declared in `X_Fragment_Data.h` (NEVER `X_Fragment.h`); requests `FCk_Request_[Feature]_[Action] : FCk_Request_Base`; processors `ck::FProcessor_[Feature]_[Phase]` self-registered with `CK_REGISTER_PROCESSOR`.
- `Plugins/CkFoundation/Source/CLAUDE.md` — module map + tier table + `CkModuleRules` authoring rules. Decision-tree rows already reserved: "navmesh integration (paths, projection) → CkNavigation"; "crowd steering/avoidance → CkCrowd"; "sidewalk/path preference + authoring (ZoneGraph-lite) → CkPathNetwork"; "grid-based pathfinding → CkAStar + CkGrid".
- Tooling caveat confirmed in practice: superproject `.ignore` blinds Grep/Glob under this plugin. All greps below used `rg --no-ignore` via Bash.
- Doc-drift warning that held: `CkNavigation/CLAUDE.md` still says "⏳ Skeleton only" and documents an API signature (`Request_FindPath` with two bespoke delegates) that does NOT match the shipped code (one generic completion delegate). Trust code.

## CkNavigation — what it actually provides (VERIFIED by reading source)

Root: `E:\Repos\CkPlugins_Other\Plugins\CkFoundation\Source\CkNavigation\`

**It is unambiguously Recast/2D-navmesh-based, not its own thing.** `CkNavigation.Build.cs` public-deps `NavigationSystem` + `AIModule`. `Nav/CkNav_Algorithm.h:10-13` forward-declares `ARecastNavMesh`, `UNavigationSystemV1`, `UNavigationQueryFilter`, `FPathFindingResult`. `FindPathSync` calls `ARecastNavMesh::FindPath` DIRECTLY (per CLAUDE.md:136-138: skips the NavSys agent-dispatch step because the query ctor already populated `Query.NavAgentProperties` from `InNavData.GetConfig()`).

**Public API surface** (`Utils/CkNav_Utils.h`, class `UCk_Utils_Nav_UE : UCk_Utils_Ecs_Base_UE`, **no typesafe handle by design** — CLAUDE.md:139-141 "pathfinding is a service exposed to any entity with a Transform feature plus the path-result fragment slot, which the Utils add lazily on first request"):
- `Request_FindPath(FCk_Handle&, const FCk_Request_Nav_FindPath&, const FCk_Delegate_Request_OnCompleted&)` :36-40 — deferred, drained next tick.
- `Get_PathResult` :46 / `Get_PathStatus` :53 / `Has_Path` :60 (BlueprintPure).
- `Try_ProjectOntoNavmesh(FCk_Handle&, FVector, float InHalfExtentUu, FVector& OutSnapped, float InVerticalHalfExtentUu = -1.0f)` :80-85.
- `Request_NavigationRebuild_ForTesting` :71 (autotest hook).
- `BindTo_/UnbindFrom_ OnPathReady` :92,:102 and `OnPathFailed` :110,:120.

**Data shapes** (`Nav/CkNav_Fragment_Data.h`):
- `ECk_Nav_PathStatus` :25-32 = None/Pending/Ready/Failed/Partial.
- `ECk_Nav_PathFailReason` :38-52 = 12 values incl. NoNavSystem/NoNavData/StartProjectFailed/NotAuthority/BudgetExceeded.
- `FCk_Nav_PathDiagnostics` :60-114 — 11 read-only fields (fail reason, agent/target/projected-start/projected-end, raw + extracted counts, query wall time + duration ms). Friends: `FCk_Nav_Algorithm`, `ck::FProcessor_Nav_HandleRequests`.
- `FCk_Nav_PathResult` :119-145 — `TArray<FVector> _Waypoints` (world-space, post-funnel), `_DestinationLocation`, `_Status`, `_Diagnostics`. **This struct is the entire downstream contract.**
- `FCk_Request_Nav_FindPath : FCk_Request_Base` :150-185 — `_TargetLocation` (essential ctor param), `_AllowPartialPath`, `FGameplayTag _QueryFilter`, `_StartOverride` (`ECk_EnableDisable`) + `_StartOverrideLocation`.
- Delegates :189-196: `FCk_Delegate_Nav_OnPathReady(FCk_Handle, FCk_Nav_PathResult)`, `FCk_Delegate_Nav_OnPathFailed(FCk_Handle)`.

**Fragments** (`Nav/CkNav_Fragment.h`): `ck::FFragment_Nav_Requests` :15-30 (std::variant of one request type); `using FFragment_Nav_PathResult = FCk_Nav_PathResult;` :34 (bridge alias — the reflected struct IS the runtime fragment); two `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` blocks :38-49.

**Processors** (`Nav/CkNav_Processor.h`): `FProcessor_Nav_HandleRequests` :13-39 (`MarkedDirtyBy = FFragment_Nav_Requests`, `TExclude<FTag_DestroyEntity_Initiate>`, `CK_IGNORE_PENDING_KILL`); `FProcessor_Nav_CancelPendingRequests` :46-65 (`Group = FGroup_EndPlay`). Note :37-38 — `_BudgetRemainingThisTick` is "Reset in DoTick but never enforced — the per-tick query cap is not wired up" (dead knob; `_MaxPathQueriesPerFrame` project setting does nothing today).

**Also present:** `NavAreaMarkup/CkNavAreaMarkup_Utils.h` — actor-free UObject nav-area painter that registers an oriented box with the nav octree (UE's nav octree, not a pathfinding octree); `NavAreaMarkup/CkNavArea_Restricted.h`; `Settings/CkNav_ProjectSettings.h` — `_MaxPathQueriesPerFrame=8`, `_NavQuerySearchHalfExtent=500`, `_NavQueryVerticalHalfExtent=-1` sentinel, `TMap<FGameplayTag, TSoftClassPtr<UNavigationQueryFilter>> _QueryFilters`.

**Known limitations from its own doc (CLAUDE.md:159-164):** no async queries; no off-mesh links/jumps; the deferred-FindPath queue `ck_nav_processor::GDeferredNavRequests` is a PROCESS-WIDE global in `CkNav_Processor.cpp` — not multi-PIE-safe, entries force-failed with `NoNavData` past `ck.Nav.MaxDeferralSeconds` (5s).

## CkNavigation — the path-provider seam (the single most important finding)

**There is NO path-provider abstraction (no interface, no virtual dispatch, no registry). What exists instead is a concrete result-injection seam plus an ad-hoc if/else provider choice at the caller.**

The seam, `Nav/CkNav_Algorithm.h:46-55` (VERIFIED, implementation read at `CkNav_Algorithm.cpp:246-294`):
```cpp
// The external path-provider seam: installs an externally-computed polyline (e.g. a
// CkPathNetwork corridor) exactly as if FindPathSync had produced it.
static auto InstallExternalPath(FCk_Handle& InHandle, TArray<FVector> InWaypoints, const FVector& InDestination) -> void;

// Parks the result at Pending while an external provider computes, so pollers don't consume
// the PREVIOUS Ready result as the answer to the new request. Creates the fragment if absent.
static auto MarkPathPending(FCk_Handle& InHandle) -> void;
```
`InstallExternalPath` does `InHandle.AddOrGet<ck::FFragment_Nav_PathResult>()`, sets `_Waypoints/_DestinationLocation/_Status = Ready`, resets diagnostics to only the honestly-reportable fields, and broadcasts `ck::UUtils_Signal_Nav_OnPathReady`. It ensures on invalid handle and on an empty waypoint list (`CkNav_Algorithm.cpp:258-261`, with the rationale "an empty Ready path would stall every consumer that walks it").

**Provider selection is a hardcoded branch in the CONSUMER, not in CkNavigation** — `CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.cpp:116-132`:
```cpp
// Followers route through the path network, which installs its compiled waypoints through
// the same FFragment_Nav_PathResult seam — everything downstream stays provider-agnostic.
if (UCk_Utils_PathNetworkFollower_UE::Has(InHandle))
{ FCk_Nav_Algorithm::MarkPathPending(InHandle); ... Request_FindRoute(...); }
else
{ Request_NavigationPath(InHandle, InParams, Goal); }
```

**Consequence for a new volumetric-3D module:** you do NOT need to add a provider abstraction to ship — you implement the same contract CkPathNetwork implements (compute a waypoint polyline, `MarkPathPending` on request, `InstallExternalPath` on ready) and add a THIRD branch to `DoHandleRequest`. INFERRED: with three providers the if/else chain becomes the wrong shape and a real seam (an ordered provider-tag list on the agent, or a `FTag_CrowdAgent_PathProvider_*` selector) is the natural refactor — but that is a design choice, not a prerequisite. Call sites of the seam today (VERIFIED, `rg --no-ignore`): `MarkPathPending` × 5 in CkCrowd (`HandleRequests_Processor.cpp:121,151`, `BlockDetect_Processor.cpp:294,305`, `PathRefresh_Processor.cpp:536,548`); `InstallExternalPath` × 1 (`OnRouteResolved_Processor.cpp:90`).

## CkAStar — genericity, and whether a voxel octree / Theta* fits

Root: `Source\CkAStar\`. Deps (Build.cs): Core, CoreUObject, CkCore, CkEcs, CkEcsExt, CkLog — **no Engine, no NavigationSystem**. It is a header-mostly template library.

**Public reflected API: essentially none.** The only `UCLASS` in the module is `UCk_Utils_AStarTest_UE` under `Public/CkAStar/Test/` (VERIFIED by rg for `UCLASS` across `CkAStar/Public/`). The module's CLAUDE.md claim that it "runs A* over CkGrid cell entities" and "returns `FCk_Handle_2dGridCell`" is STALE — there is no CkGrid dependency in the Build.cs and no such API. **Real consumers are CkGoap and CkPathNetwork** (VERIFIED: only `CkGoap/Algorithm/CkGoap_Graph.h`, `CkPathNetwork/Network/CkPathNetwork_RoutePlan.cpp`, `CkPathNetwork/Network/CkPathNetwork_RouteGraph.h` include `CkAStar/Algorithm/`).

**The genericity contract** — `Algorithm/CkAStar_GraphConcept.h:15-31`:
```cpp
template <typename T> concept AStarNodeId = std::copyable<T> && std::equality_comparable<T>;
template <typename T_Graph, typename T_NodeId> concept AStarGraph =
    AStarNodeId<T_NodeId> && requires(const T_Graph& g, const T_NodeId& a, const T_NodeId& b) {
        g.Neighbors(a);
        { g.Cost(a,b) }      -> std::convertible_to<float>;
        { g.Heuristic(a,b) } -> std::convertible_to<float>;
        { g.IsGoal(a) }      -> std::convertible_to<bool>; };
```
Four const member functions. `T_NodeId` needs `GetTypeHash` in practice (it lands in `TSet`/`TMap`) — see `pathnetwork::GetTypeHash(const FRouteNodeId&)` at `CkPathNetwork_RouteGraph.h:35-36`.

**Search engine** — `Algorithm/CkAStar_Search.h` + `.inl.h`: `TSearchState<T_NodeId, T_Graph>` with time-sliced `ContinueSearch(const FSearchParams&)`; `FSearchParams` (`CkAStar_Types.h:22-35`) carries `BudgetMicroseconds`, `MaxIterationsPerTick`, `CostThreshold`, `TimecheckInterval`; `ESearchStatus` = InProgress/Complete/Failed/CostThresholdReached. **Warm-start / plan-repair ctor exists** (`CkAStar_Search.h:39-45`, impl `.inl.h:77-119`): seeds the closed set from an existing path prefix and re-opens from the prefix tail. `ValidateExistingPath` (`.inl.h:21-49`) returns the index of the first broken step — the natural "is my cached 3D path still valid after a geometry change?" primitive.

**Can a voxel-octree graph plug in? YES, structurally clean.** Precedents are exact: `pathnetwork::FRouteGraph` holds `const FBuiltNetwork*` + a `TSharedPtr<FRouteGraphSharedData>` and has `static_assert(astar::AStarGraph<FRouteGraph, FRouteNodeId>, "...")` at `CkPathNetwork_RouteGraph.h:263-265`; `goap::FGoapGraph` holds a `TSharedPtr<FGoapGraphSharedData>` with a content-addressed state pool. Constraints to respect: (a) `T_Graph _Graph{}` is stored **BY VALUE** inside `TSearchState` (`CkAStar_Search.h:119`) — an octree graph must be a cheap copyable *view* (raw pointer to the octree + shared per-query data), exactly the CkPathNetwork pattern, and `FRouteGraph`'s comment (`CkPathNetwork_RouteGraph.h:12-14`) documents the lifetime rule: "raw pointer to the built network, valid only for one synchronous search inside a processor tick. Never store it across frames." (b) `Neighbors()` returns by value (`TArray<T_NodeId>`) — a layered-octree neighbour enumeration allocates per expansion; acceptable but a known cost.

**Does Theta* fit? NO — not without modifying CkAStar.** VERIFIED by reading the relaxation loop, `CkAStar_Search.inl.h:215-237`: it is textbook A* — for each neighbour, `TentativeG = CurrentG + Cost(Current, Neighbor)`, then unconditionally `_CameFrom.Add(Neighbor, Current)`. Theta* requires re-parenting `Neighbor` to `parent(Current)` when line-of-sight holds, i.e. it needs (1) read access to `_CameFrom` **during** relaxation, (2) a graph-supplied `LineOfSight(a,b)` predicate the concept does not declare, and (3) the ability to score against a non-adjacent parent. None of those hooks exist; the loop is a closed, non-customizable body. Three viable options (INFERRED, design choice): (i) add a relaxation-policy template parameter / optional `T_Graph::TryReparent(...)` detected by SFINAE — invasive on a shared T4 module used by GOAP and PathNetwork; (ii) write a separate any-angle search inside the new module and use CkAStar only as the fallback/graph-validation library; (iii) run plain A* on CkAStar and **post-process with a 3D string-pull/smoothing pass** — this matches how the stack already works (CkNavigation's `ExtractWaypoints` is a funnel string-pull, `CkPathNetwork_PathSimplify.h` + `CkPathNetwork_CorridorCompile.h` do the same job for corridors). Option (iii) is the lowest-friction fit for the existing shape.

**Reusable execution plumbing** — `CkAStar_Fragment.h` + `CkAStar_Processor.h`: `FFragment_AStar_Params` (budget knobs → `ToSearchParams()`), `FFragment_AStar_Debug`, tags `FTag_AStar_SearchActive`/`FTag_AStar_SearchComplete`, and templated `TFragment_AStar_SearchState<NodeId, Graph>` / `TFragment_AStar_Result<NodeId>`. `TProcessor_AStar_Execute<Derived, Handle, SearchStateFrag, ResultFrag>` is a **`TParallelProcessor`** that time-slices searches across frames and flips the tags on completion; `TProcessor_AStar_EndPlay` resets state. Derived usage pattern documented at `CkAStar/CLAUDE.md:30-43` (GOAP's aliases). NOTE: `CkPathNetwork` does NOT use this async path — it calls `Search_RouteGraph(..., int32 InMaxIterations = 200000)` synchronously inside its processor (`CkPathNetwork_RoutePlan.h:75-89`). Both patterns are house-legal; the async one is what a large octree would want.

## CkCrowd — how it consumes paths, and where a 3D path would feed flying agents

Root: `Source\CkCrowd\Public\CkCrowd\Agent\` (~45 files). Deps: Core, Ecs, EcsExt, Label, Log, **Navigation**, Physics, Pmg, Projectile, Record, Settings, Shapes, SpatialQuery.

**The consumption seam is exactly one fragment.** `FProcessor_CrowdAgent_Steering::ForEachEntity` (`CkCrowdAgent_Steering_Processor.cpp:25-34`) takes `const FFragment_Nav_PathResult& InPathResult` directly in its view and reads `Get_Status()` + `Get_Waypoints()`. It has zero knowledge of who produced the polyline. `FProcessor_CrowdAgent_OnPathResolved` polls the same fragment (rationale in CkCrowd/Claude.md:493-495: view-iteration driven rather than per-request delegate binding).

**Per-frame pipeline** (from CkCrowd/Claude.md:52-102, cross-checked against the processor files):
`HandleRequests → [provider] → FFragment_Nav_PathResult → NeighborSync → BlockDetect → Separation → Steering → AvoidanceSample → AccelClamp → VelocityBridge → FaceAngle → [CkPhysics] Velocity_Clamp → EulerIntegrator_Update → ApplyOffset → PushApart → ConstrainToNavmesh (SINGLE transform writer) → SceneNode`.
`VelocityBridge` (`CkCrowdAgent_VelocityBridge_Processor.cpp:35-39`) calls the public `UCk_Utils_Velocity_UE::Request_OverrideVelocity(VelocityHandle, InDesired.Get_Velocity(), {})` and short-circuits (Cast, not CastChecked) if the agent lacks the Velocity feature.

**What is ALREADY 3D-safe (VERIFIED by reading `CkCrowdAgent_Steering_Processor.cpp`):** the whole path-follow core. `ToTarget = TargetLoc - CurrentLoc` is a full FVector; the waypoint plane-crossing retirement test (`:72-81`) is a 3D dot product; `DistanceToFinal` accumulates 3D `FVector::Dist` along the polyline (`:110-114`); braking cap `sqrt(2*a*d)` and turn-radius cap are dimension-agnostic. The ONE planar assumption is the degenerate-separation fallback `FVector{-Direction.Y, Direction.X, 0.0}` at `:191`, and it already self-documents: `// else: Direction is near-vertical. Leave the raw force rather than invent an axis.` (`:196`).

**What BLOCKS flying agents (each VERIFIED at file:line):**
1. **`FProcessor_CrowdAgent_ConstrainToNavmesh` — the hard blocker.** It is the single Transform writer; it calls `ANavigationData::FindMoveAlongSurface` (`CkCrowdAgent_ConstrainToNavmesh_Processor.cpp:125`) and lands Z on the reached navmesh surface. Its view (`...Processor.h:36-45`) filters only `FCk_Handle_CrowdAgent` + Transform + Params + PendingDisplacement, `TExclude<FTag_CrowdAgent_Asleep>`. **Opt-out is GLOBAL only** — `UCk_Utils_Crowd_Settings_UE::Get_NavmeshConstraintMode() == Disabled` (`:65`) or "no nav data in world" (`:84`). There is NO per-agent knob: `FCk_Fragment_CrowdAgent_ParamsData` (`CkCrowdAgent_Fragment_Data.h:105-174`) has no constraint-mode field. A flying agent needs either a new exclusion tag on this processor's view or a per-agent params enum.
2. **`FProcessor_CrowdAgent_AvoidanceSample` is 2D.** `CkCrowdAgent_AvoidanceSample_Algorithm.h:255-259` builds the velocity-obstacle quadratic in `FVector2D`; penalties use `FVector::Dist2D` (:317-318); side-preference uses `FVector2D(...X, ...Y)` (:281). A 3D VO/avoidance is new work.
3. **`Separation` zeroes Z** (`CkCrowdAgent_Separation_Processor.cpp:61 OffsetPlanar.Z = 0.0f`) and **`PushApart` zeroes Z** (`CkCrowdAgent_PushApart_Processor.cpp:80 PushFromNeighbor.Z = 0.0f`). Matches the standing memory note "crowd forces must be planar".
4. **`FaceAngle` is yaw-only** (`CkCrowdAgent_FaceAngle_Processor.cpp:37 Heading.Z = 0.0f`) — no pitch/roll for banking flight.
5. **`StationaryMarkup` + `PathRefresh` are Recast-bound** — they paint `UCk_NavArea_CrowdAgent` cost discs into the nav octree and do all their disc/segment math in `FVector2D`/`Dist2D` (`CkCrowdAgent_PathRefresh_Processor.cpp:159,193-212,309-317,410-419,495-504`).

**INFERRED minimum-viable flying integration:** a flying agent could reuse Steering + AccelClamp + VelocityBridge + EulerIntegrator unchanged, and needs (a) an exclusion tag off ConstrainToNavmesh/StationaryMarkup/PathRefresh, (b) either a 3D avoidance sampler or exclusion off AvoidanceSample/Separation/PushApart, (c) a pitch-capable facing processor. That is a per-processor view-filter question, not a rewrite of the path-follow math.

## Entity composition: what a 'nav volume' + 'nav agent' should look like — exemplar = CkPathNetwork

**Name the exemplar: `CkPathNetwork` (Source\CkPathNetwork\).** It is the only existing module that is (a) a non-Recast path provider, (b) sits beside CkNavigation at the same tier, (c) depends on BOTH CkAStar and CkNavigation, and (d) already models the volume/agent PAIR. Its shape maps 1:1 onto "nav volume" + "nav agent". Secondary exemplar for the small canonical quartet: `CkTimer` (root doctrine's designated exemplar). Note: CkPathNetwork ships NO `Claude.md` (confirmed by file listing) — read its headers directly.

**The 'volume' half — `UCk_Utils_PathNetwork_UE` + `FCk_Handle_PathNetwork`** (`CkPathNetwork_Utils.h:19-124`):
- `UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_PathNetwork"))`, `CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_PathNetwork)`.
- `Add(FCk_Handle& InOwner, const FCk_Fragment_PathNetwork_ParamsData&)` → **creates a CHILD entity** carrying authored data + build params; a Setup processor builds the graph next tick (`:29-37`). This is the composition ritual from Source/CLAUDE.md §"Add a feature to an entity".
- `Has`, `Get_IsBuilt`, `Get_NumNodes`, `Get_NumEdges`, `Get_BuildEpoch`, `TryGet_ClosestPointOnNetwork(handle, location, searchRadius, OUT closestPoint)`, `TryGet_RecommendedFollowerTuning`.
- `Request_Rebuild(...)` and `Request_RebuildFromDetector(network, UCk_PathNetwork_Detector_UE*, FBox InWorldBounds, FCk_PathNetwork_VectorizeParams, delegate)` — **the runtime rebuild entrypoint; a volumetric module's "rebake this region" is exactly this shape.**
- Fragments (`CkPathNetwork_Fragment.h`): `FFragment_PathNetwork_Graph` :22-44 holding `pathnetwork::FBuiltNetwork _Network`, `int32 _Epoch`, and a policy-keyed cache of shared static route data; `FFragment_PathNetwork_Requests` :48-65 (std::variant); tag `FTag_PathNetwork_NeedsBuild` :117.
- **The epoch is the invalidation currency** — `_Epoch` bumps on every (re)build; corridors planned against an older epoch replan. A volumetric nav volume rebaking a region wants the same counter.

**The 'agent' half — `UCk_Utils_PathNetworkFollower_UE` + `FCk_Handle_PathNetworkFollower`** (`CkPathNetwork_Utils.h:128-285`):
- Fragments stamped **DIRECTLY on the agent entity** (no child) — `:138-140`: "the crowd integration expects follower state on the agent entity itself". Mirrors `UCk_Utils_CrowdAgent_UE::Add`, which also composes directly onto a `FCk_Handle_Transform`.
- `Add`, `Try_AddOrAdoptByOwnerToken(... , ECk_PathNetworkFollower_OwnershipResult& OutResult)` with `ExpandEnumAsExecs`, `Remove`, `Has`, `Get_OwnerToken`.
- `Request_FindRoute(follower, FCk_Request_PathNetworkFollower_FindRoute, delegate)` — server-only, "mirrors CkNavigation" (:191).
- `Request_SetNetwork(follower, FCk_Handle_PathNetwork, delegate)` — **the volume↔agent binding**.
- `Request_UpdateTuningAndReplan` + `Request_UpdateTuningAndReplanByOwnerToken(FCk_Handle InAnyHandleInWorld, FName, tuning)` — bulk retune by ownership token.
- `Get_RouteResult` / `Get_RouteStatus`; `BindTo_/UnbindFrom_ OnRouteReady` / `OnRouteFailed`.
- Fragments: `FFragment_PathNetworkFollower_Requests` (variant of FindRoute + UpdateTuning) and `FFragment_PathNetworkFollower_Corridor` :93-113 holding `_Result`, `_Network` handle, `_NavQueryFilter`, `_NetworkEpoch` — "the last result plus what it was planned against, so staleness is detectable without the graph."

**Graph adapter file layout to copy** (`CkPathNetwork/Network/`): `_Types.h`, `_BuiltNetwork.{h,cpp}` (the baked structure), `_Build.{h,cpp}` (bake), `_Vectorize.{h,cpp}` (detector mask → geometry), `_RouteGraph.{h,cpp}` (the `AStarGraph` impl + `static_assert`), `_RoutePlan.{h,cpp}` (world-free planning seam shared by runtime and editor preview), `_PathSimplify.{h,cpp}`, `_CorridorCompile.{h,cpp}` (polyline → waypoints), `_Fragment{,_Data}.h`, `_Processor.{h,cpp}`, `_Utils.{h,cpp}`, `_DebugDraw_Processor.{h,cpp}`, `Settings/_ProjectSettings.{h,cpp}`, `Actor/_Actor.{h,cpp}` (level-placed authoring), `Detector/_Detector.{h,cpp}` (per-game geometry source — **the natural home for a voxelizer**).

**Crowd wiring to copy** — `FProcessor_CrowdAgent_OnRouteResolved` (`CkCrowdAgent_OnRouteResolved_Processor.cpp`) is the whole integration in ~190 lines: view takes `FFragment_PathNetworkFollower_Corridor`, gates on `FTag_CrowdAgent_PathPending || FTag_CrowdAgent_Walking` (:47-51), rejects results whose goal doesn't match `_ActiveGoal` within 25cm (:55-57), dedupes via `FFragment_CrowdAgent_InstalledRoute` epoch+goal+tuning compare (:63-71), calls `InstallExternalPath` (:90-93), resets `_WaypointIndex`/`_CurrentSegmentStart` (:99-103), retires already-passed leading waypoints on a mid-walk swap (:112-116), and on `Failed` falls back to `Request_NavigationPath` guarded by `FTag_CrowdAgent_PathNetworkFallbackPending` (:150-183). **A volumetric module's Crowd integration is a rename of this file.**

## Module tier position and what a new sibling may depend on

From `Source/CLAUDE.md` tier table (regenerated 2026-07-02 from Build.cs files; re-verified against the four Build.cs files today):

| Module | Tier | Ck deps (verified against Build.cs) |
|---|---|---|
| CkAStar | **T4** | Core, Ecs, EcsExt, Log (+ engine: Core, CoreUObject only) |
| CkNavigation | **T4** | Core, Ecs, EcsExt, Label, Log, Record, Settings (+ engine: NavigationSystem, AIModule, GameplayTags, DeveloperSettings, Engine) |
| CkPathNetwork | **T4** | AStar, Core, Ecs, EcsExt, Label, Log, **Navigation**, Record, Settings (+ engine NavigationSystem) |
| CkCrowd | **T4** | Core, Ecs, EcsExt, Label, Log, **Navigation**, Physics, Pmg, Projectile, Record, Settings, Shapes, SpatialQuery |
| CkGoap | **T4** | **AStar**, Core, Ecs, EcsExt, Label, Log, Record |
| CkEqs | **T4** | Core, Ecs, EcsExt, EntityTag, Label, Log, Record, Settings, Shapes, SpatialQuery, ThirdParty |
| CkTargeting | **T4** | Actor, Core, Ecs, EcsExt, Label, Log, Provider, Record, Settings |

**Rule (Source/CLAUDE.md:255-256):** "depend only on same-or-lower tiers; runtime never on T5 [editor]." T4 is the feature band and T4→T4 deps are normal (CkPathNetwork→CkNavigation→…). So a new `CkNavVolume`/`CkNav3D` module sits at **T4** and may freely depend on: CkAStar, CkNavigation, CkCore, CkEcs, CkEcsExt, CkLabel, CkLog, CkRecord, CkSettings, CkProvider, CkEntityTag, CkShapes, CkSpatialQuery, CkJolt, CkPmg (debug draw), CkThirdParty. It must NOT be depended on by CkNavigation (that would invert). CkCrowd would gain a dep on it, exactly as it has on CkNavigation today.

**Registration checklist (Source/CLAUDE.md:246-257):** inherit `CkModuleRules` (from `CkBuildConfig/CkBuildConfig.Build.cs` — C++20, unity, `WITH_ANGELSCRIPT_CK` autodetect); add a `CkFoundation.uplugin` entry with `"Type": "Runtime", "LoadingPhase": "Default"` and the Win64/Mac/Linux allowlist (verified format at uplugin lines 69-76 for CkAStar, 729-738 CkNavigation, 739-748 CkCrowd, 749-758 CkPathNetwork); ship a `Claude.md` and add a row to the tier table + the "I need to…" decision tree; editor-only tooling goes in a separate `Ck<X>Editor` twin (precedent: `CkPathNetworkEditor`).

## CkEqs and CkTargeting (skim)

**CkEqs** (`Source\CkEqs\`) — native entity queries, explicitly **NOT UE's EQS** (Source/CLAUDE.md:70; UE-EQS wrappers live in `CkAi`). Ships no `Claude.md`. Public API `UCk_Utils_Eqs_UE : UBlueprintFunctionLibrary`, `Meta = (ScriptMixin = "FCk_Handle_EqsQuery")` (`Query/CkEqs_Utils.h:15-16`): `Cast` / handle conversion / `Get_InvalidEqsQueryHandle`; `Request_RunQuery` (:63), `Request_RunQuery_Immediate` (:82) — **note the synchronous variant, unusual in this codebase**; `Request_CancelQuery` (:99), `Request_CancelAllQueries` for a querier (:110); `BindTo_/UnbindFrom_ OnComplete` (:123,:133); results via `Get_HasResults` / `Get_BestLocation` / `Get_BestEntity` / `Get_AllCandidates` / `Get_IsComplete` / `Get_IsFailed` (:145-170). Files: `Query/CkEqs_{Algorithm,Fragment,Fragment_Data,Processor,Utils}` + `Settings/CkEqs_ProjectSettings`. **Relevance to 3D nav:** CkEqs post-processes candidates with a `_ProjectOntoNav` pass that inlines `UNavigationSystemV1::ProjectPointToNavigation` directly rather than calling `UCk_Utils_Nav_UE::Try_ProjectOntoNavmesh` — CkNavigation/CLAUDE.md:142-144 flags these two as needing to be kept in sync. A volumetric-nav equivalent ("project candidate into free space") would be a third copy unless the projection is abstracted.

**CkTargeting** (`Source\CkTargeting\`) — **its `Claude.md` is stale.** The doc describes candidate scoring, `FFragment_Targeting_BestTarget`, and scoring processors; the module on disk contains exactly ONE feature file pair: `Public/TargetPoint/CkTargetPoint_Utils.{h,cpp}` — six `UFUNCTION`s, all returning `FCk_Handle_Transform` (VERIFIED by rg). No scoring processors, no BestTarget fragment. Treat it as a target-point-entity factory only. Not relevant to 3D nav beyond `FCk_Request_CrowdAgent_FollowTarget` taking a target-point handle.

## Existing 3D / volumetric / flying-nav ambitions: NONE (exhaustive negative result)

All searches run with `rg --no-ignore -in` over `Plugins/CkFoundation/Source/`, excluding `*.generated.h`:

- `octree|SVO|voxel|volumetric` → **zero pathfinding hits.** The only matches are (a) UE's *nav octree* registration in `CkNavAreaMarkup_Utils.h:16` and `CkCrowdAgent_StationaryMarkup_Processor.h:64` and `CkComponentHost_Subsystem.h:24` — that is UE's spatial index for nav modifiers, not a pathfinding structure; (b) `CkNavigation/PLAN.md:28` "**Recast / UE Nav | Keep** | Voxelization + tile regen + editor tooling is months of work to replicate; works fine."; (c) CkUsf material translucency enums (`VolumetricDirectional`) — unrelated; (d) an entt docs link.
- `flying|aerial|flight|fly` (word-bounded) → **zero navigation hits.** All matches are "in-flight" (async request state), CkProjectile ballistic "time of flight", or `CkProjectile_Fragment.h:14` "flying projectile" (a projectile tag).
- `nav3d|3d nav|three-dimensional nav|vertical nav|full 3d` → **zero.** Only `CkPathNetwork_CorridorCompile.h:25` "vertical navmesh projection mismatch" and third-party Doxygen boilerplate.
- `theta\*|thetastar|lazy theta|jump point|JPS|HPA\*` → **one hit total:** `CkNavigation/CLAUDE.md:168` "Future work: Hierarchical path-finding (HPA*) if maps grow large enough that whole-map queries get slow."

**Adjacent stated non-goals / limitations that a 3D module would be answering:**
- `CkNavigation/CLAUDE.md:161` "No off-mesh links / jumps. Recast supports them but we don't surface them."
- `CkCrowd/Claude.md:442` "No off-mesh links / jumps. Rental store has no jumps." and `:452` future-work "Off-mesh link traversal (jumps, ladders)."
- `CkNavigation/PLAN.md:37` — LOCKED scope decision: "Off-mesh links / flowfield / ORCA | **Out of scope**".
- `CkNavigation/PLAN.md:24-37` is the authoritative record that keeping Recast was a *deliberate, argued* decision, not an accident. Any 3D-volumetric proposal is re-opening that decision for a *different agent class* (flyers), not overturning it for walkers — INFERRED framing, but it is the framing the doc invites.

**Also worth knowing:** `CkNavigation/PLAN.md` is a stale in-progress campaign doc (`last_updated: 2026-04-29`, Gates 2-7 marked ⏳ Pending) whose work has since shipped — CkCrowd/Claude.md documents features (BlockDetect, StationaryMarkup, PathRefresh, AvoidanceSample) that postdate it. Its own §"Post-ship cleanup" says to delete `Plan/`. Read it for the locked scope decisions and the module-layout template, not for status.
