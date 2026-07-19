# CkJolt

**Purpose:** Owns the Jolt physics world — `JPH::PhysicsSystem` lifetime, JobSystem (Jolt's own
thread pool or single-threaded), broadphase/object-layer filters, thread-safe contact/activation
listeners, debug-render scaffolding, and the per-tick physics update. Everything Jolt-generic lives
here; features that *use* the Jolt world (CkSpatialQuery's Probe today) are consumers.

Extracted from CkSpatialQuery (2026-07-16, jolt-collision-world campaign Phase 0) with zero behavior
change. Campaign docs: `docs/campaigns/jolt-collision-world/` in the host project.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkSettings`, `CkThirdParty` (vendored Jolt 5.2.1).
**Used by:** `CkSpatialQuery` (Probe/ProbeTrace), `CkEqs` (via registry context), `CkWatermark` (stats).

---

## Key API

- `UCk_Jolt_Subsystem` (tickable, game worlds only) — owns the world (PhysicsSystem, JobSystem,
  listeners, filters, debug renderer). It no longer steps the simulation: the step lives in ECS
  processors driving a `ck::FJoltWorld` (World/CkJoltWorld.h) published as a
  `TSharedPtr<ck::FJoltWorld>` registry context. Subsystem Tick now does only `Super::Tick` + the
  gated debug draw (skipped in async mode; may lag the step by one frame — group order is unpinned).
- **Step processors** (World/CkJoltWorld_Processor.h), all in `FGroup_Transform`, chained after
  `FProcessor_Transform_HandleRequests`: `FProcessor_JoltWorld_WaitForAsync` (consume prior async
  step + apply pose buffer) → `FProcessor_JoltWorld_DrainEvents` (drain contact queue, route to
  registered consumers; runs even when paused) → `FProcessor_JoltWorld_Step` (fixed-timestep pump:
  accumulate real delta, run N fixed sub-steps of `Update` + pose capture, sync-apply or dispatch
  async to the task graph). `_FixedTimestepHz` / `_MaxPhysicsStepsPerFrame` project settings tune the
  pump; excess accumulated time past the max-steps budget is dropped (no ensure — spiral-of-death is
  expected under load).
- **Registry context handoff**: publishes `TWeakPtr<JPH::PhysicsSystem>` via
  `Get_Registry().SetContext<>` — processors/utils that need the world read this context
  (see `CK_PROBE_FACTORY` in CkSpatialQuery, CkEqs' query processors).
- `ck::jolt::Conv(...)` (CkJolt_Utils.h) — UE↔Jolt math/color conversions. **Z-up passthrough, no
  axis swap**; capsule/cylinder shapes need `Get_ShapeAxisCorrection_YToZ()` in a
  `RotatedTranslatedShape` wrapper.
- `ck::jolt::TryGet_EntityFromBody(...)` — body UserData (versioned entity id) → live handle, with
  self-skip and non-ensuring registry-liveness check (snapshot-load safe).
- `FCk_Jolt_ContactEvent` (CkJolt_ContactEvent.h) — the contact-consumption payload; events carry
  raw UserData, consumers resolve entities themselves. Consumers register a
  `ck::FCk_Jolt_ContactEventRouter` via `UCk_Jolt_Subsystem::RegisterContactRouter(Name, Router)` /
  `UnregisterContactRouter(Name)` (routers fire on the game thread in registration order, inside
  `FProcessor_JoltWorld_DrainEvents`) — this replaced the old `Get_OnContactEventsDrained()` multicast.
- `UCk_Jolt_ProjectSettings_UE` (settings UI section "Jolt") — MaxBodies/pairs/constraints, temp
  allocator size, collision steps, threading knobs. CVar overrides `jolt.EnableParallelPhysics` /
  `jolt.EnableAsyncPhysicsUpdate` (startup-only, cmdline-first).
- Debug draw: consumers install an opt-in via `Set_DebugDrawGate` (CkSpatialQuery gates on its
  `PreviewAllProbesUsingJolt` user setting). No gate = no draw.

### Static world (Phase 1)

- `ck::jolt::bake::ExtractActor/ExtractComponent` (CkJoltBakeExtraction.h) — extracts what Chaos
  sees for a static actor: SplineMesh (per-instance deformed BodySetup) → ISM/HISM (per-instance
  bodies sharing a cached shape below `_CompoundShapeInstanceThreshold`; ONE StaticCompoundShape
  above) → StaticMesh (AggGeom per trace flag; `CTF_UseComplexAsSimple` → Chaos cooked tri-mesh
  → `JPH::MeshShape`, winding-swapped) → Brush (convex elems) → Landscape (WITH_EDITOR:
  heightfield per component, Y→Z wrap + row flip — pinned by Ck.Jolt.Bake.HeightField test;
  collision SIGNATURE comes from the paired `GetCollisionComponent()` heightfield collision
  component — the render component's is ignore-everything, which yields a body debug draw
  shows but every channel query drops).
  No valid collision → CK_ENSURE + skip; NEVER a bounding-box substitute. One carve-out:
  a BrushComponent with a null BrushBodySetup is a Verbose skip, not an ensure — Chaos also
  creates no physics state for it, and every level's default/builder brush is one.
- `UCk_JoltStaticWorld_Subsystem_UE` — per-ULevel body tracking in lockstep with
  LevelAdded/RemovedFromWorld (WP cells stream as ULevels); live extraction in PIE (default) or
  cooked data (`_PIEStaticWorldMode`; packaged = always cooked); batch
  `AddBodiesPrepare/Finalize`; OptimizeBroadPhase requested after bulk changes. Static bodies
  live on the `Static_World` object layer — pairs with NOTHING until the Phase-2 layer table
  (query targets only; probes never see them).
- **ECS-first attribution (JoltStaticActor feature):** ONE entity per source actor that contributes
  ≥1 baked body (`ck::FFragment_JoltStaticActor_Current`, transient-owned, DebugName = actor FName).
  Each baked body is stamped with its source actor's entity id as Jolt user-data, so a static-world
  hit resolves back to the entity through the SAME `TryResolve_Entity` path a dynamic JoltBody hit
  uses — there is no FName attribution table. Lifecycle is bidirectional: level-streaming /
  `Request_RemoveActor` / `Deinitialize` remove bodies AND destroy the attribution entities;
  destroying an attribution entity first routes its bodies through the same idempotent funnel via
  `FProcessor_JoltStaticActor_EndPlay` (`Request_RemoveBodiesForEntity` empties the fragment's
  body-id array — the emptiness is the idempotence guard). The subsystem `InitializeDependency`s on
  `UCk_EcsWorld_Subsystem_UE` so the registry outlives it. A level added before the ECS world is
  ready is SKIPPED (Verbose) and re-attempted by the `OnWorldBeginPlay` sweep — bodies are never
  baked without an entity.
- Cooked data: `UCk_Jolt_CookedWorldIndex_UE` (per map, found by path convention under
  `_CookedDataRootPath`) + `UCk_Jolt_CookedCell_UE` per bake-grid cell (`SaveWithChildren` blob,
  shared-dedup). `CookVersion` + `JPH_VERSION_ID` + per-actor runtime hash — stale data ensures
  loudly and is SKIPPED, never re-extracted silently.
- `UCk_Utils_JoltStaticWorld_UE` — `Request_BakeActor/RemoveActor` (runtime-spawned statics;
  ExplicitActor policy bakes Movable-mobility components), `Get_RayCastStaticWorld` (Phase-1
  introspection; the channel-filtered query API is Phase 2; the hit carries a `_Entity` handle,
  not an actor name).
- `UCk_Utils_JoltStaticActor_UE` — typesafe-handle BPFL over the attribution entity: `Has`,
  `Cast`/`DoCast`/`DoCastChecked`, `Get_SourceActor` (may be null after the actor dies),
  `Get_SourceActorName` (cached, survives actor death), `Get_NumBodies`.
- Cooker lives in `CkJoltEditor` (editor subsystem + `-run=CkJoltCook` commandlet).

### Dynamic bodies + characters (Phases 3-4)

- **JoltBody quartet** (`Body/`): `UCk_Utils_JoltBody_UE::Add` with
  `FCk_Fragment_JoltBody_ParamsData` (shape source explicit/from-actor, motion type
  Static/Kinematic/Dynamic, mass source, surface friction/restitution, collision profile →
  object layer, CCD). Requests (deferred, drained by `FProcessor_JoltBody_HandleRequests`
  BEFORE the step): SetSleepState, AddForce(/AtLocation), AddTorque, AddImpulse(/AtLocation),
  AddAngularImpulse, SetLinearVelocity, SetAngularVelocity, Teleport
  (`ECk_Jolt_TeleportVelocityPolicy`; snaps StepPose + reaps the pose-buffer entry — no
  interpolation sweep). Kinematic bodies are NOT moved via requests: `KinematicPush` drives
  every added kinematic body to its current ECS Transform each stepping frame.
  Signals: `OnJoltBodyContactAdded` / `OnJoltBodyContactPersisted` (opt-in via
  `FTag_JoltBody_PersistContacts`) / `OnJoltBodyContactRemoved` (payload OtherEntity +
  points/normal + RelativeNormalSpeed POSITIVE-WHEN-CLOSING + OtherIsSensor) and
  `OnJoltBodySleepStateChanged`. Contact routing: `"JoltBody.Signals"` router registered on
  the subsystem; UserData==0 = NO entity — never resolve raw id 0 (it is the registry's transient
  root). Baked statics now carry their source actor's JoltStaticActor entity id (not 0), so a
  dynamic-vs-baked-floor contact resolves `_OtherEntity` to that attribution entity.
- **JoltCharacter quartet** (`Character/`): `JPH::CharacterVirtual`-backed (no broadphase
  body, no BodyID). Params: capsule radius/half-height (CENTERED capsule — total half
  height = HalfHeight + Radius), MassKg, MaxSlopeAngleDegrees, MaxStrengthNewtons
  (converted ×100 to kg·uu/s²), `ECk_JoltCharacter_PushPolicy` (maps per-contact to
  `CharacterContactSettings {mCanPushCharacter, mCanReceiveImpulses}` via the shared
  `CkJoltCharacterContactListener`), collision profile. Requests: Move (CONTINUOUS desired
  velocity), Jump (one-shot, armed until supported), Teleport. Ground state mirrored to
  `ECk_JoltCharacter_GroundState` + `OnGroundStateChanged` signal. Characters step in
  `DoStepCharacters_AnyThread` (velocity compose → `ExtendedUpdate`) BEFORE each
  `PhysicsSystem::Update` sub-step; all CharacterVirtual scalars that are meters in Jolt
  samples are centimeters here (mPredictiveContactDistance 10, mCharacterPadding 2,
  mCollisionTolerance 0.1, mSupportingVolume Plane(+Z, +HalfHeight)).
- **Ownership**: Chaos XOR Jolt per entity, enforced at composition time (Phase-3 slice 2).

### Debug draw + stats (Phase 5)

- CVars `ck.Jolt.DebugDraw.Enabled` (draw ALL bodies, static + dynamic, motion-type colors)
  and `ck.Jolt.DebugDraw.SleepColoring` (SleepColor mode: awake dynamics yellow, sleeping
  red). The subsystem draws when the consumer gate (`Set_DebugDrawGate`, e.g.
  `ck.SpatialQuery.PreviewAllProbesUsingJolt`) OR the Enabled CVar says so. Skipped in
  async frames.
- The renderer (`CkJoltDebugger`, CkJolt_DebugRenderer.h) is a BATCHED `JPH::DebugRenderer`,
  not `DebugRendererSimple`: triangle batches become transient UStaticMeshes (built once per
  unique geometry — Jolt shapes cache their GeometryRef), instanced per (geometry, color)
  bucket into `UInstancedStaticMeshComponent`s and reconciled per frame; unchanged buckets
  (static + sleeping bodies) cost nothing. Translucent unlit tint via
  `ck.Jolt.DebugDraw.Opacity` (live). `ck.Jolt.DebugDraw.Velocity` (default on) and
  `ck.Jolt.DebugDraw.WorldTransform` (default OFF — line-heavy at stress counts) gate the
  remaining immediate-mode lines. Both windings are emitted per triangle (Conv is a
  handedness passthrough, so one winding renders inside-out).
- Cycle stats under `STATGROUP_CkJolt` (`stat CkJolt` / Insights): `JoltWorld_Step` (whole
  fixed-step pump), `JoltPhysics_Update(_Async)` (the Update loop), `JoltBody_
  WritebackInterpolated`, `JoltBody_KinematicPush`, contact queue/drain stats.

---

## Processor order (FGroup_Transform, after FProcessor_Transform_HandleRequests)

```
WaitForAsync ──> DrainEvents ──> PlanStep ──> SleepStateMirror ─┐
     │                                                          ├─> KinematicPush ─┐
     ├──> JoltBody_Setup ──> JoltBody_HandleRequests ───────────┘                  ├─> Step ──> WritebackInterpolated
     └──> JoltCharacter_Setup ──> JoltCharacter_HandleRequests ──> Character_PreStep ┘
```

- Every body/character Setup + HandleRequests carries an explicit `RunAfter
  FProcessor_JoltWorld_WaitForAsync` edge: the scheduler's Kahn tie-break is LEXICAL by
  processor name, so without the edge a mutation processor can run while the PREVIOUS
  frame's async step is still in flight. Any new processor that touches Jolt state must
  add the same edge.
- EndPlay processors (body + character) live in `FGroup_EndPlay` and open with the ASYNC
  GUARD (`WaitForAsyncStep()` iff a future is pending) — the async step kicked THIS frame
  is only consumed next frame, so teardown would otherwise race the task-graph loop.

## Determinism

- The pump is fixed-timestep: `ck::jolt::ComputeStepPlan` (pure, unit-tested —
  AccumulatorContinuity / SubStepAccumulation / MaxStepsClampDropsExcess / ZeroDeltaNoStep)
  converts real delta + accumulator into N fixed sub-steps at `_FixedTimestepHz`; excess
  past `_MaxPhysicsStepsPerFrame` is DROPPED by design (spiral-of-death guard, no ensure).
  Sim time therefore lags real time under load — tests must accumulate tick delta, never
  count frames.
- Same binary + same step sequence ⇒ Jolt is deterministic; across platforms/compilers it
  is NOT guaranteed. Async mode changes only LATENCY (poses apply one frame late), not the
  step math.

## Tunable knobs

- **Project settings** (`UCk_Jolt_ProjectSettings_UE`, section "Jolt"): `_FixedTimestepHz`,
  `_MaxPhysicsStepsPerFrame`, MaxBodies / MaxBodyPairs / MaxContactConstraints (a 10k-body
  single pile EXCEEDS the defaults — the update returns an error and CkJolt ensures loudly),
  temp allocator size, collision steps, threading (parallel on/off, thread count),
  `_PIEStaticWorldMode`, `_CompoundShapeInstanceThreshold`, `_CookedDataRootPath`.
- **Startup-only CVars/cmdline**: `jolt.EnableParallelPhysics`,
  `jolt.EnableAsyncPhysicsUpdate` (cmdline-first).
- **Runtime CVars**: `ck.Jolt.DebugDraw.Enabled`, `ck.Jolt.DebugDraw.SleepColoring`,
  `ck.Jolt.DebugDraw.Opacity`, `ck.Jolt.DebugDraw.Velocity`, `ck.Jolt.DebugDraw.WorldTransform`.
- Character feel: `FCk_Fragment_JoltCharacter_ParamsData` knobs (MaxStrengthNewtons,
  MaxSlopeAngleDegrees, mass) + the cm-converted ExtendedUpdate settings in
  `DoStepCharacters_AnyThread` (stick-to-floor 50, step-up 40).
- Benchmarks + engine comparison: `docs/campaigns/jolt-collision-world/VALIDATION.md`
  (Jolt ≈2.8× cheaper than Chaos at 10k spread bodies; island size dominates cost).

---

## Threading model

- Jolt's OWN JobSystem steps the simulation (`JobSystemThreadPool`, workers named `JoltWorker_{i}`,
  or `JobSystemSingleThreaded`) — NOT UE's task graph.
- Contact/activation callbacks fire on Jolt worker threads → queued under `FCriticalSection`,
  drained on the game thread (in `FProcessor_JoltWorld_DrainEvents`). Never touch ECS from a Jolt callback.
- `_EnableAsyncPhysicsUpdate` dispatches the whole fixed-step batch (`Update` + pose capture) to UE
  `Async(TaskGraph)`; results are one frame latent and debug draw is skipped in async frames. The
  async batch touches only Jolt objects + `FJoltWorld`'s pose buffer; the game thread never touches
  those while the future is pending (`FProcessor_JoltWorld_WaitForAsync` consumes it first, then
  applies the buffered poses onto entities — `FFragment_JoltBody_StepPose` + `FTag_JoltBody_TransformDirty`,
  Body/CkJoltBody_Fragment.h). The pose apply and contact routing are game-thread only.

---

## Anti-patterns

- Don't create a second `JPH::PhysicsSystem` — one world per game world, owned here.
- Don't call `PhysicsSystem::Update` yourself; `FProcessor_JoltWorld_Step` owns the step.
- Don't resolve entities inside Jolt callbacks — queue and resolve at the drain point.
- Don't bypass `Conv`/axis-correction with hand-rolled conversions.

---

## See also

- `CkSpatialQuery/CLAUDE.md` — the Probe feature (Jolt world consumer, overlap semantics).
- `CkThirdParty` — vendored Jolt source + compile defines (JPH_DEBUG_RENDERER on, ObjectStream off).
