# CkJolt

**Purpose:** Owns the Jolt physics world — `JPH::PhysicsSystem` lifetime, JobSystem (Jolt's own
thread pool or single-threaded), broadphase/object-layer filters, thread-safe contact/activation
listeners, debug-render scaffolding, and the per-tick physics update. Everything Jolt-generic lives
here; features that *use* the Jolt world (CkSpatialQuery's Probe today) are consumers.

Extracted from CkSpatialQuery (2026-07-16, jolt-collision-world campaign Phase 0) with zero behavior
change. Campaign docs: `docs/campaigns/jolt-collision-world/` in the host project.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLog`, `CkResourceLoader` (StaticMeshAsset bodies
resolve their soft mesh through a rooted batch, consumer id `JoltBody.Setup`), `CkSettings`,
`CkThirdParty` (vendored Jolt 5.2.1).
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
  AND the mesh sits under a baked root AND its collision is worth pre-baking (`Get_IsUnderBakedRoot`
  is shared with the cooker and the on-save hook for the same reason). The STALE and ORPHAN ensures
  are NOT so gated — a present-but-wrong blob is always a defect, and it is consumed in LiveExtract
  PIE too. Cook via `-run=Ck_JoltCook_Commandlet -MeshShapes` (combinable with `-Map`/`-AllMaps`;
  the FULL class token is required — `-run=CkJoltCook` resolves to no class), the
  `Cook Jolt Mesh Shapes` Tools-menu entry, or automatically on saving a mesh under a baked root
  (per-user, `UCk_JoltCook_UserSettings_UE`); incremental by BodySetupGuid; orphans logged, never
  auto-deleted.
  **The cache memoizes MISSES and STALE results for process lifetime**, so anything that rewrites a
  mesh's cooked shape must call `mesh_shape_utils::Invalidate_CacheForMesh` — otherwise the running
  editor keeps serving the pre-cook answer until it restarts. The cook already does.
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
  **Read surface for the two bodies** (P8-D55): `Get_BodyA` / `Get_BodyB` return them as
  `FCk_Handle_JoltBody`, and `Get_IsBodyBWorldAnchor` tells "anchored to the world by design" apart
  from "body B's entity died". They exist because `FFragment_JoltConstraint_Current::_BodyA/_BodyB`
  are private behind a friend list of the constraint's OWN processors, and a presentation consumer
  that only wants to NAME the pair has no business being added to it — a read accessor on the utils
  is the doctrine-conformant spot, and widening the friend list was the alternative that was rejected.
  Body B is an empty handle for a world anchor and for a dead body alike; the flag is what separates
  them.
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
  **The pass is INCREMENTAL** (see § Incremental full pass): it walks every body but only DRAWS the
  ones whose pose, shape or colour-class flags changed since the last pass.
- `DrawActiveBodies` — per frame, `GetActiveBodies`; O(active), not O(all).
- `RecolorBodiesThatFellAsleep` — diff against the previous frame's active set (bounded by that
  count); skips keys the full pass already drew this capture, so nothing double-counts.
- `ReleaseDestroyedSleepingBodies` — a sleeping body is invisible to both passes, so its slots would
  outlive its destruction; one bounds-checked `TryGetBody` per sleeping body, no draw. **Gated on the
  body-removed revision** — an unchanged token means no body has died, so the walk is skipped whole.
- `DrawCharacters` — `ck::FFragment_JoltCharacter_Current` via the registry view; each
  `CharacterVirtual`'s own shape drawn through the shared cache (no per-frame geometry). Characters
  have no BodyID, so their slot keys are lifted clear of the BodyID keyspace.

Buckets hold **persistent body→instance slots** (`BodyID::GetIndexAndSequenceNumber()` → N
`FPrimitiveInstanceId`, N because a compound emits one DrawGeometry per child). An unchanged body
re-captures as `UpdateInstanceTransformById` only; a body whose colour class changed releases and
re-adds, moving it between buckets. Any slot that fails `IsValidId` drops the whole body to the
rebuild path.

**Two change tokens**, both owned by `FJoltWorld`, both monotonic, both passed to a capture as
`ck::jolt::debug_draw::FCaptureRevisions`:

- **Static-scene revision** (`_StaticSceneRevision`, bumped via
  `UCk_Jolt_Subsystem::Request_NoteStaticSceneChanged`) gates the full pass. Funnels: JoltBody Setup
  (Static motion type, or any body spawned `InitialSleepState::Asleep`), JoltBody EndPlay (Static),
  the static-world subsystem's batch add and its removal funnel, and a Teleport request on a Static
  body (it never activates, so only the full pass can notice it moved).
- **Body-removed revision** (`_BodyRemovedRevision`, bumped via `FJoltWorld::Request_NoteBodyRemoved`,
  or `UCk_Jolt_Subsystem::Request_NoteBodyRemoved` from a UObject-side caller) gates the dead-sleeping
  sweep. Every funnel that destroys a body bumps it, whatever the body's motion type — a sleeping
  dynamic is exactly the case the sweep exists for, and a static-only funnel would not cover it. The
  funnel set is `FProcessor_JoltBody_EndPlay` and CkSpatialQuery's `FProcessor_Probe_EndPlay` (a probe
  body is not a JoltBody, so nothing else in its teardown reaches here). The static-world subsystem's
  removals are deliberately NOT in this set: baked bodies are Static, so they are released by the full
  pass their static-revision bump re-arms, never by the sweep. **Any new site that destroys a Jolt body
  must bump this token**, and must bump the static-scene revision as well if the body is Static —
  `FProcessor_Probe_EndPlay` bumps both for a Static-motion probe. A target that has never swept sweeps
  once regardless, so a body destroyed before the target existed is still reconciled.

### Incremental full pass

The full pass keeps a `position + rotation + shape pointer (+ sensor flag + motion type)` record per
inactive body it drew. On the next pass a body whose record matches exactly is **not drawn at all** —
no `Shape::Draw`, no bucket lookup, no `UpdateInstanceTransformById`. So a scene-revision bump (a
streaming cell, a re-bake, an asleep-spawn) costs one cheap comparison per body plus real work only for
what changed. Pose equality is EXACT: any difference at all re-draws.

The sensor flag and motion type are in the record because they affect the retained bucket — motion can change
the colour class, while sensor state selects the translucent material independently of colour mode. A record that
compared pose alone could therefore leave a body painted with the old colour or opacity contract.
Consequence for the stats: **`_BodiesCaptured` counts bodies DRAWN, not bodies walked**, so a re-run
over an unchanged scene reports zero while still visiting every body.

Three things deliberately defeat the skip, because each would otherwise show stale state:

- the **highlighted** body is always re-drawn, since the full pass is also what produces its overlay
  (`Set_HighlightedBody` re-arms the pass precisely for that);
- `DrawActiveBodies` **erases the record** of any body it draws, so a body that woke, moved and
  settled back at its old pose cannot match a record written while it was in a different colour bucket;
- `Set_Palette` **clears every record**, because re-arming the pass alone would skip the very bodies
  that need repainting.

One colour input is deliberately NOT in the record: BakedStatic attribution, which is resolved through
the body's `FFragment_JoltStaticActor_Current` entity. It can only flip when that entity dies, and that
already routes the body through the static-revision funnel — an accepted, documented gap rather than a
per-body registry lookup on every comparison.

The record is only ever compared against the same BodyID, because the map is rebuilt wholesale each
pass. A recycled id landing on a body at an identical position, rotation, shape address and flags would
alias onto the dead body's record and be skipped — accepted.

### Measured cost (Ck.Jolt.DebugDraw.Benchmark.ScaleMatrix, 2026-08-15)

Headless Development editor, standalone `JPH::PhysicsSystem` + transient `UWorld`, N static boxes
sharing one shape + 1000 awake dynamics. Milliseconds, single sample except steady-state (mean of 10).
BEFORE = the pre-Phase-4 full pass (every inactive body re-drawn every pass); AFTER = incremental.

| Case | N=1k before → after | N=10k before → after | N=100k before → after |
|---|---|---|---|
| first full pass | 1.67 → 1.54 | 4.93 → 6.52 | 57.6 → 69.8 |
| steady state (1k active) | 2.06 → 2.73 | 2.28 → 2.72 | 2.07 → 2.59 |
| scene-revision re-run | 5.63 → 2.83 | 22.7 → 3.93 | **260.5 → 22.9** |
| `TryPick_Body` ×1 | 0.31 → 0.32 | 1.08 → 1.79 | 13.4 → 14.0 |
| selection change (re-armed pass) | 4.63 → 3.04 | 26.5 → 4.14 | **249.9 → 23.6** |

Read it this way: the two cases that were O(all bodies) of real reconcile work are now ~11x cheaper at
100k and are bounded by the WALK (GetBodies + a hash lookup + a compare per body), not by drawing. The
first pass is unchanged-to-slightly-worse by design — nothing is cached yet, and it now also writes a
record per body. Steady state was already O(active) and stays flat in N. `TryPick_Body` is untouched
and remains O(live instances). The 100k first pass and re-run are both far inside the spec's sanity
bounds (2000 ms), which are deliberately loose: the numbers are the product, and the bounds only catch
a change of algorithmic class.

**Phase-5 re-run (default flags), same machine, same day.** Colour modes, the class-index widening, the
hover class and the contact recorder cost nothing measurable on the default `Shape`-only path — every
case is within noise of, or faster than, the Phase-4 column above:

| Case | N=1k | N=10k | N=100k |
|---|---|---|---|
| first full pass | 1.12 | 4.39 | 56.4 |
| steady state (1k active) | 1.82 | 1.94 | 1.82 |
| scene-revision re-run | 2.01 | 2.89 | 16.9 |
| `TryPick_Body` ×1 | 0.23 | 1.56 | 13.1 |
| selection change (re-armed pass) | 2.06 | 2.81 | 17.6 |

⚠ **All per-body extras on, at 100k, is a different algorithmic class — say so rather than tune it.**
Same scene, `Shape | Velocity | AngularVelocity | WorldTransform | CenterOfMassTransform | BoundingBox |
MassAndInertia`:

| Case | N=100k, default flags | N=100k, ALL body flags |
|---|---|---|
| first full pass | 56.4 | **330.3** |
| steady state (1k active) | 1.82 | **307.6** |
| scene-revision re-run | 16.9 | **318.2** |
| `TryPick_Body` ×1 | 13.1 | 13.4 |
| selection change (re-armed pass) | 17.6 | **321.7** |

The steady-state row is the one that matters: **1.8 ms → 308 ms**, a ~170x jump, because extras are
LINES and lines are cleared every capture — so while any extra is on, the incremental pass cannot skip
a single body and every capture redraws all 100k. Pick is untouched (it walks instances, not flags).
This is not a regression to fix; it is the cost the pre-Phase-5 in-world `DrawBodies` path paid
unconditionally, now opt-in per flag. The practical guidance is the honest one: **per-body extras are
for a scene you are inspecting, not for a 100k world you are flying around in** — use the isolate and
selection tools first. The all-flags row is measured but NOT gated (`EBudgetPolicy::MeasureOnly`), since
holding it to the same sanity bound would either fail the gate or widen the bound past usefulness.

**Consumers.** `UCk_Jolt_Subsystem::Register_DebugDrawTarget` / `Unregister_DebugDrawTarget` (weak
storage, game thread). A target binds to ANY `UWorld`, including an `FPreviewScene` world: its ISMs
are plain `NewObject` + `RegisterComponentWithWorld`, owned by a `TStrongObjectPtr` on the bucket —
no ObjectPooling subsystem dependency, because preview/transient worlds host none and an actorless
component has no owner to root it. Each target subscribes once to `FWorldDelegates::OnWorldCleanup`
(the shared funnel for BOTH PIE end and map unload) and releases its components there, so it never
roots a dying world and stays reusable. Dropping demand (`Set_IsDesired(false)`) calls `HideAll`,
which clears instances AND invalidates the retained sets — a re-open rebuilds against fresh world
state rather than showing a frozen snapshot.

**Selection surface (JPH-free, for presentation consumers).** A consumer names ONE drawn body and the
facility does the rest:

- `ck::jolt::debug_draw::Make_BodyKey(uint32 IndexAndSequenceNumber)` and `Make_CharacterBodyKey(Handle)`
  are the ONLY definitions of the slot keyspace. The capture keys every body it draws through
  `Make_BodyKey`, so a consumer holding a `BodyID`'s index+sequence (or a `_BodyIds` entry off a
  JoltStaticActor fragment) names the same body without knowing the layout. Characters have no BodyID
  and are lifted clear by their own bit. **A key of 0 is a VALID body key** — consumers that need
  "no body" must use an unset optional, never a sentinel.
- `Set_HighlightedBodies(TArray<uint64>)` draws a SECOND instance of EACH named body in the dedicated
  `Highlight` colour class, alongside its normal one — N coexist because each overlay is slotted under
  its own `Make_HighlightKey`. Highlight has no population toggle, so a selection can never be hidden,
  and `TryPick_Body` skips overlay instances so a pick never returns a key no consumer can resolve.
  Only the bodies that LEFT the selection have their overlays released (immediately, not at the next
  capture); releasing all of them would blink every body already in a multi-selection each time one
  more is added. `Set_HighlightedBody(TOptional<uint64>)` is the 1-element convenience over it, and an
  unset key clears the selection.
- **The FIRST key is the PRIMARY.** It alone is sampled and it alone is asked for its contacts: both
  are per-body reads a debugger renders exactly one of.
- `Set_IsolatedBodies(TSet<uint64>)` / `Clear_Isolation()` — while the set is non-empty the capture
  draws ONLY those keys and RELEASES every other body's instances, across all four technique steps
  (inactive pass, active pass, sleep re-colour, characters). This is NOT class visibility: a hidden
  class keeps its instances and unhides instantly, an isolated-out body has none at all. Both calls
  re-arm the full pass, so a static or long-asleep body leaves and re-enters on the very next
  capture. Isolation and highlight COMPOSE — an isolated body keeps its overlay.
- `Set_HoveredBody(TOptional<uint64>)` is its subdued sibling, for a hover preview: same mechanism, its
  own always-visible `Hover` class, half alpha and a smaller swell. Independent of the highlight — a
  body may be both, and then it carries both overlays.
- **Both overlays are drawn at a SCALE about the body's centre of mass** (1.03 highlight, 1.02 hover),
  their buckets sit at `TranslucencySortPriority = 1`, and a normal-body highlight ignores the palette's
  opacity entirely. Sensor highlights remain translucent. Without the scale and deterministic ordering an overlay is co-planar, half-transparent geometry behind
  half-transparent geometry, which is exactly the "I selected it and nothing looks different" the user
  reported (P5-D41). The highlight colour is `{1.00, 0.15, 0.85}` — magenta, chosen to be near no entry
  of any mode's palette, which the `HighlightAddsOverlayInstance` spec asserts mode by mode.
- `Get_HighlightedBodyBounds()` reads the highlighted bodies' NORMAL instances — the UNION over a
  multi-selection — so a consumer can frame a selection on the same click that made it, with no capture
  in between.
- `Get_BodySample()` / `Get_CharacterSample()` are sampled BY THE CAPTURE, for the PRIMARY selection
  only, in the same async-safe window it draws from — they exist precisely so a Slate consumer never
  reads `PhysicsSystem` for a body scalar. Both are re-sampled from scratch every capture: unset when
  nothing is highlighted, and when the last capture did not draw the body (a static or sleeping body is
  only drawn on a revision pass). They are mutually exclusive — a key is a rigid body or a character.
  - `FCk_Jolt_DebugDraw_BodySample`: linear + angular velocity, mass (**0 = INFINITE** — Jolt stores an
    inverse mass and a static body's is 0), friction, restitution, gravity factor, motion type, motion
    quality, object layer, broadphase layer, sensor flag, `AllowsSleeping` (a `TOptional<bool>`, UNSET
    for a static body and only for one — `Body::GetAllowSleeping` dereferences `mMotionProperties`
    without a guard), world AABB, shape type + sub-type as display strings, and shape scale (unit for
    everything that is not a `ScaledShape`, which is the only Jolt shape carrying one).
  - `FCk_Jolt_DebugDraw_CharacterSample`: ground state, ground normal, ground velocity, ground body KEY
    (in the same `Make_BodyKey` space, so a consumer can select it), character velocity and up vector.
    Every ground getter lives on `CharacterBase`, not on `CharacterVirtual`.
- `Set_WantsSelectionContacts(bool)` + `Get_SelectionContacts()` — the primary selection's contacts, as
  a `NarrowPhaseQuery::CollideShape` of its own shape at its own centre-of-mass transform, run by the
  capture ON DEMAND and only while a rigid body is selected. Each entry is
  `{other body key, num contact points, penetration depth, contact points}`; the points and their
  normals are also drawn through the LINE channel under the `ContactPoints` / `ContactNormals` flags.
  Three settings carry the whole behaviour and none is a default: faces are collected explicitly (the
  default `NoFaces` would leave no contact POINTS to report), a small positive `mMaxSeparationDistance`
  is what makes the resting-on-the-floor case appear at all (a resting pair is not penetrating, and a
  NEGATIVE penetration depth is exactly that case, not an error), and an `IgnoreSingleBodyFilter`
  excludes the selection itself before the narrow phase ever runs. **The layer filters are the
  SELECTION's own**, taken off the physics system (`GetDefaultBroadPhaseLayerFilter` /
  `GetDefaultLayerFilter` for the body's `GetObjectLayer()`) and never accept-all ones: the question a
  contact panel answers is what this body is touching *in the simulation's terms*, and accept-all filters
  answer it with every body whose bounds the shape overlaps — a probe sensor, a layer it could never
  collide with. Facility-owned keys (`Get_DebugInternalBodyKeys`) are skipped on top of that, so the drag
  anchor can never be listed as a contact. Dropping the demand empties the list
  at once rather than leaving a superseded manifold readable. **This is not the ContactListener's
  record** (§ Contact recording) — that one exists only inside the solve; this is a query.
- `TryPick_BodyHit(Origin, Direction, OutKey, OutHitPointWorld, OutDistance)` — the pick, with the HIT.
  An oriented-box slab test is the broad phase over live instances; candidates then traverse a lazy
  per-geometry triangle BVH, and the nearest actual rendered-triangle hit wins. A broad concave terrain
  therefore cannot steal a pick through empty space inside its bounds. Hidden classes and both overlays
  are not pickable. **`OutHitPointWorld` is a point ON the picked body's rendered surface**
  and `OutDistance` is the WORLD distance from the origin to that point, so an unnormalized direction
  cannot make two picks incomparable. The slab distance is parametric along `InDirection`, and an
  affine instance transform preserves that parameter, which is what lets a local-space test be turned
  back into a world point by walking the original ray. A ray that starts inside a closed body reports its
  forward exit surface.
  Consumers that place something at the grab point (the debugger's Ctrl+LMB drag) need this and nothing
  less — a bounds centre is not on the surface the user clicked, and a wrong depth gives a drag spring a
  lever arm that spins the body.
- `TryPick_Body(Origin, Direction)` — the same pick, key only. A thin wrapper: the triangle test computes
  the hit either way, and a caller that only wants to know WHICH body should not have to declare three
  out-parameters to find out. Returns `TOptional<uint64>`.

⚠ **Changing the selection or the isolation set re-arms the full inactive-body pass**
(`_FullPassEverRan = false`), so a
static or long-asleep body gains its overlay on the very next capture instead of waiting for the scene
to change. The cost is one O(all bodies) WALK per selection change, deselect included — **measured at
23.6 ms for 100k bodies** (it was 250 ms before the pass became incremental), because the only body
that pass now draws is the newly selected one. Accepted and closed; the alternative is a selection that
silently fails to highlight the exact bodies a physics debugger is most often opened for.

**Colour + wireframe.** Buckets are per-(geometry, **colour-class INDEX**, sensor state) — the index and
sensor bit ride the bucket key alongside the packed colour, so two classes whose palette entries quantise to the
same 8-bit colour still land in distinct buckets, and a sensor cannot merge into an opaque nonsensor bucket in
SleepState/ObjectLayer/ShapeType modes. `FCk_Jolt_DebugDrawPalette::Get_Color(Mode, ClassIndex)` maps an index
to a colour, with a dim factor applied to the sleeping variant and the opacity the tint uses;
`Set_Palette` invalidates the retained capture so the next one repaints everything. Normal nonsensor bodies use the
shared DefaultLit opaque CkDebugScene material; sensors, hover and highlight use the shared Unlit translucent
CkDebugScene material; and wireframe uses the shared accessor for the Engine wireframe material. The material
library owns strong references and the two plugin materials carry their ISM shader usage in the asset.
All use the `Color` parameter. Sensor fill is capped at 0.45 opacity, and sensor highlight/hover stay below 1.0.
The base sensor uses translucency sort priority 0, its contact glow uses 1, and highlight/hover use 2 so overlap
colour remains stable instead of reverting to distance-dependent translucent ordering.
The render mode has three states: `Solid` leaves every fill without wireframe, `SensorWireframe` gives only a
normal sensor ISM the wireframe MID through `SetOverlayMaterial`, and `Wireframe` uses the wire material as every
bucket's primary material. The middle state adds one material pass for each sensor bucket but no component,
instance or geometry; both other states clear that overlay pass.
`CkDebugScene` owns the explicit cook and UFS staging rules for the two plugin materials and Engine wireframe asset;
the runtime Game build contains no cook-API dependency.
**Never** `GEngine->WireframeMaterial`: it is null whenever the platform `RequiresCookedData`.
`Set_RenderMode` swaps each bucket's material 0 between the two MIDs — zero geometry rebuild.
BakedStatic is distinguished from a Static-motion JoltBody by attribution entity, not by layer: both
share the Static object-layer DOMAIN, and only a baked body's user-data resolves to a
`FFragment_JoltStaticActor_Current`.

⚠ **`[PACKAGED-VERIFY]` — live rendering of all three shared debug materials.** The cook dependency is
explicit, but only a running packaged viewport can prove the final material appearance. Exact acceptance step:

1. Package a **Development** build (DeveloperTool modules are included there, Test/Shipping exclude them).
2. Run it, open the Jolt debugger window, and confirm nonsensor bodies render **opaque with shadowless form cues**.
3. Confirm every probe/sensor is clearly coloured and translucent in every colour mode and carries a readable wire outline.
4. Hover and select bodies; confirm the overlays remain visible, and a selected sensor never becomes opaque or black.
5. Cycle the wireframe control through **Off -> Transparent -> All**. Off has no outlines, Transparent outlines
   only sensor/probe fills, and All renders every body as wireframe; none rebuild geometry.
6. Check the log for `Failed to load WireframeMaterial` — the facility degrades to Solid and ensures
   loudly rather than drawing nothing, so a silent-looking pass with that line in the log is a FAIL.

On failure, first confirm the `CkDebugScene` cook rules admitted both plugin material packages and the Engine
wireframe package.

### Colour modes + legend

Colour is a **mode**, per target: `ECk_Jolt_DebugDrawColorMode { BodyClass (default), SleepState,
ObjectLayer, ShapeType }` via `Set_ColorMode` / `Get_ColorMode`. A mode decides which class
INDEX a body lands in, and the index decides the bucket — so changing the mode re-buckets everything.
`Set_ColorMode` therefore invalidates the retained capture exactly as `Set_Palette` does, and clears the
hidden-class mask (an index means something different in the new mode, so nothing hidden should stay
hidden) — **and re-applies `SetVisibility(true)` to every surviving bucket**, because visibility lives on
the COMPONENT and a bucket the old mode hid would otherwise stay invisible under a mask that hides
nothing (`TryEnsure_BucketIsm` only reads the mask when it CREATES a component).

**One index space, shared by every mode**, packed one bit per index into a `uint64` visibility mask —
so **64 classes is the ceiling**, `static_assert`ed. `HighlightClassIndex = 62` and
`HoverClassIndex = 63` are **mode-independent**: they are the same two indices whatever the mode, they
answer `Get_IsClassVisible` with `true` always, and `Set_ClassVisibility` ignores them.

| Mode | Classes | Notes |
|---|---|---|
| `BodyClass` | `ECk_Jolt_DebugDraw_ColorClass` — Static, Kinematic, Dynamic_Awake, Dynamic_Sleeping, Sensor, BakedStatic, Character | the pre-Phase-5 behaviour, unchanged |
| `SleepState` | `ESleepStateClass` — Static, Kinematic, Awake, Asleep | what `ck.Jolt.DebugDraw.SleepColoring` selects for the in-world target |
| `ObjectLayer` | one per registered Jolt object layer, capped at index 61 | see the naming note below |
| `ShapeType` | `EShapeTypeClass` — a COMPACT re-indexing of `JPH::EShapeSubType` | never index a table by the raw sub-type: it reserves `User1..8`/`UserConvex1..8` mid-range and appends `Plane`/`TaperedCylinder`/`Empty` at the end |

`Get_LegendEntries(Mode)` returns `{ClassIndex, Name, Color}` per class, with Highlight ("Selected")
and Hover ("Hovered") appended in every mode — that is the whole surface a debugger's legend needs, and
it is why `Get_BucketColorClasses()` returns bare indices (they mean nothing without the mode).
`Get_BucketColorClasses()` reports **live** buckets only: a previous mode's bucket survives in the map
whenever its geometry is one of the renderer's never-prunable shared unit primitives, and naming a class
nothing is drawn in would be a lie.

**Object layers have no names of their own** (P5-D61/S6). They are allocated one per unique
`FCk_Jolt_CollisionSignature`, so the legend borrows the **object channel** of the signature registered
at each layer — and the PROJECT's name for that channel (`UCollisionProfile::
ReturnChannelNameFromContainerIndex`), because `ECC_GameTraceChannel3` tells a reader nothing. The
capture publishes them with `Set_ObjectLayerNames` through a strictly READ-ONLY reverse lookup
(`Get_NumLayers` + `Get_Signature`; nothing there can register a layer), only while the target is
actually colouring by layer and only when the layer count moved. A layer with no name reads `Layer N`.
Index 61 is the catch-all and reads `Layer 61+`: P5-D42 phrased the cut as "> 61 → Other", but with the
two reserved indices there is no 63rd slot for a separate "Other", so the top named index doubles as it.
**When nothing has published any name** — no layer context reachable, which is every headless fixture and
any world whose Jolt subsystem never published one — the ObjectLayer legend would otherwise be a single
`Layer 61+` row beside a viewport visibly drawing layer-coloured bodies. So it also emits a bare
`Layer N` row for **every index a live bucket is currently using**, and only while the target's own mode
IS ObjectLayer (a bucket index means nothing in any other mode).

**A character keeps the BodyClass `Character` index in every mode.** A `CharacterVirtual` has no
`JPH::Body`, so object layer, island and sleep state do not exist for it; the alternative is a capsule
that vanishes into a meaningless bucket the moment the mode changes.

### Debug pause, single step, step duration

A pause of the JOLT world alone, independent of `UWorld::IsPaused()` — the engine's pause is set by the
PlayerController and stops the whole game, this one freezes physics so a debugger can inspect it. Both
are honoured by the same two step guards.

- `FJoltWorld::Request_SetDebugPaused(bool)` / `Get_IsDebugPaused()`, forwarded on
  `UCk_Jolt_Subsystem` as `Request_SetDebugPaused` / `Get_IsDebugPaused`.
- `Request_StepOnce()` permits **exactly one** step and then re-pauses. It is **ignored (Verbose) while
  the world is not debug-paused**, and a one-shot left unconsumed is DISCARDED on resume — otherwise it
  would fire at the start of the next pause and step a world the user had just frozen.
- **The one-shot is consumed in `FProcessor_JoltWorld_PlanStep`, never in the Step processor.**
  `TryConsume_DebugPauseGate()` answers "this frame plans zero steps" and eats the one-shot in the
  process; the Step processor READS the decision through `Get_StepOnceGrantedThisFrame()` instead of
  re-consuming it. A flag the Step processor interpreted for itself would bypass the accumulator model
  (`_Accumulator` / `_PendingSimTime` / `_NumStepsLastFrame`) entirely. A paused frame never reaches
  `ComputeStepPlan`, so a long pause cannot bank real time and burst on resume.
- **The world's own block is evaluated BEFORE the gate is consumed.** An invalid world or
  `UWorld::IsPaused()` blocks the step on its own, and PlanStep only reaches
  `TryConsume_DebugPauseGate()` when it does not — otherwise a step-once would be spent on a frame that
  steps nothing and the user's click would vanish. The one-shot therefore SURVIVES an engine pause and
  fires on the first frame the engine lets through.
- **A granted frame bypasses `ComputeStepPlan` entirely**: it sets `NumStepsLastFrame = 1` and
  `PendingSimTime = FixedDt` directly and leaves `_Accumulator` untouched. That is what makes "exactly
  one" true — through the plan, a granted frame would run `floor((accumulator + frame delta) / FixedDt)`
  steps, i.e. as many as the frame it happened to land on was long.
- `Get_LastStepDurationMs()` (world and subsystem) is the wall time of the frame's SOLVE — every
  `DoPhysicsUpdate` the frame ran, character stepping and pose capture excluded. Measured INSIDE the
  step loop so the async branch times the task-graph thread's own work rather than the hand-off, and
  stored in a `std::atomic<float>` with RELAXED ordering because that loop is off the game thread and
  the value orders nothing.

### Debug drag (dev-only, sim-mutating)

⚠ **The ONLY part of this facility that changes what the simulation does.** Everything else reads. It is a
development tool — a physics sandbox's "grab and throw" — and it is **AUTHORITY ONLY**: a drag taken on a client
moves a body the server corrects on the next replication, which reads as a bug rather than as a tool.

⚠ **The whole facility is compiled behind `#if !UE_BUILD_SHIPPING`** — the world's drag state and requests,
`FProcessor_JoltDebugDrag_Apply` and its `CK_REGISTER_PROCESSOR`, and the subsystem forwarders. A sim-mutating
debug tool has no business existing in a shipping binary, and there is no shim: a Shipping consumer does not
compile against these names at all. `Get_DebugInternalBodyKeys()` deliberately stays outside the guard — it is
the debug-draw facility's surface and simply reports an empty set when nothing can populate it.

`UCk_Jolt_Subsystem::Request_BeginDrag(BodyKey, WorldGrabPoint)` / `Request_UpdateDrag(WorldTargetPoint)` /
`Request_EndDrag()`, plus `Get_IsDragging()` and `Get_DragState()`, forwarding to the same names on `FJoltWorld`.
`InBodyKey` is the **debug-draw body key** — the one `TryPick_Body` returns — so a click picks and drags the same
body with nothing to convert in between. `WorldGrabPoint` is the one `TryPick_BodyHit` reports: the pick that
selects the body is also the pick that says where on it the user grabbed, so a consumer never has to reconstruct
a depth from a bounds centre (P7-D70/i).

- **The three requests QUEUE.** They arrive from a Slate click, and `FProcessor_JoltDebugDrag_Apply`
  (`FGroup_Transform`, `RunAfter` WaitForAsync + `FProcessor_JoltBody_KinematicPush`, `RunBefore`
  `FProcessor_JoltWorld_Step`) is what drains them — the same window the capture uses, which is the only point
  where mutating Jolt is safe in both sync and async modes. After KinematicPush for the same reason the Step
  processor is: the anchor is kinematic and that pass owns the kinematic writes.
- **`Apply_DragRequests` runs whether or not anything was queued**, because draining is only half of it. It also
  ends a drag whose BODY has been destroyed under it (`TryGetBody` answers null) — no request announces that, and
  the constraint has to go before either of its ends does — and it re-caches the grab point (below). An empty-queue
  early-out is exactly what made a destroyed dragged body a dangling constraint.
- **A key with any bit above the BodyID's 32 is REFUSED at `Request_BeginDrag`** (Verbose). Those are character
  and overlay keys; truncating one to a `BodyID` would grab an unrelated rigid body.
- **The mechanism** is a kinematic ANCHOR body plus a `DistanceConstraint` between the grab point on the body and
  the anchor, with `mMinDistance = mMaxDistance = 0` and soft limits (`FrequencyAndDamping`, 2 Hz, damping 1). The
  zero range is what makes the spring load-bearing: any separation at all is over the limit, so
  `mLimitsSpringSettings` is what closes it. `WorldSpace` at creation, because `LocalToBodyCOM` would mean
  subtracting `Shape::GetCenterOfMass()` for no gain.
- **The anchor's object layer is registered LAZILY, on the first drag** (P5-D61/S2), from a **default-constructed
  `FCk_Jolt_CollisionSignature`**: an all-zero response mask makes its pair interaction `Ignore` against
  everything, so Jolt's pair filter refuses every pair AND a channel query — which reads the same mask — cannot
  see it. `_Domain = Dynamic` is what keeps `ObjectVsBroadPhaseLayerFilter` from culling it out of the dynamic
  tree the dragged body lives in. Lazy because a world that is never dragged must not spend one of the layer
  table's fixed 1024 slots; exhaustion (`cObjectLayerInvalid`) drops the drag rather than putting the anchor on a
  colliding layer.
- **Dynamic bodies only.** A static or kinematic body is driven by the level or by the ECS transform, and a spring
  on it either does nothing or fights the writer that owns it — refused at Verbose, with no side effect.
- **The dragged body is activated on Begin AND on every Update**, not once: a body dragged slowly enough settles
  back to sleep mid-drag and would then hang off a spring it no longer responds to.
- **The anchor moves by `SetPosition`, not `MoveKinematic`.** It carries no momentum of its own — the spring is
  what produces the force, and a kinematic velocity would add a second one nobody asked for.
- **It is impossible to leave an anchor or a constraint behind.** `DoEnd_Drag` is idempotent and is the single
  teardown path: `Request_EndDrag`, a second `Request_BeginDrag` (one drag at a time), `FJoltWorld::Shutdown`
  (before the Jolt pointers are nulled) and the destructor all funnel through it.

**The anchor is a raw JPH body with NO entity**, so it must be invisible to every consumer surface —
`FJoltWorld::Get_DebugInternalBodyKeys()` publishes its debug-draw key and whoever pumps a capture pushes that set
onto the target (`Set_InternalBodyKeys`). The capture then skips it in all three BODY steps and `TryPick_Body`
refuses to return it, so it is never drawn, never picked, never listed. (The character pass needs no guard:
`Make_CharacterBodyKey` lifts characters clear of the BodyID keyspace, so a rigid body can never appear there.)
Becoming internal RELEASES a body's slots immediately rather than at the next capture — a static or sleeping body
might otherwise never be walked again. **A visible anchor is a bug, not a cosmetic issue.**

The drag LINE is not drawn by the facility: a consumer draws it through an External sub-channel (P7-D54), which is
what `Get_DragState()`'s grab point and anchor point exist for. **Both are CACHED values** — the grab point is
re-projected from the body-local one once per `Apply_DragRequests`, in the same game-thread window everything else
here mutates Jolt from, so it still tracks the body's rotation while the getter itself never reads a JPH body. A
getter that read the live transform would be a `PhysicsSystem` read from the Slate tick, which this module bans.
`FCk_Jolt_DebugDragState::_BodyKey` has no "no body" value (0 is a valid key), so only a state handed back by
`Get_DragState` means anything — and that one exists only while a drag is live.

### World stats

`FCk_Jolt_DebugDraw_WorldStats`, on the target, via `Get_WorldStats()`. Filled BY THE CAPTURE for everything the
physics system owns — the same rule that puts the body sample there — and split by what each field COSTS:

| Group | Fields | Cadence |
|---|---|---|
| SAMPLED | `_NumBodies`, `_MaxBodies`, `_NumStaticBodies`, `_NumDynamicBodies`, `_NumActiveDynamicBodies`, `_NumKinematicBodies`, `_NumActiveKinematicBodies`, `_NumSoftBodies`, `_NumConstraints` | every `WorldStatsSampleInterval` = **30** captures |
| LIVE | `_NumActiveRigidBodies`, `_NumActiveSoftBodies` (`GetNumActiveBodies`) | every capture |
| PUSHED | `_LastStepDurationMs`, `_ContactPairsLastStep` | pushed by whoever pumps the capture |

- ⚠ **The drag anchor is COUNTED in the sampled block.** `GetBodyStats()` walks the body manager, which knows
  nothing about the facility's internal keys, so while a drag is live `_NumBodies` / `_NumKinematicBodies` /
  `_NumActiveKinematicBodies` each read one higher than the world the user authored. Filtering it out would mean
  a second walk to subtract a number the panel refreshes every 30 captures anyway — disclosed rather than fixed.
- **The cadence is a CONSTANT** (P5-D61/S10), not a knob. `GetBodyStats()` is documented in Jolt's own header as
  "slow, iterates through all bodies" and `GetConstraints()` returns the constraint array **by value** with a
  refcount bump per element; at the campaign's 100k bar a per-frame sample would dominate the capture.
- **The staleness is published, not hidden**: `_SampleAge` is how many captures ago the sampled block was
  refreshed (0 on the capture that refreshed it), and a UI labels those fields "(sampled)". A throttled count is
  allowed to be LATE, never wrong.
- `_HasSample` distinguishes "no bodies" from "not asked yet".
- Jolt's body stats carry an active-soft-body count too; it is deliberately NOT mirrored into the sampled block,
  because the live one means the same thing and is always fresher.
- **The two PUSHED fields belong to the Jolt WORLD, not to its PhysicsSystem**, so the capture cannot reach them.
  `FProcessor_JoltDebugDraw_Capture` pushes them (`Set_StepStats`) before its captures, and the subsystem Tick does
  the same for the in-world target — which matters because the capture processor early-outs whenever no registered
  target is demanding, i.e. exactly when the in-world draw is the only thing capturing.
- **`_ContactPairsLastStep` is per STEP, not per frame.** The atomic lives on `FJoltWorld` — the object that owns
  the step that resets it, at the top of every `DoPhysicsUpdate` — and `CkContactListener` bumps it from
  `OnContactAdded`/`OnContactPersisted`, which run on Jolt WORKER threads. Relaxed on both sides: a lone
  diagnostic scalar that orders nothing. A frame running four sub-steps therefore reports the LAST sub-step's
  pairs; reporting all four as one step's is the number a stats panel would misread as a spike.

### Health scan — problem bodies (Phase 8)

The four ways a body in a shipping scene stops being physically meaningful, answered by the capture
so a debugger never has to read `JPH::PhysicsSystem` for them either.

```cpp
Target->Set_ProblemThresholds(FCk_Jolt_DebugDraw_ProblemThresholds{RunawayVelocityCmS, KillZ});
const TMap<uint64, ECk_Jolt_DebugDraw_ProblemFlags>& Flagged = Target->Get_ProblemBodies();
Target->Set_ProblemThresholds({});   // disarm
```

- `ECk_Jolt_DebugDraw_ProblemFlags` — `NaNTransform`, `NaNVelocity`, `RunawayVelocity`, `BelowKillZ`,
  `ZeroExtentBounds`. A bitmask: one body can be several kinds of broken at once.
- **OFF by default, and unset means OFF.** The scan is an extra O(active) walk; nothing pays for it
  while no consumer is showing the result. `Set_ProblemThresholds` also **drops the last verdict**, so a
  threshold change can never leave a body flagged by the old bar.
- **The two numbers are POLICY and CkJolt owns neither.** The runaway bar is a per-user debugger
  preference and KillZ belongs to `AWorldSettings` — both are pushed in. A facility that hard-coded
  "5000 cm/s" would be making a project decision inside a physics module.
- **`Scan_ProblemBodies` is its own capture step, not a hook inside `Draw_Body`.** The body passes SKIP an
  unchanged inactive body outright (§ Incremental full pass), and a scan that inherited that skip would
  answer "nothing is wrong" for every body the incremental pass did not touch.
- ⚠ **O(ACTIVE), so it is a scan of what is MOVING.** A body that fell out of the world and then fell
  asleep down there is not re-flagged — it is caught on the way down, which is when a debugger is
  watching. Widening it to every body would make it O(all) on a walk that is already O(all).
- Facility-owned bodies (the drag anchor) are excluded, like everywhere else.
- The verdict is refilled from scratch every capture, so **a flag clears the moment its condition does**.
- `ck::jolt::debug_draw::Compute_ProblemFlags(Position, Rotation, LinearVelocity, WorldBounds, Thresholds)`
  is the predicate, public and pure. It is public deliberately: a NaN cannot be INSTALLED on a live body
  from a test — `BodyInterface::SetLinearVelocity` clamps and every `MotionProperties` setter asserts
  (`JPH_ENABLE_ASSERTS` is on in every configuration) — so calling the predicate is the only way to pin
  the two NaN arms. `BelowKillZ` tests the AABB's **max** Z, so a body straddling the plane is still in
  the world.

### Contact recording

Contacts exist ONLY during `PhysicsSystem::Update` — there is nothing left to read once it returns — so
they are the one draw the capture cannot do for itself. The shape (P5-D40 as refined by P5-D61/S1):

- **Demand is the UNION over every LIVE target's contact flags**, recomputed on target construction,
  destruction and every `Set_DrawFlags`, and written into `JPH::ContactConstraintManager::
  sDrawContactPoint` / `sDrawContactManifolds` / `sDrawSupportingFaces`
  (`ContactPoints` / `ContactNormals` / `SupportingFaces` respectively).
- **The record scope lives inside `FJoltWorld::DoPhysicsUpdate`**, wrapped around `Update` — not in
  `FProcessor_JoltWorld_Step`, which never calls it. `Begin_ContactRecord` is a no-op when nothing
  demands contacts, so the whole feature costs one **acquire** atomic load per step when it is off.
- **Buffers are PER WORLD**, keyed by `JPH::PhysicsSystem*` and passed to `Begin`/`End`/`Replay`; a
  world drops its buffers in `FJoltWorld::Shutdown` and its destructor, so a PhysicsSystem later
  allocated at the same address can never inherit a dead world's contacts. **Only ONE world may record
  per step**: a `compare_exchange` on a single "recording world" pointer decides, and a second world
  whose solve overlaps SKIPS its own record for that step (one `Verbose` line) rather than feeding its
  targets a mixed one. A `DrawLine` carries no world, so this bounds the damage to the losing world's
  targets seeing nothing — it cannot stop the winner's buffer from also catching the loser's lines.
- **While recording, `FCk_Jolt_DebugRenderer::DrawLine` appends to an `FCriticalSection`-guarded
  buffer** instead of drawing. The recording atomic is tested **BEFORE** the bound target, and that
  order is the fix for the race it replaces: a bound target is a game-thread capture, but in async mode
  a *different* world's solve may be running right now, and appending to the target's unguarded line
  array from a solve worker tears it. Accepted consequence: a capture that overlaps another world's
  async solve loses its own lines to that record for the frame. Cap **200,000 lines/frame**, one
  `Display` warning when it bites.
- **`End_ContactRecord` double-buffers**: that world's filled buffer is swapped aside whole.
- **`FProcessor_JoltDebugDraw_Capture` is the ONE consumer per world**, on the game thread, already
  after `WaitForAsync`. It calls `Replay_RecordedContacts` BEFORE its captures (a capture flushes the
  line component), assigning the lines to every **demanding** target whose flags ask for contacts and
  CLEARING the channel of every target that does not — so a target that stops asking is emptied rather
  than left showing a step that has been superseded. The record is `MoveTemp`d into the LAST demanding
  target; only the ones before it copy. **Nothing replays from the Step processor.**

Two disclosures that are contracts, not caveats:

⚠ **Contact toggles are PROCESS-WIDE, unlike every other draw flag.** Jolt's contact draw switches are
plain `static bool`s on `ContactConstraintManager` with no per-`PhysicsSystem` variant. Turning contacts
off in one target turns Jolt's emission off for the whole process — including the in-world draw and
every other preview target. Per-target *replay* still holds (a target that did not ask receives
nothing), but per-target *emission* does not.

⚠ **Under async physics the contacts lag the shapes by one frame.** The record belongs to the step
consumed at the start of the frame that replays it. Accepted (P5-D61/S1) rather than fixed: the
alternative is reading Jolt from the Slate tick, which this module bans outright.

**The in-world draw gets contacts through `ck.Jolt.DebugDraw.Contacts`** (P5-D63 v). Its target is the
subsystem's own, pumped from `Tick` rather than registered with `Register_DebugDrawTarget` — and since
the replay has exactly one consumer per world, the capture processor feeds that target too
(`Get_DefaultDebugDrawTarget`) even though it never captures it. Closing the in-world gate resets those
flags to `Shape` on purpose: the contact flags drive process-wide statics, so a switched-off in-world
draw must stop declaring a demand or every world keeps recording manifolds nothing will draw.

⚠ **Open perf item (P5-D64/F6, DEFERRED, unmeasured):** `TryRecord_ContactLine` takes the record lock
**per line**, from inside the parallel solve. At manifold counts that reach the 200k cap that is a
contended lock on every Jolt worker for the duration of `Update`. It has never been measured. The
candidate fix is thread-local batching (append to a per-thread chunk, splice under the lock once at
`End_ContactRecord`) — **measure before redesigning**.

### Draw channels (lines, labels, External)

A target owns three non-mesh channels beside its bucket map:

- **Lines** — one `ULineBatchComponent` created on first line and registered with the target's world, owned
  by a `TStrongObjectPtr` exactly like the bucket ISMs (no pooling subsystem, so preview and transient
  worlds host it too). `FCk_Jolt_DebugRenderer::DrawLine` appends to the ACTIVE target's line buffer and
  `DrawTriangle` is three of those, so every non-virtual `DebugRenderer` helper — `DrawArrow`,
  `DrawCoordinateSystem`, `DrawWireBox`, `DrawWireSphere`, the constraint helpers — lands there for free.
  **Nothing goes through `DrawDebugLine` any more**: a line drawn for the preview target must not appear in
  the game world. `BeginCapture` `Flush()`es the component; `EndCapture` pushes the whole frame in ONE
  `DrawLines` call, because `ULineBatchComponent::DrawLine` marks the render state dirty per line.
- **Labels** — `DrawText3D` stores `FCk_Jolt_DebugDrawLabel {WorldPosition, Text, Color}` rather than
  rendering; `Get_Labels()` is refilled every capture. Jolt's `inHeight` is dropped: each consumer sizes
  text in its own space (a viewport `OnPaint` projection, or `DrawDebugString`).
- **External** — `Draw_ExternalLine/Box/Sphere/Arrow(FName Channel, …)` for line work this facility does
  not produce (probe results, a grid, a drag line). Sub-channels are **RETAINED and named**: a capture
  re-emits them into the line component **without clearing them**, and only `Clear_External(Name)` empties
  one. The asymmetry is the point — JPH output is per-frame, but a contributor pushes on its own schedule
  (a Slate tick), so a per-capture clear would drop anything pushed between captures and flicker anything
  pushed before one.

### Draw flags

`ECk_Jolt_DebugDrawFlags` (bitmask, `Set_DrawFlags` / `Get_DrawFlags` / `Get_IsDrawFlagSet`, default
`Shape`) decides what a capture emits into THAT target: `Shape`, `Velocity`, `AngularVelocity`,
`WorldTransform`, `CenterOfMassTransform`, `BoundingBox`, `MassAndInertia`, `Constraints`,
`ConstraintLimits`, `ConstraintReferenceFrames`, `ContactPoints`, `ContactNormals`, `SupportingFaces`,
`Labels`. The capture draws the per-body items itself — it never calls `DrawBodies`, because it needs one
`BeginBody`/`EndBody` scope per body to reconcile instance slots. Constraint draws
(`DrawConstraints` / `DrawConstraintLimits` / `DrawConstraintReferenceFrame`) run once per capture,
outside every body scope.

Three things worth knowing:

- **`Labels` gates TEXT, not a body item of its own.** The only label the capture emits today is the
  numeric mass beside the `MassAndInertia` wire box, and it needs BOTH flags: the box is
  `MassAndInertia`'s output, the number is text and text is `Labels`'. Without that gate `Labels` is
  dead and every `Get_Labels()` consumer is fed per-body strings it never asked for.
- **`SleepStats` is deliberately absent.** Jolt draws it from `MotionProperties`' internal sleep-test
  spheres, which sit below that header's `FOR INTERNAL USE ONLY` banner with no public accessor, and the
  capture does not call `DrawBodies`. There is no cheap way to get it — dropped, not deferred.
- **Assert-safety.** `JPH_ENABLE_ASSERTS` is on in every configuration (`CkThirdParty.build.cs`), so a
  static body must never touch a checked `MotionProperties` accessor. The extras use
  `Body::GetMotionPropertiesUnchecked()` and `MotionProperties::GetInverseMassUnchecked()`, gate
  `GetInverseInertiaDiagonal()` behind `IsDynamic()`, and never call `Body::GetAllowSleeping()` (it
  dereferences `mMotionProperties` unguarded).
- **Any per-body extra defeats the incremental skip.** Extras are LINES and lines are cleared every
  capture, so while one is on, the inactive-body pass runs every capture and re-draws every body rather
  than skipping the unchanged ones. That is exactly the cost the old in-world `DrawBodies` path paid
  unconditionally, so the in-world default is no worse; a target with the default `Shape`-only flags keeps
  the measured incremental behaviour intact.
- **Sizes are Jolt's own constants ×100.** Jolt's samples are metres and this world is centimetres, so the
  0.1 arrow-head / 0.2 axis constants are two millimetres of screen space. The pre-Phase-5 in-world draw
  passed them through unconverted, which is why `ck.Jolt.DebugDraw.WorldTransform` drew axes nobody could
  see. Same ×100 rule the module applies to every other Jolt scalar.

**The in-world draw is re-hosted onto the same capture.** The subsystem's Tick no longer builds a
`JPH::BodyManager::DrawSettings` and calls `DrawBodies`; it sets the default target's opacity and draw
flags from the CVars and calls `Capture_JoltWorld`, the same entry point the capture processor uses for
registered targets. The gate, the `HideAll()` on gate-close and `NextFrame()` are unchanged, and the CVars
remain the in-world source of truth:

| CVar | Effect after the re-host |
|---|---|
| `ck.Jolt.DebugDraw.Enabled` | unchanged — draws ALL bodies, static + dynamic; OR'd with the consumer gate (`Set_DebugDrawGate`, e.g. `ck.SpatialQuery.PreviewAllProbesUsingJolt`); skipped in async frames |
| `ck.Jolt.DebugDraw.Opacity` | unchanged — written into the default target's palette before each capture |
| `ck.Jolt.DebugDraw.Velocity` (default on) | sets **both** `Velocity` and `AngularVelocity`: Jolt's single `mDrawVelocity` emitted the linear AND the angular arrow, so mapping only one would silently drop a line this CVar has always drawn |
| `ck.Jolt.DebugDraw.WorldTransform` (default off) | sets `WorldTransform` — now at a visible size (see the ×100 note above) |
| `ck.Jolt.DebugDraw.Constraints` (default on) | sets `Constraints` |
| `ck.Jolt.DebugDraw.Contacts` (default off) | sets **both** `ContactPoints` and `ContactNormals` — "contacts" is one question to a user and Jolt emits the manifold normal from the same solve pass as the point. **PROCESS-WIDE**: this arms Jolt's contact emission for every world and every debugger preview at once (see § Contact recording). The lines reach the in-world target through the capture processor's replay, not through the subsystem Tick |
| `ck.Jolt.DebugDraw.SleepColoring` | selects the in-world target's **colour mode** — `SleepState` on, `BodyClass` off — rather than a draw flag, because it was always a colour question (`EShapeColor::SleepColor` vs `MotionTypeColor`). With it on, statics and kinematics collapse to one neutral colour each and the only distinction drawn is awake vs asleep |

⚠ **Body colours changed with the re-host.** `DrawBodies`' `MotionTypeColor` gave every dynamic body
`Color::sGetDistinctColor(index)` — a per-body colour with no meaning beyond identity. The capture colours
by the facility's palette instead (grey static, green kinematic, yellow awake, dimmed red sleeping, blue
sensor, tan baked-static, magenta character), which is the same vocabulary the debugger's legend reads.

`jolt.EnableParallelPhysics` / `jolt.EnableAsyncPhysicsUpdate` are startup-only because the JobSystem is
created once in `Initialize` (cmdline form: `-jolt.EnableParallelPhysics=0`).

**Multi-world:** every Jolt subsystem now builds its OWN default target, so a server+client PIE
session draws both worlds under the same CVars. The old first-world-only behaviour was an artifact of
gating construction on `JPH::DebugRenderer::sInstance` and is gone.

- WHY batched: `DebugRendererSimple`'s `DrawGeometry` fallback decomposes EVERY triangle of EVERY
  body into individual `DrawDebugLine` calls EVERY frame — hundreds of thousands of one-frame line
  submissions per frame on the game thread at stress-gym body counts. Instead `CreateTriangleBatch`
  runs ONCE per unique geometry (Jolt shapes cache their `GeometryRef` — HeightField/Mesh/ConvexHull
  hold a mutable `mGeometry`; primitives share unit geometry), triangle data is held CPU-side and
  lazily built into the transient UStaticMesh on first draw, `DrawGeometry` only accumulates
  (batch, transform, colour class) against the open body's slots, and `EndCapture` reconciles each bucket
  into one ISM component. `DrawLine`/`DrawTriangle`/`DrawText3D` stay line/text-shaped and go to the
  target's line and label channels — velocity vectors, transform axes and contact normals are genuinely
  line-shaped and low-count. One outward winding is emitted per source triangle; before the runtime fast mesh
  build, `FStaticMeshOperations` generates the tangent basis that DefaultLit materials require. `DrawGeometry`
  outside a body scope is DROPPED: instanced geometry only
  exists inside one, and nothing that runs outside a body scope (constraints, contacts) emits geometry.
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

**Specs**, all headless — a standalone `JPH::PhysicsSystem` + a transient `UWorld`, no PIE, no ECS
registry. Every row lives in `CkTests/.../UnitTests/CkJolt/Test_JoltDebugDraw_TargetReconcile.cpp`
**except `Benchmark.ScaleMatrix`, which is its own file** (`Test_JoltDebugDraw_Benchmark.cpp`):

| Spec | Pins |
|---|---|
| `Ck.Jolt.DebugDraw.TargetReconcile.StaticPassIsIdempotent` | full pass runs on first capture; an unchanged revision skips it entirely; a bumped revision re-runs it and touches NOTHING for unchanged bodies (0 added / 0 removed / 0 updated / 0 drawn) while a body that MOVED is re-drawn into the slot it already had (1 updated, 0 added, 0 removed) and the content bounds follow it |
| `Ck.Jolt.DebugDraw.SleepTransitionRecolors` | a body already asleep before the FIRST capture still draws, coloured sleeping; a body falling asleep later moves buckets (1 removed + 1 added, instance count stable) |
| `Ck.Jolt.DebugDraw.SingleTriangleBuild` | one Jolt source triangle becomes exactly one outward-wound UE triangle with finite normalized render normals; a coplanar opposite-wound duplicate cannot reappear and a lit material cannot receive zero tangent-space data |
| `Ck.Jolt.DebugDraw.MaterialSwap` | `Set_RenderMode` flips every normal bucket between the shared lit opaque material and wireframe and back, all through `Color`, with instance counts unchanged |
| `Ck.Jolt.DebugDraw.OverlayMaterial` | normal nonsensor geometry stays lit and opaque while hover and highlight use two translucent `Color`-tinted overlay buckets at half and full alpha |
| `Ck.Jolt.DebugDraw.SensorMaterialsAcrossColorModes` | sensor identity remains independent of BodyClass/SleepState/ObjectLayer/ShapeType colour; sensor fill, hover and highlight stay translucent on the shared ISM-ready material with sort priorities 0/2; Off has no wire overlay, Transparent outlines the normal sensor on its existing ISM, and All replaces every primary material without adding instances |
| `Ck.Jolt.DebugDraw.ClassPalette` | one body per colour class on ONE shared shape → one bucket per class, so the split is provably by class and not by geometry |
| `Ck.Jolt.DebugDraw.PreviewWorldCompat` | an `EWorldType::EditorPreview` world gets registered ISMs with correct instance counts, and zero ensures |
| `Ck.Jolt.DebugDraw.MultiTargetBatchPrune` | two targets sharing one batch: destroying one leaves the other's bucket and instances intact and still slot-reusing; the batch is pruned only once every Jolt geometry reference is gone |
| `Ck.Jolt.DebugDraw.ClassVisibility` | hiding a class is component visibility, not a capture skip: buckets and instances survive, a capture while hidden still updates the class, unhiding restores every visible component |
| `Ck.Jolt.DebugDraw.ContentBounds` | bounds track drawn content off the origin, widen with a far body, EXCLUDE a hidden class, and restore exactly on unhide without a re-capture |
| `Ck.Jolt.DebugDraw.HighlightAddsOverlayInstance` | the selection ADDS an instance in its own Highlight bucket rather than moving one; hiding the body's own class leaves the overlay visible; a moving selection updates body and overlay in place (0 added / 0 removed); clearing releases the overlay immediately |
| `Ck.Jolt.DebugDraw.HighlightedBodyBounds` | an already-drawn body yields selection bounds with NO re-capture, excluding the unselected body; a never-drawn body has none; clearing clears them |
| `Ck.Jolt.DebugDraw.PickNearestBody` | a ray through two bodies returns the nearer one and the answer FLIPS when fired from the other side (so it is not iteration order); a ray over everything misses; a hidden class falls through; the overlay is never what a pick returns. Plus the HIT half: `TryPick_BodyHit` agrees with the key-only pick, its point lies on the picked body's NEAR bounds face (a bounds centre or a far face fails), its distance is the world distance to that point, an unnormalized direction changes neither, and a miss reports no hit |
| `Ck.Jolt.DebugDraw.PickUsesVisibleTrianglesNotMeshBounds` | a high disconnected mesh whose combined bounds cover the ray cannot steal hover/click through its empty centre from the lower visible box; both key-only and detailed picks return the box's real top-face hit |
| `Ck.Jolt.DebugDraw.PickRespectsInstanceTransform` | exact triangle picking survives a translated, rotated, non-uniformly scaled instance and an unnormalized world ray; key, world hit point, and world distance all remain correct |
| `Ck.Jolt.DebugDraw.SelectionSampleIsCaptureOwned` | the sample belongs to the CAPTURE: unset before one, matching the body's velocity after it, following a re-selection to the newly selected body, and cleared the moment the selection is |
| `Ck.Jolt.DebugDraw.PauseAndStepOnce` | drives `FProcessor_JoltWorld_PlanStep::DoTick` (and the Step processor) against a real `FJoltWorld` published as a registry context: a fat frame plans MORE than one step while running and the step loop records a non-zero solve duration; a debug-paused frame plans zero, frame after frame; an ENGINE pause on top of a pending step-once plans zero and does NOT eat the one-shot, which is then granted on the first unblocked frame as EXACTLY one step of one fixed dt with the accumulator untouched — the leg that separates "one step" from "this frame's worth of steps"; the next frame plans zero again; a request made while running is not banked for the next pause and an unconsumed one is discarded on resume |
| `Ck.Jolt.DebugDraw.BodySampleFields` | the sampled fields are the SELECTED body's own — friction, restitution, gravity factor, object layer, shape type/sub-type, unit shape scale, a finite positive mass and a readable sleeping permission for a dynamic body; a static body reports INFINITE mass as 0 and its sleeping permission is never read (the assert-safety leg); a sensor reports itself as one; a rigid-body selection produces no character sample |
| `Ck.Jolt.DebugDraw.SelectionContacts` | nothing is queried until a consumer asks; two touching boxes then produce an entry naming the OTHER body and never the selection itself, carrying as many points as it counts; separating them empties the list and bringing them back refills it; dropping the demand empties it immediately, and with nothing selected there is nothing to query |
| `Ck.Jolt.DebugDraw.MultiHighlightAndIsolate` | N highlighted bodies add N overlay instances (not one), the primary is the FIRST key, and dropping one releases exactly one overlay immediately; the selection bounds cover the UNION; isolating one body releases every other body's instances and composes with the highlight (normal + overlay survive), and clearing isolation restores the whole population |
| `Ck.Jolt.DebugDraw.DestroyedSleepingBodyReleasesBothSlots` | the sweep is revision-gated (runs on the first capture, skipped while the body-removed token holds) and a destroyed SLEEPING body releases its own instance AND its selection overlay with the static-scene revision held still — so the full pass is provably not what covers it |
| `Ck.Jolt.DebugDraw.LineAndLabelChannels` | a JPH `DrawLine` during a capture lands in the target's line channel and the count RESETS on the next capture (so the per-capture `Flush` is real); a `DrawText3D` lands in `Get_Labels()` at the body it describes, and dropping the flag that produced it empties the channel; an External sub-channel SURVIVES two captures, a second sub-channel does not disturb the first, and `Clear_External` empties exactly one |
| `Ck.Jolt.DebugDraw.ColorModesAndLegend` | a mode change RE-BUCKETS: three bodies split three ways by body class collapse to two under `ShapeType` (the two boxes share a sub-type), and switching back restores the body-class split; `SleepState` splits the awake box from the asleep one, which differ in nothing else. Every mode's legend is non-empty, its names unique, and carries "Selected" and "Hovered"; with NO names published the ObjectLayer legend still carries a bare `Layer N` row for the index the bodies are actually drawn in, and once names are published it reads `Layer 0 — WorldStatic` for a named layer and a bare `Layer 1` for an unnamed one |
| `Ck.Jolt.DebugDraw.HoverOverlay` | hover and highlight are independent — two bodies, two overlays, two distinct always-visible classes; the hover class cannot be hidden; a body that is BOTH carries both overlays; clearing releases the hover overlay immediately |
| `Ck.Jolt.DebugDraw.ContactRecordingReplays` | a step with nothing demanding contacts records none and leaves Jolt's statics off; a target's `ContactPoints` flag arms the process-wide demand and writes `sDrawContactPoint` (leaving `sDrawSupportingFaces` off), and after the replay that target has contact lines while a second target that did not ask stays empty; a second target's `SupportingFaces` flag proves the union is over ALL live targets; dropping the last contact flag clears both statics and EMPTIES the channel rather than leaving stale contacts. The only case that steps — `FScopedJoltWorld::Step()` exists for it |
| `Ck.Jolt.DebugDraw.DrawFlagsGatePerBodyExtras` | `Shape` alone emits no lines even for a moving body; enabling `Velocity` emits some; adding `BoundingBox` emits strictly more; clearing back to `Shape` returns the count to zero with both bodies still drawn; dropping `Shape` releases every instanced-mesh instance while the line extras keep drawing |
| `Ck.Jolt.DebugDraw.DragMovesDynamicBody` | a queued drag request does NOT begin the drag — applying the queue does; the drag adds exactly one anchor body and one constraint and publishes one internal key; over 120 fixed steps the body ends at least HALFWAY closer to where it is being pulled (the discriminating leg — "the call did not crash" would pass with no spring at all), and the state names the dragged body and the anchor point; ending it returns the body count and the constraint count to their pre-drag values, empties the internal-key set and leaves no drag state; a STATIC and a KINEMATIC body are each refused with no anchor and no constraint left behind; a second `Request_BeginDrag` REPLACES the live drag (one anchor, one constraint, the state names the SECOND body); destroying the dragged body under the drag ends it on the next apply with no dangling constraint and no anchor; `Shutdown` mid-drag leaves neither either |
| `Ck.Jolt.DebugDraw.InternalBodiesAreInvisible` | the SAME world and the SAME ray, twice: without the internal set the anchor draws like any other body and a ray straight through it picks it BY KEY; with it, becoming internal releases its instances at once (no capture needed), a capture never draws it again, and the ray picks nothing |
| `Ck.Jolt.DebugDraw.StatsSampled` | nothing is sampled before the first capture; the first one samples (age 0) and its counts are the fixture's own population and budget; the pushed contact-pair field is zero until pushed and non-zero after, over two touching bodies actually stepped; then a body is added and for the next 29 captures the SAMPLED count stays stale (never wrong) while the un-throttled active count follows immediately, and the 30th refreshes it back to the truth |
| `Ck.Jolt.DebugDraw.ProblemBodiesFlagTheBrokenOnes` | two halves. The PURE predicate first (`Compute_ProblemFlags`), because a NaN cannot be installed on a live Jolt body from a test: a NaN position flags the transform, a NaN velocity flags the velocity and is NOT also reported as a runaway, a velocity past the bar is, a zero-extent box is flagged, a body wholly under KillZ has fallen out of the world while one STRADDLING it has not, and a healthy body is flagged with nothing. Then the capture path: an unarmed target scans nothing whatever the world is doing; armed, exactly the runaway body is in the map and the healthy one is not; slowing it down clears the flag on the next capture and speeding it up re-flags it; disarming empties the verdict on the spot and a capture with no thresholds leaves it empty |
| `Ck.Jolt.DebugDraw.Benchmark.ScaleMatrix` (`Test_JoltDebugDraw_Benchmark.cpp`) | measurement with loose sanity gates at N ∈ {1k, 10k, 100k}: first pass, steady state, revision re-run, pick, and a selection-change re-armed pass, logged as `[JoltDebugDrawBench] N=… case=… ms=…`. The numbers are the product; the assertions are deliberately wide bounds (steady state < 50 ms, everything else < 2000 ms) that only an algorithmic-class regression trips, so they gate without flaking. A sixth run repeats N=100k with EVERY per-body draw flag on (`allflags_*` cases) — **measured, not gated**, because redrawing every body every capture is a different algorithmic class by construction |

---

## Processor order (FGroup_Transform, after FProcessor_Transform_HandleRequests)

```
WaitForAsync ──> DrainEvents ──> PlanStep ──> SleepStateMirror ─┬─> DebugDraw_Capture ────────────────┐
     │                                                          ├─> KinematicPush ──> DebugDrag_Apply ┤
     ├──> JoltBody_Setup ──> JoltBody_HandleRequests ───────────┘                                     ├─> Step ──> WritebackInterpolated
     └──> JoltCharacter_Setup ──> JoltCharacter_HandleRequests ──> Character_PreStep ─────────────────┘
```

- `DebugDrag_Apply` is the one processor that MUTATES Jolt on the debug facility's behalf, and the one compiled
  out of Shipping. Same window as the capture (`RunBefore` Step, `RunAfter` WaitForAsync) plus `RunAfter`
  KinematicPush, so the drag anchor — itself a kinematic body — never lands in the middle of the kinematic
  writer's pass. It runs its body EVERY frame rather than early-outing on an empty queue: the apply is also
  where a drag whose body has been destroyed is ended and where the grab point is re-cached.
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
- **`_RestitutionCombineMode`** deserves its own line, because its Jolt default is WRONG for a
  project migrating off Chaos. Jolt combines a contact pair's restitution with `max(rA, rB)`;
  Chaos reads `UPhysicsSettingsCore::RestitutionCombineMode`, which is `Average` in a default UE
  project. Symmetric pairs agree either way, so nothing looks broken until two surfaces disagree —
  and there the gap is total: a restitution-1.0 bouncy material on a default-0.3 floor combines to
  1.0 under `max()`, a perfectly elastic collision that never stops bouncing and rebounds off any
  surface it is set down on. A physical material carries its authored numbers over from Chaos
  unchanged, so the MODE has to come over too — hence the `Average` default here. Pinned by
  `Ck_AutoTest_CkJolt_RestitutionCombinesAsAverageNotMax`. Two known limits: FRICTION is still on
  Jolt's `sqrt(fA * fB)` (differs from Chaos's average by a few percent on asymmetric pairs — not
  worth re-tuning every contact), and Jolt's combine is ONE GLOBAL FUNCTION whereas Chaos picks
  per-material via `bOverrideRestitutionCombineMode`, so a material that overrides its own mode is
  not honoured. Honouring it would mean keying a per-body mode off `Body::GetUserData` inside the
  combine lambda.
- **Startup-only CVars/cmdline**: `jolt.EnableParallelPhysics`,
  `jolt.EnableAsyncPhysicsUpdate` (cmdline-first).
- **Runtime CVars**: `ck.Jolt.DebugDraw.Enabled`, `ck.Jolt.DebugDraw.SleepColoring`,
  `ck.Jolt.DebugDraw.Opacity`, `ck.Jolt.DebugDraw.Velocity`, `ck.Jolt.DebugDraw.WorldTransform`,
  `ck.Jolt.DebugDraw.Constraints` (default on — anchors/axes/limits via `DrawConstraints`),
  `ck.Jolt.DebugDraw.Contacts` (default off — points + manifold normals; PROCESS-WIDE).
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
- Don't add retained ISM, material-instance, line-batcher, bounds, or exact-pick ownership back to Jolt debug
  draw. `FCk_Jolt_DebugDrawTarget` translates Jolt capture policy into the runtime `CkDebugScene` target, which is
  the sole retained geometry/channel/pick path shared with other debuggers.
- Don't read `JPH::PhysicsSystem` from a debugger/UI tick to build debug geometry OR to read a body
  scalar (velocity, pose, sleep state) — register an `FCk_Jolt_DebugDrawTarget` and let the capture
  processor fill it, or you race the async step. A value a presentation consumer needs live belongs on
  the target's JPH-free surface, sampled inside the capture, the way
  `Get_BodySample` is.
- Don't ask for an `Island` colour mode — it was DROPPED (P5-D66), and re-adding it against the vendored
  Jolt 5.2.1 would ship a mode that paints every body one colour. `MotionProperties::mIslandIndex` is
  only ever initialised and reset to `cInactiveIndex`; `SetIslandIndexInternal` has zero call sites in
  the whole library, and `IslandBuilder` keeps its indices on its own `BodyLink` array — so
  `GetIslandIndexInternal()` answers `cInactiveIndex` for every body however long the world has stepped
  (Jolt's own `EShapeColor::IslandColor` is equally dead there). Colouring by island needs a vendored
  patch calling `SetIslandIndexInternal` from `IslandBuilder`, i.e. a third-party divergence carried
  across every Jolt bump — a ruling, not a bug fix.
- Don't call `GetBodyStats()` or `GetConstraints()` per frame — both walk every body (and the latter
  returns the constraint array BY VALUE, refcount bump per element).
- Don't take a debug drag on a CLIENT world. It mutates the simulation, so the server corrects the body on the
  next replication and the tool reads as a bug. Authority only, and dev-only.
- Don't draw, pick or list a body from `Get_DebugInternalBodyKeys()`. The drag anchor has no entity behind it, so
  a consumer that shows one is showing a body nothing can name and handing back a key nothing can resolve.
- Don't apply a drag request anywhere but `FProcessor_JoltDebugDrag_Apply`. The requests exist BECAUSE a debugger
  click arrives on the Slate tick, and mutating Jolt from there races the step the capture is careful not to.
- Don't consume the debug-pause gate anywhere but `FProcessor_JoltWorld_PlanStep`. A second consumer
  eats the step-once one-shot and the single step silently never happens.
- Don't draw debug lines for a target through `UCk_Utils_DebugDraw_UE` / `DrawDebugLine`. Those go to the
  world's own line batcher, so a line drawn for the debugger's preview target would appear in the game
  world and vice versa. Every line belongs to a target's line channel — JPH primitives get there through
  `FCk_Jolt_DebugRenderer::DrawLine`, everything else through `Draw_External*`.
- Don't clear an External sub-channel from the capture. Its contributor owns it; a capture re-emits it and
  only `Clear_External(Name)` empties it. Clearing per capture is what makes probe results, the grid and
  the drag line flicker.
- Don't re-derive the debug-draw keyspace. `Make_BodyKey` / `Make_CharacterBodyKey` are its only
  definitions, and a hand-rolled cast will drift from the capture the first time either changes.
- Don't treat a colour-class INDEX as meaningful without the mode that produced it. `Get_LegendEntries`
  is the only thing that turns an index into a name and a colour, and index 5 is `BakedStatic` in one
  mode and `Cylinder` in another.
- Don't assume the contact flags are per-target the way every other draw flag is — Jolt's contact draw
  switches are process-wide statics, so the facility takes the union and discloses it.
- Don't replay recorded contacts from anywhere but `FProcessor_JoltDebugDraw_Capture`. The record scope
  runs on the step thread; every touch of a target belongs to the game thread.
- Don't include `CkJolt_DebugDrawTarget_Impl.h` outside the debug-draw TUs — it is what keeps the
  target's public header JPH-free for presentation consumers.

---

## See also

- `CkSpatialQuery/CLAUDE.md` — the Probe feature (Jolt world consumer, overlap semantics).
- `CkThirdParty` — vendored Jolt source + compile defines (JPH_DEBUG_RENDERER on, ObjectStream off).
