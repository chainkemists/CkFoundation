# Research: external spatial-structure options (licenses verified 2026-08-03)

> Opus 5 web survey, 2026-08-03. Three roles evaluated separately. VERIFIED = source checked that
> day; KNOWLEDGE = from training. Star counts / pushes / licenses checked via GitHub API.

## ROLE 1 — static free-space for volumetric pathfinding

**Verdict: port an SVO; nothing external beats it. Port from a healthy base, and target a *merged*
octree, not a plain one.**

UE ships no volumetric navigation — structurally proven: the UE 5.8 API lists exactly three engine
subclasses of `ANavigationData`: `AAbstractNavData`, `ANavigationGraph`, `ARecastNavMesh`.
`NavMeshBoundsVolume` / Nav Modifier Volume are volume-shaped brushes stamping areas onto a 2.5D
mesh. Global Distance Field is GPU-only, no CPU readback.

### The plugin field (VERIFIED via GitHub API)

| Project | License | Stars | Last push | UE | Notes |
|---|---|---|---|---|---|
| darbycostello/Nav3D (`v2.0` = default branch; `main` is frozen UE4 v1.0, same SHA as `v1.0`) | MIT | 278 | 2026-01-12 | 5.6 (5.7 broken) | SVO + chunking, A*/Theta*/Lazy Theta*, Win64-only, **no tags**, open issues: #37 `AIController::MoveTo` always fails on v2.0; #39 dynamic occlusion broken; #30 UE 5.7 compile break (`ULandscapeMeshCollisionComponent`) |
| TheEmidee/UESVONavigation | MIT | 55 | 2026-02-13 | unstated | refactored uesvon, native nav integration, async regen, **single-threaded generation**, no streaming |
| midgen/AeonixNavigation | MIT | 45 | 2025-12-16 | **5.5**/5.6/5.7 | uesvon's official successor; Beta; depends on StateTree |
| midgen/uesvon | MIT | 262 | 2025-11-28 | 5.3 | deprecated in README → Aeonix |
| VSZue/DonAINavigation | MIT | 270 | 2023-01 | UE4 | uniform grid, author recommends native nav instead |
| ZioYuri78/GraphAStarExample | **NONE** | 101 | archived | UE4 | no license = all rights reserved — legally unusable |

Negative results: no "Kaiser1989" GraphAStar repo exists. Flying Navigation System (Ben
Sutherland / BlenderSleuth) has no public source; its docs confirm the canonical Brewer leaf
layout (4×4×4 leaf voxels in one uint64, 1 bit per subnode). AirSim's `simCreateVoxelGrid` is an
offline binvox export brute-forcing `OverlapBlockingTestByChannel` per cell — no planner, dead end.

### Evidence that shapes the design

**Brewer & Sturtevant 2018 (Warframe, shipped):** 2 km³ levels, 44 maps at 1.5M–500M voxels,
10–20 agents, ~2 path recomputes/sec, <1 ms typical / 100 ms worst target on 8-core 1.75 GHz.
Dense grids rejected for memory AND cache locality — hence SVO + Morton. Own framing: memory, time,
and path quality trade off; the design optimizes for suboptimal-but-fast.

**Massonnat & Verbrugge (McGill) 2024, "Efficient Octree-based 3D Pathfinding"** — direct critique
of the exact design being ported:
- Brewer's clustered-octree paths average **~20% longer than optimal**.
- **Node merging is the lever**: Warframe "Complex" map 8.3M voxels → 41,385 octree cells →
  **10,552 merged**; Building_1 28,190 → 4,226 → **303**. Search cost tracks leaf count.
- Query cost (Building_1): voxel baseline ~6–12 ms; plain octree ~0.5–4.5 ms; **merged octree
  consistently <1 ms**.
- Dynamic obstacles: merged octree + **local graph repair** = 0.97 ms vs 4.10 ms full rebuild.
- JPS-3D beats octrees ~2× geometric mean but degrades on long paths; octrees win long-range.
- Path refinement (visibility pruning + 3D funnel) recovers **5–10%** of path length.
- Caveat: Unity/C# measurements — take ratios, not absolute ms.

**Bake time is the universal pain point** across the lineage (Nav3D's own docs: disable automatic
generation on large levels; TheEmidee: single-threaded gen; uesvon wiki: alpha needing profiling).
Per-voxel UE channel overlaps are the documented bottleneck (VERIFIED for AirSim + TheEmidee;
INFERRED for Nav3D/Aeonix). **A Jolt-static-world voxelizer under ParallelFor is the one
optimization no existing plugin can replicate.**

GPU voxelization (Schwarz & Seidel 2010 — Brewer's cited construction basis; Crassin & Green) is an
unclaimed niche in UE nav plugins. Not phase one.

## ROLE 2 — dynamic entity proximity index

**Verdict: Jolt broadphase (already in-tree) is best-in-class; keep it. The gaps are batching, kNN,
and body-free entities — filled by one batching fix + a hand-rolled per-frame grid, not a library.**

**What Jolt's broadphase actually is** (VERIFIED from vendored 5.2.1 source): a **4-ary AABB tree
(BVH)**, branching factor 4 for SIMD; 128-byte nodes with four children's bounds in SoA atomic
float arrays; one independent tree per `BroadPhaseLayer`. Movement never re-inserts — it widens
ancestor AABBs atomically (O(1) amortized). Queries take a `shared_lock` on a **double-buffered**
`SharedMutex[2]` flipped in `UpdateFinalize`; `FrameSync()` unique-locks only the retired mutex.
**Broadphase queries are safe from any thread, concurrent with each other and with body movement.**
The locking caveats concern `Body` data access — resolving hits via UserData→entity sidesteps that.

**Trap if used as a pure index without stepping:** without `PhysicsSystem::Update`, the tree only
ever widens (degrades toward O(N)); the only rebuild hook is `OptimizeBroadPhase()` — a full
synchronous rebuild Jolt forbids per-frame. Also: `Body` = 160 bytes, hard `mMaxBodies` cap,
`BroadPhaseQuadTree` cannot be decoupled from `BodyManager` without forking.

**The concrete scaling bug in-tree:** `CkProbe_Processor.cpp:711` drives probe motion via per-body
`BodyInterface::SetPositionAndRotation` — one `BodyLockWrite` + `NotifyBodiesAABBChanged(&id, 1)`
per body. At 10k probes: 10k lock cycles + 10k single-element updates per frame. **Batch through
`NotifyBodiesAABBChanged(ids, N)` before drawing any conclusion about Jolt's limits.**

**Jolt gives no kNN.** Options: expanding-radius CollideSphere + sort (wasteful); nanoflann
(BSD-2, header-only, static kd-tree rebuilt per frame — points only, no raycast); hand-rolled grid
(serves kNN and radius equally).

**Decisive negative result:** GitHub search `spatial hash grid language:C++` → top result **6
stars**; `broadphase spatial index language:C++` → **zero results**. There is no maintained
standalone C++ spatial-hash library; it is universally hand-rolled (~200-300 lines).
Box2D v3 is MIT but **C11 and 2D-only**; Bullet zlib, 421 open issues, only `rayTest` re-entrant;
`aabbcc` dead since 2021; `libspatialindex` GIS/disk-oriented; `parallel-hashmap` (Apache-2.0) is
good but its concurrent API forces callback access and a vector-per-cell allocates per cell per
frame — the lower-allocation idiom (count → prefix-sum → scatter) needs no concurrent map at all.

**Rebuild vs incremental splits by structure:** trees converge on incremental (Jolt, Box2D v3
`Rebuild(fullBuild=false)`, Bullet `optimizeIncremental`); grids ship both — Unity DOTS boids does
a **full parallel rebuild every frame** (`NativeParallelMultiHashMap`), Epic's Mass is incremental.
Rebuild wins when it is a flat, allocation-free, parallelizable pass over SoA arrays you own.
Sizing caveat (Chipmunk2D's documented reason for abandoning its hash for an AABB tree): hashes
excel with **same-sized** objects — the horde case is the structure's best case; Epic's
`THierarchicalHashGrid2D` adds levels + spill list for varied sizes.

## ROLE 3 — Unreal built-ins (Epic EULA; usable from a UE-only plugin, never vendorable)

| Type | Path | Verdict |
|---|---|---|
| `TOctree2` | `Core/Public/Math/GenericOctree.h` | 3D, flat arrays + uint32 indices, battle-tested (renderer). Solid but update-heavy/awkward. |
| `TPointHashGrid3` | `GeometryCore/Public/Spatial/PointHashGrid3.h` | closest built-in to Role 2, but `TMultiMap` + mutex; queries not thread-safe vs updates. Tools-grade, not 10k-parallel-frame grade. |
| `FSparseDynamicOctree3` | `GeometryCore/.../SparseDynamicOctree3.h` | Epic's own header TODOs concede degenerate cell/bucket behavior. Mesh-editing tool. |
| `THierarchicalHashGrid2D` | `AIModule/Public/HierarchicalHashGrid2D.h` | **2D** (first line of header). This is Mass/`FNavigationObstacleHashGrid2D`. Right shape to copy, wrong dimensionality to use. |
| Chaos `FAABBTree` | Experimental/Chaos | pulls Chaos; we have Jolt. No. |

## Overall recommendation (as delivered)

Port the SVO; adopt nothing for either role. Deviate from a straight Nav3D port: (1) make the SVO
satisfy `ck::astar::AStarGraph` and reuse CkAStar rather than importing Nav3D's pathfinder;
(2) implement **node merging** (the ~4× leaf-count / sub-1ms lever); (3) voxelize from the baked
Jolt static world under ParallelFor; (4) budget for the ~20%-suboptimal ceiling with the
visibility-pruning/funnel refinement pass (recovers 5–10%); (5) leave a seam for a coarse graph
above the octree. Audit the Nav3D v2.0 dynamic-occlusion code (open issue #39) before porting that
machinery; Aeonix/TheEmidee are the reference alternatives. For proximity: keep Jolt, batch the
probe AABB updates, wire the unused broadphase tier, adopt `Get_OverlapEntities` at the iterate-all
sites, and fill kNN/massed queries with a hand-rolled per-frame-rebuilt flat grid whose shape is
copied from `THierarchicalHashGrid2D`/DOTS boids — measured result, not judgment: no adoptable
library exists.

**Unverified, flagged:** Flying Navigation System / Havok pricing (403s); the overlap-bottleneck
claim specifically inside Nav3D/Aeonix voxelizers (confirmed only for AirSim + TheEmidee); the
"191× grid vs kd-tree" BioDynaMo figure (secondhand); ZoneGraph/Mass volumetric status beyond the
`ANavigationData` subclass list.
