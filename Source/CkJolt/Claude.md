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
  `RotatedTranslatedShape` wrapper. Jolt's cylinder/capsule caps sit at `(0, ∓HalfHeight, 0)`; the
  correction is **+90° about X** precisely so the top cap lands at `(0, 0, +HalfHeight)` and the
  shape stands UP — `-90°` builds it upside-down. An uncorrected shape is an anisotropic query
  VOLUME, not merely a bad visual.
- `ECk_MotionType` / `ECk_BackFaceMode` / `ECk_MotionQuality` (CkJolt_Common.h) were migrated out of
  CkSpatialQuery's Probe. CoreRedirects in `Config/DefaultCkFoundation.ini` keep serialized BP
  references valid; AngelScript rebinds by short name automatically. Do NOT rename these without
  updating the redirects.
- **Collision layer table** (`CkJoltCollisionLayerTable.h`) — pair semantics mirror UE touch
  resolution: `min(A.Response[B.Channel], B.Response[A.Channel])` (documented once, on
  `ECk_Jolt_PairInteraction`). Jolt's pair filter is BINARY — anything != Ignore means "interact";
  Block-vs-Overlap is resolved at the query/contact sites. `FCk_Jolt_LayerContext` holds a
  NON-const table pointer on purpose: JoltBody setup is a sanctioned game-thread registration site
  (resolve-or-register via `Get_OrRegisterLayer`), while Probe setup only reads `_ProbeLayer`.
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
- Dense-cluster ISM/HISM path (`ExtractComponent`, ≥ `_CompoundShapeInstanceThreshold` instances):
  the single StaticCompoundShape's children are expressed RELATIVE to the component's world
  position/rotation, and each instance's scale is baked into its cached child SHAPE, not into the
  child transform. A cluster where all but one instance fails shape creation falls back to a single
  plain body — Jolt requires ≥2 children for a compound.
- `BuildShape_FromBodySetup` leaf combination: 0 leaves → null shape; 1 leaf at identity local
  transform → the leaf itself; 1 leaf with an offset → `RotatedTranslatedShape` wrap; 2+ →
  `StaticCompoundShape` (same ≥2-children rule).
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
- **Level-sweep admission is settings-driven (`FCk_Jolt_BakeFilter`).** Built from project settings
  once per sweep/cook (`Make_FromProjectSettings`) and passed explicitly through
  `ExtractActor`/`ExtractComponent` AND the source hashes — tests construct filters directly, no CDO
  mutation. Axes: `_BakeMobilityPolicy` (`All` default — Static, Stationary AND Movable bake; a
  baked Movable is a SNAPSHOT at sweep time, the static body does not follow later movement;
  `StaticAndStationary` restores the pre-v2 behavior), excluded actor classes (IsChildOf; default
  `{APawn}` — a pawn's collision is dynamic-object territory), excluded actor/component tags
  (default `{"Ck.Jolt.NoBake"}` — the designer per-instance opt-out), excluded object channels,
  excluded collision profiles, and opt-in overlap-only exclusion (default Disable — triggers keep
  UE query parity). `ExplicitActor` (`Request_BakeActor`) ignores the filter entirely: the caller
  declared the actor static-in-intent. Observability (pinned by
  `Ck.Jolt.BakeExtraction.MobilityPolicy` + `Ck.Jolt.BakeExtraction.FilterExclusions`): every
  exclusion is counted per-axis in `FCk_Jolt_ExtractionStats`, every world boot logs a one-line
  sweep summary at Log verbosity naming the active mobility policy, and an all-skipped level logs
  its own zero line at Verbose. `[0] static bodies` on that summary line = your map's geometry is
  excluded (or the map is empty) — check the Bake Filter settings, don't debug the trace.
- Cooked data: `UCk_Jolt_CookedWorldIndex_UE` (per map, found by path convention under
  `_CookedDataRootPath`) + `UCk_Jolt_CookedCell_UE` per bake-grid cell (`SaveWithChildren` blob,
  shared-dedup). `CookVersion` (v2: settings-driven bake filter) + `JPH_VERSION_ID` + per-actor
  runtime hash + the index-level `_BakeFilterHash` (cook-time filter fingerprint vs the current
  settings-built one — settings drift stales the WHOLE map) — stale data ensures loudly and is
  SKIPPED, never re-extracted silently.
- `UCk_Utils_JoltStaticWorld_UE` — `Request_BakeActor/RemoveActor` (runtime-spawned statics;
  ExplicitActor policy bakes Movable-mobility components), `Request_BakeComponent/RemoveComponent`
  (component-granular runtime bake for runtime-composed geometry — a re-bake REPLACES the
  component's bodies, so repopulated ISMs re-bake cleanly), `Get_RayCastStaticWorld` (Phase-1
  introspection; the channel-filtered query API is Phase 2; the hit carries a `_Entity` handle,
  not an actor name). CkUnrealComponent bakes hosted primitives AUTOMATICALLY
  (`_StaticWorldBakePolicy = Automatic`, the default): a collision-bearing component bakes at
  setup unless the Jolt bake-filter's component exclusions say otherwise, moving the owning
  entity RE-BAKES the bodies at the new pose (teleports/rearrangement stay query-correct; a
  continuously moving blocker churns the broadphase — that content belongs on a kinematic
  CkJoltBody), and EndPlay teardown removes them. Zero bodies under Automatic is a quiet skip —
  an ISM whose instances arrive after Add calls
  `UCk_Utils_UnrealComponent_UE::Request_BakeIntoJoltStaticWorld` once configured. `BakeOnSetup`
  declares archetype-complete collision (zero bodies = loud ensure, filter bypassed);
  `DoNotBake` opts out.
- **Per-mesh pre-baked shapes** (`UCk_Jolt_CookedMeshShape_UE` + `mesh_shape_utils`,
  StaticWorld/CkJoltMeshShape_Utils.h): the CkJoltEditor mesh cook sweeps every static mesh under
  `_BakedMeshShapeRoots` and stores its SCALE-1 shape blob at
  `<CookedDataRoot>/Meshes/<MeshPath>_JoltShape` (path convention, like the map index). Runtime
  (`FCk_Jolt_ShapeCache::GetOrCreate_Shape` + the JoltBody StaticMeshAsset source) restores the
  root once per mesh per session and wraps a `JPH::ScaledShape` per instance scale — skipping the
  expensive hull/tri-mesh builds. ONLY hull/tri-mesh collision is pre-baked
  (`Get_IsWorthPreBaking`, shared by cooker and runtime so the skip rule and the miss-loudness
  rule cannot disagree); pure primitives rebuild cheaply. Fallback-to-build (never a missing
  body): missing asset, stale asset (guid/trace-flag/version ensures), a scale the topology
  rejects (`IsValidScale` — non-uniform on spheres/capsules or rotated compound children;
  `MakeScaleValid` is deliberately NOT used, approximated collision is silently wrong), or any
  negative scale. A miss ensures loudly ONLY when cooked data is expected (packaged / PIE-Cooked)
  AND the mesh sits under a baked root AND its collision is worth pre-baking. Cook via
  `-run=Ck_JoltCook_Commandlet -MeshShapes` (combinable with `-Map`/`-AllMaps`; the FULL class
  token is required — `-run=CkJoltCook` resolves to no class) — the Tools-menu entry cooks
  only the CURRENT WORLD (`Cook_CurrentWorld`), never mesh shapes;
  incremental by BodySetupGuid; orphans logged, never auto-deleted.
- `UCk_Utils_JoltStaticActor_UE` — typesafe-handle BPFL over the attribution entity: `Has`,
  `Cast`/`DoCast`/`DoCastChecked`, `Get_SourceActor` (may be null after the actor dies),
  `Get_SourceActorName` (cached, survives actor death), `Get_NumBodies`.
- Cooker lives in `CkJoltEditor` (editor subsystem + `-run=CkJoltCook` commandlet).

### Scene queries + occupancy

- `UCk_Utils_JoltQuery_UE` (Query/CkJoltQuery_Utils.h) — the BP/AS-facing scene queries: `Get_RayCast` /
  `Get_ShapeCast` / `Get_Overlap` (+ the `*Multi` variants) / `Get_OverlapEntities`. Each takes a
  WorldContextObject and an `FCk_Jolt_QueryFilter` (UE channel + minimum response), resolves the subsystem
  and rebuilds its shape per call, collects EVERY hit, and attributes each hit to a live entity. Pure reads
  — no probe-overlap side effects.
- **Occupancy surface** — the opposite trade, for callers issuing thousands of boolean tests (coverage
  grids, volumetric navigation bakes): any-hit early-out, NO entity attribution, no per-call subsystem
  resolution, caller-owned shape and filters. Two layers:
  - `ck::jolt::Get_IsBoxOccupied` (box dimensions, or a caller-held `JPH::Ref<JPH::Shape>`),
    `ck::jolt::Get_IsSegmentBlocked` and `ck::jolt::Get_BodiesInAABox` (Query/CkJoltOccupancy_Utils.h) —
    JPH signatures, for callers that already hold a `JPH::PhysicsSystem`. `Get_BodiesInAABox` is the
    module's first `GetBroadPhaseQuery().CollideAABox` use: bounding boxes only, so the answer is
    conservative and cheap — a whole-region early-out, not a per-cell test.
  - `Get_IsSegmentBlocked` is a LINE-OF-SIGHT test, not a raycast: `AnyHitCollisionCollector<CastRayCollector>`
    over `NarrowPhaseQuery().CastRay`, so it stops at the first blocker and reports only a bool — no
    fraction, no normal, no entity. A segment starting inside a convex body reads as blocked (Jolt's
    convex-as-solid default). Callers that need hit DATA want `UCk_Utils_JoltQuery_UE::Get_RayCast`
    instead; this one exists for visibility/clearance sweeps that issue thousands of boolean tests.
  - `ck::jolt::FCk_Jolt_QuerySession` + `ck::jolt::FCk_Jolt_BoxProbe` (Query/CkJoltOccupancy_Session.h) —
    the same capability with NO Jolt type in the header: opaque `TPimplPtr`-backed, move-only value types,
    so a module that must not see JPH (a geometry-agnostic navigation backend) still gets the fast path.
    The session resolves the subsystem and builds its static-domain filters once and holds the physics
    world WEAKLY; a probe holds one reusable box shape per cell size. Bodies come back as opaque `uint64`
    ids — compare them and pass them back, never decode them.
  - An INVALID session answers every query "unoccupied" / no bodies. That is a legitimate state (no Jolt
    subsystem outside Game/PIE worlds), so consumers must gate on `Get_IsValid()` and fail loudly
    themselves rather than baking an empty world.
- Query-side filters live in CollisionLayers/CkJoltCollisionLayerTable.h: `FCk_Jolt_ChannelQueryFilter`
  (UE trace semantics), `FCk_Jolt_DomainQueryFilter` (one body domain), plus the occupancy pair
  `FCk_Jolt_StaticOccupancyFilter` (object layer, Static domain fixed) and
  `FCk_Jolt_StaticBroadPhaseQueryFilter` (broadphase, static tree only — the scene-query wrappers pass an
  accept-all broadphase filter, which is the wrong default at grid-scale query counts). The Static domain
  covers baked level geometry AND Static-motion-type JoltBodies; both are immovable world obstacles.
- **Game thread only**, as with every Jolt query here — safe off-thread only under a future step-barrier
  contract, which this module does not yet provide.

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
  dynamic-vs-baked-floor contact resolves `_OtherEntity` to that attribution entity. A baked
  static's JoltStaticActor entity arriving as the *self* side of a contact is benign — it has no
  `FFragment_JoltBody_Current`, so the router's first guard drops it; only the JoltBody's own body
  id (index+seq) may drive that entity's signals.
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
  mCollisionTolerance 0.1, mSupportingVolume Plane(+Z, +HalfHeight)). Left unconverted the
  metre-based defaults are ~1 mm in uu: characters find contacts only after penetrating, then
  jitter and stick on wall slides; `MaxStrengthNewtons` unconverted caps pushes at 1% of the
  authored intent (hence the ×100). `mSupportingVolume` is deliberately NOT the Jolt sample's
  `-radius`: our capsule is CENTERED at the character position (zero-translation wrapper from
  `CkJoltShapeFactory`), not the sample's base-at-origin shape, so the sample constant would accept
  "support" contacts up to +radius ABOVE the capsule center and waist-height ledge lips would read
  as ground.
- **Ownership**: Chaos XOR Jolt per entity, enforced at composition time (Phase-3 slice 2).
- **JoltConstraint quartet** (`Constraint/`): `UCk_Utils_JoltConstraint_UE::Create(BodyA, Params)`
  spawns a CHILD entity of body A hosting a `JPH::TwoBodyConstraint`. Types
  (`ECk_JoltConstraint_Type`): Distance (optional soft-limit SPRING —
  `ECk_JoltConstraint_SpringMode` Frequency/Stiffness + damping; min/max -1 = auto from creation
  separation), Point (the rope/chain link), Hinge (axis + degree limits [-180,0]/[0,180], friction
  torque, motor via `Request_Hinge_SetMotor` — Off/Velocity/Position,
  `Get_Hinge_CurrentAngleDegrees`). Anchors/axes are WORLD-space at creation (Jolt converts to
  body-local internally, so they track moving bodies). `Params._OtherBody` INVALID = anchored to
  the WORLD (`_BodyBIsWorldAnchor` distinguishes this from a dead body). Setup runs after
  JoltBody_Setup and resolves each body to one of three outcomes: **Ready** (JPH body id valid),
  **Retry** (the body entity exists but its batched AddBodies pass hasn't run — setup re-arms
  `FTag_JoltConstraint_NeedsSetup` and retries next frame), **Failed** (configuration error, already
  ensured — the constraint is never created).
  **Lifecycle invariant**: a JPH constraint must be gone BEFORE either of its bodies —
  `FProcessor_JoltConstraint_LivenessReap` (FGroup_EndPlay, `RunBefore` JoltBody_EndPlay) removes
  the constraint the same frame EITHER body entity begins destruction and queues the inert
  constraint entity for destroy; plain EndPlay handles the cascade case (constraint child dies
  with body A). Requests: SetEnabled (a disabled link is a heal-able rope cut),
  Distance_SetRange, Hinge_SetMotor.
- **Rope builder** (`Constraint/CkJoltRope_Utils.h`): `UCk_Utils_JoltRope_UE::Create_Rope(Owner,
  FCk_JoltRope_ParamsData)` — N Dynamic sphere segments (children of Owner) linked Rigid (point
  constraints at boundaries) or Springy (auto-distance + spring between centers), anchored to the
  world or an `_AnchorBody` (e.g. a kinematic head — the "hair" pattern is many short Springy
  strands on a moving anchor). Returns `FCk_JoltRope_Result` (segment bodies + links; cutting =
  destroy a link entity). Keep `_SegmentRadius < _SegmentLength / 2` — neighboring segments share
  the collision profile and are NOT filtered against each other.

### Debug draw + stats (Phase 5)

- CVars `ck.Jolt.DebugDraw.Enabled` (draw ALL bodies, static + dynamic, motion-type colors)
  and `ck.Jolt.DebugDraw.SleepColoring` (SleepColor mode: awake dynamics yellow, sleeping
  red). The subsystem draws when the consumer gate (`Set_DebugDrawGate`, e.g.
  `ck.SpatialQuery.PreviewAllProbesUsingJolt`) OR the Enabled CVar says so. Skipped in
  async frames. The `ck.Jolt.DebugDraw.*` CVars follow the house C++ pattern —
  `FAutoConsoleVariableRef` over a static in a filename-derived named namespace (exemplar:
  `ck.SpatialQuery.PreviewAllProbesUsingJolt` in `CkSpatialQuery_Settings.cpp`) — and are read on
  the game thread in Tick; there is no body filter. `jolt.EnableParallelPhysics` /
  `jolt.EnableAsyncPhysicsUpdate` are startup-only because the JobSystem is created once in
  `Initialize` (cmdline form: `-jolt.EnableParallelPhysics=0`).
- The renderer (`CkJoltDebugger`, CkJolt_DebugRenderer.h) is a BATCHED `JPH::DebugRenderer`,
  not `DebugRendererSimple`: triangle batches become transient UStaticMeshes (built once per
  unique geometry — Jolt shapes cache their GeometryRef), instanced per (geometry, color)
  bucket into `UInstancedStaticMeshComponent`s and reconciled per frame; unchanged buckets
  (static + sleeping bodies) cost nothing. Translucent unlit tint via
  `ck.Jolt.DebugDraw.Opacity` (live). `ck.Jolt.DebugDraw.Velocity` (default on) and
  `ck.Jolt.DebugDraw.WorldTransform` (default OFF — line-heavy at stress counts) gate the
  remaining immediate-mode lines. Both windings are emitted per triangle (Conv is a
  handedness passthrough, so one winding renders inside-out).
- WHY batched: `DebugRendererSimple`'s `DrawGeometry` fallback decomposes EVERY triangle of EVERY
  body into individual `DrawDebugLine` calls EVERY frame — hundreds of thousands of one-frame line
  submissions per frame on the game thread at stress-gym body counts. Instead `CreateTriangleBatch`
  runs ONCE per unique geometry (Jolt shapes cache their `GeometryRef` — HeightField/Mesh/ConvexHull
  hold a mutable `mGeometry`; primitives share unit geometry), triangle data is held CPU-side and
  lazily built into the transient UStaticMesh on first draw, `DrawGeometry` only accumulates
  (batch, transform, color) into per-(geometry, color) buckets, and EndFrame reconciles each bucket
  into one ISM component. `DrawLine`/`DrawTriangle`/`DrawText3D` stay immediate-mode — velocity
  vectors, transform axes and contact normals are genuinely line-shaped and low-count.
- Stale-bucket pruning: EndFrame drops a bucket whose `FBatch` refcount is 1 (only the bucket still
  holds it) — every Jolt geometry that referenced it is gone (shapes re-cooked across gym restarts,
  static-world re-bakes). Without the prune, the transient mesh + ISM component leak once per
  re-cook for the rest of the session.
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
  `_PIEStaticWorldMode`, `_CompoundShapeInstanceThreshold`, `_CookedDataRootPath`, and the
  Bake Filter block (`_BakeMobilityPolicy`, `_BakeExcludedActorClasses`, `_BakeExcludedActorTags`,
  `_BakeExcludedComponentTags`, `_BakeExcludedObjectChannels`, `_BakeExcludedCollisionProfiles`,
  `_BakeExcludeOverlapOnlyComponents`).
- **Startup-only CVars/cmdline**: `jolt.EnableParallelPhysics`,
  `jolt.EnableAsyncPhysicsUpdate` (cmdline-first).
- **Runtime CVars**: `ck.Jolt.DebugDraw.Enabled`, `ck.Jolt.DebugDraw.SleepColoring`,
  `ck.Jolt.DebugDraw.Opacity`, `ck.Jolt.DebugDraw.Velocity`, `ck.Jolt.DebugDraw.WorldTransform`,
  `ck.Jolt.DebugDraw.Constraints` (default on — anchors/axes/limits via `DrawConstraints`).
- **Unit conversion**: Jolt's `PhysicsSettings` defaults are METRES-tuned and this world is
  CENTIMETRES, so every length/velocity field is ×100 (squared manifold tolerances ×100²); ratios
  (`mBaumgarte`, `mLinearCast*`), iteration counts and times keep their defaults. Left unconverted,
  the 0.02 cm penetration slop keeps stacked bodies in permanent micro-jitter and the 0.03 cm/s
  sleep threshold makes stacks effectively unable to sleep — exposed by the Ck.Jolt
  BoxStackOfFiveSettlesAndStays test once it gated on real velocity quiescence.
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
- `FCk_Jolt_CharacterEntry` is field-split by owner: the in-fields (MoveVelocity, PushPolicy,
  JumpVelocity, HasJump) are written by the game thread in `FProcessor_JoltCharacter_PreStep`
  BEFORE the step is kicked; the out-fields are written by `DoStepCharacters_AnyThread` (task
  graph) and read by `DoApplyCharacterPoses_GameThread` AFTER the async step is waited. HasJump is
  the one field ARMED by the game thread and CONSUMED (cleared) by the step loop — both accesses
  are serialized by the WaitForAsync gate, so it is never touched concurrently. The step while-loop
  touches ONLY Jolt objects + `_PoseBuffer` + the character registry's Character pointers and
  out-fields.
- `UCk_Jolt_Subsystem::Deinitialize` CANNOT clear the `TSharedPtr<ck::FJoltWorld>` registry context:
  `FCk_Registry::SetContext` wraps entt `ctx::emplace` (try_emplace semantics — it does NOT
  overwrite) and there is no overwrite variant. That is safe — `FJoltWorld::Shutdown` nulls the
  non-owning Jolt pointers so the still-published world is inert, and the registry (with its
  context) is destroyed alongside the world during teardown.

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
