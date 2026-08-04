# Research: spatial structures & proximity mechanisms already in CkFoundation

> Opus 5 read-only survey, 2026-08-03. Commissioned to answer: does CkFoundation need a new
> general-purpose parallel spatial index, or do existing structures cover it? VERIFIED = code read.

**Headline:** there is exactly **one** real spatial index in the entire plugin — Jolt's broadphase,
owned by CkJolt. Every other "spatial" structure is either a dense authored grid (not an index of
entities), a bitmap, or an unindexed linear scan. No first-party octree, BVH, hash grid, kd-tree,
loose grid, or Morton code exists anywhere (VERIFIED via `rg --no-ignore` sweep excluding
CkThirdParty).

## 1. Jolt broadphase + narrowphase — the only true spatial index

- **Structure:** Jolt's `BroadPhaseQuadTree` (one tree per broadphase layer), vendored Jolt 5.2.1.
  Exactly two broadphase layers: `Static{0}` / `Dynamic{1}` — `CkJoltCollisionLayerTable.h:80-81`.
  Object layers signature-driven, registered on demand.
- **Indexes:** entire static level (baked per-`ULevel`, streaming-aware,
  `CkJoltStaticWorld_Subsystem.cpp`, 887 lines) + every dynamic `JoltBody`, `Probe` sensor,
  `JoltCharacter`. Bodies carry the versioned entity id as Jolt UserData → every hit resolves to an
  ECS handle (`ck::jolt::TryGet_EntityFromBody`).
- **Update model:** incremental. Static = bake once; `OptimizeBroadPhase` re-requested after
  `_BroadphaseOptimizeThreshold` = 512 add/removes (`CkJolt_ProjectSettings.h:129`). Dynamic =
  Jolt's per-step update inside `FProcessor_JoltWorld_Step` (fixed-timestep, `FGroup_Transform`).
- **Threading:** Jolt's own JobSystem; queries documented thread-safe and already called from
  worker threads — `CkEqs_Algorithm.cpp:351` ("Worker threads: Jolt narrow-phase is thread-safe").
- **Consumers:** CkSpatialQuery (Probe/ProbeTrace), CkEqs, CkProjectile, CkCrowd (transitively),
  CkWatermark (stats). Only `.Build.cs` dependents (VERIFIED).
- **Size/maturity:** CkJolt ≈ 13,800 lines, heavily documented, test-pinned, campaign-hardened.

**Critical gap (VERIFIED):** `PhysicsSystem->GetBroadPhaseQuery()` is called **nowhere**. All six
query sites use `GetNarrowPhaseQuery()` — `CkJoltQuery_Utils.cpp:128,177,225,268,325`,
`CkJoltStaticWorld_Subsystem.cpp:195`, `CkProbe_Processor.cpp:770`,
`CkProbeTrace_Utils.cpp:413,679`. `BroadPhaseQuery::CollideSphere/CollideAABox` — the cheap
candidate-culling primitive a hand-rolled index would exist to provide — sits unused.

**Second gap (VERIFIED):** `UCk_Utils_JoltQuery_UE::Get_OverlapEntities(...) -> TArray<FCk_Handle>`
(the general entity-proximity API, CkJolt Phase 2) has **zero callers** outside CkJolt.

## 2. CkSpatialQuery — a Jolt facade, not a structure

No spatial structure of its own; 6,176 lines of Probe/ProbeTrace plumbing over CkJolt.
- **Probe** (`CkProbe_Fragment.h:43-64`): per-entity `JPH::BodyID` + `TSet<FCk_Probe_OverlapInfo>
  _CurrentOverlaps` — an overlap cache maintained by Jolt's contact listener via
  `UCk_SpatialQuery_Subsystem::ProcessQueuedContacts`.
- `FProcessor_Probe_UpdateTransform` (game-thread serial, `CkProbe_Processor.h:189`) pushes ECS
  transforms into Jolt bodies each frame; LinearCast variant is `TParallelProcessor` (`:230`).
- **ProbeTrace**: stateless immediate ray/shape casts, no persistence.

## 3. CkCrowd neighbor gathering — Jolt-backed, zero brute force

Files: `CkCrowdAgent_Neighbors_Fragment.h` / `CkCrowdAgent_Neighbors_Processor.{h,cpp}` (class
`FProcessor_CrowdAgent_NeighborSync`).
- Each agent spawns a **child probe entity** with a kinematic cylinder Jolt sensor, radius =
  `_Radius + _SeparationLookahead` (142 cm default) — `CkCrowdAgent_Processor.cpp:40-90`.
  Jolt's broadphase does the neighbor finding.
- NeighborSync (`.cpp:30-119`) reads previous frame's probe overlaps, hops probe→owner→agent,
  computes offset/velocity/distance, sorts, truncates to `_MaxNeighborsForSteering` (default **6**,
  `CkCrowdAgent_Fragment_Data.h:152`). `TParallelProcessor`, `FGroup_Physics`.
- Consumers: Separation, PushApart, AvoidanceSample — all read the same cached ≤6-element array.
- **Scaling note:** probe-per-agent + contact events is fine at ~150 agents; at 3-5k agents this is
  the path that dies (thousands of sensor bodies + contact churn + narrowphase per pair).

## 4. CkGrid 2dGridSystem — dense authored grid, not an index

Nested `FCk_Registry` of cell entities, entity id == row-major index + 1 → O(1) `Get_CellAt`
(`Ck2dGridSystem_Utils.cpp:292-312`). Generic index↔coord↔location math in `UCk_Utils_Grid2D_UE` /
`UCk_Utils_Grid3D_UE` (`CkGrid_Utils.h:25,134`). Cannot answer "which entities near this point".
Consumers: CkInventory + CkGridEditor only. (CkAStar's CLAUDE.md claim of CkGrid use is stale —
no Build.cs dep.)

## 5. CkPoi / CkCompass / CkMinimap — iterate-all, ParallelFor'd

**The brute-force site.** `CkCompass_Processor.cpp:305-326`, `CkMinimap_Processor.cpp:357-378`:
full `View<FTag_Poi,...>` sweep → scratch array → `ParallelFor` (single-threaded below
`MinPoisForParallel = 64`), `FVector::Dist` + filters per POI. No spatial pre-filter.
FogOfWar (CkMinimap) is a genuine uniform grid but indexes *visibility*, not entities
(bit-packed explored bitmap, `_CellSize` 500, stamped per revealer @0.25 s).

## 6. CkEqs

Generators are procedural point patterns (SimpleGrid/Grid/Donut/Cone/OnCircle) — no structure.
**`EntitiesWithTag`** (`CkEqs_Algorithm.cpp:462-495`) pulls every entity with the tag world-wide
(per-tag entt storage pool, so O(entities-with-tag), not O(world)) with **no spatial bound**;
distance culling only later as a Distance test. Trace/Overlap tests are Jolt casts under
`ParallelFor` (`:359,807,964`).

## 7. Others

| Module | Mechanism | Structure |
|---|---|---|
| CkPerception (Hearing) | linear scan over registered listener `TSet`, per noise event — `CkHearingPerception_Component.cpp:268-283` | none |
| CkTargeting | target-point entity factory only (229 lines) | none |
| CkAggro | no spatial discovery — threat enters via `Request_AddThreat`; distance used only for scoring | none |
| CkRaySense | **UE Chaos** traces (`UKismetSystemLibrary::LineTraceSingle`/`OverlapAnyTest`, `CkRaySense_Processor.cpp:113,166`) | UE scene |
| CkOverlapBody | **UE `UShapeComponent` overlap delegates** → ECS signals | UE Chaos |
| CkInteraction | no spatial mechanism at all; explicit handles | none |
| CkEcs/CkEcsExt/CkCore | no spatial containers (views/parallel processors are the substrate, not an index) | none |

## Assessment (as delivered)

A new general-purpose spatial index module is **not justified at Rewind99 scale** (~150 crowd
agents / ~130 NPCs; every brute-force site is small-N or event-driven). The real gaps: (1) the
unused broadphase tier, (2) `Get_OverlapEntities` unadopted at the three iterate-all sites, (3) the
genuine duplication is two *collision backends* (CkRaySense/CkOverlapBody on Chaos vs everything
else on Jolt), not two indexes.

**Post-review amendment (orchestrator, same day):** the maintainer corrected the scale premise —
CkFoundation must serve future horde-scale games (vampire-survivors style, hundreds-to-thousands of
navigating enemies). At that scale the probe-per-agent neighbor path does not survive, and a
per-frame-rebuilt flat spatial hash (plus kNN) becomes a justified framework capability. See
`research-external-survey.md` and campaign decisions.
