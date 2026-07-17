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
  heightfield per component, Y→Z wrap + row flip — pinned by Ck.Jolt.Bake.HeightField test).
  No valid collision → CK_ENSURE + skip; NEVER a bounding-box substitute.
- `UCk_JoltStaticWorld_Subsystem_UE` — per-ULevel body tracking in lockstep with
  LevelAdded/RemovedFromWorld (WP cells stream as ULevels); live extraction in PIE (default) or
  cooked data (`_PIEStaticWorldMode`; packaged = always cooked); batch
  `AddBodiesPrepare/Finalize`; OptimizeBroadPhase requested after bulk changes. Static bodies
  live on the `Static_World` object layer — pairs with NOTHING until the Phase-2 layer table
  (query targets only; probes never see them).
- Cooked data: `UCk_Jolt_CookedWorldIndex_UE` (per map, found by path convention under
  `_CookedDataRootPath`) + `UCk_Jolt_CookedCell_UE` per bake-grid cell (`SaveWithChildren` blob,
  shared-dedup). `CookVersion` + `JPH_VERSION_ID` + per-actor runtime hash — stale data ensures
  loudly and is SKIPPED, never re-extracted silently.
- `UCk_Utils_JoltStaticWorld_UE` — `Request_BakeActor/RemoveActor` (runtime-spawned statics;
  ExplicitActor policy bakes Movable-mobility components), `Get_RayCastStaticWorld` (Phase-1
  introspection; the channel-filtered query API is Phase 2).
- Cooker lives in `CkJoltEditor` (editor subsystem + `-run=CkJoltCook` commandlet).

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
