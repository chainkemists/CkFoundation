# HEADLINE

Nav3D 2.0 (~25.3k LOC runtime across 39 .cpp/.h in Source/Nav3D, plus an 859-line editor module) is structurally two things bolted together: a genuinely engine-light sparse-voxel-octree core (Morton codes, layer/leaf arrays, A*/Theta*/LazyTheta*, an octree ray-marcher, region/tactical analysis) and a thick Unreal shell (ANavigationData subclass + FNavDataGenerator + APartitionActor chunk actors + a DebugRenderSceneProxy). The UE coupling is narrow but pervasive at three points. (1) GEOMETRY: exactly ONE query API is used for voxelization — `UWorld::OverlapMultiByChannel` with `FCollisionShape::MakeBox` — at 5 sites, backed by a UE-specific per-actor refinement pass that walks `UStaticMesh` render-LOD triangle buffers, `UInstancedStaticMeshComponent` instance transforms, and `ULandscapeHeightfieldCollisionComponent::GetHeight`. This is the Jolt seam and it is small: 2 functions (`IsPositionOccluded`, `IsPositionOccludedPhysics`) plus `GatherOverlappingObjects` and `CacheLayer1Overlaps`. (2) NAV-SYSTEM REGISTRATION: `ANav3DData::FindPath` (Nav3DData.cpp:2154) has its actual pathfinding body COMMENTED OUT — UNavigationSystemV1 integration is already vestigial; the live path API is the standalone `FNav3DPathCoordinator::FindPath` singleton. Nearly all of the ~30 ANavigationData virtual overrides can be deleted. (3) LIFECYCLE: chunk actors are the persistence vehicle AND the runtime spatial index AND the adjacency graph node identity (`TWeakObjectPtr<ANav3DDataChunkActor>` is baked into serialized `FNav3DChunkAdjacency`) — this is the one thing that genuinely needs redesign for an ECS-driven port. Threading is trivially portable (one `FAsyncTask` per volume, a static `TAtomic<bool>` cancel flag, `FPlatformAtomics` counters, no ParallelFor). Debug draw is 100% inside a `FDebugRenderSceneProxy` subclass and is `!UE_BUILD_SHIPPING`-gated. The single largest porting hazard is that voxelization runs `OverlapMultiByChannel` FROM A WORKER THREAD (`FNav3DBoxGeneratorTask::StartBackgroundTask`, Nav3DDataGenerator.cpp:450), reading live UObject/component state off-thread — a Jolt backend must supply an equivalent thread-safe, immutable, off-thread-readable geometry snapshot or the port silently inherits a data race it currently gets away with.

## 0. Module inventory + build dependencies (VERIFIED)

`F:\Nav3D-2.0\Source\Nav3D\Nav3D.Build.cs`:
- Public: `Core`, `AIModule`
- Private: `CoreUObject`, `Engine`, `Slate`, `SlateCore`, `RHI`, `RenderCore`, `DeveloperSettings`, `GameplayTasks`, `AIModule`, `NavigationSystem`, `Landscape`, `InputCore`
- `if (Target.bBuildEditor) PublicDependencyModuleNames.Add("UnrealEd")` — note this is a *Public* dep added in editor builds; ANav3DData/UNav3DRaycaster call `GEditor` directly from the runtime module (`Nav3DRaycaster.cpp:90`).
- Third party: `Source/ThirdParty/libmorton` (header-only, 8 headers) — fully portable, no UE deps.

Runtime line counts (largest first): `Nav3DData.cpp` 4305, `Nav3DVolumeNavigationData.cpp` 2510, `Tactical/Nav3DTacticalReasoning.cpp` 2502, `Nav3DTypes.h` 1575, `Nav3DDataGenerator.cpp` 1421, `Tests/Nav3DTestVolume.cpp` 1181, `Nav3DNavDataRenderingComponent.cpp` 1086, `Pathfinding/Core/Nav3DVolumePathfinder.cpp` 999, `Nav3DUtils.cpp` 873, `Raycasting/Nav3DRaycaster.cpp` 825. Total 25,265.

Editor module `Source/Nav3DEditor` (859 lines): only registers a details customization for `ANav3DData` (`Nav3DEditor.cpp:22-27`) plus `FPropertySection` "Nav3D". Nothing algorithmic. Drop entirely.

`Nav3D.uplugin`: Win64-only whitelist on both modules.

## 1. NAVIGATION SYSTEM REGISTRATION — classification: (a)/(c), mostly DELETABLE

`ANav3DData : public ANavigationData` — `Nav3DData.h:36`. Declares ~30 virtual overrides (`Nav3DData.h:251-318`).

**The FindPath delegate is DEAD CODE.** `ANav3DData::FindPath` (`Nav3DData.cpp:2154-2209`) is a `static FPathFindingResult(const FNavAgentProperties&, const FPathFindingQuery&)`. Its non-trivial branch is entirely commented out (`Nav3DData.cpp:2196-2204` — the `FNav3DPathFinder::GetPath` call is inside `/* */`). It only handles the degenerate `Start≈End` case. **I did not find any assignment to `FindPathImplementation`/`FindHierarchicalPathImplementation` anywhere in the plugin** (grep for those symbols returns nothing), so UNavigationSystemV1 pathfinding through Nav3D never worked in 2.0. Conclusion: UE nav-system pathfinding integration is already abandoned; nothing to preserve.

**The live pathfinding entry point** is `FNav3DPathCoordinator::FindPath(FNav3DPath&, const FNav3DPathingRequest&)` — `Pathfinding/Core/Nav3DPathCoordinator.h:17`, static, backed by a `TUniquePtr<FNav3DPathCoordinator> Instance` file-static singleton (`Nav3DPathCoordinator.cpp:13`). It is called from exactly one place in-plugin: `UNav3DPathLibrary::FindNav3DPath` (`Nav3DPathLibrary.cpp:64`), a BlueprintCallable that does `TActorIterator<ANav3DData>` to find the nav data (`Nav3DPathLibrary.cpp:36-40`). **This is already the "our own pathfinding API" you want** — it takes plain FVector start/end, an `FNavAgentProperties` (only `.AgentRadius` is used, `Nav3DPathCoordinator.cpp:109/116`), and an algorithm enum.

**Query filters are a stub.** `FNav3DQueryFilter : INavigationQueryFilterInterface` (`Pathfinding/Search/Nav3DQueryFilter.h:20`) — every single method in `Nav3DQueryFilter.cpp` is an empty "Not implemented - Nav3D uses voxel-based navigation instead of nav areas" body except `GetHeuristicScale()` (returns `QueryFilterSettings.HeuristicScale`) and `CreateCopy()`. Installed via `ANav3DData::RecreateDefaultFilter()` → `DefaultQueryFilter->SetFilterType<FNav3DQueryFilter>()` (`Nav3DData.cpp:1272`). **The only surviving datum is a float.** Replace `FNav3DQueryFilterSettings` (`Nav3DPathingTypes.h:85`) with a plain struct.

**Path objects.** `FNav3DPath : public FNavigationPath` (`Pathfinding/Core/Nav3DPath.h:59`) — adds only `TArray<float> PathPointCosts` and overrides `GetCostFromNode`/`GetCostFromIndex`. Everything the algorithms touch is `GetPathPoints()` (a `TArray<FNavPathPoint>`), `ResetForRepath()`, `MarkReady()`, `IsValid()`, `IsPartial()`. `FNavPathPoint` is used for `{FVector Location; NavNodeRef NodeRef;}` only. There is already a POD mirror: `FNav3DPathData`/`FNav3DPathPoint` (`Nav3DPath.h:11,42`) produced by `CreatePathData()`. **Port target: make `FNav3DPath` a plain struct of `TArray<FNav3DPathPoint> + TArray<float> Costs + flags`; the search algorithms need no other FNavigationPath behavior.**

**Path invalidation** is the one real ANavigationData service consumed: `ANav3DData::InvalidateAffectedPaths` (`Nav3DData.cpp:1983-2038`) walks the base class's `ActivePaths` / `ActivePathsLock` and calls `SharedPath->Invalidate()`. Under ECS this becomes "iterate entities holding a path fragment whose points fall in dirty bounds". Small (~55 lines).

**Supported agents.** Only two things read agent config: `FNav3DVolumeNavigationDataGenerator::DoWork` does `GenerationSettings.VoxelExtent = NavDataConfig.AgentRadius * 2.0f` where `NavDataConfig = Owner->GetConfig()` (`Nav3DDataGenerator.cpp:21,31`), and `ANav3DData::GetVoxelExtent()` = `GetConfig().AgentRadius * 2.0f` (`Nav3DData.cpp:3428-3433`). Plus `FNav3DVolumeNavigationData::GetMinLayerIndexForAgentSize(float AgentRadius)` (`Nav3DVolumeNavigationData.cpp:1177`). **Total agent coupling = one float.**

**Virtuals worth keeping as free functions (all pure-octree, no UE nav-system logic inside):** `GetRandomPoint` (632), `GetRandomReachablePointInRadius` (665), `GetRandomPointInNavigableRadius` (756), `BatchRaycast` (798), `FindMoveAlongSurface` (829), `ProjectPoint` (881), `BatchProjectPoints` (940/957), `DoesNodeContainLocation` (996). Note `ProjectPoint` and `GetRandomReachablePointInRadius` do LINEAR SCANS over every node of every layer (`Nav3DData.cpp:906-934`, `692-709`) — O(n) per query, a pre-existing perf bug worth fixing during the port.

**Virtuals that are pure noise and should evaporate:** `OnNavAreaChanged/Added`, `GetNewAreaID`, `GetMaxSupportedAreas` (returns 32, `Nav3DData.cpp:1059`), `ShouldExport`, `LogMemUsed`, `TickActor` (pure Super:: passthrough, `Nav3DData.cpp:1066-1069`), `OnStreamingLevelAdded/Removed`, `CheckToDiscardSubLevelNavData` (`Nav3DData.cpp:1247`, uses `GEngine->IsSettingUpPlayWorld()` + `CleanUpAndMarkPendingKill()`).

**`NavNodeRef` packing is load-bearing and portable:** `FNav3DNodeAddress::GetNavNodeRef()` = `LayerIndex<<28 | NodeIndex<<6 | SubNodeIndex` (`Nav3DTypes.h:140-144`). Keep the packing, drop the typedef dependency.

## 2. GEOMETRY GATHERING / VOXELIZATION — THE CRITICAL SEAM. Classification: (b), and it is small

**Complete list of UE collision-query call sites in the whole plugin** (grep for Overlap*/Sweep*/LineTrace* across Source, excl. ThirdParty — 5 sites total, ALL `OverlapMultiByChannel`, plus 1 LineTrace):

| # | File:line | Purpose | Shape |
|---|---|---|---|
| 1 | `Nav3DVolumeNavigationData.cpp:784` | `GatherOverlappingObjects()` — one whole-volume query to build the candidate actor list | `MakeBox(VolumeBounds.GetExtent())` |
| 2 | `Nav3DVolumeNavigationData.cpp:1638` | `CacheLayer1Overlaps()` — ONE QUERY PER LAYER-1 VOXEL, sequential loop over `Layer1.GetMaxNodeCount()` | `MakeBox(L1Extent + Clearance)` |
| 3 | `Nav3DVolumeNavigationData.cpp:1705` | `IsPositionOccludedPhysics()` — per leaf sub-node (64 per leaf) | `MakeBox(BoxExtent + Clearance)` |
| 4 | `Tactical/Nav3DTacticalReasoning.cpp:1082` | `VerifyRegionsAgainstStaticGeometry()` | `MakeBox(RegionExtent*0.9)` |
| 5 | `Tactical/Nav3DTacticalReasoning.cpp:1189` | `IsRegionInsideGeometry()` | `MakeSphere(10.0f)` |
| 6 | `Tactical/Nav3DTacticalReasoning.cpp:532` | tactical visibility LOS, `LineTraceSingleByChannel` on `ECC_Visibility` w/ `bTraceComplex=true` | ray |

No sweeps. No `GeometryExport`/`FNavigableGeometryExport`/`INavRelevantInterface` usage anywhere. No `OverlapBlockingTestByChannel`.

**Channel/params config:** `FNav3DDataGenerationSettings` (`Nav3DTypes.h:41-78`): `CollisionChannel = ECC_WorldStatic` (default), `Clearance` (float, default 0), `AdjacencyClearance` (500), `FCollisionQueryParams CollisionQueryParameters` with `bFindInitialOverlaps=true, bTraceComplex=false, TraceTag="Nav3DRasterize"`. Both call sites 2 and 3 force `QueryParams.bTraceComplex = false` locally.

**Voxelization pipeline** (`FNav3DVolumeNavigationData::GenerateNavigationData`, `Nav3DVolumeNavigationData.cpp:585-660`):
1. `Nav3DData.Initialize(VoxelExtent, Bounds)` — `Nav3DTypes.cpp:67`. LeafSize = VoxelSize*4; LayerCount = ceil(log2(VolumeMaxDim/LeafSize))+1; NavigationBounds is snapped to a power-of-2 cube. Layer L node size = NavBoundsSize / 2^(exp-L).
2. `GatherOverlappingObjects()` (781) — site #1, then aggressive filtering via `RemoveAllSwap` (793-844): drops `!CanEverAffectNavigation()`, drops `IsCollisionOnlyComponent()` (= `USphereComponent`/`UBoxComponent`/`UCapsuleComponent`, 854-882), drops SMC/ISM without `HasValidCollisionGeometry()` (= `UBodySetup::AggGeom` has any Convex/Box/Sphere/Sphyl/TaperedCapsule elem, 885-950), keeps `ULandscapeHeightfieldCollisionComponent`/`ULandscapeMeshCollisionComponent`.
3. **Early-out**: if `OverlappingObjects.Num()==0 && DynamicOccluders.Num()==0` → volume marked valid with zero rasterization (610-617). Empty volumes are free.
4. `FirstPass()` (1526) → `CacheLayer1Overlaps()` (1593) [site #2] then per-Layer-0 node `IsPositionOccluded(pos, L0Extent)` (1561); blocked-node sets propagate up via parent Morton codes (1577-1586).
5. `RasterizeInitialLayer()` (1804) — iterates children of blocked L0 parents only, `IsPositionOccluded(leafPos, leafExtent)` at 1832; then `RasterizeLeaf()` at 1868.
6. `RasterizeLeaf()` (1784-1802) — **64 sub-nodes, each a full `IsPositionOccludedPhysics()` physics overlap** [site #3]. This is the hot loop.
7. `RasterizeLayer(L)` for L=1..N, `BuildParentLinkForLeafNodes`, `BuildNeighbourLinks(L)` downward — all pure octree, no world access.

**The refinement layer (this is what a Jolt backend must replace, not just the overlap):** `IsPositionOccluded` (`Nav3DVolumeNavigationData.cpp:952-1175`) has two paths.
- Fast path (963-1036): looks up the parent L1 Morton in `Layer1VoxelOverlapCache`; if that cache entry has zero actors → immediately not occluded. Otherwise, for each cached actor: `Actor->GetComponentsBoundingBox(true)` AABB reject, then `Cast<ALandscapeProxy>` → `CheckLandscapeProxyOcclusion`, then `Actor->GetComponents<UInstancedStaticMeshComponent>` → `CheckInstancedStaticMeshOcclusion`, then `GetComponents<UStaticMeshComponent>` → `CheckStaticMeshOcclusion`.
- Slow path (1038-1174): dynamic occluders first (`DynamicOccluders`, `TWeakObjectPtr<const AActor>`), then `OverlappingObjects` with the same Cast chain.

**Leaf-level truth is tri-box overlap against RENDER geometry, not collision geometry.** `CheckStaticMeshTrianglesWithTransform` (`Nav3DVolumeNavigationData.cpp:1335-1385`): `StaticMesh->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer` + `.IndexBuffer`, loops every triangle, `Transform.TransformPosition`, then `Nav3D::TriBoxOverlapUtils::TriBoxOverlap(Position, FVector(BoxExtent), V0,V1,V2)`. **Note the semantic mismatch worth flagging: the broadphase filters on `BodySetup->AggGeom` (simple collision) but the narrowphase tests LOD0 render triangles.** A Jolt port would naturally use the Jolt shape for both, which will CHANGE occupancy results (usually for the better, but it is a behavior change).

ISM: `CheckInstancedStaticMeshOcclusion` (1404-1442) loops `GetInstanceCount()`, `GetInstanceTransform(i, T, /*bWorldSpace=*/true)`, re-runs the full triangle test per instance. No per-instance culling — O(instances × triangles) per sub-voxel. HISM gets no special handling (it's an ISM subclass, so it inherits this path, losing the cluster tree).

Landscape: `CheckLandscapeProxyOcclusion` (1444-1504) — `LandscapeProxy->GetActorLocation().Z` floor test, then `LandscapeActorToWorld().InverseTransformPosition`, `ComponentSizeQuads` → `LandscapeInfo->XYtoCollisionComponentMap`, `CollisionComponent->GetHeight(x, y, EHeightfieldSource::Complex)`. **Landscape has no Jolt analogue in your stack — either drop it (Rewind99 is an interior sim; likely fine) or feed heightfields in as Jolt HeightFieldShapes.**

**Minimal geometry-query interface that would suffice (VERIFIED as sufficient — these are the only world reads in the voxelizer):**
```cpp
struct FNav3DGeomHandle { uint64 Id; };   // opaque body/shape id

class INav3DGeometryBackend {
public:
  // (1) replaces GatherOverlappingObjects + CacheLayer1Overlaps.
  //     Must be callable from a worker thread.
  virtual void QueryOverlappingBodies(const FBox& WorldAABB,
                                      TArray<FNav3DGeomHandle>& Out) const = 0;
  // (2) replaces IsPositionOccluded/IsPositionOccludedPhysics entirely.
  //     THE one call the rasterizer actually needs.
  virtual bool IsBoxOccupied(const FVector& Center, float HalfExtent) const = 0;
  // (2b) restricted form, for the L1-cache fast path
  virtual bool IsBoxOccupiedBy(const FNav3DGeomHandle& Body,
                               const FVector& Center, float HalfExtent) const = 0;
  // (3) cheap AABB reject, replaces GetComponentsBoundingBox / Bounds.GetBox()
  virtual FBox GetBodyBounds(const FNav3DGeomHandle& Body) const = 0;
  // (4) replaces the tactical LineTraceSingleByChannel LOS pass
  virtual bool RaycastBlocked(const FVector& From, const FVector& To) const = 0;
};
```
Jolt maps 1:1: (1) = `BroadPhaseQuery::CollideAABox`, (2) = `NarrowPhaseQuery::CollideShape` with a `BoxShape` (or `CollidePoint` for the trivial case), (4) = `NarrowPhaseQuery::CastRay`. `Clearance` folds into the half-extent as it does today. `CollisionChannel` becomes a Jolt `ObjectLayerFilter`.

**Everything else in the voxelizer is pure math** and needs no backend.

## 3. THREADING / ASYNC — classification: (a), trivially portable

Complete inventory (grep across Source, excl. ThirdParty):
- **One async primitive**: `using FNav3DBoxGeneratorTask = FAsyncTask<FNav3DBoxGeneratorWrapper>` (`Nav3DDataGenerator.h:50`); `FNav3DBoxGeneratorWrapper : FNonAbandonableTask` (`Nav3DDataGenerator.h:34`) whose `DoWork()` calls `FNav3DVolumeNavigationDataGenerator::DoWork()` (one volume). Launched at `Nav3DDataGenerator.cpp:450` `RunningElement.AsyncTask->StartBackgroundTask()`. Concurrency cap: `MaximumGeneratorTaskCount = min(max(TaskGraph worker count * 2, 1), GenerationSettings.MaxSimultaneousBoxGenerationJobsCount)` (`Nav3DDataGenerator.cpp:57-61`, default cap 4).
- **No `ParallelFor` anywhere.** `CacheLayer1Overlaps` even declares an unused `FCriticalSection CriticalSection;` (`Nav3DVolumeNavigationData.cpp:1622`) and then logs "Using sequential overlap queries" (1625) — a parallel version was clearly removed.
- **No `TFuture`/`TPromise`/`FRunnable`/`UE::Tasks`.**
- Cancellation: `static TAtomic<bool> FNav3DVolumeNavigationData::bSCancelRequested` (`Nav3DVolumeNavigationData.cpp:38`, decl `Nav3DVolumeNavigationData.h:154`) with `RequestCancelBuildAll/ClearCancelBuildAll/IsCancelRequested` inline (`Nav3DVolumeNavigationData.h:209-211`). Polled ~15 times through the rasterizer. There is also a dead `TAtomic<bool>* CancelFlag` field in the settings struct (`Nav3DVolumeNavigationData.h:30`) that is never assigned.
- Counters: `FPlatformAtomics::InterlockedIncrement(&NumOccludedVoxels)` at 998, 1013, 1028, 1066, 1167, 1769 — on a `mutable int32`.
- Locks: `FCriticalSection` ×3 — `UNav3DWorldSubsystem::Mutex` (spatial grid), `ANav3DData::VolumeLoadingMutex` (declared, **never used** — grep shows zero `FScopeLock` on it), `ANav3DData::CachedDiscoverableVolumesMutex` (guards the render-thread cache).
- Game-thread marshalling: `FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(..., ENamedThreads::GameThread)` in `RequestDrawingUpdate` (`Nav3DData.cpp:1220-1224`) — debug-draw only, `!UE_BUILD_SHIPPING`.
- Explicit thread check: `ANav3DData::GetAllDiscoverableVolumes()` returns a cached copy `if (!IsInGameThread())` because `TActorIterator` is illegal off-thread (`Nav3DData.cpp:3699-3703`).

**THE hazard for the port (INFERRED but high-confidence):** the worker task calls `Settings.World->OverlapMultiByChannel` (site #2, #3) and then dereferences `AActor*`/`UPrimitiveComponent*`/`UStaticMesh*` and reads `GetRenderData()->LODResources[0]` vertex/index buffers from that worker thread. UE physics scene reads are (mostly) thread-safe under a read lock; touching UObject render data and calling `Actor->GetComponents<>()` off-thread is not. The plugin gets away with it because builds happen in-editor with a quiescent world. **An ECS/Jolt port must not inherit this by accident: give the backend an immutable snapshot (Jolt bodies are naturally snapshot-able under `PhysicsSystem` read lock, or bake a triangle soup up front).** This is also the reason the port could legitimately go wider on parallelism than the current code does.

Deferred/timer-driven work is NOT threading, it's game-thread chunking — see §8.

## 4. SERIALIZATION & DATA — classification: (a) for the payload, (c) for the container

Two `FArchive` entry points, both on UObjects:

**`UNav3DDataChunk::Serialize`** (`Nav3DDataChunk.cpp:4-81`). Writes `ENav3DVersion Version` (enum, single value `V_2025090900`, `Nav3DTypes.h:33-38`), a size-patch field (Tell/Seek back-patch, 10-13 & 71-80), then `VolumeCount` and per-volume `FNav3DVolumeNavigationData::Serialize(Ar, Version)`, then boundary voxels (`Morton`, `AdjacentChunkVoxels`, a navigable flag byte). Rebuilds `MortonToBoundaryIndex` on load (62-69). **Note: the `FNav3DEdgeVoxel` face-flag bitfields and `LayerIndex` are NOT serialized** — only Morton/adjacency/navigable survive a save/load round trip.

**`FNav3DVolumeNavigationData::Serialize`** (`Nav3DVolumeNavigationData.cpp:741-770`): size patch, `VolumeBounds`, `Nav3DData`, `bInNavigationDataChunk`, `TacticalData`. `operator<<(FArchive&, FNav3DData&)` (`Nav3DTypes.h:701-709`) writes `Layers`, `LeafNodes`, `NavigationBounds`, `VolumeBounds` — **`BlockedNodes` is deliberately not serialized** (build-time scratch), and `bIsValid` is reconstructed as `LayerCount > 0` (`Nav3DVolumeNavigationData.cpp:755-759`).

**PORTABILITY LANDMINE**: `operator<<(FArchive&, FNav3DNodeAddress&)` does `Archive.Serialize(&Data, sizeof(FNav3DNodeAddress))` — a **raw memory dump of a mixed-type bitfield struct** (`Nav3DTypes.h:170-174`). The struct is `uint8 LayerIndex:4; uint_fast32_t NodeIndex:22; uint8 SubNodeIndex:6;` (`Nav3DTypes.h:154-156`). MSVC does not pack bitfields of differing underlying types into one allocation unit, so this is ~12 bytes including padding, and the on-disk format is compiler/ABI-dependent. Any port that changes the struct (or compiler) silently invalidates all baked data. Fix this during the port: write the packed `uint32` from `GetNavNodeRef()` instead.

**Size model** (INFERRED from the layout, not measured): `FNav3DNode` = MortonCode(8) + Parent + FirstChild + Neighbours[6] = 8 + 8×sizeof(FNav3DNodeAddress) ≈ 104 bytes/node. `FNav3DLeafNode` = `uint64 SubNodes` + Parent ≈ 24 bytes and encodes 64 sub-voxels as a bitmask (`Nav3DTypes.h:184-185`) — very compact. Leaf count is allocated as `BlockedNodes[0].Num() * 8` (`Nav3DVolumeNavigationData.cpp:631`), i.e. only leaves under occupied L0 parents exist. Reported live via `FNav3DData::GetAllocatedSize()` / `ANav3DData::LogMemUsed()` (`Nav3DData.cpp:1149-1175`) and `ANav3DData::EstimateMemoryUsage()` (3248).

**Cooked vs runtime-built**: `SupportsRuntimeGeneration()` returns **`false`** (`Nav3DData.cpp:622-625`) — Nav3D is bake-only. `ConditionalConstructGenerator` only builds a generator when `SupportsRuntimeGeneration() || !World->IsGameWorld()` (`Nav3DData.cpp:1184-1185`), i.e. **editor only**. `SupportsStreaming()` returns `RuntimeGeneration != Dynamic` (627). So: octree is baked into `ANav3DDataChunkActor` packages in the editor and is read-only at runtime; the *only* runtime mutation path is `RebuildDirtyBounds` for dynamic occluders (§8).

**What lands in the chunk actor package** (`Nav3DDataChunkActor.h:22-43`): `TArray<TObjectPtr<UNav3DDataChunk>> Nav3DChunks` (the octree), `FBox DataChunkActorBounds`, `TArray<FNav3DChunkAdjacency> ChunkAdjacency` (**contains `TWeakObjectPtr<ANav3DDataChunkActor> OtherChunkActor` — a serialized cross-actor object reference**, see §9), `FCompactTacticalData CompactTacticalData`, `TArray<FCompactRegion> CompactRegions`, `TMap<FVector, FChunkConnectionInterface> ConnectionInterfaces` (**a TMap keyed by FVector — float-keyed map, porter hazard**). `ANav3DDataChunkActor::Serialize` (`Nav3DDataChunkActor.cpp:19-63`) adds nothing but logging.

Tactical data is deliberately dual-format: legacy `FNav3DRegion`/`FConsolidatedTacticalData` (marked `@deprecated`, `Nav3DTypes.h:889,1261`) kept alive **only for debug rendering**, and the shipping `FCompactRegion`(LayerIndex+Center+Size, `Nav3DTypes.h:1309`) / `FCompactTacticalData` with a `FVolumeRegionMatrix` sparse `TMap<uint16,uint64>` bitmask visibility (`Nav3DTypes.h:1367`). Hard caps: **max 64 regions per volume** (`EncodeKey` checkSlow `LocalRegionID < 64`, `Nav3DTypes.h:1379`) and **max 1024 volumes** (`TargetVolumeID < 1024`, 1380).

## 5. ACTOR / UOBJECT SURFACE — classification: (c) for chunk actors, (a) for the rest

| Class | Base | What it actually does beyond placement/persistence |
|---|---|---|
| `ANav3DData` | `ANavigationData` | God object (4305 lines). Owns `TArray<TObjectPtr<ANav3DDataChunkActor>> ChunkActors` (`Nav3DData.h:362`) as the registry, `TUniquePtr<FNav3DTacticalReasoning>`, consolidated tactical data, generation settings, perf stats. Real logic: chunk registration/cleanup, region consolidation & global-ID remapping, cross-chunk adjacency/visibility building, tactical queries. **Its ANavigationData-ness contributes almost nothing** — see §1. |
| `ANav3DDataChunkActor` | `APartitionActor` | `Nav3DDataChunkActor.h:11`. **This is the hard one.** It is simultaneously (i) the World Partition streaming cell (`GetDefaultGridSize()` returns 25600, `.cpp:137-140`; `GetStreamingBounds()` returns `DataChunkActorBounds`, `.cpp:149-152`), (ii) the serialization container for one volume's octree, (iii) the runtime spatial-index entry (registers into `UNav3DWorldSubsystem` on BeginPlay, `.cpp:160-215`), (iv) **the identity of a node in the chunk adjacency graph — baked `TWeakObjectPtr` in `FNav3DChunkAdjacency::OtherChunkActor`**, and (v) the boundary-voxel bake host. Ctor: `SetCanBeDamaged(false); SetActorEnableCollision(false)` — no gameplay role at all. `ContainsPoint()` is `DataChunkActorBounds.IsInsideXY(Point)` — **XY only, a latent bug for a 3D nav system** (`.cpp:259-262`). |
| `ANav3DBoundsVolume` | `ANavMeshBoundsVolume` | `Nav3DBoundsVolume.h:8`. **44 lines of implementation total.** Adds a `FGuid VolumeGUID` auto-generated in `PostLoad`/`OnConstruction`, and `GetVolumeID()` = `GetTypeHash(GUID) & 0xFFFE` (`.cpp:25-44`). Everything else — the box brush, the bounds, the nav-system registration that makes `GetNavigationBoundsForNavData` return it — is inherited. **Pure editor placement vehicle for an FBox + a uint16 id.** |
| `UNav3DDataChunk` | `UNavigationDataChunk` | Thin: `TArray<FNav3DVolumeNavigationData> NavigationData` + boundary voxels + `Serialize`. `GetVolumeNavigationData()` always returns `&NavigationData[0]` (`.cpp:91-94`) — **the array is de facto single-element everywhere**. |
| `UNav3DWorldSubsystem` | `UWorldSubsystem` | 73 lines. A 2D (`FIntVector` with Z always 0, `.h:40-46`) uniform-grid spatial index of chunk actors, `CellSize` forced to 25600 in `Initialize` (`.cpp:6`). Trivially replaceable. |
| `UNav3DDynamicOcclusion` | `UActorComponent` | The only runtime-tick consumer. See §8. |
| `UNav3DSettings` | `UDeveloperSettings` | `config=Engine, defaultconfig`. Pure config bag (algorithm defaults, heuristic scale, partition sizes, `MaxRegions`). `UNav3DSettings::Get()` singleton. → plain struct or a CkFoundation asset definition. |
| `UNav3DRaycaster` | `UObject` | **`UObject` for no reason.** No UPROPERTY that matters (`uint8 bShowLineOfSightTraces:1`), no reflection use — and it is `NewObject<>`'d per call in hot paths: `Nav3DMultiChunkRaycaster.cpp:123` (per chunk segment!), `Nav3DThetaStar.cpp:230`, `Nav3DData.cpp:807`. **Make it a plain class; this is also a live GC-pressure bug.** |
| `UNav3DMultiChunkRaycaster`, `UNav3DConeCaster` | `UObject` | Same — stateless, `UObject` only by habit. |
| `UNav3DPathHeuristicCalculator` / `UNav3DPathTraversalCostCalculator` | `UObject` (abstract, `EditInlineNew`) | Genuine strategy-pattern polymorphism (Manhattan/Euclidean; Distance/Fixed). UObject-ness buys only `TSubclassOf` config + `NewObject<>(GetTransientPackage(), Settings->DefaultCostCalculator)` at `Nav3DPathCoordinator.cpp:62,66`. → plain vtable + a factory enum, or CkFoundation asset definitions. |
| `ANav3DTestVolume` | `AActor` | 1181 lines of procedural obstacle generation (Uniform/Clustered/Perlin/Ring/Disc/Spline) into a `UInstancedStaticMeshComponent`. Test-content only — **drop or reimplement as a gym.** |
| `ANav3DPathTest`, `ANav3DRaycasterTest` | `AActor` + rendering components | Editor visualization harnesses. Drop. |
| `UNav3DNavDataRenderingComponent` | `UPrimitiveComponent` | §6. |

## 6. DEBUG DRAW — classification: (a) delete-and-rewrite; already isolated

**There is essentially no `DrawDebug*` usage.** `DrawDebugHelpers.h` is included at `Nav3DData.cpp:8` and `Nav3DVolumeNavigationData.cpp:27` but grep finds **zero `DrawDebugBox/Line/Sphere/String` call sites**. All visualization goes through the scene-proxy path:

- `FNav3DMeshSceneProxy : public FDebugRenderSceneProxy` (`Nav3DNavDataRenderingComponent.h:32`) — overrides `GetDynamicMeshElements`, `GetViewRelevance`, plus `RenderVoxelSurfaces(FPrimitiveDrawInterface*, FMeshElementCollector&)`, `DebugDrawRegions/RegionIds/Adjacency/Visibility/BestCover/Portals/RegionInfo`. Uses `GEngine->DebugMeshMaterial->GetRenderProxy()` (`Nav3DNavDataRenderingComponent.cpp:675-676`).
- `FNav3DDebugDrawDelegateHelper : FDebugDrawDelegateHelper` — wrapped in `#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST` (`Nav3DNavDataRenderingComponent.h:86,123`).
- Gate: `UNav3DNavDataRenderingComponent::IsNavigationShowFlagSet(World)` (`.cpp:889-920`) reads `WorldContext->GameViewport->EngineShowFlags.Navigation`, and in editor iterates `GEditor->GetAllViewportClients()`. So it piggybacks the stock `show Navigation` flag.
- Trigger: `ANav3DData::RequestDrawingUpdate` (`Nav3DData.cpp:1200-1227`), whole body inside `#if !UE_BUILD_SHIPPING`; `UpdateDrawing()` (1275-1290) same.
- `ANav3DData::ConstructRenderingComponent()` (1015) is the ANavigationData hook that creates it.
- `Nav3DPathTestRenderingComponent.cpp` (156 lines) and `Nav3DRaycasterTest.cpp` are the same pattern for the test actors; both call `GEditor->RedrawLevelEditingViewports()` (`Nav3DPathTest.cpp:155`, `Nav3DRaycasterTest.cpp:331`).

**The whole 1086-line rendering component is deletable in one piece.** Nothing else depends on it except `RequestDrawingUpdate` calls (which become no-ops) and `ANav3DData::PostLoad`'s compact→legacy tactical conversion, which exists *solely* to feed the debug renderer (`Nav3DData.cpp:438-478`, comment at 465: "Convert to Build format for debug rendering only"). **Killing debug draw also lets you kill the entire deprecated `FNav3DRegion`/`FConsolidatedTacticalData` shadow format** — a meaningful simplification. Reimplement against CkGameplayDebugger.

## 7. MATH / CONTAINER TYPES — classification: (a), keep as-is

Everything used is standard and stays free in a CkFoundation module: `FVector`, `FVector2D`, `FVector3f`, `FIntVector`, `FIntPoint`, `FBox`, `FBoxSphereBounds`, `FQuat`, `FTransform`, `FMath`, `FRandomStream`, `TArray`, `TMap`, `TSet`, `TQueue`, `TOptional`, `TUniquePtr`, `TSharedPtr/Ref`, `TWeakObjectPtr`, `TObjectPtr`, `TInlineComponentArray`, `FGuid`, `FName`, `FString`, `Algo::Reverse`, `FPlatformTime::Seconds()`.

Exotic / noteworthy:
- `using MortonCode = uint_fast64_t` (`Nav3DTypes.h:9`) — **`uint_fast64_t` is platform-variable width**; combined with the raw-bytes serialization of node addresses this is an ABI hazard. Pin to `uint64`.
- `FNav3DNodeAddress` mixed-type bitfield (`Nav3DTypes.h:154-156`) — see §4. Hard caps it imposes: **15 layers max** (LayerIndex:4, 15 == invalid sentinel), **4,194,303 nodes per layer** (NodeIndex:22), 64 sub-nodes.
- `TNavStatArray<T>` (AIModule) used for the generator's pending/running/registered bound arrays (`Nav3DDataGenerator.h:167,193,199-201`) — a stats-instrumented TArray; **swap for TArray, it's the only AIModule container dependency**.
- `TMap<FBox, int32> LoadedVolumeReferenceCounts` (`Nav3DData.h:392`) and `TMap<FVector, FChunkConnectionInterface> ConnectionInterfaces` (`Nav3DDataChunkActor.h:43`) — **float-keyed TMaps**. Exact-equality lookups on floats; fragile and worth replacing with integer volume/face ids during the port.
- `libmorton` (`Source/ThirdParty/libmorton/morton.h`) — header-only BMI2/LUT Morton encode/decode. Ports untouched.
- `TriBoxOverlap` (`TriBoxOverlap.h/.cpp`) — Akenine-Möller SAT, raw `float[3]` arrays, one `TBO_FINDMINMAX` macro. Zero UE dependency beyond `FVector` in the signature. Ports untouched.
- `GraphAStar.h` is `#include`d in `Nav3DUtils.h:5` and `FNav3DUtils::GraphAStarResultToNavigationTypeResult(EGraphAStarResult)` exists — but the actual searches are hand-rolled `FNav3DAStar`/`ThetaStar`/`LazyThetaStar`, **not** `FGraphAStar`. Dead dependency.

## 8. TICK / UPDATE — classification: (c), but the surface is tiny

**Nothing ticks per-frame for pathfinding.** `ANav3DData::TickActor` is a bare `Super::` passthrough (`Nav3DData.cpp:1066-1069`).

The update machinery is entirely **editor-build-time**, driven by `FTimerManager` on the game thread:
- `FNav3DDataGenerator::TickAsyncBuild(float)` (`Nav3DDataGenerator.cpp:250-274`) — the `FNavDataGenerator` virtual, called by UNavigationSystemV1's build tick. Reads `NavigationSystem->GetNumRunningBuildTasks()` to decide submission count, calls `ProcessAsyncTasks`, then `OnNavigationDataUpdatedInBounds` + `RequestDrawingUpdate`.
- `FNav3DDataGenerator::StartChunkedBuildCompletion()` (`.cpp:106-114`) → `World->GetTimerManager().SetTimer(ChunkedBuildTimerHandle, ..., 0.1f, /*loop*/true)` → `ProcessBuildChunk()` (116-239) with a **50 ms budget per tick** (`MaxChunkTimeSeconds = 0.05f`) and a hard break every 10 iterations. On drain, it fires the whole tactical build pass inline (151-237).
- `FNav3DTacticalReasoning::BuildVisibilitySetsForLoadedRegionsAsync` (`Nav3DTacticalReasoning.cpp:383-428`) → timer at **0.01 s, looping**, `ProcessVisibilityBuildChunk()` with a **30 ms budget** (471). Despite the name, this is game-thread chunking, not async.
- `ANav3DData::PerformDeferredTacticalRefresh` via a 0.1 s one-shot from `PostEditChangeProperty` (`Nav3DData.cpp:1136-1139`), `WITH_EDITOR` only.

**The only runtime dynamic update path** is `UNav3DDynamicOcclusion` (`Nav3DDynamicOcclusion.cpp`): a ticking `UActorComponent` (`PrimaryComponentTick.bCanEverTick = true`, `TickInterval = 0.0f`, ctor lines 7-12) that every frame does `GetOwner()->GetActorTransform()` + `GetComponentsBoundingBox(true)`, compares against a cached transform, and on change calls `NavData->RebuildDirtyBounds({PrevBounds, CurBounds})` (198-276). Its `OnRegister` force-disables navigation relevance on all owner primitives (`SetCanEverAffectNavigation(false)`, 21-30) and it retries registration on a 0.5 s timer until it finds an `ANav3DData` in `NavSys->NavDataSet` (78-87).

`ANav3DData::RebuildDirtyBounds` (`Nav3DData.cpp:2350-2389`) fans out to every intersecting volume's `FNav3DVolumeNavigationData::RebuildDirtyBounds` (`Nav3DData.cpp:2211-2284` — note: this *definition* lives in Nav3DData.cpp, not the volume file), which does **two full O(layers × nodes) scans of the whole octree per dirty box** to compute the affected-node set — and then never uses `AffectedNodes` for anything (it's a dead local). The actual work is `RebuildLeafNodesInBounds` (`Nav3DVolumeNavigationData.cpp:1203-1334`) + `PropagateChangesToHigherLayers` (2244).

**ECS port shape (INFERRED):** replace `UNav3DDynamicOcclusion` with an occluder fragment + a processor that compares transform-vs-cached and emits a dirty-bounds request; replace both timer pumps with CkEcs processor groups (they're already written as resumable, budgeted, index-carrying state machines — `CurrentVisibilityRegionIndex`, `ProcessedTasksCount` — so they map cleanly). Fix the two dead O(n) scans while you're in there.

## 9. MULTI-VOLUME / CHUNKING — classification: (c), the deepest actor coupling in the plugin

**Three-level hierarchy (VERIFIED):** `ANav3DBoundsVolume` (author-placed FBox) → auto-partitioned into sub-volumes → one `ANav3DDataChunkActor` per sub-volume, each holding one `UNav3DDataChunk` holding one `FNav3DVolumeNavigationData` (one octree).

**Partitioning**: `FNav3DDataGenerator::PartitionVolumeIfNeeded` (`Nav3DDataGenerator.cpp:629-705`) — divisions = `ceil(size/MaxVolumePartitionSize)` per axis, clamped to `MaxSubVolumesPerAxis` (default 8), optionally forced cubic (`bPreferCubePartitions`). Defaults: `MaxVolumePartitionSize = 250000` (2.5 km), so a 2.5 km volume is one chunk. Source bounds come from `NavigationSystem->GetNavigationBoundsForNavData(NavigationData, ...)` or `GetWorldBounds()` if "generate everywhere" (`Nav3DDataGenerator.cpp:603-627`) — **the one place UE's nav system supplies real input.** Replaceable with a `TActorIterator<ANav3DBoundsVolume>` (which `ANav3DData::GetAllDiscoverableVolumes` already does, `Nav3DData.cpp:3696-3740`) or an ECS query.

**Adjacency bake** (`FNav3DDataGenerator::BuildAdjacencyBetweenChunkActors` 844-899 → `BuildAdjacencyBetweenTwoChunkActors` 901+): O(N²) over all chunk actors; AABB expand-by-voxel-size prefilter (880-885); then a face-sharing test per axis with `FaceTolerance = VoxelSize * 1.5f` (974-1028); then boundary-voxel matching via `FNav3DUtils::IdentifyBoundaryVoxels` / `AreChunksAdjacent` / `CheckVoxelBoundaryConnection` (`Nav3DUtils.h:28,31,36,86`). Output: `FNav3DChunkAdjacency { TWeakObjectPtr<ANav3DDataChunkActor> OtherChunkActor; TArray<FCompactPortal> CompactPortals; FVector SharedFaceNormal; float ConnectionWeight; }` (`Nav3DTypes.h:783-811`), `FCompactPortal { uint64 Local; uint64 Remote; FVector ConnectionPoint; }` (766-780).

**THE actor-identity problem:** the adjacency graph edge is a serialized `TWeakObjectPtr<ANav3DDataChunkActor>`. It survives save/load only by luck, and `ANav3DDataChunkActor::PostLoad` contains an explicit repair heuristic — if the weak ptr is stale it iterates every chunk actor in the world and re-links to the *nearest spatially-adjacent* one (`Nav3DDataChunkActor.cpp:81-133`). **That is a bug-shaped workaround and the strongest argument for replacing actor identity with a stable id (volume GUID + chunk index) in the port.** `ANav3DBoundsVolume::VolumeGUID`/`GetVolumeID()` already provides half of that scheme; `FNav3DVolumeIDSystem` (90 lines) is the lookup, and it does linear `TActorIterator` scans with a `ValidateNoCollisions` pass because the 16-bit id is a truncated GUID hash (`Nav3DBoundsVolume.cpp:34-43`) — i.e. **id collisions are known-possible and merely logged.**

**Cross-volume pathfinding** — `FNav3DVolumePathfinder::FindPath` (`Nav3DVolumePathfinder.cpp:30-86`) is a 4-way decision tree: no chunks → straight line; same chunk → `FindPathInChunk`; same `ANav3DBoundsVolume` → `FindPathWithinVolume` (BFS over the chunk adjacency graph, `FindChunkPathWithinVolume` 463-516, then portal-to-portal segments); else `FindPathCrossVolume` (366) using `FindVolumeExitPoint`/`FindVolumeEntryPoint` (908/929). Segments are stitched by `ProcessPathSegments` (688) / `CombinePathSegments` (958).

**Actor lookups in the hot path**: `FindChunkContaining` uses the `UNav3DWorldSubsystem` grid but **falls back to a full `TActorIterator<ANav3DDataChunkActor>` scan** (`Nav3DVolumePathfinder.cpp:108-115`); `FindVolumeContaining` is a **`TActorIterator<ANav3DBoundsVolume>` scan with no index at all, on every pathfinding call** (127-138). `UNav3DMultiChunkRaycaster::BuildChunkSegments` iterates `GetAllChunkActors()` per traversal test (`Nav3DMultiChunkRaycaster.cpp:64-92`). **All three become plain spatial-index queries over fragment data in the ECS port — a straight win, not just a port.**

**World Partition coupling** is thinner than it looks: `APartitionActor` base + `GetDefaultGridSize()` (25600) + `GetStreamingBounds()`, and a single `World->IsPartitionedWorld()` branch choosing `InitializeForWorldPartition()` vs `InitializeForStandardLevel()` (`Nav3DDataGenerator.cpp:812-819`) — **and both of those functions do the identical thing**, `RegisterWithNavigationSystem()` (`Nav3DDataChunkActor.cpp:241-257`). The streaming lifecycle hooks that matter are `BeginPlay→AddNav3DChunkToWorld` / `EndPlay→RemoveNav3DChunkFromWorld` (160-239) and `ANav3DData::OnChunkActorLoaded/Unloaded` (`Nav3DData.cpp:2428,2447`) which invalidate consolidated tactical data. **Map those onto ECS entity construct/destruct + a load-scope tag.**

## 10. SURPRISES FOR A PORTER

1. **`ANav3DData::FindPath`'s body is commented out** (`Nav3DData.cpp:2196-2204`). If anyone believes UE nav-system integration currently works, they are wrong. Verify this against your expectations before planning around it.
2. **`UNav3DRaycaster` is `NewObject<>`'d inside hot loops** — per chunk segment in `TraceCorridorInChunk` (`Nav3DMultiChunkRaycaster.cpp:123`), per Theta* LOS check (`Nav3DThetaStar.cpp:230`), per `BatchRaycast` (`Nav3DData.cpp:807`). Every pathfinding call allocates GC objects. Making it a POD class is a free perf win.
3. **Six `FAutoConsoleCommand` file-statics in `Nav3DData.cpp:44-204`** (`Nav3D.ConsolidateTactical`, `.LogPerformanceStats`, `.LogLoadedRegions`, `.RebuildCompactTactical`, `.ListVolumeIDs`, `.TestTacticalConversion`). Four use `GEngine->GetWorldFromContextObject(nullptr, ...)`; **two use raw `GWorld`** (144, 179). Plus `TAutoConsoleVariable` at `Nav3DRaycaster.cpp:9` and two **function-local statics** `CVarPruneMaxLOS`/`CVarPruneMaxBackscan` in `Nav3DVolumePathfinder.cpp:845,849` (registered lazily on first pathfinding call — a thread-safety smell).
4. **`UNav3DRaycaster::GetWorldContext()` calls `GEditor->GetEditorWorldContext(false).World()` from the runtime module** (`Nav3DRaycaster.cpp:87-94`) — the reason `UnrealEd` is a *Public* dep in editor builds.
5. **`FNav3DPathCoordinator` is a process-wide singleton** (`TUniquePtr<FNav3DPathCoordinator> Instance`, `Nav3DPathCoordinator.cpp:13`) holding a `TObjectPtr<UNav3DMultiChunkRaycaster>` created with bare `NewObject<>` (21) and **no `AddToRoot`/GC reference** — it is a dangling-pointer-after-GC waiting to happen, and it is not per-world. Multiple PIE worlds share it.
6. **`ANav3DDataChunkActor::ContainsPoint` is XY-only** (`IsInsideXY`, `.cpp:261`) in a 3D nav plugin. Used by `GetVolumeNavigationDataContainingPoint` (`Nav3DData.cpp:3360,3380,3393`), so vertically-stacked chunks resolve wrong.
7. **`FNav3DLeafNode::IsCompletelyOccluded()` is `SubNodes == -1`** on a `uint_fast64_t` (`Nav3DTypes.h:207`) — signed/unsigned comparison; works by promotion but is fragile.
8. **Semantic mismatch between broadphase and narrowphase**: filtering keeps meshes by `BodySetup->AggGeom` (simple collision) but occupancy is decided by LOD0 *render* triangles (§2). Switching to Jolt unifies these and **will change baked results**.
9. **`bTreatAsEngineModule = true`** in both `.Build.cs` files — stricter validation, e.g. mandatory `Category` on UPROPERTYs. If you keep any UPROPERTYs, keep this in mind.
10. **`FCollisionQueryParams CollisionQueryParameters` is a non-UPROPERTY member of a `USTRUCT`** (`Nav3DTypes.h:68`) — not serialized, not editable; the tuning knob authors think they have doesn't persist.
11. **Landscape is a first-class occluder** via `Landscape` module + `ULandscapeInfo::XYtoCollisionComponentMap` + `GetHeight(..., EHeightfieldSource::Complex)`. No Jolt equivalent in your stack unless you feed heightfields to Jolt.
12. `ANav3DData::AnalyzeActualSpatialDistribution` / `AnalyzeSpatialClustering` / `EstimateOctreeSize` (`Nav3DData.cpp:1594/1660/1739`, ~220 lines) are diagnostic-only, reachable from the `Analyse()` BlueprintCallable. Drop.
13. `ANav3DData::VolumeLoadingMutex` (`Nav3DData.h:393`) and `LoadedVolumeReferenceCounts` (392) are **declared and never used**. Dead.

## PROPOSED MINIMAL WORLD-BACKEND INTERFACE (pseudo-signatures)

Three narrow interfaces cover every world touch in the runtime. Nothing else in the octree/pathfinding core needs the engine.

```cpp
// ---- 1. GEOMETRY (the Jolt seam). Called from worker threads during bake.
//        Must be safe to call concurrently and must NOT read mutable game state.
struct FNav3DBodyHandle { uint64 Value = 0; };

class INav3DGeometryBackend
{
public:
    virtual ~INav3DGeometryBackend() = default;

    // Broadphase: bodies whose AABB overlaps. Replaces GatherOverlappingObjects
    // (Nav3DVolumeNavigationData.cpp:784) and CacheLayer1Overlaps (:1638).
    virtual void  QueryBodies(const FBox& WorldAABB,
                              TArray<FNav3DBodyHandle>& Out) const = 0;

    // Narrowphase, unrestricted. Replaces IsPositionOccludedPhysics (:1689)
    // and the slow path of IsPositionOccluded (:1038-1174).
    virtual bool  IsBoxOccupied(const FVector& Center, float HalfExtent) const = 0;

    // Narrowphase, restricted to a pre-resolved body. Replaces the L1-cache fast
    // path (:963-1036) incl. CheckStaticMeshOcclusion / CheckInstancedStaticMesh-
    // Occlusion / CheckLandscapeProxyOcclusion.
    virtual bool  IsBoxOccupiedBy(FNav3DBodyHandle Body,
                                  const FVector& Center, float HalfExtent) const = 0;

    // Cheap reject. Replaces Actor->GetComponentsBoundingBox / Prim->Bounds.GetBox().
    virtual FBox  GetBodyBounds(FNav3DBodyHandle Body) const = 0;

    // Replaces the tactical-visibility LineTraceSingleByChannel
    // (Nav3DTacticalReasoning.cpp:532). Optional if tactical is deferred.
    virtual bool  IsSegmentBlocked(const FVector& From, const FVector& To) const = 0;
};

// Bake-time knobs, replacing FNav3DDataGenerationSettings' UE half.
struct FNav3DBakeParams {
    float  VoxelExtent = 0.f;   // == AgentRadius * 2 today
    float  Clearance   = 0.f;   // added to every half-extent
    uint32 LayerMask   = ~0u;   // was TEnumAsByte<ECollisionChannel>
};

// ---- 2. WORLD TOPOLOGY (replaces TActorIterator + UNav3DWorldSubsystem +
//        UNavigationSystemV1::GetNavigationBoundsForNavData). ECS-backed.
struct FNav3DVolumeId { uint16 Value; };   // stable; was GUID-hash + actor ptr
struct FNav3DChunkId  { uint32 Value; };   // stable; replaces TWeakObjectPtr<ANav3DDataChunkActor>

class INav3DWorldIndex
{
public:
    // Bake input. Replaces GetOriginalNavigationBounds (Nav3DDataGenerator.cpp:603)
    // and GetAllDiscoverableVolumes (Nav3DData.cpp:3696).
    virtual void GetAuthoredVolumes(TArray<TPair<FNav3DVolumeId, FBox>>& Out) const = 0;

    // Runtime query. Replaces FindChunkContaining / FindVolumeContaining
    // (Nav3DVolumePathfinder.cpp:88, :120) and QueryActorsInBounds.
    virtual void GetChunksOverlapping(const FBox& B, TArray<FNav3DChunkId>& Out) const = 0;
    virtual const FNav3DVolumeNavigationData* GetChunkData(FNav3DChunkId) const = 0;
    virtual FBox                              GetChunkBounds(FNav3DChunkId) const = 0;
    virtual FNav3DVolumeId                    GetOwningVolume(FNav3DChunkId) const = 0;
    virtual bool                              IsChunkLoaded(FNav3DChunkId) const = 0;
};

// ---- 3. STORE (replaces ANav3DDataChunkActor-as-package + FArchive-on-UObject).
class INav3DDataStore
{
public:
    virtual bool Load(FNav3DChunkId, FNav3DChunkPayload& Out) const = 0;
    virtual bool Save(FNav3DChunkId, const FNav3DChunkPayload& In)  = 0;
};
// FNav3DChunkPayload = { FBox Bounds; FNav3DData Octree;
//                        TArray<FNav3DEdgeVoxel> BoundaryVoxels;
//                        TArray<FNav3DChunkAdjacency> Adjacency; // keyed by FNav3DChunkId, NOT TWeakObjectPtr
//                        FCompactTacticalData Tactical; }

// ---- Public API the library exposes (already exists, just needs decoupling)
ENav3DResult Nav3D_FindPath(const INav3DWorldIndex&,
                            const FNav3DPathRequest&,   // FVector start/end + float AgentRadius
                                                        //   + algo enum + smoothing
                            FNav3DPathResult& Out);     // TArray<FVector> + costs + partial flag
bool Nav3D_Raycast(const FNav3DVolumeNavigationData&, FVector From, FVector To, FNav3DRaycastHit&);
bool Nav3D_ProjectPoint(const INav3DWorldIndex&, FVector P, FVector Extent, FVector& Out);
```

Signature-level notes: `IsBoxOccupied` is deliberately axis-aligned-only — **every voxelization query in the plugin passes `FQuat::Identity`** (verified at all 5 overlap sites), so no oriented-box support is needed. `Clearance` is always folded into the half-extent before the call, so the backend never sees it.

## FILES THAT PORT NEARLY UNTOUCHED

**Zero or near-zero UE-subsystem coupling (FVector/FBox/TArray only) — copy across, retarget includes:**
- `Private/TriBoxOverlap.cpp` + `Public/TriBoxOverlap.h` (92+68) — Akenine-Möller SAT. Pure.
- `Source/ThirdParty/libmorton/*` (8 headers) — pure.
- `Private/VolumeRegionMatrix.cpp` (88) — sparse bitmask visibility matrix. Only includes `Nav3DTypes.h`.
- `Private/Pathfinding/Utils/Nav3DPathSmoothing.cpp` (71) — includes only its own header.
- `Private/Pathfinding/Core/Nav3DPath.cpp` (29) — after `FNav3DPath` is de-FNavigationPath'd.
- `Private/Pathfinding/Core/INav3DPathFinder.cpp` (40) — only `Logging/LogMacros.h`.
- `Private/Pathfinding/Search/Nav3DPathHeuristicCalculator.cpp` (25) and `Nav3DPathTraversalCostCalculator.cpp` (25) — only include `Nav3DVolumeNavigationData.h`. Drop the UCLASS wrappers, keep the math.
- `Private/Pathfinding/Search/Nav3DAStar.cpp` (334) — includes `Nav3D.h` (log), `Nav3DVolumeNavigationData.h`, `Nav3DUtils.h`, the two calculators. **No world, no actor, no UObject alloc.** Only external type is `ENavigationQueryResult::Type` (→ own enum).
- `Private/Pathfinding/Search/Nav3DLazyThetaStar.cpp` (268) — includes only `Nav3D.h` + `Nav3DUtils.h`. Cleanest of the three.
- `Private/Pathfinding/Search/Nav3DQueryFilter.cpp` (68) — all stubs; delete rather than port.

**Ports with one mechanical fix each:**
- `Private/Pathfinding/Search/Nav3DThetaStar.cpp` (297) — one `NewObject<UNav3DRaycaster>()` at :230. Make the raycaster a value member.
- `Private/Raycasting/Nav3DRaycaster.cpp` (825) — the SVO ray-marcher (Revelles octree traversal: `FOctreeRay`, `GetFirstNodeIndex`, `GetNextNodeIndex`, `DoesRayIntersectOccludedLeaf`). **Entirely octree-internal — it never touches the physics world.** Fixes: drop `UCLASS`/`UObject`, delete `GetWorldContext()` (:87-94, the `GEditor` call), keep or drop the CVar at :9.
- `Private/Raycasting/Nav3DMultiChunkRaycaster.cpp` (203) — swap `ANav3DDataChunkActor*` for `FNav3DChunkId` + `INav3DWorldIndex`; `RayIntersectsBox` (:170) is pure slab-test math.
- `Private/Nav3DTypes.cpp` (317) — `FNav3DData::Initialize` (layer sizing), leaf/layer allocators. Only `Nav3D.h` for logging.
- `Private/Nav3DVolumeNavigationData.cpp` **minus** lines 781-851 (`GatherOverlappingObjects`), 854-950 (component filters), 952-1175 (`IsPositionOccluded`), 1335-1504 (mesh/ISM/landscape narrowphase), 1593-1687 (`CacheLayer1Overlaps`), 1689-1776 (`IsPositionOccludedPhysics`) — i.e. **~600 of 2510 lines are the backend seam; the other ~1900 (Morton addressing, neighbour linking, leaf rasterization structure, layer propagation, node lookup, `GetRandomPoint`) port as-is.**
- `Private/Nav3DUtils.cpp` (873) — mostly pure Morton/box/adjacency math. Strip `GetNav3DData(UWorld*)` (:455, iterates `NavSys->NavDataSet`), `GetNavAgentPropsFromQuerier` (`NavMovementComponent`), `GetNav3DQueryFilter`, and `GraphAStarResultToNavigationTypeResult`.
- `Private/Tactical/Nav3DTacticalReasoning.cpp` (2502) — the region-growing (`BuildBoxRegions`, `ExtractFreeVoxelsWithCoords`, `BuildVoxelLevelAdjacency`), pruning strategies, and compact conversion are pure data. Only 3 world calls (:532, :1082, :1189) → `INav3DGeometryBackend`. The timer pump (:383-428, :430+) → an ECS processor.
- `Private/Tactical/Nav3DTacticalDataConverter.cpp` (277) — pure format conversion, but **exists only to feed the debug renderer**; drop with §6.

**Discard outright:** `Nav3DNavDataRenderingComponent.cpp/.h` (1086+136), `Tests/Nav3DTestVolume.cpp/.h` (1181+193), `Pathfinding/Core/Nav3DPathTest.cpp/.h` (279+190), `Pathfinding/Utils/Nav3DPathTestRenderingComponent.cpp/.h` (156+19), `Raycasting/Nav3DRaycasterTest.cpp/.h` (372+158), all of `Source/Nav3DEditor` (859), `Nav3DBoundsVolume.cpp/.h` (44+26 → an FBox + uint16), `Nav3DWorldSubsystem.cpp/.h` (73+51 → a CkFoundation spatial index), `Nav3DDynamicOcclusion.cpp/.h` (276+51 → an occluder fragment + processor), `Nav3DSettings.cpp/.h` (47+85 → a settings struct/asset).

**Rewrite, don't port:** `Nav3DData.cpp/.h` (4305+410) — split into (i) a chunk registry over the ECS, (ii) the tactical-consolidation layer (~1200 lines, portable logic, actor-typed signatures), (iii) delete the ~30 ANavigationData virtuals and ~220 lines of Analyse diagnostics. `Nav3DDataGenerator.cpp/.h` (1421+226) — the partitioning (`PartitionVolumeIfNeeded`, :629) and adjacency bake (`BuildAdjacencyBetweenTwoChunkActors`, :901) are portable algorithms wearing `ANav3DDataChunkActor*` signatures; the `FNavDataGenerator` scaffolding and `World->SpawnActor` (:741) go away. `Nav3DDataChunkActor.cpp/.h` (352+116) and `Nav3DDataChunk.cpp/.h` (103+51) — collapse into one serializable chunk payload + an ECS entity.
