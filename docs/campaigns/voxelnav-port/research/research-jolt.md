# HEADLINE

CkFoundation has a mature, first-class Jolt integration in a dedicated `CkJolt` module (extracted from CkSpatialQuery in 2026-07, "jolt-collision-world" campaign, Phases 0-5 landed). Jolt 5.2.1 is vendored source-in-tree under CkThirdParty and compiled into that module; any module can include JPH headers by depending on `CkThirdParty` (+ `CkJolt` for the wrappers). Exactly ONE `JPH::PhysicsSystem` exists per Game/PIE UWorld, owned by `UCk_Jolt_Subsystem` and published into the ECS registry as a `TWeakPtr<JPH::PhysicsSystem>` context. Critically for a voxelizer: the WHOLE static level already gets baked into Jolt automatically — `UCk_JoltStaticWorld_Subsystem_UE` sweeps every ULevel on stream-in and extracts what Chaos sees (StaticMesh AggGeom/cooked tri-mesh, ISM/HISM, SplineMesh, Brush convex, Landscape heightfields), so no geometry-feeding step is needed. Units are UE centimeters with a pure Z-up passthrough `Conv` (no scaling, no handedness swap). A channel-filtered `Get_Overlap` already exists and is nearly an `IsBoxOccupied`, and `FCk_Jolt_DomainQueryFilter` already isolates static-vs-dynamic. The main constraint is threading: all existing query call sites are game-thread, and the fixed-timestep step runs inside ECS processors in `FGroup_Transform` — off-thread voxelization is possible against Jolt's locking `NarrowPhaseQuery` but ONLY outside the step window, which this codebase does not currently expose a gate for.

## 1. WHERE Jolt lives (vendoring, version, Build.cs exposure)

VERIFIED.

**Vendored source, not a prebuilt lib.** Full Jolt source tree at `E:\Repos\CkPlugins_Other\Plugins\CkFoundation\Source\CkThirdParty\Public\CkThirdParty\JoltPhysics\Jolt\` — compiled as part of the `CkThirdParty` module (`.cpp` files live under `Public/`, so UBT builds them).

**Version: Jolt 5.2.1** — `Source/CkThirdParty/Public/CkThirdParty/JoltPhysics/Jolt/Core/Core.h:8-10` (`JPH_VERSION_MAJOR 5`, `MINOR 2`, `PATCH 1`). `JPH_VERSION_ID` at `Core.h:71` is used as a cooked-data staleness key.

**Build.cs exposure** — `Source/CkThirdParty/CkThirdParty.build.cs`:
- `PublicIncludePaths` adds `Public/CkThirdParty/JoltPhysics` → any module with `CkThirdParty` as a dependency can `#include <Jolt/...>` directly.
- `PublicDefinitions`: `JPH_ENABLE_ASSERTS`, `JPH_DEBUG_RENDERER`, and `JPH_SHARED_LIBRARY` (client/editor only — NOT for `TargetType.Server`; server builds omit both `JPH_SHARED_LIBRARY` and `JPH_BUILD_SHARED_LIBRARY`). `PrivateDefinitions`: `JPH_BUILD_SHARED_LIBRARY`.
- `CppStandard = Cpp20`, `bUseUnity = false`, `IWYUSupport = None`.

**Modules that currently include Jolt headers directly** (verified by grepping Build.cs deps on `"CkJolt"` and JPH includes):
- `CkJolt` (`Source/CkJolt/CkJolt.Build.cs`) — owns everything; deps include `CkThirdParty`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLog`, `CkSettings`, plus `PhysicsCore`/`Chaos`/`ChaosCore`/`Landscape` (static-world baking reads Chaos's own cooked collision) and `MeshDescription`/`StaticMeshDescription` (batched debug renderer).
- `CkJoltEditor` — cooker commandlet + editor subsystem.
- `CkSpatialQuery` — Probe/ProbeTrace; raw `GetNarrowPhaseQuery()` calls.
- `CkEqs` — query processors take `TWeakPtr<JPH::PhysicsSystem>`.
- `CkWatermark` — stats only.

**A new nav module would add `"CkJolt"` (and transitively `CkThirdParty`) to `PublicDependencyModuleNames`.** Exemplar: `Source/CkSpatialQuery/CkSpatialQuery.Build.cs`.

Module registration: `CkFoundation.uplugin:980` (`CkJolt`), `:990` (`CkJoltEditor`). Module doc: `Source/CkJolt/Claude.md` (320 lines — read it; it is unusually accurate and current).

## 2. WHO owns the Jolt world

VERIFIED.

**Owner: `UCk_Jolt_Subsystem`** — `Source/CkJolt/Public/CkJolt/Subsystem/CkJolt_Subsystem.h:35`, derives `UCk_Game_TickableWorldSubsystem_Base_UE`. `DoesSupportWorldType` returns true only for `EWorldType::Game || EWorldType::PIE` (`Source/CkCore/Public/CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.cpp:186-191`). So: **exactly ONE `JPH::PhysicsSystem` per game/PIE UWorld; zero in editor-preview/inactive worlds.** In PIE multi-instance, each PIE world gets its own (Jolt's global `RegisterTypes` init is ref-counted — `ck::jolt::Request_GlobalJoltInit/Shutdown`, `CkJolt_Utils.cpp:50-76`).

Owned members (`CkJolt_Subsystem.h:59-84`): `TPimplPtr<JPH::TempAllocatorImpl> _TempAllocator`, `JPH::JobSystem* _JobSystem`, `_LayerTable`, the three layer-filter impls, `TSharedPtr<JPH::PhysicsSystem> _PhysicsSystem`, activation/contact listeners, `TSharedPtr<ck::FJoltWorld> _JoltWorld`, debug renderer.

Creation: `Source/CkJolt/Public/CkJolt/Subsystem/CkJolt_Subsystem.cpp:~425` — `_PhysicsSystem = MakeShared<PhysicsSystem>(); _PhysicsSystem->Init(MaxBodies, NumBodyMutexes, MaxBodyPairs, MaxContactConstraints, *_BroadPhaseLayerInterface, *_ObjectVsBroadPhaseLayerFilter, *_ObjectVsObjectFilter);`

**Access handoff — three registry contexts** (`CkJolt_Subsystem.cpp:454-487`):
```cpp
_EcsWorldSubsystem->Get_Registry().SetContext<TWeakPtr<JPH::PhysicsSystem>>(_PhysicsSystem);          // :454
_EcsWorldSubsystem->Get_Registry().SetContext<ck::jolt::FCk_Jolt_LayerContext>({_LayerTable.Get(), _LayerTable->Get_ProbeLayer()}); // :455
_EcsWorldSubsystem->Get_Registry().SetContext<TSharedPtr<ck::FJoltWorld>>(_JoltWorld);                // :487
```
The canonical processor-side pattern is a factory macro reading the context at construction — `CK_PROBE_FACTORY` in `Source/CkSpatialQuery/Public/CkSpatialQuery/Probe/CkProbe_Processor.cpp:54-70` and the identical macro in `Source/CkEqs/Public/CkEqs/Query/CkEqs_Processor.cpp:24-25`:
```cpp
const auto& PhysicsSystem = InRegistry.GetContext<TWeakPtr<JPH::PhysicsSystem>>();
return ProcessorType{InRegistry, PhysicsSystem};
```
CAUTION (from MEMORY + the module doc): processors that cache the registry/context at construction go stale after snapshot restore. `FProcessor_JoltWorld_*` re-reads via `TryGetContext` each tick (`CkJoltWorld_Processor.cpp:44`) — prefer that.

Non-processor access: `UCk_Jolt_Subsystem::Get_PhysicsSystem() -> TWeakPtr<JPH::PhysicsSystem>` (`CkJolt_Subsystem.h:88`), resolved from a WorldContextObject — see `ck_jolt_query_utils::Get_QueryContext` at `Source/CkJolt/Public/CkJolt/Query/CkJoltQuery_Utils.cpp:35-52`.

**How bodies get in — three distinct paths:**
1. **Static world (automatic, whole level)** — `UCk_JoltStaticWorld_Subsystem_UE`. See §6.
2. **Dynamic/kinematic entity bodies (JoltBody feature)** — `UCk_Utils_JoltBody_UE::Add` with `FCk_Fragment_JoltBody_ParamsData` (shape source explicit-or-from-actor, motion type Static/Kinematic/Dynamic, mass source, friction/restitution, collision profile → object layer, CCD). Quartet in `Source/CkJolt/Public/CkJolt/Body/`. Setup drains in `FProcessor_JoltBody_Setup`; domain assignment at `Body/CkJoltBody_Processor.cpp:223-234`.
3. **Characters** — `JPH::CharacterVirtual`-backed (`Source/CkJolt/Public/CkJolt/Character/`). NOTE: **no broadphase body and no BodyID** — characters are INVISIBLE to any narrow/broadphase query. Relevant if the voxelizer should treat characters as obstacles (it should not, but worth knowing).

Geometry sources supported: convex prims (box/sphere/capsule/cylinder from `FKAggregateGeom`), Chaos cooked triangle meshes → `JPH::MeshShape`, `JPH::HeightFieldShape` for Landscape, `StaticCompoundShape`/`RotatedTranslatedShape` composition. See `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltBakeExtraction.h:114-129`.

## 3. QUERY API — exact functions and layer

VERIFIED.

**Ck wrapper layer (BP + AS exposed): `UCk_Utils_JoltQuery_UE`** — decl `Source/CkJolt/Public/CkJolt/Query/CkJoltQuery_Utils.h`, impl `.cpp`. All take `(const UObject* InWorldContextObject, ...)` and a `FCk_Jolt_QueryFilter`:

| Function | Header | Impl | Underlying JPH call |
|---|---|---|---|
| `Get_RayCast` (closest) | `CkJoltQuery_Utils.h:31` | `.cpp:105-141` | `GetNarrowPhaseQuery().CastRay(Ray, RayResult, BroadPhaseLayerFilter{}, ChannelFilter)` — `.cpp:128` |
| `Get_ShapeCast` (closest sweep) | `:41` | `.cpp:143-193` | `NarrowPhaseQuery().CastShape(...)` + `ClosestHitCollisionCollector<CastShapeCollector>` — `.cpp:177` |
| `Get_Overlap` (all) | `:56` | `.cpp:195-242` | `NarrowPhaseQuery().CollideShape(Shape, scale, Transform, CollideShapeSettings{}, RVec3::sZero(), Collector, BroadPhaseLayerFilter{}, ChannelFilter)` — `.cpp:225-227` |
| `Get_RayCastMulti` (sorted) | `:66` | `.cpp:244-289` | `CastRay(Ray, RayCastSettings{}, AllHitCollisionCollector<CastRayCollector>, ...)` — `.cpp:268` |
| `Get_ShapeCastMulti` (sorted) | `:76` | `.cpp:291-345` | `CastShape` + `AllHitCollisionCollector` — `.cpp:325` |
| `Get_OverlapEntities` (deduped handles) | `:89` | `.cpp:347-371` | wraps `Get_Overlap`, drops hits with no live entity |

**Data types** — `Source/CkJolt/Public/CkJolt/Query/CkJoltQuery_Data.h`:
- `FCk_Jolt_ShapeDimensions` (`:41`) — `ECk_Jolt_ShapeType{Box,Sphere,Capsule,Cylinder}`, `_HalfExtents` (Box), `_Radius`, `_HalfHeight`. **Box is half-extents** — directly usable for a voxel cell.
- `FCk_Jolt_QueryFilter` (`:65`) — `TEnumAsByte<ECollisionChannel> _Channel` (default `ECC_Visibility`) + `ECk_Jolt_PairInteraction _MinResponse` (default `Block`). Semantics: a body passes when its authored response to `_Channel` is ≥ `_MinResponse`.
- `FCk_Jolt_HitResult` (`:88`) — `_HasHit`, `_Position`, `_Normal`, `_Fraction`, `FCk_Handle _Entity`.

**Static-world-only introspection ray:** `UCk_JoltStaticWorld_Subsystem_UE::Get_RayCastStaticWorld(Start, End)` — `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h:88`, impl `.cpp:174-206`. Uses `FCk_Jolt_DomainQueryFilter{LayerTable, ECk_Jolt_BodyDomain::Static}` at `.cpp:192-196`. Returns `ck::jolt::FCk_Jolt_StaticWorldRayHit` (`.h:31`). Also exposed via `UCk_Utils_JoltStaticWorld_UE`.

**Raw JPH call sites outside the wrapper** (patterns to copy for a custom collector):
- `Source/CkSpatialQuery/Public/CkSpatialQuery/Probe/CkProbeTrace_Utils.cpp:412-413` (CastRay + custom `CastRayCollector`), `:678-679` (CastShape).
- `Source/CkSpatialQuery/Public/CkSpatialQuery/Probe/CkProbe_Processor.cpp:770` (CastShape under `SCOPE_CYCLE_COUNTER`).
- `Source/CkEqs/Public/CkEqs/Query/CkEqs_Algorithm.cpp:328-369, 766-830, 926-974`.

**Broadphase-only AABB query: NOT used anywhere in Ck code today.** `PhysicsSystem::GetBroadPhaseQuery()` exists (`Jolt/Physics/PhysicsSystem.h:90`) and `BroadPhaseQuery::CollideAABox(const AABox&, CollideShapeBodyCollector&, BroadPhaseLayerFilter, ObjectLayerFilter)` is available (`Jolt/Physics/Collision/BroadPhase/BroadPhaseQuery.h:37`), but the only in-tree callers are Jolt's own `SoftBodyMotionProperties.cpp:238` and `BodyInterface.cpp:233`. Also unused: `NarrowPhaseQuery::CollidePoint` (`Jolt/Physics/Collision/NarrowPhaseQuery.h:~38`) and `CollideShapeWithInternalEdgeRemoval`.

**AngelScript exposure:** `Script/Generated/utils_jolt_query.as` (all six), `utils_jolt_static_world.as`, `utils_jolt_body.as`, `utils_jolt_character.as`, `utils_jolt_constraint.as`, `utils_jolt_rope.as`, `utils_jolt_static_actor.as`. Note AS strips the WorldContextObject param.

## 4. 'IsBoxOccupied(AABB, filter)' — what exists, and thread safety

**A suitable API is ~90% there. VERIFIED; the threading answer is INFERRED from code reading, not tested.**

**Closest existing call, today, one line:**
```cpp
const auto Hits = UCk_Utils_JoltQuery_UE::Get_Overlap(WorldCtx, CellCenter, FRotator::ZeroRotator,
    FCk_Jolt_ShapeDimensions{ECk_Jolt_ShapeType::Box}.Set_HalfExtents(CellHalfExtents),
    FCk_Jolt_QueryFilter{}); // _Channel=ECC_Visibility, _MinResponse=Block
return Hits.Num() > 0;
```
Gaps for a voxelizer, all cheap to close inside CkJolt:
1. **`AllHitCollisionCollector` allocates and collects every hit** (`CkJoltQuery_Utils.cpp:223`). A voxelizer wants `AnyHitCollisionCollector<CollideShapeCollector>` — early-out on first hit. Big win at voxel-grid scale.
2. **`Fill_HitAttribution` resolves an ECS entity per hit** (`.cpp:76-85`, calls `TryResolve_Entity` → `Get_RegistryView().IsValid`). Pure overhead for occupancy, and it is **game-thread-bound** (touches the ECS registry). An occupancy variant must skip it.
3. **Shape is rebuilt per call** — `CreateShape_FromDimensions` (`.cpp:213`) allocates a new `JPH::BoxShape` every query. Hoist one `JPH::Ref<JPH::Shape>` per voxel size.
4. Wrapper resolves subsystems per call (`Get_QueryContext`, `.cpp:35-52`).

**Recommendation:** add `ck::jolt::Get_IsBoxOccupied(const JPH::PhysicsSystem&, AABB, ObjectLayerFilter&)` in CkJolt at the `ck::jolt` namespace level (below the BPFL), taking the PhysicsSystem by reference like `CkProbeTrace_Utils` does — reusable shape, any-hit collector, no ECS resolution. Cheaper still: `GetBroadPhaseQuery().CollideAABox` for a conservative first pass (bounding-box only, no shape test) then narrow-phase only on cells the broadphase flags — classic two-level voxelization.

**Thread safety — Jolt's model:**
- `PhysicsSystem::GetNarrowPhaseQuery()` returns `mNarrowPhaseQueryLocking` (`Jolt/Physics/PhysicsSystem.h:93`), which takes per-body read locks internally via `BodyLockInterfaceLocking`. `GetNarrowPhaseQueryNoLock()` (`:94`) is the unlocked variant, documented "use with great care!".
- Jolt's contract: the **locking** NarrowPhaseQuery is safe to call from multiple threads concurrently **as long as `PhysicsSystem::Update` is not running** and no one is mutating bodies through the NoLock interfaces. During `Update`, worker jobs hold body write locks — a locking query would block or (worse for a nav build) read torn mid-step state.
- `NumBodyMutexes` is a fixed pool (passed to `PhysicsSystem::Init`, `CkJolt_Subsystem.cpp:~426`); heavy concurrent locking queries contend on that pool.

**How this codebase actually uses it:** every existing query site is game-thread.
- `UCk_Utils_JoltQuery_UE::*` are BPFLs called from BP/AS/game thread; `Fill_HitAttribution` touches the ECS registry so they are **game-thread only by construction**.
- `Get_SurfaceNormal` (`CkJoltQuery_Utils.cpp:87-100`) is the ONE place taking an explicit `JPH::BodyLockRead{PhysicsSystem->GetBodyLockInterface(), BodyId}` — with a `Lock.Succeeded()` guard (`.cpp:96`).
- `FJoltWorld::DoCapturePoses_AnyThread` (`World/CkJoltWorld.cpp:238-285`) is the only off-game-thread Jolt code, and it deliberately uses `GetBodyInterfaceNoLock()` (`.cpp:253`) with the comment: *"NoLock is safe here: PhysicsSystem::Update has returned on this thread, so no worker is mutating bodies, and the game thread does not touch Jolt while an async step is pending."*

**Practical conclusion for off-thread voxelization:**
- Statics never move. Once `OptimizeBroadPhase` has run and no bodies are being added/removed, the static broadphase tree + static body shapes are **immutable**, so concurrent locking-NarrowPhaseQuery reads restricted to the Static domain are logically safe — the risk is contention/blocking against a running `Update`, not corruption of static data.
- BUT there is **no existing gate** exposing "the step is not running" to an outside thread. `FJoltWorld::_AsyncFuture` / `WaitForAsyncStep()` (`CkJoltWorld.h:193-194`) are private-ish, game-thread-only, and the sync path steps inline inside `FProcessor_JoltWorld_Step::DoTick`. A worker thread has no way to know it is outside the window.
- **Also blocking:** `UCk_JoltStaticWorld_Subsystem_UE` adds/removes bodies on level streaming (game thread, `AddBodiesPrepare/Finalize` + `OptimizeBroadPhase`) — a background voxelizer must be cancelled/re-run on stream events, and must NOT be mid-query while `OptimizeBroadPhase` runs (Jolt: *"Don't call this function while bodies are being modified from another thread"*, `PhysicsSystem.h:~116`).
- **Safest designs, in order:** (a) budgeted game-thread voxelization from an ECS processor in a group AFTER `FProcessor_JoltWorld_Step`, N cells/frame; (b) off-thread but explicitly sequenced — CkJolt grows a "step barrier" gate (e.g. an `FRWLock` the step takes for write and voxel workers take for read), which is a genuine new CkJolt contract, not something to bolt on from the nav module.
- Speculation, worth measuring: for static-only voxelization the highest-throughput option is to bypass the live world entirely — snapshot the static bodies' `JPH::Shape` refs and transforms once (they are ref-counted and immutable) and run `Shape::CollidePoint`/`CastRay` against a private, read-only structure off-thread with zero locking.

The `_Entity` attribution channel is a real bonus: every baked static body carries its source actor's ECS entity id as Jolt UserData, so a nav voxel can record WHICH actor blocked it (for nav-area markup, dynamic invalidation) without any FName table.

## 5. UNITS & CONVENTIONS

VERIFIED — this is the single most reassuring finding.

**Units: UE centimeters throughout. There is NO scaling in the conversion layer.** `ck::jolt::Conv` (`Source/CkJolt/Public/CkJolt/CkJolt_Utils.h:44-57`, impl `CkJolt_Utils.cpp:140-205`) is a pure component-wise copy:
```cpp
auto Conv(const FVector& InVector) -> JoltVec3
{ return JoltVec3{(float)InVector.X, (float)InVector.Y, (float)InVector.Z}; }   // CkJolt_Utils.cpp:140-149
auto Conv(JoltVec3 InVector) -> FVector
{ return FVector{InVector.GetX(), InVector.GetY(), InVector.GetZ()}; }          // :161-166
```
Same for `FQuat`/`JoltQuat` (`.cpp:184-192`) and `FMatrix`/`Mat44` (`.cpp:88-138`) — element-for-element, no transpose, no negation.

**Handedness/axis: Z-up passthrough, no axis swap.** Jolt's own convention is Y-up; this codebase deliberately does NOT convert — it treats Jolt's coordinate space as UE's directly. Consequences, all handled inside CkJolt:
- **Gravity is overridden** because Jolt's default is `(0,-9.81,0)` Y-down in meters: `_PhysicsSystem->SetGravity(ck::jolt::Conv(FVector{0,0,GetWorld()->GetGravityZ()}))` — `CkJolt_Subsystem.cpp:~433`.
- **Capsules/cylinders need axis correction.** Jolt's are Y-axis aligned; `ck::jolt::Get_ShapeAxisCorrection_YToZ()` (`CkJolt_Utils.h:66`) returns +90° about X and callers wrap in `RotatedTranslatedShapeSettings`. `CreateShape_FromDimensions` (`CkJoltShapeFactory.h:19`) applies it internally, so shapes come out UPRIGHT. **Boxes and spheres need no correction** — good news for a voxelizer.
- **Heightfields need the same wrap plus a row flip** — `CreateHeightFieldShape` (`CkJoltBakeExtraction.h:124-127`): Jolt heightfields are local Y-up spanning X/Z, so it wraps in `RotatedTranslatedShape(+90° about X)` with rows flipped and a `-(N-1)*scaleY` local-Z offset.
- Debug renderer emits **both windings** per triangle because Conv is a handedness passthrough and one winding renders inside-out (`Claude.md` § Debug draw).

**Because Jolt is metre-tuned by default and this world is cm, all PhysicsSettings lengths/velocities are ×100** (`CkJolt_Subsystem.cpp:~438-448`):
```cpp
mSpeculativeContactDistance  = 2.0f;    // was 0.02 m
mPenetrationSlop             = 2.0f;    // 0.02 m
mMaxPenetrationDistance      = 20.0f;   // 0.2 m
mManifoldToleranceSq         = 1.0e-2f; // 1.0e-6 m^2  (squared → x100^2)
mPointVelocitySleepThreshold = 3.0f;    // 0.03 m/s
mMinVelocityForRestitution   = 100.0f;  // 1 m/s
```
Ratios (`mBaumgarte`, `mLinearCast*`), iteration counts and times keep Jolt defaults. Character scalars are likewise cm-converted (predictive contact distance 10, padding 2, tolerance 0.1) and `MaxStrengthNewtons` is ×100.

**No scaling helper exists or is needed — pass UE world-space FVectors straight through `Conv`.** `mPenetrationSlop = 2.0f` cm is the one number a voxelizer should respect: features thinner than ~2 cm are below the sim's tolerance, and voxel cells should be comfortably larger.

## 6. STATIC WORLD GEOMETRY — already fully in Jolt

VERIFIED. **This is the decisive finding: the voxelizer does NOT need a geometry-feeding step.**

**`UCk_JoltStaticWorld_Subsystem_UE`** (`Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h:60`, `UCk_Game_WorldSubsystem_Base_UE` → Game/PIE only) automatically bakes the whole static level into the Jolt world:
- Hooks `LevelAddedToWorld` / `LevelRemovedFromWorld` (`_LevelAddedHandle`/`_LevelRemovedHandle`, `.h:206-207`) so bodies add/remove in lockstep with streaming. **World Partition runtime cells stream as ULevels, so one delegate pair covers WP and legacy sublevels uniformly** — WP streaming is handled.
- `OnWorldBeginPlay` (`.h:76`) does a sweep for levels that were added before the ECS world was ready.
- `InitializeDependency`s on `UCk_EcsWorld_Subsystem_UE` so the registry outlives it.
- Batched adds via `AddBodiesPrepare/Finalize` (`DoBatchAdd_Bodies`, `.h:157`); `OptimizeBroadPhase` requested after bulk changes (`_BodyChurnSinceOptimize`, `.h:222`).

**What gets extracted** — `ck::jolt::bake::ExtractActor` / `ExtractComponent` (`Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltBakeExtraction.h:94-105`). Per the module doc and header, in priority order:
- **SplineMesh** → per-instance deformed BodySetup.
- **ISM/HISM** → per-instance bodies sharing a cached shape below `_CompoundShapeInstanceThreshold` (default 32); ONE `StaticCompoundShape` above it.
- **StaticMesh** → `FKAggregateGeom` per trace flag; `CTF_UseComplexAsSimple` → Chaos cooked tri-mesh → `JPH::MeshShape` (winding-swapped). `BuildShape_FromBodySetup` (`.h:114-117`) honors `CollisionTraceFlag`, falling back to tri-mesh only when AggGeom is empty and the flag permits complex.
- **Brush** → convex elems (a null `BrushBodySetup` is a Verbose skip — every level's default/builder brush).
- **Landscape** → `WITH_EDITOR` heightfield per component (`CreateHeightFieldShape`, `.h:124`), pinned by the `Ck.Jolt.Bake.HeightField` test. Collision signature comes from the paired `GetCollisionComponent()` heightfield component, not the render component.
- **Skipped:** unregistered, editor-only/visualization, `NoCollision`, simulating-physics, and (LevelSweep policy) **Movable mobility** components (`.h:27-34, 91-93`). Collision-enabled-but-no-valid-geometry → `CK_ENSURE` + skip; **never a bounding-box substitute** — so the Jolt world is faithful, not conservative.

**Two data modes** (`_PIEStaticWorldMode`, default `LiveExtract` — `Settings/CkJolt_ProjectSettings.h:107`): live extraction in PIE, or cooked data. Packaged builds are **always cooked** (`UCk_Jolt_CookedWorldIndex_UE` per map + `UCk_Jolt_CookedCell_UE` per bake-grid cell; cooker in `CkJoltEditor`, `-run=CkJoltCook`). Stale cooked data (CookVersion / `JPH_VERSION_ID` / per-actor runtime hash mismatch) **ensures loudly and is SKIPPED, never silently used or re-extracted** — a voxelizer running against a stale-skipped cell would see a hole in the world. Worth an explicit check: `Get_NumStaticBodies()` (`.h:81`) is the cheap sanity signal.

**Runtime-spawned statics:** `UCk_Utils_JoltStaticWorld_UE::Request_BakeActor(AActor)` / `Request_RemoveActor` (`.h:94-100`, `ECk_Jolt_ExtractionPolicy::ExplicitActor` bakes Movable-mobility components). A nav module can force-bake geometry the level sweep skipped.

**Attribution:** ONE `FFragment_JoltStaticActor_Current` entity per source actor contributing ≥1 baked body, DebugName = actor FName; each baked body's Jolt UserData is that entity id. So a voxel hit resolves to an entity handle through the same path a dynamic body hit uses (`UCk_Utils_JoltStaticActor_UE::Get_SourceActor` / `Get_SourceActorName` / `Get_NumBodies`).

**Caveat worth flagging to the planner:** the static world is intentionally NOT paired with probes at the broadphase (`FCk_Jolt_ObjectVsBroadPhaseLayerFilter_Table::ShouldCollide`, `CkJoltCollisionLayerTable.h:113-127` — statics don't pair with the static tree, and the probe layer is explicitly excluded from the static tree). That is a SIMULATION/contact filter. **Scene queries pass `JPH::BroadPhaseLayerFilter{}` (accept-all) plus an ObjectLayerFilter**, so `Get_RayCast`/`Get_Overlap` DO see static-world bodies. Verified at `CkJoltQuery_Utils.cpp:129, 179, 227`.

## 7. LAYERS / FILTERS — how to select 'static world'

VERIFIED. `Source/CkJolt/Public/CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h` + `CkJoltCollisionLayer_Data.h`.

**Broadphase layers — exactly TWO** (`CkJoltCollisionLayerTable.h:78-83`):
```cpp
namespace broadphase_layers {
    static constexpr JPH::BroadPhaseLayer Static{0};
    static constexpr JPH::BroadPhaseLayer Dynamic{1};
    static constexpr JPH::uint Num_Layers{2};
}
```
Mapped from a body's object layer by `FCk_Jolt_BroadPhaseLayerInterface_Table::GetBroadPhaseLayer` (`:96-101`) via `_Table.Get_Domain(inLayer)`.

**Object layers — dynamic, signature-derived, one per unique collision signature.** `FCk_Jolt_CollisionLayerTable` (`:23`), `MaxLayers = 1024` (`:28`). `FCk_Jolt_CollisionSignature` (`CkJoltCollisionLayer_Data.h:41`) captures the complete UE/Chaos interaction inputs per body: `_ObjectChannel` (ECollisionChannel), `_ResponseMask` (2 bits × 32 channels: 0=Ignore, 1=Overlap, 2=Block), `_CollisionEnabled`, and `_Domain` (`ECk_Jolt_BodyDomain{Static, Dynamic}`, `:13-17`). Built from a component's EFFECTIVE collision setup including per-component custom response edits (`Make_FromComponent`, `:74-76`). Seeded from `UCollisionProfile` at subsystem init (`Build_FromCollisionProfiles`, table `.h:35`) and grown on demand via `Get_OrRegisterLayer` (`:39`).

**THREAD CONTRACT, documented at `CkJoltCollisionLayerTable.h:20-22`:** registration is **GAME THREAD only**; Jolt's filter callbacks read from worker threads during `Update`. The signature array is reserved to fixed capacity up front (never reallocates) and the visible count is published through `std::atomic<int32> _PublishedCount` (`:70`) — so **reads are lock-free and worker-thread safe**. This directly matters: a voxelizer's ObjectLayerFilter reads this table, and reading is safe off-thread; registering a new layer is not.

**Domain assignment:**
- Static-world baked bodies: `FCk_Jolt_CollisionSignature::Make_FromComponent(InComponent, ECk_Jolt_BodyDomain::Static)` — `StaticWorld/CkJoltBakeExtraction.cpp:488`, `:688`; layer resolved at `StaticWorld/CkJoltStaticWorld_Subsystem.cpp:557` (cooked) and `:634` (live).
- JoltBody entity bodies: `Body/CkJoltBody_Processor.cpp:223-225` — `MotionType == Static ? Domain::Static : Domain::Dynamic`. **So a Static-motion-type JoltBody entity also lands in the Static domain** — a domain filter alone does NOT perfectly mean "level geometry"; it means "non-moving". Discriminate further via the `_Entity` handle (`Has<FFragment_JoltStaticActor_Current>` vs `FFragment_JoltBody_Current`) if that distinction matters.

**Ready-made filters for a nav voxelizer** — all `JPH::ObjectLayerFilter` subclasses in `CkJoltCollisionLayerTable.h`:
- **`FCk_Jolt_DomainQueryFilter{Table, ECk_Jolt_BodyDomain::Static}`** (`:176-192`) — **this is the one.** `ShouldCollide` returns `_Table.Get_Domain(inLayer) == _Domain`. Already proven in `Get_RayCastStaticWorld` (`CkJoltStaticWorld_Subsystem.cpp:192-196`).
- `FCk_Jolt_ChannelQueryFilter{Table, Channel, MinResponse}` (`:155-173`) — UE trace semantics: `Get_ResponseOfLayerToChannel(inLayer, _Channel) >= _MinResponse`. Use if the voxelizer should respect a specific channel (e.g. a dedicated `ECC_GameTraceChannelN` "NavBlocking").
- Compose both: no AND-combinator exists today; write a small `FCk_Jolt_NavQueryFilter` in the nav module (or better, in CkJolt) doing `Domain==Static && ResponseToChannel>=Block`. ~15 lines, matching the existing pattern exactly.
- Additionally pair with a **`JPH::BroadPhaseLayerFilter` that accepts only `broadphase_layers::Static`** — the existing query wrappers pass accept-all `BroadPhaseLayerFilter{}`, leaving broadphase perf on the table. For a voxelizer doing thousands of queries this is a meaningful, easy win.

Semantics note (documented once on `ECk_Jolt_PairInteraction`, `CkJoltCollisionLayer_Data.h:23-31`): pair resolution mirrors UE — `min(A.Response[B.Channel], B.Response[A.Channel])`. **Jolt's pair filter is BINARY**: anything != Ignore means "interact"; Block-vs-Overlap is resolved at the query/contact sites, which is exactly what `_MinResponse` does.

## 8. THREADING MODEL & step phases

VERIFIED from code + module doc.

**Jolt's own JobSystem runs the sim — NOT UE's task graph.** `CkJolt_Subsystem.cpp:~380-415`: `JPH::JobSystemThreadPool` with workers named `JoltWorker_{i}` (count = `_NumPhysicsThreads`, or `hardware_concurrency()-1`), or `JPH::JobSystemSingleThreaded`. Selected by `jolt.EnableParallelPhysics` — **startup-only** (the JobSystem is created once in `Initialize`; cmdline form `-jolt.EnableParallelPhysics=0`).

**The step lives in ECS processors, not in subsystem Tick.** `UCk_Jolt_Subsystem::Tick` now does only `Super::Tick` + gated debug draw. Step processors — `Source/CkJolt/Public/CkJolt/World/CkJoltWorld_Processor.h`, all `Group = FGroup_Transform`, chained after `FProcessor_Transform_HandleRequests`:
1. `FProcessor_JoltWorld_WaitForAsync` (`:24`, `RunAfter FProcessor_Transform_HandleRequests`) — consumes the prior async step, applies the pose buffer.
2. `FProcessor_JoltWorld_DrainEvents` (`:44`, `RunAfter WaitForAsync`) — drains the contact queue to registered routers; **runs even when paused**.
3. `FProcessor_JoltWorld_PlanStep` (`:66`, `RunAfter DrainEvents`) — pure fixed-timestep planner (`ck::jolt::ComputeStepPlan`, `CkJoltWorld.h:50-54`); owns the world-invalid/paused gate.
4. `FProcessor_JoltWorld_Step` (`:88`, `RunAfter TDepList<FProcessor_JoltBody_KinematicPush, FProcessor_JoltCharacter_PreStep>`) — `OptimizeBroadPhase` if requested, then N planned sub-steps of `PhysicsSystem::Update` + pose capture; sync-applies or dispatches async.

Full order (module doc):
```
WaitForAsync → DrainEvents → PlanStep → SleepStateMirror ─┐
     │                                                    ├→ KinematicPush ─┐
     ├→ JoltBody_Setup → JoltBody_HandleRequests ─────────┘                 ├→ Step → WritebackInterpolated
     └→ JoltCharacter_Setup → ..._HandleRequests → Character_PreStep ───────┘
```

**HARD RULE for a new module — `Claude.md` states it explicitly:** *"Any new processor that touches Jolt state must add the same `RunAfter FProcessor_JoltWorld_WaitForAsync` edge."* The scheduler's Kahn tie-break is LEXICAL by processor name, so without the edge a processor can run while the PREVIOUS frame's async step is still in flight. **A nav voxelizer processor querying Jolt MUST declare `RunAfter FProcessor_JoltWorld_WaitForAsync`** (and realistically `RunAfter FProcessor_JoltWorld_Step` if it wants post-step state). EndPlay processors additionally open with the async guard (`WaitForAsyncStep()` iff a future is pending).

**Fixed timestep:** `_FixedTimestepHz` default 60, `_MaxPhysicsStepsPerFrame` default 4 (`Settings/CkJolt_ProjectSettings.h:77,83`). Excess accumulated time past the budget is DROPPED by design (spiral-of-death guard, no ensure) — **sim time lags real time under load; tests must accumulate delta, never count frames.**

**Async mode:** `jolt.EnableAsyncPhysicsUpdate` (startup-only) dispatches the whole fixed-step batch (`Update` + pose capture + `DoStepCharacters_AnyThread`) to UE `Async(TaskGraph)`. Results are one frame latent; debug draw is skipped in async frames. **The async batch touches only Jolt objects + `FJoltWorld::_PoseBuffer` + character out-fields; the game thread must not touch Jolt while the future is pending.** That is the window a background voxelizer would collide with — and there is no public gate for it today (`WaitForAsyncStep`/`_AsyncFuture` are `FJoltWorld` internals, `CkJoltWorld.h:193-194, 213`).

**Callbacks:** contact/activation listeners fire on Jolt worker threads → queued under `FCriticalSection` → drained on the game thread in `FProcessor_JoltWorld_DrainEvents`. **Never touch ECS from a Jolt callback.** Consumers register via `UCk_Jolt_Subsystem::RegisterContactRouter(FName, ck::FCk_Jolt_ContactEventRouter)` (`CkJolt_Subsystem.h:93`); routers fire game-thread in registration order.

**Constraints on off-thread queries, summarized:** (1) no exposed "step not running" gate; (2) `OptimizeBroadPhase` runs on the game thread inside `FProcessor_JoltWorld_Step` and Jolt forbids concurrent body modification during it; (3) static-world level streaming adds/removes bodies on the game thread; (4) the layer table is lock-free to READ off-thread but game-thread-only to register; (5) any ECS entity resolution (`TryResolve_Entity`, `Fill_HitAttribution`) is game-thread only. Budgeted game-thread voxelization in a post-Step processor is the low-risk path; true off-thread requires a new synchronization contract inside CkJolt.

## 9. Adjacent notes for the planner

- **A `CkNavigation` module ALREADY EXISTS** at `Source/CkNavigation` (registered `CkFoundation.uplugin:730`), but it is a **Recast / `UNavigationSystemV1` wrapper** — path queries only, explicitly "no agents, no steering, no avoidance" (those live in `CkCrowd`). Its `CLAUDE.md` marks it *"⏳ Skeleton only — full module landed in Gate 1"*, and it has a `PLAN.md` + `CONTINUATION_PROMPT_DiagnosticGym.md`. Its Build.cs depends on `NavigationSystem`/`AIModule`, NOT on CkJolt. **The planning session must decide: extend CkNavigation, or add a sibling (e.g. `CkNavVolume`/`CkNav3D`).** Given the existing module is Recast-2D-surface-shaped and the new one is Jolt-voxel-3D-shaped, a sibling module is the cleaner read — but this is a genuine fork worth an explicit decision.
- **Existing test coverage to model against** — `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkJolt/`: `Test_JoltBake_BoxConvexRadius.cpp`, `Test_JoltBake_HeightField_KnownHeightsZUp.cpp`, `Test_JoltBake_ShapeBlob_RoundTrip.cpp`, `Test_JoltLayers_TableBuild_FromCollisionProfiles.cpp`, `Test_JoltWorld_FixedTimestep.spec.cpp`, `Test_JoltBody_Lifecycle.spec.cpp`, `Test_JoltBody_OwnershipExclusivity.spec.cpp`, `Test_Jolt_GeometryParitySampler.spec.cpp`, `Test_JoltBody_Benchmark.cpp`. Note `Test_Jolt_GeometryParitySampler` — a Chaos-vs-Jolt geometry parity sampler, directly reusable to validate that voxel occupancy matches UE collision.
- **Stress gym exists:** `Plugins/CkTests/Source/CkTests/Public/CkJoltStressGym_Utils.h` + `CkTestsEditor/Private/CkGymJoltStaticBakeAuthoring.cpp`.
- **Debug visualization is free.** `ck.Jolt.DebugDraw.Enabled` draws all bodies (static + dynamic) via a BATCHED `JPH::DebugRenderer` (`CkJoltDebugger`, `Subsystem/CkJolt_DebugRenderer.h`) that instances transient UStaticMeshes — so you can eyeball exactly what geometry the voxelizer is querying against. Also `ck.Jolt.DebugDraw.Opacity`, `.SleepColoring`, `.Velocity`, `.WorldTransform`, `.Constraints`. Consumers install an opt-in gate via `Set_DebugDrawGate` (`CkJolt_Subsystem.h:113`).
- **Perf datapoint** (from the campaign's VALIDATION.md, referenced in `Claude.md` — I did NOT read that file, it lives in the host project's `docs/campaigns/jolt-collision-world/`): Jolt ≈2.8× cheaper than Chaos at 10k spread bodies; island size dominates cost. Treat as second-hand.
- **Stats:** `STATGROUP_CkJolt` (`stat CkJolt` / Insights) — `JoltWorld_Step`, `JoltPhysics_Update(_Async)`, `JoltBody_WritebackInterpolated`, `JoltBody_KinematicPush`, contact queue/drain. A voxelizer should add its own cycle stats in the same group (`Source/CkJolt/CkJolt_Stats.h`).
- **Capacity ceiling to watch:** `_MaxBodies` default 65536 (`Settings/CkJolt_ProjectSettings.h:41`). The doc warns a 10k-body single pile already exceeds default MaxBodyPairs/MaxContactConstraints and CkJolt ensures loudly. Voxelization adds queries, not bodies, so this should not bind — unless the design ever considers inserting probe bodies per cell (don't).
- **Anti-patterns CkJolt states explicitly:** don't create a second `JPH::PhysicsSystem`; don't call `PhysicsSystem::Update` yourself; don't resolve entities inside Jolt callbacks; don't bypass `Conv`/axis-correction with hand-rolled conversions.
- **Files to read first in a planning session, in order:** `Source/CkJolt/Claude.md` (320 lines, the map), `Source/CkJolt/Public/CkJolt/Query/CkJoltQuery_Utils.cpp` (373 lines, the whole query surface), `Source/CkJolt/Public/CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h` (205 lines, all filters), `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h`.
