# Phase 1 port map — octree core + Jolt voxelization

> Executor spec. Written to be followed literally. Every claim is labelled **VERIFIED** (file:line
> read during authoring) or **INFERRED** (reasoned, not confirmed).
> Authored 2026-08-03 at the Phase 0→1 boundary. Sources: `research-coupling.md`, `research-jolt.md`,
> `research-navStack.md`, `PROMPT.md`, `PHASE_0.md`, plus direct reads of `F:\Nav3D-2.0` and
> `Source/{CkTimer,CkAStar,CkPathNetwork,CkJolt,CkEcs}`.
>
> **Executors never make design decisions.** Section 6 lists every fork this map could not close.
> If you hit one, STOP and report — do not pick a branch.

---

## 0. Orientation — what Phase 1 actually builds

Three things, in this order:

1. **`Octree/`** — the engine-light SVO: types, Morton addressing, layer/leaf containers, position↔address
   queries, neighbour enumeration. No UObject, no UPROPERTY, no Jolt, no ECS. Pure `FVector`/`FBox`/`TArray`.
2. **`Backend/`** — the geometry seam: one pure-C++ interface + one CkJolt-backed implementation. This is
   the ONLY place CkVoxelNav learns that a physics world exists.
3. **`Volume/`** — the ECS feature: the built-octree fragment, the resumable build state machine, the
   budgeted build processor, `Request_Build`, the epoch.

Phase 2 (pathfinding) is *designed for* here (§4) but not built here. Phase 3 (chunks/dynamics) is
explicitly out — every function tagged `[PHASE 3]` below is listed only so you don't port it by accident.

**Line-range convention.** `VND:NNN` = `F:\Nav3D-2.0\Source\Nav3D\Private\Nav3DVolumeNavigationData.cpp`
line NNN. `Types.h:NNN` = `...\Public\Nav3DTypes.h`. `Types.cpp:NNN` = `...\Private\Nav3DTypes.cpp`.

---

## 1. TYPE TRANSLATION TABLE

### 1.1 Scalar aliases

All in `namespace ck::voxelnav`, declared in `Public/CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h`.
**Engine-light: no UObject, no USTRUCT, no UPROPERTY on anything in §1.1–§1.6** unless the row says otherwise.

| Nav3D (`Types.h:9-14`, VERIFIED) | CkVoxelNav | Why it changed |
|---|---|---|
| `using MortonCode = uint_fast64_t` | `using MortonCode = uint64;` | **LOCKED.** `uint_fast64_t` is platform-variable width (`research-coupling.md` §7). Pinning is non-negotiable — it is half of the serialization ABI hazard. |
| `using LayerIndex = uint8` | `using LayerIndex = uint8;` | unchanged (4 bits used; 15 = invalid sentinel) |
| `using NodeIndex = uint32` | `using NodeIndex = int32;` | 22 bits used. `int32` so `INDEX_NONE` comparisons are honest; upstream stores `INDEX_NONE` into an unsigned 22-bit field (`VND:1835`, `VND:212`) and it silently becomes `0x3FFFFF`. |
| `using LeafIndex = int32` | `using LeafIndex = int32;` | unchanged |
| `using SubNodeIndex = uint8` | `using SubNodeIndex = uint8;` | 6 bits used (0..63) |
| `using NeighbourDirection = uint8` | `using NeighbourDirection = uint8;` | unchanged (0..5, indexes `GNeighbourDirections`) |

**Keep the upstream spellings.** The port is mechanical if the names match; and the house `In*`
parameter convention (root `CLAUDE.md` § Naming) automatically kills upstream's shadowing hazard —
Nav3D writes `float GetLayerNodeSize(const LayerIndex LayerIndex) const` (`VND:2449`, VERIFIED), where
the parameter shadows the type. Ours is `InLayerIndex`, so the collision cannot occur.

`GNeighbourDirections[6]` (`Types.h:26-29`, VERIFIED) ports verbatim as
`inline constexpr FIntVector GNeighbourDirections[6]` inside `ck::voxelnav`. The azimuth/elevation
sample constants (`Types.h:17-23`) are **tactical-reasoning only — do NOT port.**

### 1.2 `FNav3DNodeAddress` → `ck::voxelnav::FNodeAddress`

Upstream (`Types.h:108-174`, VERIFIED) is a mixed-type bitfield struct
(`uint8 LayerIndex:4; uint_fast32_t NodeIndex:22; uint8 SubNodeIndex:6;`) with a raw-memory
`operator<<`. **Replace with a single packed `uint32`.** This is the locked serialization fix
(PROMPT.md decision 6) and it also fixes three upstream defects.

```cpp
// Octree/CkVoxelNav_Octree_Types.h
namespace ck::voxelnav
{
    // Packed layout — IDENTICAL to Nav3D's GetNavNodeRef (Types.h:140-144):
    //   bits [31..28] LayerIndex (0..14; 15 == invalid)
    //   bits [27..6]  NodeIndex  (0..4'194'303)
    //   bits [5..0]   SubNodeIndex (0..63)
    // The packed uint32 IS the wire/serialization form — never memcpy the struct.
    struct CKVOXELNAV_API FNodeAddress
    {
    public:
        static constexpr uint32 InvalidPacked = 0xF0000000u;   // LayerIndex nibble == 15
        static constexpr LayerIndex InvalidLayer = 15;
        static constexpr LayerIndex MergedCellLayer = 14;      // reserved for [C-D4]; see §1.3
        static constexpr NodeIndex MaxNodeIndex = 0x3FFFFF;    // 22 bits

        FNodeAddress() = default;

        static auto
        Make(LayerIndex InLayerIndex, NodeIndex InNodeIndex, SubNodeIndex InSubNodeIndex = 0) -> FNodeAddress;

        static auto
        Make_FromPacked(uint32 InPacked) -> FNodeAddress;

        auto Get_Packed()       const -> uint32      { return _Packed; }
        auto Get_LayerIndex()   const -> LayerIndex  { return static_cast<LayerIndex>(_Packed >> 28); }
        auto Get_NodeIndex()    const -> NodeIndex   { return static_cast<NodeIndex>((_Packed >> 6) & 0x3FFFFFu); }
        auto Get_SubNodeIndex() const -> SubNodeIndex{ return static_cast<SubNodeIndex>(_Packed & 0x3Fu); }
        auto Get_IsValid()      const -> bool        { return Get_LayerIndex() != InvalidLayer; }

        auto Set_NodeIndex(NodeIndex InNodeIndex) -> FNodeAddress&;
        auto Invalidate() -> void { _Packed = InvalidPacked; }

        auto operator==(const FNodeAddress&) const -> bool = default;

    private:
        uint32 _Packed = InvalidPacked;
    };

    CKVOXELNAV_API auto GetTypeHash(const FNodeAddress& InAddress) -> uint32;   // == InAddress.Get_Packed()
}
```

`Make` must `CK_ENSURE_IF_NOT` each field is in range, then early-out to an invalid address through a
separate ordinary `if` (root `CLAUDE.md` non-negotiable #3 — the ensure body must be empty).

**Upstream defects this closes (all VERIFIED):**

| Defect | Site | Effect |
|---|---|---|
| Raw-bytes `operator<<` on a mixed-type bitfield | `Types.h:170-174` | Compiler/ABI-dependent on-disk format; ~12 bytes with padding. Any struct or toolchain change silently invalidates every bake. |
| `explicit FNav3DNodeAddress(const int32 Index)` shifts *into* each field (`LayerIndex(Index << 28)`) instead of extracting | `Types.h:114-117` | The "unpack a NavNodeRef" ctor is written backwards and produces garbage. Our `Make_FromPacked` shifts right. |
| Free-space marker stuffs a 64-bit Morton code into a **6-bit** field: `OutNodeAddress.SubNodeIndex = LayerMortonCode;` | `VND:211-214` | Truncation to 6 bits. `GetNodeAddressFromPosition` returns `true` with a corrupt address for every point in genuinely free space. **This is why §2 replaces that function's out-param with a result struct.** |

### 1.3 The cell abstraction — `ck::voxelnav::FCellId` [decision C-D4]

Nav3D has no such concept: the graph addresses `FNav3DNodeAddress` directly. C-D4 requires an abstract
cell id from day 1 so node merging drops in at Phase 5 without reworking search or neighbours.

```cpp
// Octree/CkVoxelNav_Octree_Types.h
namespace ck::voxelnav
{
    enum class ECellKind : uint8
    {
        OctreeNode,   // _Packed is an FNodeAddress
        Merged        // _Packed's low 28 bits index the volume's merged-cell table [PHASE 5]
    };

    // Volume-qualified cell identity. Copyable, equality-comparable, hashable — satisfies
    // ck::astar::AStarNodeId. 8 bytes. The volume qualifier is what makes cross-volume paths
    // (Phase 3) addressable without a second id type.
    struct CKVOXELNAV_API FCellId
    {
    public:
        static auto Make_FromNodeAddress(FVolumeId InVolume, FNodeAddress InAddress) -> FCellId;
        static auto Make_FromMergedIndex(FVolumeId InVolume, int32 InMergedIndex) -> FCellId;   // [PHASE 5]

        auto Get_Volume()   const -> FVolumeId;
        auto Get_Kind()     const -> ECellKind;   // MergedCellLayer nibble == Merged, else OctreeNode
        auto Get_IsValid()  const -> bool;

        // Valid only when Get_Kind() == OctreeNode. Ensures + returns an invalid address otherwise.
        auto Get_NodeAddress()  const -> FNodeAddress;
        auto Get_MergedIndex()  const -> int32;   // [PHASE 5]

        auto operator==(const FCellId&) const -> bool = default;

    private:
        uint32 _Volume = FVolumeId::InvalidValue;
        uint32 _Packed = FNodeAddress::InvalidPacked;
    };

    CKVOXELNAV_API auto GetTypeHash(const FCellId& InCell) -> uint32;
}
```

**Why the layer-nibble sentinel and not a wider id:** the layer nibble already has 15 as "invalid";
realistic volumes use layers 0..~8 (see the worked example in §3.6), so 14 is free forever. Reserving
it keeps `FCellId` at 8 bytes — it lands in `TSet`/`TMap` inside every A* search, so width is a real
cost. The alternative (widen to `uint64` with an explicit kind field) is listed in §6 as OQ-5.

**The rule that makes merging droppable:** search code must never call `Get_NodeAddress()`. Everything
search needs goes through kind-dispatching free functions on the octree:

```cpp
CKVOXELNAV_API auto Get_CellCenter (const FOctree& InOctree, const FCellId& InCell) -> FVector;
CKVOXELNAV_API auto Get_CellExtent (const FOctree& InOctree, const FCellId& InCell) -> float;
CKVOXELNAV_API auto Get_CellIsFree (const FOctree& InOctree, const FCellId& InCell) -> bool;
CKVOXELNAV_API auto Get_CellNeighbors(const FOctree& InOctree, const FCellId& InCell, TArray<FCellId>& OutNeighbors) -> void;
```

Phase 5 adds a `Merged` branch inside these four and nothing else changes.

`FVolumeId` is `struct CKVOXELNAV_API FVolumeId { uint32 _Value = InvalidValue; ... }` — a stable
integer, **never an actor pointer** (PROMPT.md decision 6; the upstream `TWeakObjectPtr` adjacency and
its nearest-neighbour repair heuristic are `research-coupling.md` §9's strongest argument).

### 1.4 `FNav3DLeafNode` → `ck::voxelnav::FLeafNode`

Upstream `Types.h:176-220`, VERIFIED.

| Nav3D field/method | CkVoxelNav | Change |
|---|---|---|
| `uint_fast64_t SubNodes = 0` | `uint64 _SubNodes = 0;` | pinned width |
| `FNav3DNodeAddress Parent` | `FNodeAddress _Parent;` | packed |
| `MarkSubNodeAsOccluded(Index)` | `auto Request_MarkSubNodeOccluded(SubNodeIndex InIndex) -> void` | — |
| `IsSubNodeOccluded(MortonCode)` | `auto Get_IsSubNodeOccluded(SubNodeIndex InIndex) const -> bool` | param retyped: upstream declares `MortonCode` but every caller passes a 0..63 index |
| `IsOccluded()` | `auto Get_IsAnySubNodeOccluded() const -> bool` | renamed — "IsOccluded" read as "fully" at three call sites |
| `IsCompletelyOccluded()` → `SubNodes == -1` | `auto Get_IsFullyOccluded() const -> bool { return _SubNodes == TNumericLimits<uint64>::Max(); }` | **FIX:** upstream compares an unsigned to `-1` (`Types.h:207`) — works by promotion, fragile |
| `IsCompletelyFree()` | `auto Get_IsFullyFree() const -> bool` | — |

Accessors are `CK_PROPERTY_GET(_SubNodes)` / `CK_PROPERTY_GET(_Parent)`; the rasterizer and the layer
container are `friend`s. No `CK_GENERATED_BODY` (not a reflected type) — but do give it
`CK_DEFINE_CONSTRUCTORS` only if a ctor is genuinely needed; default-construction suffices.

### 1.5 `FNav3DNode` / `FNav3DLayer` / `FNav3DLeafNodes` / `FNav3DData`

| Nav3D | CkVoxelNav | Notes |
|---|---|---|
| `FNav3DNode` (`Types.h:222-256`) | `ck::voxelnav::FNode` | `MortonCode _MortonCode; FNodeAddress _Parent, _FirstChild, _Neighbours[6];` — ~32 bytes now (4×`uint32`+8+6×4 = 8+4+4+24 = 40) vs upstream's ~104 (`research-coupling.md` §4). `operator<` on `_MortonCode` is kept — `Algo::BinarySearch` depends on it (`VND:1964`). |
| `FNav3DLayer` (`Types.h:530-583`) | `ck::voxelnav::FLayer` | `TArray<FNode> _Nodes; int32 _MaxNodeCount; float _NodeSize;` + `Get_Nodes/Get_NodeCount/Get_Node/Get_NodeSize/Get_NodeExtent/Get_MaxNodeCount/Get_AllocatedSize`. **`_MaxNodeCount` is the CUBE count (edge³)** — see the §2 fix for `FindNeighbourInDirection`, which compares a per-axis coordinate against it. Add `auto Get_EdgeNodeCount() const -> int32;` and use THAT for coordinate bounds. |
| `FNav3DLeafNodes` (`Types.h:454-528`) | `ck::voxelnav::FLeafNodes` | `TArray<FLeafNode> _LeafNodes; float _LeafNodeSize;` + `Get_LeafNode/Get_LeafNodes/Get_LeafNodeSize/Get_LeafNodeExtent/Get_LeafSubNodeSize/Get_LeafSubNodeExtent`. Sizes: leaf extent = size/2, subnode size = size/4, subnode extent = size/8 (`Types.h:496-514`, VERIFIED). |
| `FNav3DData` (`Types.h:585-709`) | `ck::voxelnav::FOctree` | `TArray<FLayer> _Layers; FLeafNodes _LeafNodes; FBox _NavigationBounds; FBox _VolumeBounds; bool _IsValid;` — **`BlockedNodes` moves OUT** (see below). |

**`BlockedNodes` is build scratch and must not live on the finished octree.** Upstream keeps
`TArray<TArray<NodeIndex>> BlockedNodes` on `FNav3DData` and deliberately excludes it from
serialization (`Types.h:637`, `Types.h:701-709`, VERIFIED). In the port it lives on
`ck::FFragment_VoxelNavVolume_BuildState` (§5) and is dropped when the build completes. This matters
for C-D7 (immutable post-bake octree, lock-free parallel reads).

**Naming trap worth stating once, because the code is confusing:** upstream's "layer 0" holds the
**leaf nodes**, and `BlockedNodes[0]` holds **layer-1 Morton codes** (the parents of leaves). Verified
at `VND:1546-1563` (`FirstPass` iterates layer 1's node count and calls `AddBlockedNode(0, NodeIndex)`)
and `VND:1817-1823` (`RasterizeInitialLayer` treats each `BlockedNodes[0]` entry as a
`ParentMortonCode` and expands it to 8 children). Keep the indexing identical; add a `/** contract */`
block on `FOctree::Get_LayerBlockedNodes` stating it.

### 1.6 `FNav3DDataGenerationSettings` → split in two

Upstream (`Types.h:40-78`, VERIFIED) is a `USTRUCT` mixing bake knobs with UE collision plumbing, and
its `FCollisionQueryParams CollisionQueryParameters` member is **not a UPROPERTY** — the tuning knob
authors think they have does not persist (`research-coupling.md` §10.10).

| Nav3D field | Goes to | As |
|---|---|---|
| `TEnumAsByte<ECollisionChannel> CollisionChannel` (default `ECC_WorldStatic`) | `FCk_Fragment_VoxelNavVolume_ParamsData` | `TEnumAsByte<ECollisionChannel> _QueryChannel = ECC_WorldStatic;` — feeds the backend's filter. See OQ-3. |
| `float Clearance` (default 0) | `FCk_Fragment_VoxelNavVolume_ParamsData` | `float _ClearanceUu = 0.0f;` — folded into every half-extent before the backend call, exactly as upstream (`VND:1617`, `VND:1698`), so the backend never sees it |
| `float AdjacencyClearance` (default 500) | — | **[PHASE 3]** do not port |
| `FCollisionQueryParams` | — | deleted; the Jolt backend has no analogue |
| `int32 MaxSimultaneousBoxGenerationJobsCount` | — | deleted; no worker tasks in Phase 1 (`PROMPT.md` decision 5: budgeted game-thread) |
| *(from `FNav3DVolumeNavigationDataSettings`, `Nav3DVolumeNavigationData.h:19-31`)* `float VoxelExtent` | `FCk_Fragment_VoxelNavVolume_ParamsData` | **RENAME to `_FinestCellSizeUu`** — see OQ-9; PHASE_0's skeleton field is called `_VoxelExtent` and the name is a lie |
| `UWorld* World` | — | deleted; the backend owns world access |
| `TAtomic<bool>* CancelFlag` | — | deleted (never assigned upstream — `research-coupling.md` §3) |
| `FString DebugLabel`, `int32 DebugVolumeIndex` | — | deleted; CkLabel + the entity handle carry identity |

`static TAtomic<bool> FNav3DVolumeNavigationData::bSCancelRequested` (`VND:38`, VERIFIED) is a
**process-wide** cancel flag polled ~15 times through the rasterizer. **Do not port it.** Cancellation
is per-entity: remove `FTag_VoxelNavVolume_BuildInProgress` and reset the build-state fragment (§5).

### 1.7 Types deliberately NOT ported

`FVoxelOverlapCache` (`Types.h:81-106`) — dies with the L1 actor cache (§3.5).
`FNav3DVolumeNavigationDataSettings` — dissolved into Params + the backend.
Everything from `Types.h:258` onward that is tactical/debug/region (`FNav3DRegion`, `FCompactRegion`,
`FVolumeRegionMatrix`, `FConsolidatedTacticalData`, `FNav3DTacticalSettings`, `FNav3DVolumeDebugData`,
`FNav3DPerformanceStats`, `FNav3DMetadata`, `FRegionIdList`, `FNav3DVoxelConnection`,
`FNav3DActorPortal`, `FCompactPortal`, `FNav3DChunkAdjacency`) — none of it is Phase 1, and most of it
is deferred-pool or dead (`research-coupling.md` §6, PROMPT.md non-goals).

---

## 2. FUNCTION-LEVEL PORT MAP — `Nav3DVolumeNavigationData.cpp` (2510 lines)

Target files, all under `Public/CkVoxelNav/Octree/` (house layout: no `Private/`, .cpp lives beside .h):

| File | Contents |
|---|---|
| `CkVoxelNav_Octree_Types.h/.cpp` | §1 types, `FOctree::Initialize`, allocators |
| `CkVoxelNav_Octree_Morton.h/.cpp` | libmorton wrappers (from `Nav3DUtils.cpp`) |
| `CkVoxelNav_Octree_Query.h/.cpp` | position↔address, extents, neighbour enumeration, free-node walk |
| `CkVoxelNav_Octree_Rasterize.h/.cpp` | the build stages (backend-driven, resumable) |
| `CkVoxelNav_Octree_Raycast.h/.cpp` | **[PHASE 2]** — do not create in Phase 1 |

`Backend/CkVoxelNav_GeometryBackend.h` and `Backend/CkVoxelNav_GeometryBackend_Jolt.{h,cpp}` hold §2.1.

### 2.1 The backend seam — interface + final signatures

**Naming, per house precedent (VERIFIED):** the only pure-C++, non-UObject interface in the runtime tree
is `ICk_PathNetwork_VectorizationSegmentEvaluator`
(`Source/CkPathNetwork/Public/CkPathNetwork/Network/CkPathNetwork_Vectorize.h:29-38`) — global
namespace, `ICk_<Module>_<Thing>`, `CK<MODULE>_API`, `virtual ~X() = default`, pure-virtual methods with
trailing returns and `In*` params. Two other runtime examples confirm the shape
(`ICk_LoadingProcess`, `ICk_OverlapBody_Interface`). **So: `ICk_VoxelNav_GeometryBackend`, NOT
`ck::voxelnav::IGeometryBackend` and NOT `FCk_VoxelNav_GeometryBackend`.**

```cpp
// Backend/CkVoxelNav_GeometryBackend.h
// NOTE: NO Jolt include, NO JPH forward declaration — the exit criterion
// `rg --no-ignore -l "Jolt/" Source/CkVoxelNav` must stay at zero.

// Opaque geometry-body identity. Never dereferenced by the octree; only compared and passed back.
struct CKVOXELNAV_API FCk_VoxelNav_BodyId
{
    uint64 _Value = 0;
    auto Get_IsValid() const -> bool { return _Value != 0; }
    auto operator==(const FCk_VoxelNav_BodyId&) const -> bool = default;
};

/** The ONLY world-geometry surface the voxelizer sees. One shipped implementation
 *  (CkJolt-backed). Every call is game-thread-only in Phase 1; the implementation documents
 *  the exact window it is safe in. */
class CKVOXELNAV_API ICk_VoxelNav_GeometryBackend
{
public:
    virtual ~ICk_VoxelNav_GeometryBackend() = default;

    /** Any geometry inside the axis-aligned box? Narrowphase, any-hit, early-out.
     *  Replaces IsPositionOccludedPhysics (VND:1689-1776) and the slow path of
     *  IsPositionOccluded (VND:1038-1174). InHalfExtents is ALREADY clearance-inflated. */
    virtual auto
    Get_IsBoxOccupied(
        const FVector& InCenter,
        const FVector& InHalfExtents) const -> bool = 0;

    /** Broadphase AABB sweep. Replaces GatherOverlappingObjects (VND:781-851).
     *  Used once per build for the whole-volume early-out; NOT used per voxel (see §3.5). */
    virtual auto
    Get_BodiesInBox(
        const FBox& InWorldBounds,
        TArray<FCk_VoxelNav_BodyId>& OutBodies) const -> void = 0;

    /** Cheap AABB reject. Replaces Actor->GetComponentsBoundingBox / Prim->Bounds.GetBox(). */
    virtual auto
    Get_BodyBounds(
        FCk_VoxelNav_BodyId InBody) const -> FBox = 0;

    /** Segment blocked? [PHASE 2] refinement (visibility pruning) — declared now so the
     *  interface does not churn, implemented in Phase 1, first CONSUMED in Phase 2. */
    virtual auto
    Get_IsSegmentBlocked(
        const FVector& InFrom,
        const FVector& InTo) const -> bool = 0;
};
```

**Deliberately absent: `IsBoxOccupiedBy(Body, ...)`.** `research-coupling.md`'s proposed interface has
it, for the L1-cache fast path. §3.5 recommends dropping that cache; without the cache the restricted
form has no caller, and Jolt has no cheap public body-restricted `CollideShape` anyway. Adding an
unused pure virtual is speculative abstraction (root `CLAUDE.md` § Simplicity).

**Concrete implementation:**

```cpp
// Backend/CkVoxelNav_GeometryBackend_Jolt.h
struct CKVOXELNAV_API FCk_VoxelNav_GeometryBackend_Jolt final : public ICk_VoxelNav_GeometryBackend
{
public:
    // Resolves and pins the Jolt query session ONCE. Invalid session == every query returns
    // "unoccupied"; the caller (the build processor) must check Get_IsValid() and fail the
    // build loudly rather than baking an empty world.
    explicit FCk_VoxelNav_GeometryBackend_Jolt(
        const UObject* InWorldContextObject,
        const FCk_VoxelNav_BackendParams& InParams);

    auto Get_IsValid() const -> bool;

    auto Get_IsBoxOccupied(const FVector& InCenter, const FVector& InHalfExtents) const -> bool override;
    auto Get_BodiesInBox(const FBox& InWorldBounds, TArray<FCk_VoxelNav_BodyId>& OutBodies) const -> void override;
    auto Get_BodyBounds(FCk_VoxelNav_BodyId InBody) const -> FBox override;
    auto Get_IsSegmentBlocked(const FVector& InFrom, const FVector& InTo) const -> bool override;

private:
    ck::jolt::FCk_Jolt_QuerySession _Session;              // opaque, JPH-free — see OQ-1
    TArray<ck::jolt::FCk_Jolt_BoxProbe> _ProbesByExtent;   // one per distinct half-extent — see OQ-1
    FCk_VoxelNav_BackendParams _Params;
};

// Backend/CkVoxelNav_GeometryBackend.h — plain struct, not reflected
struct CKVOXELNAV_API FCk_VoxelNav_BackendParams
{
    TEnumAsByte<ECollisionChannel> _QueryChannel = ECC_WorldStatic;
    float _ClearanceUu = 0.0f;
};
```

> **OQ-1 blocks this.** As PHASE_0 unit 0B is written, `ck::jolt::Get_IsBoxOccupied` takes
> `const JPH::PhysicsSystem&` and an optional `JPH::Ref<JPH::Shape>` — **neither type is nameable from
> CkVoxelNav** under the no-JPH-includes fence. The signatures above assume CkJolt grows two opaque
> pimpl value types (`FCk_Jolt_QuerySession`, `FCk_Jolt_BoxProbe`). **Do not start §2.1 until OQ-1 is
> ruled.**

**Per-call requirements on the Jolt side** (from `research-jolt.md` §4, VERIFIED against
`CkJoltQuery_Utils.cpp`):
- `JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector>` — never `AllHitCollisionCollector`
  (`CkJoltQuery_Utils.cpp:223` collects every hit; the voxelizer wants first-hit early-out).
- **No `Fill_HitAttribution`** (`CkJoltQuery_Utils.cpp:76-85`) — it resolves an ECS entity per hit,
  which is pure overhead here and game-thread-bound.
- Reuse one box shape per distinct half-extent. There are exactly **three** in the whole pipeline:
  L1 extent, leaf extent, leaf-subnode extent (§3). Build them once in the ctor.
- Filters: `FCk_Jolt_StaticOccupancyFilter` (Domain == Static) **plus** a `JPH::BroadPhaseLayerFilter`
  accepting only `broadphase_layers::Static` — the existing wrappers pass accept-all
  (`CkJoltQuery_Utils.cpp:129,179,227`), leaving broadphase perf on the table at voxel-grid query
  counts. Both are PHASE_0 unit 0B deliverables.
- Boxes and spheres need **no axis correction** (`research-jolt.md` §5 — only capsules/cylinders/
  heightfields do). Units are UE centimetres, pure passthrough `Conv`.
- `mPenetrationSlop = 2.0f` cm is the sim's tolerance floor (`research-jolt.md` §5) — a build whose
  finest cell approaches 2 cm is below the physics' own resolution. Ensure on
  `_FinestCellSizeUu < 4.0f` at `Add`/`Request_Build` time.

### 2.2 Surviving functions — the table

**Legend.** `PORT` = mechanical translation. `PORT+FIX` = translation plus a named behavior fix.
`SEAM` = replaced by a backend call. `DROP` = do not port. `[PHASE N]` = later phase, listed so you
don't port it now.

#### Position ↔ address (→ `CkVoxelNav_Octree_Query.h/.cpp`)

| Nav3D | Lines | Disposition | Target signature |
|---|---|---|---|
| `GetNodePositionFromAddress` | `VND:40-123` | PORT+FIX | `auto Get_NodePositionFromAddress(const FOctree& InOctree, const FNodeAddress& InAddress, ECk_VoxelNav_SubNodePrecision InPrecision) -> FVector` |
| `GetNodePositionFromLayerAndMortonCode` | `VND:125-143` | PORT | `auto Get_NodePositionFromLayerAndMorton(const FOctree& InOctree, LayerIndex InLayerIndex, MortonCode InMortonCode) -> FVector` |
| `GetLeafNodePositionFromMortonCode` | `VND:145-161` | PORT | `auto Get_LeafNodePositionFromMorton(const FOctree& InOctree, MortonCode InMortonCode) -> FVector` |
| `GetNodeAddressFromPosition` | `VND:163-303` | PORT+FIX | `auto TryGet_NodeAddressFromPosition(const FOctree& InOctree, const FVector& InPosition, LayerIndex InMinLayerIndex) -> FCk_VoxelNav_AddressLookupResult` |
| `FindNearestNavigableNode` | `VND:305-375` | PORT+FIX | `auto TryGet_NearestFreeNodeAddress(const FOctree& InOctree, const FVector& InPosition, LayerIndex InMinLayerIndex) -> FCk_VoxelNav_AddressLookupResult` |
| `GetNodeExtentFromNodeAddress` | `VND:524-560` | PORT | `auto Get_NodeExtentFromAddress(const FOctree& InOctree, const FNodeAddress& InAddress) -> float` |
| `GetLayerNodeSize` / `GetLayerNodeExtent` | `VND:2449-2481` | PORT | `auto Get_LayerNodeSize(const FOctree&, LayerIndex InLayerIndex) -> float` (+ `..Extent`) |
| `GetLayerRatio` / `GetLayerInverseRatio` | `VND:512-522` | PORT | `auto Get_LayerRatio(const FOctree&, LayerIndex InLayerIndex) -> float` (+ `..Inverse`) |
| `GetNodeIndexFromMortonCode` | `VND:1959-1965` | PORT | `auto Get_NodeIndexFromMorton(const FOctree&, LayerIndex InLayerIndex, MortonCode InMortonCode) -> NodeIndex` (`Algo::BinarySearch`; returns `INDEX_NONE`) |
| `GetNodeAddressFromMortonCode` | `VND:2483-2506` | PORT | `auto TryGet_NodeAddressFromMorton(const FOctree&, MortonCode InMortonCode, LayerIndex InLayerIndex) -> FNodeAddress` (invalid address on miss) |
| `GetNodeFromAddress` | `Nav3DVolumeNavigationData.h:169-207` | PORT+FIX | `auto TryGet_NodeFromAddress(const FOctree&, const FNodeAddress&) -> const FNode*` |
| `GetMinLayerIndexForAgentSize` | `VND:1177-1201` | PORT | `auto Get_MinLayerIndexForAgentRadius(const FOctree&, float InAgentRadiusUu) -> LayerIndex` |
| `IsNodeInBounds` | `VND:2387-2391` | PORT | `auto Get_IsNodeInBounds(const FVector& InNodePosition, float InNodeExtent, const FBox& InBounds) -> bool` |
| `GetParentMortonCodeAtLayer` | `VND:2433-2447` | PORT | `auto Get_ParentMortonAtLayer(MortonCode InChildCode, LayerIndex InTargetLayer, LayerIndex InChildLayer) -> MortonCode` |
| `GetLayerBlockedNodes` | `VND:2508-2510` | MOVE | onto the build-state fragment (§1.5) |

**Fixes, in detail:**

- **`Get_NodePositionFromAddress`** — the `bool TryGetSubNodePosition` param becomes
  `enum class ECk_VoxelNav_SubNodePrecision : uint8 { NodeCenter, SubNodeCenter };` (root `CLAUDE.md`
  § Naming: enums over bool options). Upstream's ten `UE_LOG(Verbose)`-and-return-`ZeroVector` guards
  (`VND:47-103`) become `CK_ENSURE_IF_NOT` + a separate ordinary early-out returning
  `FVector::ZeroVector`. **Do not silently log-and-continue** (root non-negotiable #3).
- **`TryGet_NodeAddressFromPosition`** — upstream returns `bool` + an out-address, and encodes
  "free space at this layer" by writing `NodeIndex = INDEX_NONE` and stuffing the layer Morton code
  into the 6-bit `SubNodeIndex` (`VND:211-214`, VERIFIED corruption). Replace the out-param with:
  ```cpp
  enum class ECk_VoxelNav_AddressLookup : uint8 { Found, FreeSpaceAtLayer, OutOfBounds, NoLayers };
  struct CKVOXELNAV_API FCk_VoxelNav_AddressLookupResult
  {
      ECk_VoxelNav_AddressLookup _Outcome = ECk_VoxelNav_AddressLookup::OutOfBounds;
      FNodeAddress _Address;      // valid iff Found
      LayerIndex   _Layer = 0;    // meaningful for FreeSpaceAtLayer
      MortonCode   _Morton = 0;   // FULL-WIDTH; meaningful for FreeSpaceAtLayer
  };
  ```
  Keep the `ExpandBy(1.0f)` FP-tolerance bounds test (`VND:173`) and the coarse→fine layer walk.
- **`TryGet_NearestFreeNodeAddress`** — upstream is a full O(layers × nodes × 64) linear scan
  (`VND:317-363`), one of two known O(n) query bugs (`research-coupling.md` §1). **Port the semantics
  as-is in Phase 1** (correctness first) but add a `/** contract */` block flagging the complexity and
  a `SCOPE_CYCLE_COUNTER`. Fixing it needs a spatial acceleration structure — that is a Phase 5 item,
  and inventing one here is out of scope. Log it as a follow-up, do not fix silently.
- **`TryGet_NodeFromAddress`** — upstream returns a `static const FNav3DNode InvalidNode&` on every
  failure path (`Nav3DVolumeNavigationData.h:172`), which callers cannot distinguish from a real node.
  Return `const FNode*` (nullptr on miss). Note upstream's dead branch at
  `Nav3DVolumeNavigationData.h:187-206` (the layer-0 path falls through to the identical final return).

#### Neighbour enumeration (→ `CkVoxelNav_Octree_Query.h/.cpp`)

| Nav3D | Lines | Disposition | Target signature |
|---|---|---|---|
| `GetNodeNeighbours` | `VND:377-510` | PORT+FIX | `auto Get_NodeNeighbours(const FOctree& InOctree, const FNodeAddress& InAddress, TArray<FNodeAddress>& OutNeighbours) -> void` |
| `GetLeafNeighbours` | `VND:2079-2171` | PORT | `auto Get_LeafNeighbours(const FOctree& InOctree, const FNodeAddress& InLeafAddress, TArray<FNodeAddress>& OutNeighbours) -> void` |
| `GetFreeNodesFromNodeAddress` | `VND:2173-2227` | PORT+FIX | `auto Get_FreeNodesUnderAddress(const FOctree& InOctree, const FNodeAddress& InAddress, TArray<FNodeAddress>& OutFreeNodes) -> void` |
| `GetRandomPoint` | `VND:562-583` | PORT+FIX | `auto TryGet_RandomFreePoint(const FOctree& InOctree, FRandomStream& InOutStream) -> TOptional<FVector>` |

- **`Get_NodeNeighbours` fixes:** keep the two constant tables verbatim —
  `ChildOffsetsDirections[6][4]` (`VND:439-442`) and `LeafChildOffsetsDirections[6][16]`
  (`VND:478-485`) — they encode Morton child ordering per face and are the heart of the SVO neighbour
  walk. Reindent the mangled `while` body (`VND:410-508` has broken indentation and a stray brace
  structure). Add `OutNeighbours.Reserve(24)` before the loop (measured fanout: 6 faces, up to 16
  leaf subnodes each = 96 worst case; typical ≤ 24).
- **`TryGet_RandomFreePoint` fix (VERIFIED bug):** `VND:565` constructs
  `FNav3DNodeAddress TopMostNodeAddress(GetLayerCount(), 0, 0)` — `GetLayerCount()` is `Layers.Num()`,
  so the layer index is **one past the last valid layer**, and `Get_FreeNodesUnderAddress` then indexes
  `_Layers[LayerCount]`. Use `GetLayerCount() - 1`. Also replace the global `FMath::RandRange` with an
  injected `FRandomStream&` (determinism; house tests need repeatable results) and return
  `TOptional<FVector>` rather than `TOptional<FNavLocation>` (no `NavigationSystem` dependency).

#### Build pipeline (→ `CkVoxelNav_Octree_Rasterize.h/.cpp`)

Every one of these becomes a **stage function taking an explicit cursor** so the state machine in §3
can resume mid-stage. Signatures below are final.

| Nav3D | Lines | Disposition | Target |
|---|---|---|---|
| `FNav3DData::Initialize` | `Types.cpp:69-111` | PORT+FIX | `auto Request_InitializeOctree(FOctree& InOutOctree, float InFinestCellSizeUu, const FBox& InVolumeBounds) -> bool` |
| `GenerateNavigationData` | `VND:585-660` | **REPLACED** | dissolved into the §3 state machine; the function itself does not survive |
| `FirstPass` | `VND:1526-1591` | SPLIT | `Stage_RasterizeL1` (§3.3 stage C) + `Stage_PropagateBlockedUpward` (§3.3 stage D) |
| `CacheLayer1Overlaps` | `VND:1593-1687` | **SEAM/DROP** | folded into `Stage_RasterizeL1` as one `Get_IsBoxOccupied` per L1 cell — see §3.5 |
| `RasterizeInitialLayer` | `VND:1804-1881` | PORT+SEAM | `Stage_RasterizeLeafLayer` |
| `RasterizeLeaf` | `VND:1784-1802` | PORT+SEAM | `Rasterize_OneLeaf` (helper, 64 probes) |
| `RasterizeLayer` | `VND:1883-1957` | PORT+FIX | `Stage_RasterizeLayer` |
| `BuildParentLinkForLeafNodes` | `VND:2229-2242` | PORT+FIX | `Stage_BuildParentLinks` |
| `BuildNeighbourLinks` | `VND:1967-2009` | PORT | `Stage_BuildNeighbourLinks` |
| `FindNeighbourInDirection` | `VND:2011-2077` | PORT+FIX | `TryFind_NeighbourInDirection` |
| `Reset` | `VND:772-779` | PORT | `auto Request_ResetOctree(FOctree& InOutOctree) -> void` |
| `LogNavigationStats` | `VND:1506-1524` | REPLACE | write counters into `FFragment_VoxelNavVolume_BuildStats`; one summary `voxelnav::Display` on completion |
| `UpdateCoreProgress` | `VND:689-730` | REPLACE | `_Progress01` on the build-state fragment; `Get_BuildProgress` reads it |
| `GetLogPrefix` / `FormatElapsedTime` | `VND:662-739` | DROP | `ck::voxelnav::*` log functions + the entity handle carry identity |

```cpp
// Octree/CkVoxelNav_Octree_Rasterize.h  — namespace ck::voxelnav
// Every stage is a pure function over (octree, scratch, backend, budget). No member state,
// no hidden globals, no cancel flag. Each returns how many probes it actually spent so the
// caller can keep its budget honest.

struct CKVOXELNAV_API FRasterizeScratch
{
    TArray<TArray<NodeIndex>> _BlockedNodes;              // was FNav3DData::BlockedNodes
    TMap<LeafIndex, MortonCode> _LeafIndexToParentMorton; // was the local map at VND:636
    int32 _Cursor = 0;                                    // within-stage resume point
    int32 _LayerCursor = 0;                               // for the per-layer stages
};

struct CKVOXELNAV_API FRasterizeStageResult
{
    bool  _StageComplete = false;
    int32 _ProbesSpent   = 0;
};

CKVOXELNAV_API auto
Stage_RasterizeL1(
    FOctree& InOutOctree,
    FRasterizeScratch& InOutScratch,
    const ICk_VoxelNav_GeometryBackend& InBackend,
    float InClearanceUu,
    int32 InProbeBudget) -> FRasterizeStageResult;

CKVOXELNAV_API auto
Stage_PropagateBlockedUpward(
    const FOctree& InOctree,
    FRasterizeScratch& InOutScratch) -> void;              // O(blocked), unbudgeted

CKVOXELNAV_API auto
Stage_RasterizeLeafLayer(
    FOctree& InOutOctree,
    FRasterizeScratch& InOutScratch,
    const ICk_VoxelNav_GeometryBackend& InBackend,
    float InClearanceUu,
    int32 InProbeBudget) -> FRasterizeStageResult;

CKVOXELNAV_API auto
Stage_RasterizeLayer(
    FOctree& InOutOctree,
    FRasterizeScratch& InOutScratch,
    LayerIndex InLayerIndex) -> FRasterizeStageResult;     // no probes; budgeted by node count

CKVOXELNAV_API auto
Stage_BuildParentLinks(
    FOctree& InOutOctree,
    const FRasterizeScratch& InScratch) -> void;

CKVOXELNAV_API auto
Stage_BuildNeighbourLinks(
    FOctree& InOutOctree,
    FRasterizeScratch& InOutScratch,
    LayerIndex InLayerIndex) -> FRasterizeStageResult;     // no probes; budgeted by node count
```

**Fixes, in detail:**

- **`Request_InitializeOctree`** (`Types.cpp:69-111`): keep the math verbatim —
  `LeafSize = FinestCellSize * 4`, `VoxelExponent = ceil(log2(VolumeMaxDim / LeafSize))`,
  `LayerCount = VoxelExponent + 1`, `NavigationBoundsSize = 2^VoxelExponent * LeafSize`,
  `NavigationBounds = BuildAABB(VolumeBounds.GetCenter(), FVector(NavBoundsSize * 0.5f))`,
  layer L: `EdgeNodeCount = 2^(exp - L)`, `MaxNodeCount = Edge³`, `NodeSize = NavBoundsSize / Edge`.
  Fixes: (a) `LayerCount < 2` returns false — keep, but `CK_ENSURE_IF_NOT` first with the bounds and
  cell size in the message, since it means "your volume is smaller than one leaf"; (b) drop the
  `UE_LOG(Warning)` at `Types.cpp:93` (a Warning on every successful init fails AutoTests — see the
  standing `feedback_autotest_warning_escalation` note); (c) `BlockedNodes.SetNumZeroed(LayerCount + 1)`
  keeps the +1 (`Types.cpp:108`) — `Stage_PropagateBlockedUpward` writes one past the top layer.
- **`Stage_RasterizeL1`** — the merged `CacheLayer1Overlaps` + `FirstPass` step-2 loop. Iterates
  `Layer1.Get_MaxNodeCount()` cells; per cell computes
  `Position = Get_NodePositionFromLayerAndMorton(InOctree, 1, Cursor)` and calls
  `InBackend.Get_IsBoxOccupied(Position, FVector(Layer1Extent + InClearanceUu))`; on true,
  `Scratch._BlockedNodes[0].Add(Cursor)`. Note the naming trap from §1.5: layer-1 codes go into
  `BlockedNodes[0]`. One probe per cell → budget is exactly `InProbeBudget` cells.
- **`Stage_PropagateBlockedUpward`** — `VND:1577-1586`: for L = 1..LayerCount-1, for each code in
  `BlockedNodes[L-1]`, `BlockedNodes[L].Add(Get_ParentMorton(code))`. Pure and cheap; run whole.
  **FIX:** upstream uses `TArray::Add` and produces duplicates (a parent with 8 blocked children is
  added 8 times), which then makes `RasterizeLayer`'s `LayerBlockedNodes.Contains(...)` an O(n) scan
  over a list up to 8× larger than necessary. Use `TSet<MortonCode>` for the upper layers (or
  `AddUnique` + `Sort` and binary-search). This is a straight win with no behavior change.
- **`Stage_RasterizeLeafLayer`** (`VND:1804-1881`) — for each `BlockedNodes[0]` parent code, expand to
  8 children; for each child, one `Get_IsBoxOccupied(LeafPos, FVector(LeafExtent + Clearance))`; if
  occupied, mark `_FirstChild` layer 0 and call `Rasterize_OneLeaf` (64 further probes); if free, add
  an empty leaf node. Then sort layer-0 nodes by Morton and fix up `_FirstChild.NodeIndex`.
  **Resume granularity:** the cursor must be a `(ParentIndex, ChildIndex, SubNodeIndex)` triple — a
  single leaf costs 65 probes, so a per-leaf-only cursor cannot honour a small budget. **FIX:**
  upstream builds `TempNodes` then sorts then walks (`VND:1848-1878`), which cannot be resumed
  mid-walk because `RasterizeLeaf` runs inside the post-sort walk. Restructure: (1) budgeted pass
  fills `TempNodes` with occupancy flags only; (2) sort (one shot, unbudgeted); (3) budgeted pass runs
  `Rasterize_OneLeaf` per occupied node. Two cursors, two sub-stages. **Do not try to resume inside
  upstream's fused loop.**
- **`Rasterize_OneLeaf`** (`VND:1784-1802`) — 64 probes at `LeafSubNodeExtent + Clearance`, positions
  `NodePosition - LeafNodeExtent + MortonCoords * LeafSubNodeSize + LeafSubNodeExtent`. Keep verbatim.
- **`Stage_RasterizeLayer`** (`VND:1883-1957`) — **FIX:** upstream scans `0..LayerMaxNodeCount` (the
  full cube, e.g. 32 768 iterations for a 32³ layer) doing a `Contains` per iteration. Iterate the
  blocked-code SET instead: for each blocked parent code at this layer, that IS the node to create.
  Same output, O(blocked) not O(cube). Keep the child↔parent linking (`VND:1913-1932`) and the final
  Morton sort verbatim. Delete the `UpdateCoreProgress` arithmetic at `VND:1943-1956` (it computes two
  different progress values and calls the setter three times).
- **`Stage_BuildParentLinks`** (`VND:2229-2242`) — replace `check(NodeIndex != INDEX_NONE)` with
  `CK_ENSURE_IF_NOT` + a separate ordinary skip that leaves the leaf's parent invalid and records a
  build warning on the stats fragment.
- **`TryFind_NeighbourInDirection`** (`VND:2011-2077`) — **VERIFIED BUG:** `VND:2017` reads
  `const auto MaxCoordinates = Nav3DData.GetLayer(LayerIndex).GetMaxNodeCount();` and then compares
  per-axis coordinates against it (`VND:2025-2027`). `MaxNodeCount` is the **cube** count (edge³,
  `Types.cpp:100-101`), so the bounds check is ~edge² too permissive: out-of-range coordinates are not
  rejected, encode to Morton codes outside the layer, fail the search, and silently degrade to the
  walk-to-parent path. Compare against `Get_EdgeNodeCount()` (§1.5). Also keep the leaf-fully-occluded
  invalidation at `VND:2050-2057` — it is load-bearing (it prevents paths through solid leaves).
- **`Stage_BuildNeighbourLinks`** (`VND:1967-2009`) — the driving loop runs
  `for (LayerIdx = LayerCount-2; LayerIdx != (LayerIndex)-1; --LayerIdx)` (`VND:649`), i.e. downward
  from LayerCount-2 to 0 using uint8 wraparound. Use an `int32` cursor and `>= 0`. Replace
  `check(NodeIndexFromMorton != INDEX_NONE)` (`VND:2003`) with `CK_ENSURE_IF_NOT` + break out of the
  climb.

#### The ~600 backend-seam lines — exact replacements

| Nav3D | Lines | Replacement |
|---|---|---|
| `GatherOverlappingObjects` | `VND:781-851` | `InBackend.Get_BodiesInBox(VolumeBounds, OutBodies)` — used **once**, only for the whole-volume early-out (`VND:610-617`). The entire `RemoveAllSwap` filter chain (`VND:793-844`) evaporates: CkJolt's static world is already filtered at bake time (`research-jolt.md` §6 — unregistered / editor-only / NoCollision / simulating / Movable components are never baked). |
| `IsCollisionOnlyComponent` | `VND:854-882` | DELETE |
| `HasValidCollisionGeometry` ×2 | `VND:885-950` | DELETE |
| `IsPositionOccluded` | `VND:952-1175` | `InBackend.Get_IsBoxOccupied(InCenter, FVector(InExtent + InClearanceUu))`. Both the L1-cache fast path (`VND:963-1036`) and the dynamic-occluder + `OverlappingObjects` slow path (`VND:1038-1174`) collapse into that one call. |
| `CheckStaticMeshTrianglesWithTransform` | `VND:1335-1385` | DELETE — Jolt's narrowphase is the truth now |
| `CheckStaticMeshOcclusion` | `VND:1387-1402` | DELETE |
| `CheckInstancedStaticMeshOcclusion` | `VND:1404-1442` | DELETE — CkJolt bakes per-instance bodies / one `StaticCompoundShape` above `_CompoundShapeInstanceThreshold` (`research-jolt.md` §6), so the O(instances × triangles) loop is gone |
| `CheckLandscapeProxyOcclusion` | `VND:1444-1504` | DELETE — see OQ-4 (landscape heightfield extraction is `WITH_EDITOR`-only in CkJolt) |
| `CacheLayer1Overlaps` | `VND:1593-1687` | folded into `Stage_RasterizeL1` — §3.5 |
| `IsPositionOccludedPhysics` | `VND:1689-1776` | same single `Get_IsBoxOccupied` call |
| `ClearOverlapCache` | `VND:1778-1782` | DELETE with the cache |
| `TriBoxOverlap.{h,cpp}` (92+68 lines) | — | **DO NOT PORT.** It exists only to serve the deleted triangle narrowphase. |

**Behavior change to state in `Source/CkVoxelNav/Claude.md` and in the Phase 1 exit report:** upstream
filters candidates on `BodySetup->AggGeom` (simple collision) but decides occupancy on **LOD0 render
triangles** (`research-coupling.md` §2). Jolt unifies both onto the baked collision shape, so **baked
occupancy will differ from Nav3D's** — usually more correct, always different. This is expected, not a
regression.

#### Deliberately NOT ported in Phase 1

| Nav3D | Lines | Why |
|---|---|---|
| `Serialize` | `VND:741-770` | Cooked/serialized bake is deferred pool (PROMPT.md non-goals). The packed-`uint32` node-ref rule (§1.2) still governs whoever writes it later. |
| `RebuildLeafNodesInBounds` | `VND:1203-1333` | [PHASE 3] dynamic occluders |
| `PropagateChangesToHigherLayers` | `VND:2244-2385` | [PHASE 3] |
| `AddDynamicOccluder` / `RemoveDynamicOccluder` | `VND:2393-2431` | [PHASE 3]; and audit upstream issue #39 first [C-D9] |
| `RequestCancelBuildAll` / `ClearCancelBuildAll` / `IsCancelRequested` | `Nav3DVolumeNavigationData.h:209-211` | process-wide static — replaced by per-entity tag removal (§5) |

---

## 3. VOXELIZATION AS A BUDGETED RESUMABLE PROCESSOR

### 3.1 Where it runs — the scheduling window

**Group: `ck::FGroup_Transform`. `RunAfter = TDepList<ck::FProcessor_JoltWorld_WaitForAsync>`,
`RunBefore = TDepList<ck::FProcessor_JoltWorld_Step>`.**

Rationale (VERIFIED against `CkJolt/Claude.md` § Processor order and
`CkJolt/Public/CkJolt/World/CkJoltWorld_Processor.h:24-88`):

- CkJolt's hard rule is the `RunAfter FProcessor_JoltWorld_WaitForAsync` edge — the scheduler's Kahn
  tie-break is lexical by name, so without it a processor can run while the previous frame's async step
  is still in flight.
- The *only* provably-safe window in async mode is **between** `WaitForAsync` (which consumes the prior
  step) and `Step` (which kicks the next). A processor in `FGroup_PostTransform` — where CkEqs lives
  (`CkEqs_Processor.h:34,83,122,166,190`, VERIFIED) — sits **after** `Step` has already dispatched the
  async batch, so with `jolt.EnableAsyncPhysicsUpdate=1` it queries Jolt concurrently with the task-graph
  step. That is a pre-existing exposure in CkEqs; do not copy it into a processor that fires thousands
  of queries per frame.
- Cross-group `RunAfter`/`RunBefore` edges are legal: `FProcessorGraphBuilder::DoAddExplicitEdges`
  (`CkEcs/Public/CkEcs/Scheduler/CkProcessorGraph.cpp:324-388`, VERIFIED) resolves dependency names
  against the whole graph and also understands group start/end nodes. Placing our processor inside
  `FGroup_Transform` avoids relying on that anyway.

> **OQ-2 confirms or overrides this placement.** It is the one scheduling decision this map cannot
> close alone (it touches CkJolt's contract, and `FGroup_Transform` is a crowded group).

**No pump.** The processor's view requires `FTag_VoxelNavVolume_BuildInProgress` and declares **no
`MarkedDirtyBy`**, so the scheduler runs it exactly once per main tick and never pumps it. That is
deliberate: the whole point of the budget is one bounded slice per frame, and pump passes would
multiply it. (If a `MarkedDirtyBy` is later wanted for scheduler-edge reasons, it MUST carry
`static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;` — sticky marker + non-idempotent
work is exactly the condition `CkEcs/Claude.md` names.) The empty-view skip then makes the processor
free when nothing is building.

`ck::RunPacedSteps` (`CkEcs/Public/CkEcs/Pacing/CkPacedWork.h:50-81`, VERIFIED) is the house
budgeted-work primitive, but it is built on the **pump** mechanism and refills its budget on
`DeltaT > 0`. Using it would put probe batches in pump passes whose position relative to
`FProcessor_JoltWorld_Step` is not pinned. **Recommendation: do not use `RunPacedSteps` here**; a plain
per-tick budget loop inside `ForEachEntity` is both simpler and provably inside the safe window.
Mention this reasoning in `Source/CkVoxelNav/Claude.md` so the next reader doesn't "fix" it.

### 3.2 The processor

```cpp
// Volume/CkVoxelNavVolume_Processor.h
namespace ck
{
    class CKVOXELNAV_API FProcessor_VoxelNavVolume_Build : public ck_exp::TProcessor<
        FProcessor_VoxelNavVolume_Build,
        FCk_Handle_VoxelNavVolume,
        ck::TReadOnly<FFragment_VoxelNavVolume_Params>,
        ck::TReadWrite<FFragment_VoxelNavVolume_BuildState>,
        ck::TReadWrite<FFragment_VoxelNavVolume_Octree>,
        FTag_VoxelNavVolume_BuildInProgress,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group     = FGroup_Transform;
        using RunAfter  = TDepList<FProcessor_JoltWorld_WaitForAsync>;
        using RunBefore = TDepList<FProcessor_JoltWorld_Step>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            FFragment_VoxelNavVolume_Octree& InOctree) const -> void;
    };
}
// CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavVolume_Build);   // top of the .cpp
```

Sibling processors: `FProcessor_VoxelNavVolume_Setup` (consumes `FTag_VoxelNavVolume_NeedsSetup`,
validates params, arms `NeedsBuild` when `_AutoBuild == Enabled`),
`FProcessor_VoxelNavVolume_HandleRequests` (drains `FFragment_VoxelNavVolume_Requests`),
`FProcessor_VoxelNavVolume_CancelPendingRequests` (`FGroup_EndPlay`, `CK_IF_END_PLAY`,
`ck::request::FireCancelledForPending`).

### 3.3 The stage machine

```cpp
// Volume/CkVoxelNavVolume_Fragment_Data.h  (reflected — the debugger and BP read the stage)
UENUM(BlueprintType)
enum class ECk_VoxelNavVolume_BuildStage : uint8
{
    NotStarted,
    InitializeOctree,      // Request_InitializeOctree            — unbudgeted, one tick
    ProbeVolumeEmpty,      // Get_BodiesInBox on the whole volume — 1 probe, early-out
    RasterizeL1,           // Stage_RasterizeL1                   — BUDGETED (1 probe/cell)
    PropagateBlocked,      // Stage_PropagateBlockedUpward        — unbudgeted, O(blocked)
    AllocateLeaves,        // reserve BlockedNodes[0].Num() * 8   — unbudgeted
    ProbeLeafOccupancy,    // Stage_RasterizeLeafLayer, sub-stage 1 — BUDGETED (1 probe/leaf)
    SortLeafLayer,         //                           sub-stage 2 — unbudgeted, one sort
    RasterizeSubNodes,     //                           sub-stage 3 — BUDGETED (64 probes/leaf)
    RasterizeLayers,       // Stage_RasterizeLayer, L = 1..N-1     — BUDGETED (by node count)
    BuildParentLinks,      // Stage_BuildParentLinks               — unbudgeted
    BuildNeighbourLinks,   // Stage_BuildNeighbourLinks, L = N-2..0 — BUDGETED (by node count)
    Complete,
    Failed
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VoxelNavVolume_BuildStage);
```

Mapping back to Nav3D's `GenerateNavigationData` (`VND:585-660`, VERIFIED):

| Nav3D step | Line | Stage |
|---|---|---|
| `Nav3DData.Initialize(VoxelExtent, VolumeBounds)` | `VND:599` | `InitializeOctree` |
| `GatherOverlappingObjects()` | `VND:605` | `ProbeVolumeEmpty` |
| early-out on zero overlaps | `VND:610-617` | `ProbeVolumeEmpty` → `Complete` |
| `FirstPass()` | `VND:624` | `RasterizeL1` + `PropagateBlocked` |
| `AllocateLeafNodes(Blocked[0].Num() * 8)` | `VND:630-632` | `AllocateLeaves` |
| `RasterizeInitialLayer(map)` | `VND:637` | `ProbeLeafOccupancy` + `SortLeafLayer` + `RasterizeSubNodes` |
| `for L=1..N: RasterizeLayer(L)` | `VND:640-644` | `RasterizeLayers` |
| `BuildParentLinkForLeafNodes(map)` | `VND:646` | `BuildParentLinks` |
| `for L=N-2..0: BuildNeighbourLinks(L)` | `VND:649-653` | `BuildNeighbourLinks` |
| `bIsValid = true` | `VND:655` | `Complete` (publish + bump epoch) |

**Resumable checkpoints = every stage boundary**, plus a cursor inside the four BUDGETED stages. The
unbudgeted stages are bounded by blocked-node count, not by the volume's cube, so they cannot spike on
a sparse world — the one that could (`SortLeafLayer`) is a single `TArray::Sort` over ≤ 8× the blocked
L1 count.

**Invariant to enforce with an ensure:** the octree fragment must not be readable while
`_Stage != Complete`. Publish at the `Complete` transition (§5), keep the in-progress octree in the
build-state fragment. `Get_IsBuilt` returns false for anything else.

### 3.4 The budget knob

```cpp
// Settings/CkVoxelNav_ProjectSettings.h — UCk_VoxelNav_ProjectSettings_UE, section "VoxelNav"
int32 _MaxOccupancyProbesPerTick = 2048;   // PRIMARY budget — deterministic, testable
int32 _MaxLinkOpsPerTick        = 8192;    // for the probe-free stages (RasterizeLayers, NeighbourLinks)
float _MaxBuildMillisecondsPerTick = 4.0f; // SECONDARY guard only; 0 = off
```

**Probe count is the primary budget, wall-clock is only a guard.** Reasons: (a) a probe count is
deterministic, so an autotest can assert "this bake takes exactly N ticks" and catch a regression in
pruning; (b) Nav3D's 50 ms/tick wall-clock budget with a hard break every 10 iterations
(`research-coupling.md` §8) is a visible hitch, not a budget; (c) a time-only budget makes the number
of probes machine-dependent, which makes test counts flaky.

Per-volume override on Params (enum-mode + value, per house optionality rules):
`ECk_EnableDisable _BuildBudgetOverride` + `int32 _MaxOccupancyProbesPerTickOverride`.

The processor consumes both budgets in one `ForEachEntity` call, advancing through as many stages as
the remaining budget allows (an unbudgeted stage may run to completion even at zero remaining probe
budget — it spends none).

### 3.5 The L1-overlap-cache decision — **DROP THE CACHE**

**Recommendation: drop the per-L1-voxel body list. Keep the hierarchical empty-skip.**

What upstream does (VERIFIED): `CacheLayer1Overlaps` (`VND:1593-1687`) runs one
`OverlapMultiByChannel` per layer-1 voxel and stores a `TArray<TWeakObjectPtr<AActor>>` per voxel.
`IsPositionOccluded` (`VND:963-1036`) then looks up the parent L1 voxel's list and either early-outs
(empty list → not occluded) or walks **only those actors**, doing `GetComponentsBoundingBox` +
`Cast<ALandscapeProxy>` + `GetComponents<UInstancedStaticMeshComponent>` +
`GetComponents<UStaticMeshComponent>` + per-triangle SAT per actor.

Why the cache exists upstream: the *narrowphase* is a LOD0 triangle walk. Without the actor list, every
sub-voxel test would re-walk every candidate actor's triangles. The cache is a fix for an expensive
narrowphase, not for an expensive broadphase.

Why it is redundant against Jolt:

1. **The narrowphase it was hiding is gone.** `Get_IsBoxOccupied` is one
   `NarrowPhaseQuery::CollideShape` with an any-hit collector and a static-only broadphase filter —
   Jolt descends its own AABB quadtree and stops at the first hit. There is nothing to amortise.
2. **A body-restricted Jolt query is not cheap or simple.** Jolt exposes no public
   "collide shape against this body list" call; you would pass a custom `JPH::BodyFilter` that rejects
   ids outside a per-cell `TSet`, which still pays the full broadphase descent and adds a set lookup per
   candidate. More code, more allocation, probably slower.
3. **The pruning survives without it.** The *value* of the cache in the logs is the
   "% reduction" number (`VND:1517-1519`) — the fraction of L1 voxels with zero overlaps, whose 8
   leaves × 64 subnodes are never probed. That pruning comes from the **hierarchy**, not the actor
   list: one `Get_IsBoxOccupied` at L1 extent returning false skips up to **512 sub-probes**. We keep
   that exactly — it is `Stage_RasterizeL1` writing `BlockedNodes[0]`, and `Stage_RasterizeLeafLayer`
   only ever visiting children of blocked L1 nodes (`VND:1817`).
4. **Memory and churn.** One `TArray<JPH::BodyID>` per L1 voxel is 4 096–32 768 arrays for realistic
   volumes (§3.6), allocated and freed per build, held across frames in a resumable build — for a
   structure whose only consumer would be a query Jolt does better itself.
5. **CkVoxelNav cannot hold `JPH::BodyID` anyway** without breaking the no-JPH fence; it would have to
   be the opaque `FCk_VoxelNav_BodyId`, adding a translation layer to a cache we do not need.

**What we keep instead (both cheap, both already in the interface):**

- **Volume-level early-out** — one `Get_BodiesInBox(VolumeBounds, Out)`; if empty, the volume is fully
  free and the build completes with zero rasterization (upstream `VND:610-617`, VERIFIED — "empty
  volumes are essentially free"). This is the only surviving use of `Get_BodiesInBox` in Phase 1.
- **Hierarchical empty-skip** — the L1 → leaf → subnode probe cascade described above.

**Fallback, if profiling later says otherwise (Phase 5, not now):** re-introduce a per-L1
`TArray<FCk_VoxelNav_BodyId>` from `Get_BodiesInBox` plus a `Get_IsBoxOccupiedBy` on the interface.
Record it as a perf lever in `Claude.md`; do not build it speculatively. **Measure before claiming**
(root non-negotiable #7) — the Phase 1 exit report should carry the actual probe counts, not an estimate.

### 3.6 Worked budget example (INFERRED arithmetic from VERIFIED formulas)

Interior gym, volume max dimension 6 400 uu, `_FinestCellSizeUu = 50`:

- `LeafSize = 50 × 4 = 200`; `VoxelExponent = ceil(log2(6400 / 200)) = ceil(log2(32)) = 5`;
  `LayerCount = 6`; `NavBoundsSize = 2⁵ × 200 = 6 400`.
- Layer 0 (leaves): edge 32, node size 200; each leaf holds 4³ = 64 sub-cells of 50 uu.
- Layer 1: edge 16, node size 400 → **4 096 L1 cells → 4 096 probes in `RasterizeL1`.**
- Say 600 L1 cells are blocked → `ProbeLeafOccupancy` = 600 × 8 = **4 800 probes**.
- Say 2 000 of those leaves are occupied → `RasterizeSubNodes` = 2 000 × 64 = **128 000 probes**.
- Total ≈ **137 000 probes**. At `_MaxOccupancyProbesPerTick = 2048`, ≈ **67 ticks** ≈ 1.1 s at 60 Hz.

The subnode stage dominates by ~27×, which is the number to watch: it is why the leaf-level gate must
stay, and why the Phase 5 perf work targets merging (C-D4) rather than the broadphase.

---

## 4. ASTAR ADAPTER SKETCH — Phase 2, designed now

Built in Phase 2. Listed here so Phase 1's types support it without rework. **Do not create these
files in Phase 1** — but do run the `static_assert` mentally against `FCellId` before finalising §1.3.

### 4.1 NodeId

`ck::voxelnav::FCellId` (§1.3) already satisfies `ck::astar::AStarNodeId`
(`CkAStar/Algorithm/CkAStar_GraphConcept.h:15-18`, VERIFIED: `std::copyable` +
`std::equality_comparable`). It additionally needs a free `GetTypeHash` — the concept does not require
it, but `TSearchState` puts the id in `TSet`/`TMap` (`CkAStar_Search.h:124-126`), exactly as
`pathnetwork::GetTypeHash(const FRouteNodeId&)` does (`CkPathNetwork_RouteGraph.h:35-36`, VERIFIED).

`GetTypeHash(FCellId) = HashCombineFast(GetTypeHash(_Volume), GetTypeHash(_Packed))`.

### 4.2 The graph view

`TSearchState` stores `T_Graph _Graph{}` **BY VALUE** (`CkAStar_Search.h:119`, VERIFIED), so the graph
must be a cheap-copy view — raw pointer + shared per-query data, per the `FRouteGraph` pattern
(`CkPathNetwork_RouteGraph.h:183-259`) and its documented lifetime rule
(`CkPathNetwork_RouteGraph.h:12-14`: *"raw pointer to the built network, valid only for one synchronous
search inside a processor tick. Never store it across frames."*).

```cpp
// Path/CkVoxelNav_OctreeGraph.h   — [PHASE 2]
namespace ck::voxelnav
{
    // Per-query, network-independent seed. Immutable, shareable across concurrent searches.
    struct CKVOXELNAV_API FOctreeGraphSharedData
    {
        FVector    _GoalLocation = FVector::ZeroVector;
        FCellId    _GoalCell;
        LayerIndex _MinLayerIndex = 0;     // Get_MinLayerIndexForAgentRadius(octree, agentRadius)
        float      _HeuristicScale = 1.0f; // > 1 = weighted A* (INADMISSIBLE); document it
        float      _CoarseCellBias = 1.0f; // > 1 penalises coarse cells; 1.0 = pure Euclidean
    };

    struct CKVOXELNAV_API FOctreeGraph
    {
    public:
        FOctreeGraph() = default;

        FOctreeGraph(
            const FOctree* InOctree,
            FVolumeId InVolume,
            TSharedPtr<const FOctreeGraphSharedData> InShared);

        auto Neighbors(const FCellId& InCell)                        const -> TArray<FCellId>;
        auto Cost     (const FCellId& InFrom, const FCellId& InTo)   const -> float;
        auto Heuristic(const FCellId& InCurrent, const FCellId& InGoal) const -> float;
        auto IsGoal   (const FCellId& InCell)                        const -> bool;

        auto Get_CellLocation(const FCellId& InCell) const -> FVector;

    private:
        // LIFETIME: valid only for one synchronous search inside a processor tick.
        // Never store across frames. The owning entity's octree must not rebuild during the search.
        const FOctree* _Octree = nullptr;
        FVolumeId _Volume;
        TSharedPtr<const FOctreeGraphSharedData> _Shared;
    };

    static_assert(
        astar::AStarGraph<FOctreeGraph, FCellId>,
        "FOctreeGraph must satisfy the AStarGraph concept");
}
```

**Stronger option worth considering at Phase 2 (see OQ-6):** hold
`TSharedPtr<const FOctree>` instead of `const FOctree*`. C-D7 wants an immutable post-bake octree for
lock-free parallel reads; a shared-pointer graph view then survives a concurrent rebuild by construction
and the "never store across frames" caveat weakens to "you're pinning memory". Cost: one atomic
refcount bump per graph copy — and `TSearchState` copies the graph once per search, not per expansion.

### 4.3 `Neighbors()` — enumeration strategy

Wrap the ported `Get_CellNeighbors` (§1.3):

1. **Kind dispatch.** `OctreeNode` → the SVO walk below. `Merged` → the merged-cell adjacency list
   [PHASE 5]. Search never sees the difference.
2. **Leaf sub-node cells** (`Layer == 0` with valid `_FirstChild`) → `Get_LeafNeighbours`
   (`VND:2079-2171`): the 6 face directions; a neighbour inside the same leaf is a direct sub-node
   index test against the 64-bit occupancy mask; a neighbour outside wraps the coordinate to the
   opposite face (`VND:2135-2158`) and tests the adjacent leaf's mask.
3. **Interior/coarse cells** → `Get_NodeNeighbours` (`VND:377-510`): follow the baked `_Neighbours[6]`;
   if the neighbour has children, descend the face-facing children only —
   `ChildOffsetsDirections[6][4]` for interior layers, `LeafChildOffsetsDirections[6][16]` at the leaf
   boundary. This is the Brewer/Payne parent↔child transition and it is what makes the SVO graph
   correct across layer changes.
4. **Agent-size filter.** Reject any candidate whose `Get_NodeExtentFromAddress` is smaller than the
   agent needs, using `_MinLayerIndex` from `Get_MinLayerIndexForAgentRadius` (`VND:1177-1201`). This
   is the entirety of Nav3D's agent coupling — one float (`research-coupling.md` §1).
5. **Allocation.** `Neighbors` returns `TArray<FCellId>` by value (concept-mandated —
   `CkAStar_GraphConcept.h:27`). `Reserve(24)`. This allocation-per-expansion is the known cost of the
   concept (`research-navStack.md`); it is acceptable and must not be "optimised" by changing CkAStar
   (PROMPT.md decision 3: CkAStar stays untouched).

### 4.4 Cost and Heuristic

- **`Cost(A, B) = FVector::Dist(Get_CellLocation(A), Get_CellLocation(B)) × Bias`**, where
  `Bias = 1.0` by default. Centre-to-centre Euclidean is the parity choice — Nav3D's shipping default
  is its "Distance" traversal-cost calculator (`research-coupling.md` FILES-THAT-PORT list). Its
  "Fixed" alternative is not worth porting.
- **`Heuristic(Current, Goal) = FVector::Dist(centre(Current), _Shared->_GoalLocation) × _HeuristicScale`.**
  With `_HeuristicScale == 1.0` this is admissible and consistent for the Euclidean cost, so A* is
  optimal. Nav3D's `FNav3DQueryFilterSettings::HeuristicScale` is the **only** surviving datum from its
  entire query-filter stub (`research-coupling.md` §1) — carry it as a float with a `/** contract */`
  block stating that any value > 1 trades optimality for speed.
- **`IsGoal(Cell) = (Cell == _Shared->_GoalCell)`.** Cell equality, not position proximity — the search
  is over cells, and the caller resolves the goal position to a cell before constructing the graph
  (`TryGet_NodeAddressFromPosition`, falling back to `TryGet_NearestFreeNodeAddress`).
- **`_CoarseCellBias`** exists so Phase 5 can make coarse (merged) cells attractive without touching
  the search. Default 1.0 = no bias; leave it unused in Phase 2.

**Warm start:** `TSearchState`'s plan-repair ctor (`CkAStar_Search.h:39-45`) and
`ValidateExistingPath` (`CkAStar_Search.h:15-20`) are the natural "is my cached 3D path still valid
after a rebuild?" primitives. `ValidateExistingPath` returns the index of the first broken step —
combine it with the volume epoch (§5) so a path is only re-validated when the epoch moved.

---

## 5. VOLUME ENTITY DESIGN — what Phase 1 ADDS to the Phase 0 skeleton

Phase 0 unit 0C ships: `FCk_Handle_VoxelNavVolume`, `FCk_Fragment_VoxelNavVolume_ParamsData`
(`_VolumeBounds`, `_VoxelExtent`), `FTag_VoxelNavVolume_NeedsBuild`, a no-op Setup processor, and
`UCk_Utils_VoxelNavVolume_UE` with `Add`/`Has`/`CastChecked` following CkPathNetwork's
Add-creates-child-entity ritual.

### 5.1 Params — extend `FCk_Fragment_VoxelNavVolume_ParamsData`

```cpp
// Volume/CkVoxelNavVolume_Fragment_Data.h
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FBox _VolumeBounds = FBox{ForceInit};

    // The FINEST navigable cell's EDGE LENGTH in uu (not a half-extent). LeafSize = this * 4.
    // Nav3D calls this "VoxelExtent" and derives it as AgentRadius * 2 — the name is wrong there
    // and is corrected here. See OQ-9.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "4.0"))
    float _FinestCellSizeUu = 50.0f;

    // Added to every probe half-extent. Grows obstacles; shrinks free space.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _ClearanceUu = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TEnumAsByte<ECollisionChannel> _QueryChannel = ECC_WorldStatic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AutoBuildOnSetup = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _BuildBudgetOverride = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _MaxOccupancyProbesPerTickOverride = 2048;

public:
    CK_PROPERTY_GET(_VolumeBounds);
    CK_PROPERTY_GET(_FinestCellSizeUu);
    CK_PROPERTY(_ClearanceUu);
    CK_PROPERTY(_QueryChannel);
    CK_PROPERTY(_AutoBuildOnSetup);
    CK_PROPERTY(_BuildBudgetOverride);
    CK_PROPERTY(_MaxOccupancyProbesPerTickOverride);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VoxelNavVolume_ParamsData, _VolumeBounds, _FinestCellSizeUu);
```

Essentials in the ctor, optionals via fluent setters — root `CLAUDE.md` § Requests / canonical
fragment-data shape (exemplar `CkTimer_Fragment_Data.h:113-152`, VERIFIED).

### 5.2 Fragments — `Volume/CkVoxelNavVolume_Fragment.h`

```cpp
namespace ck
{
    using FFragment_VoxelNavVolume_Params = FCk_Fragment_VoxelNavVolume_ParamsData;

    // The published, immutable-after-build octree. _Epoch bumps on every completed (re)build;
    // paths planned against an older epoch replan. (CkPathNetwork's FFragment_PathNetwork_Graph
    // is the precedent — CkPathNetwork_Fragment.h:21-44.)
    struct CKVOXELNAV_API FFragment_VoxelNavVolume_Octree
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_Octree);

        friend class FProcessor_VoxelNavVolume_Build;
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    private:
        TSharedPtr<const voxelnav::FOctree> _Octree;   // null until the first build completes
        voxelnav::FVolumeId _VolumeId;
        int32 _Epoch = 0;

    public:
        CK_PROPERTY_GET(_Octree);
        CK_PROPERTY_GET(_VolumeId);
        CK_PROPERTY_GET(_Epoch);
    };

    // Everything the resumable build needs, and nothing the finished octree carries.
    struct CKVOXELNAV_API FFragment_VoxelNavVolume_BuildState
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_BuildState);

        friend class FProcessor_VoxelNavVolume_Build;
        friend class FProcessor_VoxelNavVolume_HandleRequests;
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    private:
        ECk_VoxelNavVolume_BuildStage _Stage = ECk_VoxelNavVolume_BuildStage::NotStarted;
        voxelnav::FOctree _WorkingOctree;                 // moved into the Octree fragment at Complete
        voxelnav::FRasterizeScratch _Scratch;
        TUniquePtr<ICk_VoxelNav_GeometryBackend> _Backend; // created at InitializeOctree, freed at Complete
        float _Progress01 = 0.0f;
        FCk_VoxelNav_BuildStats _Stats;
        FCk_Time _ElapsedBuildTime;

    public:
        CK_PROPERTY_GET(_Stage);
        CK_PROPERTY_GET(_Progress01);
        CK_PROPERTY_GET(_Stats);
    };

    struct CKVOXELNAV_API FFragment_VoxelNavVolume_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VoxelNavVolume_Requests);

        friend class FProcessor_VoxelNavVolume_HandleRequests;
        friend class ::UCk_Utils_VoxelNavVolume_UE;

    public:
        using RequestType = std::variant<FCk_Request_VoxelNavVolume_Build,
                                         FCk_Request_VoxelNavVolume_CancelBuild>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_NeedsBuild);        // from Phase 0
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_BuildInProgress);
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_Built);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKVOXELNAV_API,
        VoxelNavVolume_OnBuildComplete,
        FCk_Delegate_VoxelNavVolume_OnBuildComplete,
        FCk_Handle_VoxelNavVolume,
        FCk_VoxelNav_BuildStats);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_VoxelNavVolume_Requests);
}
```

`_Backend` is a `TUniquePtr` on a fragment, not a UObject — no GC concern (root `CLAUDE.md` § UObject
refs in fragments applies only to UObjects). It is created at the `InitializeOctree` transition and
**reset at `Complete`/`Failed`/cancel** so the pinned Jolt session does not outlive the build.

### 5.3 Tag flow

```
Add()  ──► child entity + Params + FTag_VoxelNavVolume_NeedsSetup
             │
             ▼  FProcessor_VoxelNavVolume_Setup  (consumes NeedsSetup)
        validate params ──► ensure+fail, or:
        _AutoBuildOnSetup == Enable ──► add FTag_VoxelNavVolume_NeedsBuild
             │
             ▼  FProcessor_VoxelNavVolume_HandleRequests  (also armed by Request_Build)
        NeedsBuild present ──► create backend, reset BuildState, Stage = InitializeOctree,
                               remove NeedsBuild, add FTag_VoxelNavVolume_BuildInProgress
             │
             ▼  FProcessor_VoxelNavVolume_Build          (one budgeted slice per tick)
        ... stages ...
             │
             ├─ Complete ──► MoveTemp working octree into a TSharedPtr<const FOctree>,
             │               ++_Epoch, remove BuildInProgress, add FTag_VoxelNavVolume_Built,
             │               reset _Backend + _Scratch, broadcast OnBuildComplete,
             │               fire the request's completion delegate with Succeeded
             │
             └─ Failed  ──► remove BuildInProgress, leave Built as-is (a failed REbuild keeps the
                            previous octree published), broadcast with a failure reason,
                            fire the completion delegate with Failed
```

**A rebuild does not unpublish the old octree.** `FTag_VoxelNavVolume_Built` and the `_Octree`
shared pointer stay live while a rebuild runs in the build-state fragment, and the swap is atomic at
`Complete`. Consumers therefore never see a half-built graph, and in-flight searches holding a
`TSharedPtr<const FOctree>` copy keep a valid structure. This is the C-D7 (immutable post-bake)
requirement made concrete.

### 5.4 Requests + Utils surface

```cpp
// Volume/CkVoxelNavVolume_Fragment_Data.h
USTRUCT(BlueprintType)
struct CKVOXELNAV_API FCk_Request_VoxelNavVolume_Build : public FCk_Request_Base
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_VoxelNavVolume_Build);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoxelNavVolume_Build);

private:
    // Restart from scratch even if a build is already in progress.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _ForceRestart = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY(_ForceRestart);
};

USTRUCT(BlueprintType)
struct CKVOXELNAV_API FCk_Request_VoxelNavVolume_CancelBuild : public FCk_Request_Base
{ /* no payload; CK_GENERATED_BODY + CK_REQUEST_DEFINE_DEBUG_NAME */ };
```

```cpp
// Volume/CkVoxelNavVolume_Utils.h  — UCk_Utils_VoxelNavVolume_UE : UCk_Utils_Ecs_Base_UE
//   UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VoxelNavVolume"))
//   CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VoxelNavVolume)

UFUNCTION(BlueprintCallable, Category = "Ck|Utils|VoxelNavVolume",
          DisplayName = "[Ck][VoxelNavVolume] Request Build",
          meta = (AutoCreateRefTerm = "InDelegate"))
static FCk_Handle_VoxelNavVolume
Request_Build(
    UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
    const FCk_Request_VoxelNavVolume_Build& InRequest,
    const FCk_Delegate_Request_OnCompleted& InDelegate);   // ALWAYS LAST, no C++ default

UFUNCTION(...) static FCk_Handle_VoxelNavVolume
Request_CancelBuild(
    UPARAM(ref) FCk_Handle_VoxelNavVolume& InVolume,
    const FCk_Request_VoxelNavVolume_CancelBuild& InRequest,
    const FCk_Delegate_Request_OnCompleted& InDelegate);

UFUNCTION(BlueprintPure, ...) static bool    Get_IsBuilt        (const FCk_Handle_VoxelNavVolume& InVolume);
UFUNCTION(BlueprintPure, ...) static int32   Get_BuildEpoch     (const FCk_Handle_VoxelNavVolume& InVolume);
UFUNCTION(BlueprintPure, ...) static float   Get_BuildProgress  (const FCk_Handle_VoxelNavVolume& InVolume);
UFUNCTION(BlueprintPure, ...) static ECk_VoxelNavVolume_BuildStage Get_BuildStage(const FCk_Handle_VoxelNavVolume& InVolume);
UFUNCTION(BlueprintPure, ...) static FCk_VoxelNav_BuildStats Get_BuildStats(const FCk_Handle_VoxelNavVolume& InVolume);
UFUNCTION(BlueprintPure, ...) static int32   Get_NumLayers      (const FCk_Handle_VoxelNavVolume& InVolume);
UFUNCTION(BlueprintPure, ...) static bool    Get_IsPointFree    (const FCk_Handle_VoxelNavVolume& InVolume, FVector InLocation);
UFUNCTION(...) static void BindTo_OnBuildComplete   (UPARAM(ref) FCk_Handle_VoxelNavVolume&, ECk_Signal_BindingPolicy, ECk_Signal_PostFireBehavior, const FCk_Delegate_VoxelNavVolume_OnBuildComplete&);
UFUNCTION(...) static void UnbindFrom_OnBuildComplete(UPARAM(ref) FCk_Handle_VoxelNavVolume&, const FCk_Delegate_VoxelNavVolume_OnBuildComplete&);
```

**Request-completion contract (root `CLAUDE.md` § Request completion, reference `CkTimer`):**
`Request_Build` builds a named local, does
`if (InDelegate.IsBound()) { Request.Set_CompletionDelegate(InDelegate); }`, then enqueues that same
local. A synchronous rejection (invalid handle, degenerate bounds, cell size below the physics slop)
fires `InDelegate.ExecuteIfBound(InVolume, ECk_Request_OperationResult::Failed_NotEnqueued)` from a
**separate ordinary `if`**, never from inside a `CK_ENSURE_IF_NOT` body. The build is long-running, so
the completion guard does **not** live in `HandleRequests` — the request struct (delegate included) is
copied onto the build-state fragment and completed at the `Complete`/`Failed` transition. A
`Request_Build` on an already-building volume with `_ForceRestart == Disable` completes the *new*
request with `Succeeded` (idempotent no-op semantics, root `CLAUDE.md` § Result semantics) and leaves
the running build alone.

`Get_IsPointFree` is the Phase 1 acceptance surface: the autotest bakes a known box in a gym, then
asserts free outside and occupied inside — the "occupancy spot-checks match Jolt geometry" exit
observation from PROMPT.md's phase map.

### 5.5 Not in Phase 1

Replication (a baked octree is deterministic from level geometry — clients rebuild locally), save/load
persistence handlers, chunking/partitioning, the debugger inspector, `Serialize`. None of these appear
in the fragments above; do not add hooks "for later".

---

## 6. OPEN QUESTIONS — each with a recommendation. STOP and report if you hit one.

**OQ-1 — CkJolt's occupancy API is not callable from CkVoxelNav as PHASE_0 specifies it.**
`PHASE_0.md` unit 0B item 1 defines `ck::jolt::Get_IsBoxOccupied(const JPH::PhysicsSystem&, ...)` plus
"an overload accepting a caller-held `JPH::Ref<JPH::Shape>`". CkVoxelNav cannot name `JPH::PhysicsSystem`
or `JPH::Ref` — the Phase 0 exit criterion is literally
`rg --no-ignore -l "Jolt/" Source/CkVoxelNav` → zero. As written, the only way for CkVoxelNav to query
is `UCk_Utils_JoltQuery_UE::Get_Overlap`, which resolves the subsystem per call, rebuilds the box shape
per call, collects *all* hits, and resolves an ECS entity per hit — every one of the four gaps
`research-jolt.md` §4 names.
**Recommendation:** CkJolt additionally exposes two opaque, JPH-free value types in a public header —
`ck::jolt::FCk_Jolt_QuerySession` (constructed from a `UObject* WorldContext`; pins the
`TSharedPtr<JPH::PhysicsSystem>` behind a `TPimplPtr`) and `ck::jolt::FCk_Jolt_BoxProbe`
(`Make_BoxProbe(FVector InHalfExtents)`; holds the `JPH::Ref<JPH::Shape>` behind a `TPimplPtr`) — plus
`ck::jolt::Get_IsBoxOccupied(const FCk_Jolt_QuerySession&, const FCk_Jolt_BoxProbe&, const FVector& InCenter, ...)`
and `ck::jolt::Get_BodiesInAABox(const FCk_Jolt_QuerySession&, const FBox&, ..., TArray<uint32>& OutBodyIds)`.
Keeps the JPH allowlist closed, keeps the capability in CkJolt [C-D5], and costs ~40 lines. **This is a
CkJolt change and therefore a PHASE_0 amendment, not a Phase 1 file.**

**OQ-2 — Build-processor group placement.** §3.1 recommends `FGroup_Transform` with
`RunAfter FProcessor_JoltWorld_WaitForAsync` + `RunBefore FProcessor_JoltWorld_Step`, because that is
the only window provably outside the async step. The alternative is `FGroup_PostTransform` (where CkEqs
queries Jolt today — `CkEqs_Processor.h:34`, VERIFIED), which is simpler and consistent with existing
practice but sits *after* `Step` has dispatched the async batch.
**Recommendation:** `FGroup_Transform` between WaitForAsync and Step. Cheap, provably safe, and it does
not depend on `jolt.EnableAsyncPhysicsUpdate` being off. Secondary note for the maintainer: CkEqs'
current placement looks like a latent async-mode exposure — worth a separate follow-up, **not ours to
fix**.

**OQ-3 — Query filter composition.** PHASE_0 unit 0B ships `FCk_Jolt_StaticOccupancyFilter`
(Domain == Static) only. `_QueryChannel` on Params implies a channel-response filter too. Also note
(`research-jolt.md` §7, VERIFIED) that Domain == Static includes **Static-motion-type JoltBody
entities**, not only baked level geometry.
**Recommendation:** ship a composed `FCk_Jolt_NavQueryFilter` in **CkJolt** doing
`Domain == Static && Get_ResponseOfLayerToChannel(layer, InChannel) >= Block` (~15 lines, mirrors the
existing filter pattern exactly). Treat "Static-motion JoltBodies block navigation" as **desired** —
they are non-moving world obstacles — and say so in `Claude.md`.

**OQ-4 — Landscape has no Jolt representation in packaged builds.** CkJolt extracts landscape
heightfields under `WITH_EDITOR` only (`research-jolt.md` §6, VERIFIED against
`CkJoltBakeExtraction.h:124-127`). A volume over landscape would voxelize as free space in a packaged
build. Nav3D treats landscape as a first-class occluder (`VND:1444-1504`).
**Recommendation:** out of scope for Phase 1 — Rewind99 is an interior sim and the gyms are interiors.
Record it as a **fence** in `Source/CkVoxelNav/Claude.md` ("CkVoxelNav does not see Landscape in
packaged builds") and add a Phase 1 build-time sanity check: if
`UCk_JoltStaticWorld_Subsystem_UE::Get_NumStaticBodies()` is 0 for a non-empty volume, `CK_ENSURE` with
the volume bounds. Fixing it means runtime heightfield extraction in CkJolt — a separate effort.

**OQ-5 — `FCellId` merge encoding.** §1.3 reserves layer-nibble value 14 as the `Merged` discriminator,
keeping `FCellId` at 8 bytes. Alternative: widen `_Packed` to `uint64` with an explicit kind byte —
more headroom, no sentinel cleverness, 12 bytes per id in every A* container.
**Recommendation:** the nibble-14 sentinel. Layer 15 is already a sentinel upstream, realistic volumes
use ≤ 9 layers (§3.6), and the search containers' width is a real cost. Whichever is chosen, the
kind-dispatching free functions in §1.3 are what actually make merging droppable — the encoding is
secondary.

**OQ-6 — `TSharedPtr<const FOctree>` vs raw pointer in the graph view.** §5.2 stores the published
octree as `TSharedPtr<const FOctree>`; §4.2 sketches `FOctreeGraph` holding a raw `const FOctree*` per
the `FRouteGraph` precedent (`CkPathNetwork_RouteGraph.h:12-14`, VERIFIED). Holding the shared pointer
in the graph instead would make C-D7's lock-free-parallel-reads property structural rather than
by-convention, at one atomic bump per graph copy.
**Recommendation:** publish as `TSharedPtr<const FOctree>` in Phase 1 (that part is settled — it is what
makes the atomic rebuild swap in §5.3 safe), and defer the graph-view question to Phase 2 when the
search processor exists. Flagging it now only so Phase 1 does not store the octree by value.

**OQ-7 — Leaf-node allocation shape.** Upstream `AllocateLeafNodes` is `Reserve`-only
(`Types.cpp:33-36`) and `AddLeafNode` grows one at a time under
`if (LeafIndex > LeafNodes.Num() - 1) { AddEmptyLeafNode(); }` (`Types.cpp:38-51`) — correct only if
callers pass strictly increasing indices, which `RasterizeInitialLayer` happens to do.
**Recommendation:** not a design fork, just port it as `SetNum(LeafCount)` + direct indexed writes.
Listed here so the executor does not faithfully reproduce the fragile growth pattern.

**OQ-8 — `TryGet_NearestFreeNodeAddress` is O(layers × nodes × 64).** `VND:317-363`, one of the two
known O(n) query bugs (`research-coupling.md` §1). It is on the hot path of every path request whose
endpoint lands in a blocked cell.
**Recommendation:** port the semantics unchanged in Phase 1 with a `/** contract */` complexity note and
a cycle counter; fix in Phase 5 with a proper acceleration structure. Do **not** invent one mid-port.

**OQ-9 — PHASE_0's `_VoxelExtent` field name is wrong and should be renamed now.** VERIFIED:
`ANav3DData::GetVoxelExtent()` returns `AgentRadius * 2` (agent *diameter*), it is passed as
`FNav3DData::Initialize(const float VoxelSize, ...)` (`Types.cpp:69`), `LeafSize = VoxelSize * 4`
(`Types.cpp:77`), and `LeafSubNodeSize = LeafNodeSize * 0.25` (`Types.h:506-509`) — so the value is the
**edge length of the finest cell**, not a half-extent. PHASE_0 unit 0C item 4 ships the field as
`_VoxelExtent`.
**Recommendation:** rename to `_FinestCellSizeUu` as part of Phase 1's first commit (Phase 0 has no
consumers yet, so the rename is free, and CkFoundation forbids back-compat shims —
`feedback_no_backcompat_refactors`). If the maintainer prefers the Phase 0 name, keep it but add a
`/** contract */` block stating it is a size, not an extent.

**OQ-10 — Deterministic randomness for `TryGet_RandomFreePoint`.** §2.2 injects an `FRandomStream&`
where upstream uses global `FMath::RandRange`. This changes the public shape of any BP/AS surface that
later exposes "get a random navigable point".
**Recommendation:** injected `FRandomStream&` on the C++ free function; if a BP/AS surface is wanted in
Phase 2, the Utils wrapper owns a per-volume stream seeded from the volume id. Tests need repeatability.

---

## Appendix — Phase 1 file manifest

```
Source/CkVoxelNav/Public/CkVoxelNav/
  Octree/
    CkVoxelNav_Octree_Types.h            §1.1-§1.5   (MortonCode, FNodeAddress, FCellId, FVolumeId,
    CkVoxelNav_Octree_Types.cpp                       FNode, FLayer, FLeafNode, FLeafNodes, FOctree,
                                                      Request_InitializeOctree, allocators)
    CkVoxelNav_Octree_Morton.h/.cpp      libmorton wrappers (from Nav3DUtils.cpp)
    CkVoxelNav_Octree_Query.h/.cpp       §2.2 position↔address, extents, neighbours, free-node walk
    CkVoxelNav_Octree_Rasterize.h/.cpp   §2.2 Stage_* functions + FRasterizeScratch
  Backend/
    CkVoxelNav_GeometryBackend.h         §2.1 ICk_VoxelNav_GeometryBackend, FCk_VoxelNav_BodyId,
                                              FCk_VoxelNav_BackendParams
    CkVoxelNav_GeometryBackend_Jolt.h    §2.1 the one shipped impl
    CkVoxelNav_GeometryBackend_Jolt.cpp
  Volume/
    CkVoxelNavVolume_Fragment_Data.h     §5.1/§5.4 Params, handle, requests, stage enum, stats, delegates
    CkVoxelNavVolume_Fragment.h          §5.2 fragments, tags, signals
    CkVoxelNavVolume_Fragment.cpp
    CkVoxelNavVolume_Processor.h         §3.2 Setup / HandleRequests / Build / CancelPendingRequests
    CkVoxelNavVolume_Processor.cpp       (CK_REGISTER_PROCESSOR at the top)
    CkVoxelNavVolume_Utils.h/.cpp        §5.4
  Settings/
    CkVoxelNav_ProjectSettings.h/.cpp    §3.4 budget knobs
```

Not created in Phase 1: `Octree/CkVoxelNav_Octree_Raycast.*`, everything under `Path/`.

Update at the end of Phase 1: `Source/CkVoxelNav/Claude.md` (boundary paragraph, the Jolt-vs-render
occupancy behavior change from §2.2, the landscape fence from OQ-4, the no-`RunPacedSteps` rationale
from §3.1, the dropped-L1-cache rationale from §3.5, MIT attribution) and `PROGRESS.md` (unit evidence,
decisions ruled from §6, session log).
