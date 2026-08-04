# HEADLINE

Nav3D 2.0 (F:\Nav3D-2.0) is an MIT-licensed, single-author (Darby Costello) UE5 plugin implementing Brewer's "3D Flight Navigation Using Sparse Voxel Octrees" (Game AI Pro 3), plus a bolted-on region-based tactical-reasoning layer. Two modules: Nav3D (Runtime, 37 .cpp + 37 .h + Build.cs, ~25,355 LOC) and Nav3DEditor (Editor, 2 .cpp + 2 .h + build.cs, 897 LOC), plus header-only libmorton (8 headers, 1,146 LOC, MIT). Total Source/ = 27,398 LOC counting third-party. It plugs into UE's navigation system as an ANavigationData subclass (ANav3DData) with its own FNavDataGenerator (FNav3DDataGenerator), storing octree data in APartitionActor-derived chunk actors (ANav3DDataChunkActor) holding UNavigationDataChunk payloads — i.e. it is deeply wired into UE's nav/World-Partition machinery, not a standalone library. Rough split of first-party LOC: ~45% algorithm (octree rasterization 3,858; pathfinding A*/Theta*/LazyTheta* 2,920; octree raycast/conecast 1,729; tactical regioning/pruning/visibility 3,228), ~30% UE integration glue (7,780, dominated by the 4,305-line Nav3DData.cpp), ~7% reflected type/settings definitions (1,893), ~5% debug scene proxies (1,398), ~9% in-editor test/demo actors (2,375), ~3.4% editor module. The editor module is 100% cosmetic — one IDetailCustomization with rebuild buttons; nothing it does is unreachable from the runtime API.

## 1. File listing with line counts, grouped by module

VERIFIED via `find … -exec wc -l`. Total 27,373 by wc (27,398 counting files without trailing newline). Paths relative to F:\Nav3D-2.0\Source.

**Nav3D (Runtime) — 37 .cpp, 37 .h, 1 .cs = 25,355 LOC**

Build + module root:
- Nav3D/Nav3D.Build.cs — 70
- Nav3D/Private/Nav3D.cpp — 16 | Nav3D/Public/Nav3D.h — 12

Core nav-data / UE integration (Private / Public):
- Nav3DData.cpp — 4305 | Nav3DData.h — 410
- Nav3DDataGenerator.cpp — 1421 | Nav3DDataGenerator.h — 226
- Nav3DVolumeNavigationData.cpp — 2510 | Nav3DVolumeNavigationData.h — 211
- Nav3DTypes.cpp — 317 | Nav3DTypes.h — 1575  (largest header; all shared structs)
- Nav3DUtils.cpp — 873 | Nav3DUtils.h — 103
- Nav3DNavDataRenderingComponent.cpp — 1086 | .h — 136
- Nav3DDataChunkActor.cpp — 352 | .h — 116
- Nav3DDataChunk.cpp — 103 | .h — 51
- Nav3DDynamicOcclusion.cpp — 276 | .h — 51
- Nav3DVolumeIDSystem.cpp — 90 | .h — 19
- Nav3DWorldSubsystem.cpp — 73 | .h — 51
- Nav3DBoundsVolume.cpp — 44 | .h — 26
- Nav3DSettings.cpp — 47 | .h — 85
- TriBoxOverlap.cpp — 92 | .h — 68
- VolumeRegionMatrix.cpp — 88 (no matching header; type lives in Nav3DTypes.h)

Pathfinding/Core:
- Nav3DVolumePathfinder.cpp — 999 | .h — 108
- Nav3DPathTest.cpp — 279 | .h — 190
- Nav3DPathCoordinator.cpp — 138 | .h — 37
- Nav3DPathLibrary.cpp — 80 | .h — 21
- INav3DPathFinder.cpp — 40 | INav3DPathfinder.h — 24  (note: .cpp/.h casing differs)
- Nav3DPath.cpp — 29 | .h — 92
- Nav3DPathingTypes.h — 109 (header-only)

Pathfinding/Search:
- Nav3DAStar.cpp — 334 | .h — 52
- Nav3DThetaStar.cpp — 297 | .h — 27
- Nav3DLazyThetaStar.cpp — 268 | .h — 28
- Nav3DQueryFilter.cpp — 68 | .h — 39
- Nav3DPathHeuristicCalculator.cpp — 25 | .h — 43
- Nav3DPathTraversalCostCalculator.cpp — 25 | .h — 51

Pathfinding/Utils:
- Nav3DPathSmoothing.cpp — 71 | .h — 14
- Nav3DPathTestRenderingComponent.cpp — 156 | .h — 19

Raycasting:
- Nav3DRaycaster.cpp — 825 | .h — 186
- Nav3DRaycasterTest.cpp — 372 | .h — 158
- Nav3DMultiChunkConeCaster.cpp — 256 | .h — 171
- Nav3DMultiChunkRaycaster.cpp — 203 | .h — 86

Tactical:
- Nav3DTacticalReasoning.cpp — 2502 | .h — 308
- Nav3DTacticalDataConverter.cpp — 277 | .h — 51

Tests:
- Nav3DTestVolume.cpp — 1181 | .h — 193

**Nav3DEditor (Editor) — 897 LOC**
- Nav3DEditor/Nav3DEditor.build.cs — 36
- Private/Nav3DDataDetailCustomization.cpp — 775 | Public/…h — 29
- Private/Nav3DEditor.cpp — 43 | Public/Nav3DEditor.h — 12

**ThirdParty/libmorton — 8 headers, 1,146 LOC**
- morton3D.h — 276, morton2D.h — 254, morton3D_LUTs.h — 224, morton2D_LUTs.h — 117, morton_LUT_generators.h — 98, morton.h — 80, morton_BMI.h — 57, morton_common.h — 37 (+ LICENSE, README.md)

Also present outside Source/: Nav3D.uplugin, LICENSE (MIT), README.md (16 KB), .gitignore, Resources/ (4 files), Binaries/Win64/ (prebuilt UnrealEditor-Nav3D.dll 1.75 MB, UnrealEditor-Nav3DEditor.dll 251 KB, UnrealEditor.modules — checked in, dated 2026-01-12).

## 2. Class map — reflected types (24 UCLASS / 40 USTRUCT / 7 UENUM)

VERIFIED by reading headers. Format: Name — base — responsibility — file (Source/Nav3D/Public/…).

**Actors / UObjects (UCLASS)**
- ANav3DBoundsVolume — ANavMeshBoundsVolume — level-placed volume marking where nav data is built; carries a persistent FGuid hashed to a stable uint16 volume ID — Nav3DBoundsVolume.h:8
- ANav3DData — ANavigationData — the plugin's nav-data actor; implements every ANavigationData virtual (ProjectPoint, BatchRaycast, CalcPathCost, GetRandomPoint…), owns the chunk-actor registry, tactical consolidation, and static FindPath entry point — Nav3DData.h:36
- UNav3DDataChunk — UNavigationDataChunk — serialized per-level payload holding TArray<FNav3DVolumeNavigationData> + Morton-coded boundary voxels — Nav3DDataChunk.h:30
- ANav3DDataChunkActor — APartitionActor — World-Partition-streamable actor owning UNav3DDataChunks, baked chunk adjacency, and compact tactical data — Nav3DDataChunkActor.h:11
- UNav3DDynamicOcclusion — UActorComponent — marks an actor as a runtime occluder; re-rasterizes leaf subnodes into registered ANav3DData as it moves — Nav3DDynamicOcclusion.h:30
- UNav3DNavDataRenderingComponent — UPrimitiveComponent — debug-draw component created by ANav3DData::ConstructRenderingComponent — Nav3DNavDataRenderingComponent.h:105
- UNav3DSettings — UDeveloperSettings (config=Engine) — project settings: default algorithm, cost/heuristic classes, smoothing, MaxRegions, volume-partitioning limits, parallel build count — Nav3DSettings.h:9
- UNav3DWorldSubsystem — UWorldSubsystem — 2D-cell spatial hash of chunk actors for bounds queries — Nav3DWorldSubsystem.h:18
- UNav3DPathLibrary — UBlueprintFunctionLibrary — one BP node, FindNav3DPath(Start, End, AgentRadius) → TArray<FVector> — Pathfinding/Core/Nav3DPathLibrary.h:8
- ANav3DPathTest — AActor — editor test actor: pairs with another ANav3DPathTest, CallInEditor Find/Clear Path, draws result — Pathfinding/Core/Nav3DPathTest.h:86
- UNav3DPathTestRenderingComponent — UPrimitiveComponent — scene proxy for the above — Pathfinding/Utils/Nav3DPathTestRenderingComponent.h:8
- UNav3DPathHeuristicCalculator — UObject (abstract, EditInlineNew) — heuristic policy interface — Pathfinding/Search/Nav3DPathHeuristicCalculator.h:9
- UNav3DPathHeuristicCalculator_Manhattan / _Euclidean — the two concrete heuristics — same file :22, :34
- UNav3DPathTraversalCostCalculator — UObject (abstract) — traversal-cost policy interface — Pathfinding/Search/Nav3DPathTraversalCostCalculator.h:9
- UNav3DPathCostCalculator_Distance / _Fixed — distance-based and constant-cost implementations — same file :21, :36
- UNav3DQueryFilter — UNavigationQueryFilter — asset-side wrapper holding FNav3DQueryFilterSettings — Pathfinding/Search/Nav3DQueryFilter.h:8
- UNav3DRaycaster — UObject (EditInlineNew) — the octree ray traversal engine (Revelles parametric algorithm); Trace / TraceCountingOccludedVoxels / CountOccludedVoxelsAlongRay — Raycasting/Nav3DRaycaster.h:95
- UNav3DMultiChunkRaycaster — UObject — static HasLineOfTraversal spanning multiple chunks with a 5-ray agent-radius corridor — Raycasting/Nav3DMultiChunkRaycaster.h:12
- UNav3DMultiChunkConeCaster — UObject — static FindOccludedVoxelsInCone, hierarchical frustum-culled octree walk — Raycasting/Nav3DMultiChunkConeCaster.h:74
- UNav3DRaycasterRenderingComponent — UPrimitiveComponent — scene proxy for the raycaster test actor — Raycasting/Nav3DRaycasterTest.h:75
- ANav3DRaycasterTest — AActor — editor actor that fires a debug ray and visualizes traversed nodes — Raycasting/Nav3DRaycasterTest.h:98
- ANav3DTestVolume — AActor — procedural obstacle generator (uniform/clustered/perlin/ring/disc/spline distributions) for exercising the octree — Tests/Nav3DTestVolume.h:37

**UENUM (7)**
- ETacticalVisibility / ETacticalDistance / ETacticalRegion — tactical query preference enums — Nav3DTypes.h:819, :829, :839
- ENav3DPathingAlgorithm (AStar/ThetaStar/LazyThetaStar), ENav3DPathingLogVerbosity — Pathfinding/Core/Nav3DPathingTypes.h:10, :23
- ENav3DTestDistribution, ENav3DDiscOrientation — Tests/Nav3DTestVolume.h:11, :22
- (plain, non-reflected) ENav3DVersion — serialization version, single value V_2025090900 — Nav3DTypes.h:33

**USTRUCT (40)** — grouped:
- Settings/debug: FNav3DDataGenerationSettings (:41), FNav3DTacticalSettings (:369), FNav3DVolumeDebugData (:268), FNav3DTacticalDebugData (:303), FNav3DPerformanceStats (:332), FNav3DMetadata (:425), FNav3DMetadataList (:446), FNav3DQueryFilterSettings (PathingTypes.h:85), FNav3DPathTestDebugDrawOptions (Nav3DPathTest.h:15), FNav3DRaycasterDebugDrawOptions (Nav3DRaycasterTest.h:15) — all Nav3DTypes.h unless noted
- Portals/adjacency: FNav3DVoxelConnection (:712), FNav3DActorPortal (:751), FCompactPortal (:766), FNav3DChunkAdjacency (:783), FChunkConnectionInterface (:1081), FNav3DEdgeVoxel (Nav3DDataChunk.h:10), FRegionMapping (Nav3DData.h:14)
- Tactical regions: FPositionCandidate (:849), FNav3DRegion (:891, marked @deprecated), FNav3DRegionBuilder (:940), FBoxRegion (:982), FRegionIdArray (:1021), FRegionIdList (:259), FLocalTacticalData (:1053, deprecated), FNav3DTacticalData (:1107), FRegionPruningData (:1137), FDensityRegionPruningData (:1193), FConsolidatedTacticalData (:1264, deprecated), FCompactRegion (:1309), FVolumeRegionMatrix (:1367), FCompactTacticalData (:1451), FConsolidatedCompactTacticalData (:1515)
- Path: FNav3DPathPoint (Nav3DPath.h:11), FNav3DPathData (:42), FNav3DPathingRequest (PathingTypes.h:39)
- Raycast: FNav3DConeCastParams (ConeCaster.h:14), FNav3DOccludedVoxel (:46)
- Misc: FVoxelOverlapCache (Nav3DTypes.h:82), FVoxelOcclusionData (Nav3DDynamicOcclusion.h:11), FNav3DSpatialCell (Nav3DWorldSubsystem.h:10)

Note: a first-class duplication exists — FNav3DRegion/FLocalTacticalData/FConsolidatedTacticalData are explicitly marked @deprecated in favour of FCompactRegion/FCompactTacticalData/FConsolidatedCompactTacticalData, but both formats are live, both are stored on ANav3DData (ConsolidatedTacticalData + ConsolidatedCompactTacticalData), and FNav3DTacticalDataConverter exists solely to shuttle between them. That is ~1,000+ LOC of pure format-conversion debt.

## 2b. Class map — plain (non-reflected) C++ classes/structs

VERIFIED by reading headers.

**Octree data model (Nav3DTypes.h)**
- FNav3DNodeAddress (:108) — packed 4/22/6-bit {LayerIndex, NodeIndex, SubNodeIndex}; converts to/from NavNodeRef; sentinel LayerIndex==15 = invalid
- FNav3DLeafNode (:176) — one 64-bit bitmask = 4×4×4 subnode occupancy + parent address
- FNav3DNode (:222) — MortonCode + Parent + FirstChild + 6 orthogonal neighbour addresses
- FNav3DLeafNodes (:454) — array-of-leaves container with size accessors, friend-serialized
- FNav3DLayer (:530) — one octree layer: TArray<FNav3DNode> + NodeSize + MaxNodeCount
- FNav3DData (:585) — the SVO itself: TArray<FNav3DLayer> + FNav3DLeafNodes + blocked-node lists + nav/volume bounds. NOTE the name collision with the actor ANav3DData; also FNav3DData::GetNodeFromAddress-adjacent helpers are all FORCEINLINE in-header

**Volume / generation**
- FNav3DVolumeNavigationDataSettings (Nav3DVolumeNavigationData.h:19) — per-volume build params incl. TAtomic<bool>* CancelFlag
- FNav3DVolumeNavigationData (:33) — THE octree owner + builder + query surface: FirstPass/RasterizeLeaf/RasterizeInitialLayer/RasterizeLayer/BuildNeighbourLinks, GetNodeAddressFromPosition, FindNearestNavigableNode, GetNodeNeighbours, RebuildDirtyBounds, dynamic occluders, static per-primitive occlusion tests (static mesh, ISM, landscape). Carries a static TAtomic<bool> global cancel flag
- FNav3DVolumeNavigationDataGenerator (Nav3DDataGenerator.h:11, FNoncopyable) — per-box worker that builds one volume's data
- FNav3DBoxGeneratorWrapper (:34, FNonAbandonableTask) + alias FNav3DBoxGeneratorTask = FAsyncTask<…> — async task wrapper
- FPendingBoundsDataGenerationElement (:52) / FRunningBoundsDataGenerationElement (:85) — generator queue entries (pending sorted by seed distance; running holds the FAsyncTask*)
- FNav3DDataGenerator (:108, FNavDataGenerator + FNoncopyable) — the UE nav-data generator: RebuildAll/TickAsyncBuild/CancelBuild/RebuildDirtyAreas, volume partitioning, chunk-actor creation, cross-chunk adjacency baking, staged tactical generation

**Utilities**
- FNav3DUtils (Nav3DUtils.h:12) — static grab-bag: Morton encode/decode wrappers, sub-node offsets, ray/box intersection, boundary-voxel identification, chunk adjacency building, portal validation, nav-agent props, chunk colour palette; nested FEndpointProjectionResult (:49)
- FNav3DVolumeIDSystem (Nav3DVolumeIDSystem.h:6) — GUID→uint16 volume-ID lookup + collision validation
- namespace Nav3D::TriBoxOverlapUtils (TriBoxOverlap.h) — Akenine-Möller triangle-box SAT overlap (free functions + macro)
- FNav3DModule (Nav3D.h:7) — IModuleInterface; both Startup/Shutdown are empty

**Pathfinding**
- INav3DPathfinder (INav3DPathfinder.h:8) — pure-virtual FindPath(FNav3DPath&, request, volume data) + shared logging helpers
- FNav3DAStar (Nav3DAStar.h:8) — A* over the SVO; nested FSearchNode; TMap<FNav3DNodeAddress,FSearchNode> AllNodes + TArray OpenSet (linear-scan open set, not a binary heap)
- FNav3DThetaStar (Nav3DThetaStar.h:6, : FNav3DAStar) — any-angle Theta* using UNav3DRaycaster for LOS
- FNav3DLazyThetaStar (:9, : FNav3DThetaStar) — deferred-LOS variant; plugin default
- FNav3DPath (Nav3DPath.h:59, : FNavigationPath) — path type with per-point costs and a CreatePathData() BP bridge
- FNav3DPathCoordinator (Nav3DPathCoordinator.h:11) — singleton facade owning one instance of each solver + FNav3DVolumePathfinder + a multi-chunk raycaster; TryDirectTraversal shortcut
- FNav3DVolumePathfinder (Nav3DVolumePathfinder.h:13) — cross-volume/cross-chunk orchestration: locate chunk/volume, chunk-graph BFS, portal selection, segment stitching; nested FPathSegment (:25)
- FNav3DPathSmoothing (Nav3DPathSmoothing.h:4) — Catmull-Rom subdivision
- FNav3DQueryFilter (Nav3DQueryFilter.h:20, : INavigationQueryFilterInterface) — UE query-filter shim carrying FNav3DQueryFilterSettings

**Raycasting**
- FNav3DRaycastHit / FNav3DRaycasterTraversedNode / FNav3DRaycasterDebugInfos (Nav3DRaycaster.h:11, :30, :43)
- FNav3DRaycasterProcessor (:54) + FNav3DRaycasterProcessor_GenerateDebugInfos (:76) — visitor hooks for debug capture
- nested UNav3DRaycaster::FOctreeRay (:131) / FRaycastState (:139) — parametric traversal state

**Tactical**
- FNav3DTacticalReasoning (Nav3DTacticalReasoning.h:17) — region extraction from free voxels (greedy box regioning), adjacency, sample-based visibility (async/timer-sliced), geometry verification, region pruning to a 64-region cap, best-location query. 2,502 LOC .cpp — the second-largest file
- FDensityFocusedPruningStrategy (:204) — alternative pruning that scores regions by local geometry density / visibility complexity / chokepoint-ness with a 50/30/20 budget split
- FNav3DTacticalDataConverter (Nav3DTacticalDataConverter.h:10) — bidirectional build↔compact tactical format converter

**Debug scene proxies**
- FNav3DMeshSceneProxyData / FNav3DMeshSceneProxy (: FDebugRenderSceneProxy) / FNav3DDebugDrawDelegateHelper — Nav3DNavDataRenderingComponent.h:14, :32, :87 (helper guarded by `#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST`)
- FNav3DPathTestSceneProxyData / FNav3DPathTestSceneProxy — Nav3DPathTest.h:48, :59
- FNav3DRaycasterSceneProxyData / FNav3DRaycasterSceneProxy / FNav3DRaycasterDebugDrawDelegateHelper — Nav3DRaycasterTest.h:38, :45, :60

## 3. Module dependencies and .uplugin

VERIFIED by reading the files.

**Source/Nav3D/Nav3D.Build.cs (70 lines)**
- PCHUsage = UseExplicitOrSharedPCHs; `bUseUnity = true` (explicit); `bTreatAsEngineModule = true` with the comment "Treat plugin as Engine module for stricter validation"
- PublicIncludePaths: `ModuleDirectory + "/../ThirdParty"` → Source/ThirdParty (this is what makes `#include <libmorton/morton.h>` resolve)
- PrivateIncludePaths: `"Nav3D/Private"`
- PublicDependencyModuleNames: **Core, AIModule**
- PrivateDependencyModuleNames: **CoreUObject, Engine, Slate, SlateCore, RHI, RenderCore, DeveloperSettings, GameplayTasks, AIModule (duplicate of the public entry), NavigationSystem, Landscape, InputCore**
- DynamicallyLoadedModuleNames: empty
- `if (Target.bBuildEditor) PublicDependencyModuleNames.Add("UnrealEd")` — **the runtime module publicly depends on UnrealEd in editor targets.** Anything consuming Nav3D in an editor build inherits UnrealEd. Not a shipping-build problem (guarded), but it means the runtime module contains editor-only code paths.
- Landscape is a real dependency: FNav3DVolumeNavigationData::CheckLandscapeProxyOcclusion and includes of LandscapeComponent/LandscapeProxy/LandscapeInfo/LandscapeMeshCollisionComponent in Nav3DVolumeNavigationData.cpp.
- InputCore and GameplayTasks appear vestigial (no obvious use found in a quick scan) — INFERRED, not exhaustively grepped.

**Source/Nav3DEditor/Nav3DEditor.build.cs (36 lines)** — note the lowercase `.build.cs` while the runtime one is `.Build.cs`; UBT is case-insensitive on Windows but this would break a case-sensitive host.
- PublicDependencyModuleNames: Core, CoreUObject, Engine, **Nav3D**, InputCore
- PrivateDependencyModuleNames: Slate, SlateCore, PropertyEditor, EditorStyle, UnrealEd, GraphEditor, BlueprintGraph — GraphEditor and BlueprintGraph are declared but the module contains no graph/K2 node code (VERIFIED: only 2 .cpp files, neither touches graphs). Dead deps.
- bTreatAsEngineModule = true; PCHUsage = UseExplicitOrSharedPCHs

**Nav3D.uplugin**
- FileVersion 3, Version 1, VersionName "2.0.0", FriendlyName "Nav3D", Category "AI", CreatedBy "Darby Costello", DocsURL/MarketplaceURL/SupportURL all empty
- `CanContainContent: false`, not beta/experimental
- Modules: `Nav3D` (Type Runtime, LoadingPhase Default, `WhitelistPlatforms: ["Win64"]`); `Nav3DEditor` (Type Editor, LoadingPhase **PostEngineInit**, WhitelistPlatforms Win64)
- **`WhitelistPlatforms` is the UE4-era key**; UE5 uses `PlatformAllowList`. In UE5.5 it is still parsed with a deprecation warning (INFERRED — not verified against 5.5 source). Either way the plugin declares itself Win64-only, so a non-Windows target would need this edited.

## 4. Nav3DEditor — what it actually contains, and how much matters

VERIFIED by reading both .cpp files in full-scan.

Contents, in their entirety:
1. **FNav3DEditorModule** (Nav3DEditor.cpp, 43 lines) — StartupModule does exactly two things: (a) `PropertyModule.FindOrCreateSection("Actor", "Nav3D", …)` + `Section->AddCategory("Nav3D")` so a "Nav3D" filter button appears in the Details panel; (b) `RegisterCustomClassLayout(ANav3DData::StaticClass(), …FNav3DDataDetailCustomization::MakeInstance)`. ShutdownModule unregisters the layout. Uses IMPLEMENT_GAME_MODULE (not IMPLEMENT_MODULE) — works, but non-idiomatic for a plugin.
2. **FNav3DDataDetailCustomization** (775 lines, the whole rest of the module) — an IDetailCustomization for ANav3DData that hand-builds Slate rows in the "Nav3D" category:
   - a build-in-progress warning row (SImage Icons.Warning + STextBlock)
   - a "Volumes" section listing each discovered volume with per-volume **"Rebuild Nav"** and **"Rebuild Tactical"** buttons
   - a nested "Chunks" list per volume: colour swatch (from `FNav3DUtils::GetChunkColorByIndex`, shared with the runtime debug draw), size in MB, **"Rebuild Chunk"** and **"Unload Chunk"** buttons
   - `AddConsolidatedTacticalStatusPanel` — read-only tactical status text

**What is NOT in the editor module:** no component visualizers, no asset type actions or factories, no thumbnail renderers, no editor modes, no custom K2 nodes, no toolbar/menu extensions, no commandlets, no automation tests.

**Essential vs cosmetic:** cosmetic, entirely. Every action the panel triggers is a public method on ANav3DData reachable without it — `BuildSingleVolume`, `RebuildSingleChunk(FBox)`, `RebuildSingleChunk(ANav3DDataChunkActor*)`, `RebuildTacticalDataForVolume`, `BuildNavigationData`, `ClearNavigationData` — and ANav3DData additionally exposes `UFUNCTION(CallInEditor)` entry points (`RebuildConsolidatedTacticalDataFromCompact`, `DebugPrintChunkAdjacency`, `ValidateAllChunkAdjacency`, `ValidateConsolidatedTacticalData`, `LogPerformanceStats`) plus BlueprintCallable `Validate`, `Analyse`, `CleanupInvalidChunkActorsBP`, `GetPartitionedVolumes`. ANav3DDataChunkActor has its own `CallInEditor` "Rebuild This Chunk". The standard UE Build menu drives generation through FNav3DDataGenerator regardless.

Coupling cost paid by the runtime module for this panel: `#if WITH_EDITORONLY_DATA int32 ChunkRevision` + `GetChunkRevision`/`IncrementChunkRevision`/`OnTacticalBuildCompleted` and the `FOnTacticalBuildCompleted` multicast delegate on ANav3DData exist to let the details panel know when to refresh (Nav3DData.h:71-83, :137-138 region). Dropping the editor module would orphan those, not break anything.

**Verdict for a port/strip:** deleting Nav3DEditor costs 897 LOC and 7 editor-module dependencies, and loses only the per-volume/per-chunk rebuild UI. Highest-value thing to keep if you want ergonomics; lowest-risk thing to cut if you want a lean runtime.

## 5. libmorton — what it is, license, size, call sites

VERIFIED by reading LICENSE, README, headers, and grepping every call site.

- **What:** Forceflow/libmorton — a header-only C++ library for encoding/decoding Morton (Z-order) codes in 2D and 3D at 16/32/64 bits. `morton.h` `#define`s the fastest available implementation, dispatching to BMI2 intrinsics (`m3D_e_BMI`) when `__BMI2__` is defined, otherwise LUT/magicbits versions.
- **License:** MIT, "Copyright (c) 2016 Jeroen Baert" (Source/ThirdParty/libmorton/LICENSE). Compatible with the plugin's own MIT (Copyright (c) 2025 Darby Costello). Attribution-only obligation; no source-disclosure requirement. Note the plugin's own LICENSE file has mangled punctuation ("and\or" → "andor", missing quotes around "AS IS") — cosmetic, but it is a modified MIT text rather than the canonical one.
- **Size:** 8 headers, 1,146 LOC. **~1,024 of those lines are the two LUT tables** (morton3D_LUTs.h 224, morton2D_LUTs.h 117) plus the alternative-implementation files. The actually-reachable surface for Nav3D is tiny.
- **Vendored version:** no version file; README carries the 2018 citation block and the warning "This library is under active development. SHIT WILL BREAK." No local modifications detected (no Ck/Nav3D markers in the headers).

**Every call into it (VERIFIED, exhaustive grep for morton2D_*/morton3D_*):**
- Source/Nav3D/Private/Nav3DUtils.cpp:4 — `#include "ThirdParty/libmorton/morton.h"`
  - :13 `morton3D_64_encode(Vector.X, Vector.Y, Vector.Z)` in `FNav3DUtils::GetMortonCodeFromVector`
  - :40 `morton3D_64_decode` in `GetVectorFromMortonCode`
  - :48 `morton3D_64_decode` in `GetIntVectorFromMortonCode`
  - :66 `morton3D_64_decode` in `GetSubNodeOffset`
- Source/Nav3D/Private/Nav3DVolumeNavigationData.cpp:3 — `#include <libmorton/morton.h>`
  - :2090 `morton3D_64_decode(LeafIndex, X, Y, Z)` in `GetLeafNeighbours`

That is **5 call sites, all `morton3D_64_encode`/`morton3D_64_decode`**. Nothing uses 2D or 32-bit variants. `FNav3DUtils::GetMortonCodeFromIntVector` (Nav3DUtils.cpp:16-35) does NOT use libmorton at all — it hand-rolls the bit interleave. Parent/child code derivation is plain shifts (`>> 3`, `<< 3`, Nav3DUtils.cpp:52-60).

**Portability note:** the two include styles differ (`"ThirdParty/libmorton/morton.h"` vs `<libmorton/morton.h>`) — only the second matches the declared `PublicIncludePaths` entry (Source/ThirdParty); the first relies on Source/ also being on the search path. Replacing libmorton with ~30 lines of local encode/decode would remove the entire ThirdParty tree with no behavioural risk beyond re-verifying the BMI2 fast path.

## 6. Content / Resources assets

VERIFIED by directory listing.

- **No Content/ folder exists.** `Nav3D.uplugin` declares `"CanContainContent": false`. There are no .uasset/.umap files anywhere in the plugin. Nothing to cook, no asset references, no soft-object paths to fix up.
- **Resources/** (4 files, ~2.35 MB):
  - `Icon128.png` (9,960 B) — the plugin browser icon. Required by convention for a listed plugin; harmless to keep, cosmetically missed if dropped.
  - `nav3d-chunk-data.png` (61,938 B), `nav3d-chunks.png` (735,586 B), `nav3d-debug.gif` (1,549,386 B) — **documentation images only**, referenced from README.md via raw.githubusercontent URLs. Zero runtime or editor use.
- **Binaries/Win64/** is checked into the repo: `UnrealEditor-Nav3D.dll` (1,755,136 B), `UnrealEditor-Nav3DEditor.dll` (251,392 B), `UnrealEditor.modules` — all dated 2026-01-12. These are stale prebuilt artifacts for one specific engine build; any consuming project must rebuild from source. Delete before vendoring.

Net: runtime asset footprint is **zero**. Only Icon128.png is worth carrying; the other ~2.3 MB is README decoration.

## 7. LOC proportions — algorithm vs UE glue

VERIFIED by scripted per-file bucketing (script output reproduced below); the bucket *assignment* is my judgement, and the caveats after it are the honest corrections.

```
ALGO_octree_build            3858   (VolumeNavigationData .cpp/.h, TriBoxOverlap, Nav3DUtils)
ALGO_pathfinding             2920   (Core + Search + Smoothing, 23 files)
ALGO_raycast                 1729   (Raycaster, MultiChunkRaycaster, ConeCaster)
ALGO_tactical                3228   (TacticalReasoning, DataConverter, VolumeRegionMatrix)
DATA_types                   1893   (Nav3DTypes.h/.cpp)
GLUE_navsystem               7780   (Nav3DData, DataGenerator, ChunkActor, DataChunk,
                                     BoundsVolume, WorldSubsystem, VolumeIDSystem,
                                     DynamicOcclusion, Settings, module)
GLUE_debugdraw               1398   (NavDataRenderingComponent, PathTestRenderingComponent)
GLUE_testactors              2375   (Nav3DPathTest, Nav3DRaycasterTest, Nav3DTestVolume)
GLUE_bp_library               103
EDITOR                        897
BUILD_CS                       71
THIRDPARTY_libmorton         1146
TOTAL                       27398
```

**Headline split (first-party = 26,252 LOC, excluding libmorton):**
- Pure algorithm: **11,735 LOC ≈ 45%**
- UE integration glue (nav system + generator + chunk actors + settings + subsystem + occlusion + module): **7,780 ≈ 30%**
- Reflected type/settings definitions: **1,893 ≈ 7%** (mostly USTRUCT boilerplate + debug toggles; arguably glue)
- Debug draw / scene proxies: **1,398 ≈ 5%**
- In-editor test/demo actors: **2,375 ≈ 9%**
- Editor module: **897 ≈ 3.4%**
- BP library + Build.cs: 174

**Caveats that move the number (VERIFIED by reading the files, boundaries approximate):**
1. `Nav3DUtils.cpp` (873) is bucketed as algorithm but roughly half is chunk boundary-voxel extraction and cross-chunk adjacency plumbing — that half belongs in glue. Only ~60 lines are Morton math.
2. `Nav3DDataGenerator.cpp` (1,421) is bucketed as glue but contains real algorithm: `PartitionVolumeIfNeeded` (:629), `ValidatePartitionedVolumes` (:707), `CreateChunkActorForVolume` (:726), `BuildAdjacencyBetweenTwoChunkActors` (:901) — the volume-partitioning and portal-construction pass, ~500-600 LOC.
3. `Nav3DData.cpp` (4,305), the single largest file, decomposes roughly as: ~2,500 LOC UE glue (ANavigationData overrides at :590-1200, chunk-actor registry at :3464-3750, build orchestration at :1818-2211, drawing/generator plumbing); **~1,300 LOC tactical consolidation** (:2391-3300 and :3933-4305 — ConsolidateRegionsFromChunks, BuildCrossChunkAdjacency/Visibility, compact↔build refresh, FindBestTacticalLocation); and **~500 LOC of pure diagnostic logging** (`Analyse`, `AnalyzeActualSpatialDistribution`, `AnalyzeSpatialClustering`, `EstimateOctreeSize`, :1318-1818) that could be deleted outright.
4. `Nav3DTypes.h` (1,575) is 90% declarations/FORCEINLINE accessors/serialization operators, not logic — it inflates the "data" bucket without representing work.

**Adjusted read:** algorithm ≈ 12,500-13,000 LOC (**~48-50%**), UE glue ≈ 7,000-7,500 (**~28%**), the rest debug/test/editor/type-boilerplate (**~22%**). 

**Concentration:** four files are 40% of the runtime module — Nav3DData.cpp (4,305), Nav3DVolumeNavigationData.cpp (2,510), Nav3DTacticalReasoning.cpp (2,502), Nav3DTypes.h (1,575). 

**If the goal is a minimal port:** dropping the editor module (897), test/demo actors (2,375), debug scene proxies (1,398), the diagnostic logging in Nav3DData.cpp (~500), and the deprecated non-compact tactical format + converter (~800-1,000 in TacticalReasoning/DataConverter/Nav3DData) removes roughly **6,000-6,200 LOC (~24% of first-party)** without touching octree construction, pathfinding, or raycasting.
