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
  gated in-world debug draw (skipped in async mode; may lag the step by one frame — group order is
  unpinned). Consumer-registered debug-draw targets do NOT go through Tick — they are pumped by
  `FProcessor_JoltDebugDraw_Capture` inside the async-safe window (see § Debug draw + stats).
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
- Debug draw, two surfaces: the in-world draw takes a consumer opt-in via `Set_DebugDrawGate`
  (CkSpatialQuery gates on its `PreviewAllProbesUsingJolt` user setting) — no gate = no draw; a
  presentation consumer instead owns an `FCk_Jolt_DebugDrawTarget` and registers it with
  `Register_DebugDrawTarget` / `Unregister_DebugDrawTarget`. Targets are demand-driven, bindable to
  any world, and never touch `JPH::PhysicsSystem` (see § Debug draw + stats).

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

**Front-end / target split.** Jolt permits exactly ONE `JPH::DebugRenderer` per process
(`JPH_ASSERT(sInstance == nullptr)` in its ctor), so the facility is split in two:

- `FCk_Jolt_DebugRenderer` (Subsystem/CkJolt_DebugRenderer.h) — the single instance, reached ONLY
  via `Get_OrCreate()` (module-level `TUniquePtr`, reset on `FCoreDelegates::OnEnginePreExit`).
  It owns nothing world-shaped: just the world-agnostic geometry/batch cache and a transient
  "active target" pointer for the current draw session.
- `FCk_Jolt_DebugDrawTarget` (Subsystem/CkJolt_DebugDrawTarget.h) — per-world retained state:
  the bucket map, its `UInstancedStaticMeshComponent`s and MIDs, render mode, palette, demand flag,
  and the persistent body→slot maps. Its public header contains **no JPH type at all** — it is
  `TPimplPtr`-pimpl'd exactly like `FCk_Jolt_QuerySession`, so a presentation module binds a target,
  flips demand/mode and reads counts without seeing Jolt. The Jolt-shaped guts live in
  `CkJolt_DebugDrawTarget_Impl.h`, included by the three debug-draw TUs and nothing else: CkJolt has
  no `Private/` dir (even .cpp files sit under `Public/`), so **inclusion, not path, is the privacy
  boundary**. Non-copyable; the dtor destroys its components.

**Capture pipeline.** `FProcessor_JoltDebugDraw_Capture` (Subsystem/CkJoltDebugDraw_Processor.h),
`FGroup_Transform`, `RunAfter` WaitForAsync + `FProcessor_JoltBody_SleepStateMirror`, `RunBefore`
`FProcessor_JoltWorld_Step` — the only window where Jolt state is stable in BOTH sync and async
physics modes (unlike the legacy subsystem-Tick draw below, which stays sync-only). It early-outs
when no registered target is demanding, so a closed debugger costs one map walk. The capture is a
`ck::Technique` pipeline whose step names carry the model:

- `DrawInactiveBodiesOnSceneRevisionChange` — the revision-keyed FULL pass over every **inactive**
  body: statics AND sleeping dynamics. Sleeping dynamics belong here because neither the active pass
  nor the sleep diff can see a body that was already asleep when the target began capturing — without
  them a settled pile is invisible until something wakes it. Keys not re-found are released.
- `DrawActiveBodies` — per frame, `GetActiveBodies`; O(active), not O(all).
- `RecolorBodiesThatFellAsleep` — diff against the previous frame's active set (bounded by that
  count); skips keys the full pass already drew this capture, so nothing double-counts.
- `ReleaseDestroyedSleepingBodies` — a sleeping body is invisible to both passes, so its slots would
  outlive its destruction; one bounds-checked `TryGetBody` per sleeping body, no draw.
- `DrawCharacters` — `ck::FFragment_JoltCharacter_Current` via the registry view; each
  `CharacterVirtual`'s own shape drawn through the shared cache (no per-frame geometry). Characters
  have no BodyID, so their slot keys are lifted clear of the BodyID keyspace.

Buckets hold **persistent body→instance slots** (`BodyID::GetIndexAndSequenceNumber()` → N
`FPrimitiveInstanceId`, N because a compound emits one DrawGeometry per child). An unchanged body
re-captures as `UpdateInstanceTransformById` only; a body whose colour class changed releases and
re-adds, moving it between buckets. Any slot that fails `IsValidId` drops the whole body to the
rebuild path.

**Static-scene revision** (`FJoltWorld::_StaticSceneRevision`, bumped via
`UCk_Jolt_Subsystem::Request_NoteStaticSceneChanged`) is the full pass's change token. Funnels:
JoltBody Setup (Static motion type, or any body spawned `InitialSleepState::Asleep`), JoltBody
EndPlay (Static), the static-world subsystem's batch add and its removal funnel, and a Teleport
request on a Static body (it never activates, so only the full pass can notice it moved).

**Consumers.** `UCk_Jolt_Subsystem::Register_DebugDrawTarget` / `Unregister_DebugDrawTarget` (weak
storage, game thread). A target binds to ANY `UWorld`, including an `FPreviewScene` world: its ISMs
are plain `NewObject` + `RegisterComponentWithWorld`, owned by a `TStrongObjectPtr` on the bucket —
no ObjectPooling subsystem dependency, because preview/transient worlds host none and an actorless
component has no owner to root it. Each target subscribes once to `FWorldDelegates::OnWorldCleanup`
(the shared funnel for BOTH PIE end and map unload) and releases its components there, so it never
roots a dying world and stays reusable. Dropping demand (`Set_IsDesired(false)`) calls `HideAll`,
which clears instances AND invalidates the retained sets — a re-open rebuilds against fresh world
state rather than showing a frozen snapshot.

**Colour + wireframe.** Buckets are per-(geometry, **colour class**) — the class rides the bucket key
alongside the packed colour, so two classes whose palette entries quantise to the same 8-bit colour
still land in distinct buckets. `FCk_Jolt_DebugDrawPalette` maps the class enum (Static, Kinematic,
Dynamic_Awake, Dynamic_Sleeping, Sensor, BakedStatic, Character) to a colour, with a dim factor
applied to the sleeping variant and the opacity the tint uses; `Set_Palette` invalidates the retained
capture so the next one repaints everything. Solid mode = `M_SimpleUnlitTranslucent`, wireframe =
`/Engine/EngineDebugMaterials/WireframeMaterial` — both loaded by direct `LoadObject` and held in a
`TStrongObjectPtr` (a bare function-local `UMaterial*` static dangles after the GC that collects it).
**Never** `GEngine->WireframeMaterial`: it is null whenever the platform `RequiresCookedData`.
`Set_RenderMode` swaps each bucket's material 0 between the two MIDs — zero geometry rebuild.
BakedStatic is distinguished from a Static-motion JoltBody by attribution entity, not by layer: both
share the Static object-layer DOMAIN, and only a baked body's user-data resolves to a
`FFragment_JoltStaticActor_Current`.

**The legacy in-world draw is unchanged.** CVars `ck.Jolt.DebugDraw.Enabled` (draw ALL bodies, static
+ dynamic, motion-type colors) and `ck.Jolt.DebugDraw.SleepColoring` (SleepColor mode: awake dynamics
yellow, sleeping red) gate the subsystem's own Tick draw — NOT registered targets, which are
demand-driven. The subsystem draws when the consumer gate (`Set_DebugDrawGate`, e.g.
`ck.SpatialQuery.PreviewAllProbesUsingJolt`) OR the Enabled CVar says so; skipped in async frames.
Every `ck.Jolt.DebugDraw.*` CVar now lives in the subsystem TU's `ck_jolt_subsystem::cvar` namespace
(including `Opacity`, which the Tick writes into the default target's palette before each
`BeginFrame`), following the house pattern — `FAutoConsoleVariableRef` over a static in a
filename-derived named namespace. `ck.Jolt.DebugDraw.Velocity` (default on) and
`ck.Jolt.DebugDraw.WorldTransform` (default OFF — line-heavy at stress counts) gate the remaining
immediate-mode lines. `jolt.EnableParallelPhysics` / `jolt.EnableAsyncPhysicsUpdate` are startup-only
because the JobSystem is created once in `Initialize` (cmdline form: `-jolt.EnableParallelPhysics=0`).

**Multi-world:** every Jolt subsystem now builds its OWN default target, so a server+client PIE
session draws both worlds under the same CVars. The old first-world-only behaviour was an artifact of
gating construction on `JPH::DebugRenderer::sInstance` and is gone.

- WHY batched: `DebugRendererSimple`'s `DrawGeometry` fallback decomposes EVERY triangle of EVERY
  body into individual `DrawDebugLine` calls EVERY frame — hundreds of thousands of one-frame line
  submissions per frame on the game thread at stress-gym body counts. Instead `CreateTriangleBatch`
  runs ONCE per unique geometry (Jolt shapes cache their `GeometryRef` — HeightField/Mesh/ConvexHull
  hold a mutable `mGeometry`; primitives share unit geometry), triangle data is held CPU-side and
  lazily built into the transient UStaticMesh on first draw, `DrawGeometry` only accumulates
  (batch, transform, colour class) into buckets, and EndFrame/EndCapture reconciles each bucket into
  one ISM component. `DrawLine`/`DrawTriangle`/`DrawText3D` stay immediate-mode — velocity vectors,
  transform axes and contact normals are genuinely line-shaped and low-count. Both windings are
  emitted per triangle (Conv is a handedness passthrough, so one winding renders inside-out).
- Stale-bucket pruning is a HOLDER CENSUS, not a refcount-of-1 test: the front end tracks, per
  `FBatch`, how many live buckets across ALL targets hold a keep-alive, and a bucket is dropped only
  when `Get_RefCount() == that count` — i.e. no Jolt geometry references it any more. The census is
  what lets two targets share one batch without either pruning it out from under the other. It
  applies ONLY to per-shape geometry (ConvexHull / Mesh / HeightField / TaperedCapsule); box, sphere
  and capsule primitives draw through the renderer's shared unit geometry, which it holds for its
  whole lifetime, so their batches are deliberately never prunable. Without the prune the transient
  mesh + ISM component leak once per re-cook for the rest of the session.
- Cycle stats under `STATGROUP_CkJolt` (`stat CkJolt` / Insights): `JoltWorld_Step` (whole
  fixed-step pump), `JoltPhysics_Update(_Async)` (the Update loop), `Jolt_DebugDraw_Capture` (one
  whole capture), `Jolt_DebugDraw_Reconcile` (bucket reconcile, both draw paths), `JoltBody_
  WritebackInterpolated`, `JoltBody_KinematicPush`, contact queue/drain stats.

**Specs** (`CkTests/.../UnitTests/CkJolt/Test_JoltDebugDraw_TargetReconcile.cpp`, all headless — a
standalone `JPH::PhysicsSystem` + a transient `UWorld`, no PIE, no ECS registry):

| Spec | Pins |
|---|---|
| `Ck.Jolt.DebugDraw.TargetReconcile.StaticPassIsIdempotent` | full pass runs on first capture; an unchanged revision skips it entirely (0 visited/added/updated); a bumped revision re-runs it and REUSES slots (added 0, removed 0, updated N) |
| `Ck.Jolt.DebugDraw.SleepTransitionRecolors` | a body already asleep before the FIRST capture still draws, coloured sleeping; a body falling asleep later moves buckets (1 removed + 1 added, instance count stable) |
| `Ck.Jolt.DebugDraw.MaterialSwap` | `Set_RenderMode` flips every bucket's material 0 between the solid and wireframe base materials and back, instance counts unchanged |
| `Ck.Jolt.DebugDraw.ClassPalette` | one body per colour class on ONE shared shape → one bucket per class, so the split is provably by class and not by geometry |
| `Ck.Jolt.DebugDraw.PreviewWorldCompat` | an `EWorldType::EditorPreview` world gets registered ISMs with correct instance counts, and zero ensures |
| `Ck.Jolt.DebugDraw.MultiTargetBatchPrune` | two targets sharing one batch: destroying one leaves the other's bucket and instances intact and still slot-reusing; the batch is pruned only once every Jolt geometry reference is gone |

---

## Processor order (FGroup_Transform, after FProcessor_Transform_HandleRequests)

```
WaitForAsync ──> DrainEvents ──> PlanStep ──> SleepStateMirror ─┬─> DebugDraw_Capture ─┐
     │                                                          ├─> KinematicPush ─────┤
     ├──> JoltBody_Setup ──> JoltBody_HandleRequests ───────────┘                      ├─> Step ──> WritebackInterpolated
     └──> JoltCharacter_Setup ──> JoltCharacter_HandleRequests ──> Character_PreStep ──┘
```

- `DebugDraw_Capture` is the one processor that only READS Jolt: it sits after SleepStateMirror (so
  the frame's activation events are already reflected) and `RunBefore` Step, which is what makes it
  correct in async mode too. No demanding target = immediate early-out.
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
- Don't construct a second `FCk_Jolt_DebugRenderer` — Jolt asserts on a second `JPH::DebugRenderer`.
  Always `FCk_Jolt_DebugRenderer::Get_OrCreate()`; per-world state belongs on a target, not a
  renderer.
- Don't read `JPH::PhysicsSystem` from a debugger/UI tick to build debug geometry — register an
  `FCk_Jolt_DebugDrawTarget` and let the capture processor fill it, or you race the async step.
- Don't include `CkJolt_DebugDrawTarget_Impl.h` outside the debug-draw TUs — it is what keeps the
  target's public header JPH-free for presentation consumers.

---

## See also

- `CkSpatialQuery/CLAUDE.md` — the Probe feature (Jolt world consumer, overlap semantics).
- `CkThirdParty` — vendored Jolt source + compile defines (JPH_DEBUG_RENDERER on, ObjectStream off).
