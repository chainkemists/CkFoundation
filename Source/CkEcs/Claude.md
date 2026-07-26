# CkEcs

**Purpose:** The Entity-Component-System core. Owns entities, fragments (components), processors (systems), the registry (world), signals, EntityScript lifecycle, context ownership, scheduling, replication, and the handle system.

**Depends on:** `CkCore`, `CkLog`, `CkMemory`, `CkProfile`, `CkSettings`, `CkThirdParty` (EnTT).
**Used by:** Every gameplay module.

---

## Terminology mapping

| CkFoundation | Classic ECS | Notes |
|---|---|---|
| Entity / `FCk_Entity` | Entity | Raw ID from EnTT; avoid holding raw entities — use handles |
| Fragment | Component | Data-only struct on an entity |
| Processor | System | Iterates entities with a fragment set; game logic lives here |
| Handle / `FCk_Handle` | Entity reference | High-level, validated entity reference |
| Registry / `FCk_Registry` | World | All entities + fragments |
| EntityScript | Actor-like scripting | `UObject`-derived; each entity can have a C++/Blueprint/AS script |

---

## Module layout

```
CkEcs/Public/CkEcs/
├── Handle/          – FCk_Handle, FCk_Handle_TypeSafe, FCk_Handle_ReadOnly
├── Entity/          – FCk_Entity raw type, entity utilities
├── Processor/       – TProcessorBase, TProcessor, parallel processors, deferred commands
├── Registry/        – FCk_Registry, registry utilities
├── Signal/          – signal macros + runtime system
├── EntityLifetime/  – create/destroy entity utilities
├── EntityScript/    – UCk_EntityScript_UE base + processor + utils
├── ContextOwner/    – entity context-ownership chain
├── OwningActor/     – actor-owned entity fragments
├── DeferredEntity/  – deferred entity creation
├── Fragments/       – shared built-in fragment types
├── Net/             – replication fragments, Iris driver, net mode policy
├── Persistence/     – save/load handler registry + hydration dispatch (Net → Persistence only)
├── World/           – ck::FEcsWorld (RAII private registry + slot)
├── Archetype/       – named feature amalgamations, typed archetypes, DebugFeatureFlags
├── Scheduler/       – processor ordering, debug data
├── Request/         – request struct base types
├── Concepts/        – ECS-specific C++20 concepts
├── Delegates/       – shared delegate declarations
└── Settings/        – ECS-level settings
```

---

## Handles

The handle system is the primary interface to entities. Never hold `FCk_Entity` directly in game code.

```
FCk_Handle           – generic handle, usable anywhere. Validates by checking the registry.
FCk_Handle_TypeSafe  – base for all feature-specific handles (FCk_Handle_AudioTrack, etc.).
FCk_Handle_ReadOnly  – non-mutating read access, used when a system should not drive state.
```

**Type-safe handles** (e.g. `FCk_Handle_AudioTrack`) must be declared in `_Fragment_Data.h`, never `_Fragment.h`. They inherit `FCk_Handle_TypeSafe`. This separation lets UHT process them without pulling in fragment implementation details.

```cpp
// _Fragment_Data.h
USTRUCT(BlueprintType)
struct MYMODULE_API FCk_Handle_MyFeature : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Handle_MyFeature);
};
```

Casting:

```cpp
auto TypedHandle = ck::StaticCast<FCk_Handle_MyFeature>(GenericHandle);
```

Validity: always `ck::IsValid(Handle)`, never raw null checks.

### Handle internals

- **`_Entity` / `_RegistryHandle` are `Transient` on purpose.** Both are runtime, session-specific values, meaningless across a save/load. CkSnapshot is the only system that persists a handle, and it does so by REMAPPING the entity id through `FSnapshotContext::Snapshot_Handle` (entt continuous-loader) — never by serializing the fields. `Transient` keeps them out of the whole-fragment data pass (which is NOT SaveGame-gated, see `CkSnapshot_Archive_Writer.h`), so a handle inside a dynamic fragment round-trips through the remap path instead of being double-serialized. Runtime copy/duplication (non-persistent archives) and replication (`FCk_HandleNetSerializer`) are separate, unaffected paths. `_RegistryHandle` replaced `TOptional<FCk_Registry>` and is re-homed onto the live registry by `Snapshot_Handle` on load.
- **`GetTypeHash(FCk_Handle)` must never key on validity.** The hash mirrors `operator==` exactly: a tombstone entity compares equal regardless of registry so it hashes without one; every other handle hashes entity + registry-slot fields (pure data, no resolution, safe on stale handles). Keying on validity made a live handle stored in a `TSet`/`TMap` change hash when its entity died — find/remove then missed the bucket and the container silently stranded the stale entry.
- **Transient-entity resolution.** `Get_IsTransientEntity` / `Get_TransientEntity` resolve through the `FCk_Registry` view, which reads its transient entity from the registry's ctx. The comparison is therefore registry-defined and works for any handle bound to a registry — no subsystem hop, no World-fragment dependency, no Initialize→OnWorldBeginPlay race window.
- **Handle remapping on save/load** — `snapshot::RemapHandles` was lifted verbatim from `ck_dynamic_snapshot::RemapHandles` so the dynamic-fragment serialize path and the save capture share one incident-hardened walker. Handle detection must be `IsChildOf`, NEVER exact struct equality: AngelScript-dynamic handles reflect as `FCk_Handle::StaticStruct()` but C++-native typesafe handles reflect as their own `UScriptStruct`, so equality silently skips every native typed handle (nothing written on save, restored as tombstones). `static_assert(sizeof(FCk_Handle_TypeSafe) == sizeof(FCk_Handle))` in `CkHandle_TypeSafe.h` is what makes base-pointer remapping complete. Container element counts/order are materialized by the preceding data pass before the walk runs on load.
- **`FSnapshotContext` layering.** `FCk_Handle::Get_Entity()` returns `FCk_Entity` (the wrapper around `entt::entity`) and `FCk_Entity::Get_ID()` exposes the raw id; `Snapshot_Handle` bridges those layers so call sites use `FCk_Handle`-family handles naturally. `ck::SnapshotRegistryType` aliases `ck::registry_table::EnttRegistryType` (same type as `entt::registry` today) so a future entity-id-type swap carries every snapshot template with it.
- **Lifetime-tag debug cache resets on remove.** `FEntity_FragmentMapper::Add_FragmentInfo` seeds `_LifetimeTag`/`_LifetimeTagName` when a lifetime tag is added; `Remove_FragmentInfo` resets both to the neutral "Valid" state when one goes away. Before that reset existed, the handle debugger kept reporting the tag the entity was born with ("JustCreated") forever — hours of false-positive chasing.

### AngelScript handle registration traps

- **Never pass a `TCHAR_TO_ANSI()` temporary to `FAngelscriptBinds::ExistingClass`.** `FBindString` stores the `const ANSICHAR*` by raw pointer, so the temporary dangles at end of statement while the binder is still used later in the loop. The freed slot is then reused by the `TCHAR_TO_ANSI()` of a method declaration, so the binder's object-type name reads back as the declaration string, corrupting `RegisterObjectMethod`'s object-type argument (`asINVALID_TYPE` → the engine's `configFailed` flag latches → every subsequent AS registration fails). Only bites packaged builds, where freed-slot reuse is deterministic. Pass the `FString` so the binder owns a copy.
- **Dynamic AS handle value-classes stash `FCk_Handle::StaticStruct()`** via `SetTypeUserData`. `FAngelscriptManager::GetUnrealStructFromAngelscriptTypeId` returns whatever is in `asITypeInfo::plainUserData`; without the stash it returns null for dynamic handles and the engine fork throws "Not a valid USTRUCT" on `FInstancedStruct::Make`, `FAngelscriptAnyStructParameter`, the struct printer, etc. `FCk_Handle` is the right target because every dynamic handle is binary-identical to it (the ValueClass is even sized `sizeof(FCk_Handle)`); boxed payloads round-trip by extracting an `FCk_Handle` and re-applying `.As_<TypeName>()`. Synthesizing a unique `UScriptStruct` per dynamic type would add UASStruct / class-generator coupling and per-type GC bookkeeping for no semantic gain.

---

## Processors

Processors are the only place game logic should live. They own the `ForEachEntity` loop over a fragment set.

```cpp
// Header
class FProcessor_MyFeature_DoThing
    : public ck::TProcessor<FProcessor_MyFeature_DoThing,
                            FFragment_MyFeature_Params,     // required fragment
                            FFragment_MyFeature_Current>    // required fragment
{
public:
    using Super = ck::TProcessor<FProcessor_MyFeature_DoThing,
                                 FFragment_MyFeature_Params,
                                 FFragment_MyFeature_Current>;
    using Super::Super;

    auto ForEachEntity(
        const FCk_Handle& InHandle,
        const FFragment_MyFeature_Params& InParams,
        FFragment_MyFeature_Current& InCurrent) -> void;
};

// Implementation
auto
    FProcessor_MyFeature_DoThing::
    ForEachEntity(
        const FCk_Handle& InHandle,
        const FFragment_MyFeature_Params& InParams,
        FFragment_MyFeature_Current& InCurrent)
    -> void
{
    // main logic
}
```

Key `TProcessorBase` API (all processors inherit):

- `Tick(InDeltaT)` — advance the processor one tick.
- `Pump()` — tick with zero DeltaT (process deferred requests without advancing time).
- `_TransientEntity` — a scratch entity the processor owns; use for deferred commands.

### Fixed tick rate (compile-time trait)

A processor with a per-type fixed cadence declares ONE line; the base derives everything else
(throttle, every-tick fast path, catch-up, registration — all unchanged):

```cpp
class FProcessor_X : public ck::TProcessor<FProcessor_X, /* fragments */>
{
public:
    static constexpr FCk_Time TickRate = ck::time::Hz(4);      // or ck::time::Seconds(0.25)
};
```

- Declaring nothing = every tick (the default; byte-identical to before the trait existed).
- `ck::time::Hz` / `ck::time::Seconds` (`CkCore/Time/CkTime.h`) are consteval `FCk_Time` factories.
  Misuse is a compile error: a zero/negative rate is rejected by the consteval factory; a raw number or any
  non-`FCk_Time` type, and non-static/non-constexpr spellings, fail static_asserts in `Get_TickRate`.
- Cadence is fully compile-time — there is no runtime `Set_TickRate` / `Set_TickPhaseOffset`.
- Optional: `static constexpr auto TickCatchUpPolicy = ECk_ProcessorTickCatchUp::SampleLatestOnly;`
  fires DoTick once with summed elapsed intervals after a hitch instead of replaying per interval
  (default `ReplayMissedTicks` = fixed-timestep replay, the original behavior).
- A rated processor's accumulator freezes while the scheduler's empty-view skip bypasses its
  dispatch — its phase re-aligns to when its view last became non-empty.

For PER-ENTITY intervals (each entity chooses its own rate), don't poll a chrono per entity — use
the bucketed-cadence primitive in `Processor/CkProcessor_CadenceBuckets.h` (quantize-at-Add into
per-bucket sub-processor instantiations; authoring recipe below; reference
consumer: `CkVisibleRange`; design: `DESIGN_SubInstancedCadenceProcessors.md`).

#### Cadence-bucket authoring recipe

Declare ONE class template over the bucket index:

```cpp
template <int32 T_BucketIndex>
class FProcessor_MyFeature_Bucket
    : public ck::TCadenceBucketProcessor<FProcessor_MyFeature_Bucket<T_BucketIndex>,
                                         FProcessor_MyFeature_Bucket, T_BucketIndex,
                                         FCk_Handle_MyFeature,
                                         ck::TReadOnly<FFragment_MyFeature_Params>,
                                         ck::TReadWrite<FFragment_MyFeature_Current>,
                                         CK_IGNORE_PENDING_KILL>
{
public:
    using Group = ck::FGroup_Gameplay;
    using BucketBase = typename FProcessor_MyFeature_Bucket::TCadenceBucketProcessor;
    using BucketBase::BucketBase;   // unqualified lookup never examines a dependent base

    // shared, spelled with CONCRETE types — the base's TimeType/HandleType aliases
    // are not visible unqualified inside the consumer template
    static auto ForEachEntity(FCk_Time, FCk_Handle_MyFeature, const FFragment_MyFeature_Params&,
                              FFragment_MyFeature_Current&) -> void;
};
```

- Define `ForEachEntity` as a template in the .cpp with `cadence::TryConsume_FirstEval<Handle>` as its FIRST statement, then put `CK_REGISTER_CADENCE_BUCKET_PROCESSORS(ck::FProcessor_MyFeature_Bucket)` BELOW that definition (implicit instantiation needs the body).
- The feature's `Add` calls `cadence::AddCadenceTags<Handle>(Handle, Interval)`.
- Quantization is toward FASTER. Vacant buckets are near-free (the scheduler's empty-view skip). Bucket 0 declares no `TickRate`, so it is byte-identical to an ordinary every-tick processor; immediate-first-eval folds into bucket 0 via a transient bucket-0 membership + first-eval tag that `TryConsume_FirstEval` strips together.

Access policies (see `CkProcessor_AccessPolicy.h`) control whether a fragment is read-only or read-write for a given processor, enabling safe parallel execution.

### Pump policy (`PumpPolicy`)

Processors with `MarkedDirtyBy` are pump-eligible by default — the scheduler invokes `Pump()` (DoTick with `DeltaT=0`) in additional passes after the main Tick so cascading reactive work drains in one frame instead of slipping per-stage.

This is correct only when the processor's body **consumes/removes the marker** (e.g. `FTag_*_NeedsSetup` removed by Setup; `FFragment_*_Requests` drained via `CopyAndRemove`). If the marker is sticky and the processor is not idempotent w.r.t. `DeltaT`, repeated pump passes re-apply cached state and multiply observed work.

Time-stepping consumers (apply-offset, anything that reads a per-frame integration result and enqueues a side effect) must opt out:

```cpp
class FProcessor_X : public ck_exp::TProcessor</* ... */>
{
public:
    using MarkedDirtyBy = FTag_Sticky;
    static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;
};
```

`SkipPump` keeps the dirty-marker metadata for diagnostics + scheduler edges, but the pump phase is bypassed.

`SkipPump` is required only when BOTH hold: (1) the processor reads a STICKY marker it does NOT consume (canonical: `FTag_EulerIntegrator_NeedsUpdate`, present for as long as the entity participates in integration), AND (2) it is not idempotent w.r.t. `DeltaT` — e.g. an apply-offset consumer that enqueues a `Request_AddLocationOffset` every run, where pumping re-applies the same `DistanceOffset` and multiplies observed motion by the pass count. Either condition alone is fine.

The pump short-circuit is **load-bearing, not an optimisation**: `Has_AnyEntityWith` reports a tombstone-only (`in_place_delete`) marker pool as non-empty forever after first use, so without the version compare every ever-used marker processor phantom-pumps every frame. Per-hash versions are monotonic, so their sum is too. `DoPump` stores the PRE-pump version deliberately — work the pump added recursively (EntityScript `Construct` → `DefineState` → `AddTask` queueing a SpawnEntity request) must still be observed by the next pass; storing the POST-pump version absorbs it into `LastSeen` and defers it a frame (the cascade bug). A pump that provably visited zero entities does not count as work when scheduling further passes; `-1` means a custom `DoTick` reports no count and is treated conservatively as work.

**Dynamic (script-struct) dirty markers** participate in the same version-counter mechanism, in an FName-derived hash domain: the CkDynamic mutation paths call `FCk_Registry::BumpDirtyMarkerVersion` with `UCk_Utils_DynamicFragment_UE::Get_DirtyMarkerHash`, and that is the value a script-processor host must register on its descriptor. Historically script processors registered a hash nothing ever bumped and were silently pump-deaf — if a script processor stops reacting to a dynamic fragment mutation, check that the two hashes agree.

### Paced work (`CkPacedWork.h`)

The pacer's marker is whatever fragment the consumer declares as `MarkedDirtyBy` (never with `SkipPump`). Simple case — the marker IS `ck::FFragment_PacedWork`; `Add` it to the work entity to start. Existing-queue case — the marker is a work-queue fragment every enqueue path already touches (so each enqueue re-marks dirty): pass that type as `RunPacedSteps<TMarkerFragment>` and keep the `FFragment_PacedWork` pacer separate (`AddOrGet` internally, budget only).

### Empty-view skip (`EmptyViewPolicy`)

The scheduler's main pass skips dispatching any eligible processor whose view is **provably empty** — some required fragment/tag type of its view has zero live entities — bypassing the Tick call, view construction, tombstone walk, and per-processor trace/stat overhead. Tracking mirrors the pump short-circuit: per-include-type mutation version counters + a per-node cached verdict; the tombstone-aware storage scan re-runs only on change. The check runs at the node's dispatch position, so an include added earlier in the same frame wakes the processor the same frame.

Eligibility is automatic and conservative: only processors whose `DoTick` is the template-generated view iteration participate (detected at registration — a custom `DoTick` body may do non-view work a skip would drop). `TParallelProcessor`, composites, and script processors never qualify. Opt out per-processor:

```cpp
static constexpr auto EmptyViewPolicy = ECk_ProcessorEmptyViewPolicy::AlwaysTick;
```

Toggle: `_EnableEmptyViewMainPassSkip` (ECS project settings, default on, cached at scheduler construction). Dev tripwire: `ck.Scheduler.VerifyEmptyViewSkip 1` re-scans every skipped node and ensures the verdict still holds (catches registry writes that bypass the mutation counters).

Processor scripts (`CkProcessorScript_UE`) are a Blueprint/AS-scriptable wrapper around a processor. Use them when artists or designers need to author behaviour without writing C++.

### Descriptor derivation (registration-time)

- **Fragment access.** `BuildDescriptor` relies on the `static_assert` in `ck::TProcessor` / `ck_exp::TProcessor` that every non-excluded, non-empty fragment is wrapped in `TReadOnly<T>` / `TReadWrite<T>`. FragmentList entries keep their declaration order; each non-skipped entry maps `TReadOnly<F>` → the RO hash bucket, `TReadWrite<F>` → RW. Hashes use `entt::type_hash`, so they agree with any consumer speaking EnTT hashes.
- **Generated-`DoTick` detection** (`Get_HasGeneratedViewDoTick`, the empty-view-skip eligibility gate): name lookup on `&T_Processor::DoTick` resolves to the DERIVED declaration when one exists (name hiding), and a pointer-to-member-of-derived does not implicitly convert to pointer-to-member-of-base — so target-typed direct-initialization against the base's member-pointer type compiles only when the name still means the inherited (generated) `DoTick`. `TProcessorBase` types without the `GeneratedDoTickHost` alias (`TParallelProcessor`, fully custom processors) are ineligible outright.

### Scheduler diagnostics

- **Stat nesting.** Per-processor `STAT_Tick`/`STAT_ForEachEntity` scopes (`STATGROUP_CkProcessors`) nest INSIDE the `STATGROUP_CkScheduler` scopes, so each CkScheduler scope's SELF-time is the gap work around processors (dispatch loop, dev-build timing capture, pump passes, per-pump dirty rescans). `Scheduler::EmptyViewCheck` self-time is the total cost of DECIDING to skip — weigh it against the Dispatch time the skipped processors no longer spend. `Scheduler::DebugRecord` is carved out of Dispatch self-time so the redundant dev measurement is visible separately from inherent dispatch cost (node fetch + `entt::poly` call); zero when DebugTiming is off.
- **Insights scope per processor** (`CPUPROFILERTRACE_ENABLED`), named by the node (C++ canonical type name, or the script host's `script::<DevClass>` display name). This is what decomposes the ECS world actor's tick on the trace timeline — the stat system's per-processor cycle counters emit NO trace events unless `-statnamedevents` is on. Near-free when the `cpu` channel is off (one branch); the event spec is created lazily on first traced dispatch and cached on the node. The pump reuses the same spec id, so pump invocations show as extra instances of the processor's timer rather than a separate row.
- **`ck.Scheduler.DebugTiming`** (default ON) gates the Scheduler Debugger's per-processor wall-clock timing. Costs two `FPlatformTime::Seconds()` (QueryPerformanceCounter) calls per processor per frame and is REDUNDANT with the stat system; on machines with slow QPC it is the dominant unattributed cost inside `Scheduler::Dispatch`. Set it to 0 while profiling.
- **Pump-pressure log throttle** (`GCk_Scheduler_PumpWarningThrottleSeconds = 5.0s`): a persistently over-budget frame used to log every frame. The live signal now belongs to CkWatermark; the logs are retained for dedicated/headless servers where no watermark renders. A change in the still-dirty SET bypasses the throttle so a newly-stuck processor surfaces promptly. The per-processor breakdown must ride INSIDE the header message, not as N follow-up Warnings — CkTests' AutoTest-runner default-suppression list matches one `Contains` pattern against a whole log ENTRY, so a separate `"  - [Name]"` line is its own entry, slips the pattern, and fails otherwise-passing tests. One entry also collapses N+1 trips through the log pipeline.
- **Graph export** (dev builds only, `CkEcsWorld_Subsystem.cpp`): `Ck.Ecs.Scheduler.ExportGraph [path]` dumps the processor graph (one subgraph cluster per tick group) as Graphviz DOT — render with `dot -Tsvg SchedulerGraph.dot -o SchedulerGraph.svg`; default `<ProjectSaved>/CkEcs/SchedulerGraph.dot`. `Ck.Ecs.Scheduler.ExportOrder [path]` dumps the final topologically-sorted execution order (one section per tick group) as plain text; default `<ProjectSaved>/CkEcs/SchedulerOrder.txt`. An absolute argument is treated as absolute, a relative one as relative to the project root.

---

## Processor group pipeline order

Group processors define the top-level ordering; a processor joins one with `using Group = FGroup_X;`. The `RunAfter` chain in `CkProcessorGroups.h` is the authority — this roster is the human-readable view of it. All `TG_PrePhysics` unless noted.

```
FGroup_DestructionPipeline  (OwningActor_Destroy, DestructionPhase_Finalize, DestructionPhase_Await —
                             start-of-tick destruction completion for entities already past EndPlay)
  -> FGroup_Gameplay_TimeDelta   (Timer, Tween, Substep)
    -> FGroup_Gameplay           (core gameplay: Interaction, Inventory, Objectives, ...)
      -> FGroup_Gameplay_AI      (StateMachine)
        -> FGroup_Gameplay_Audio (AudioDirector, AudioTrack, Sfx, Vfx, VfxCue)
          -> FGroup_Gameplay_Rendering (ISM, PMG, Shapes, RenderStatus, Camera)
            -> FGroup_Gameplay_Script  (EntityScript construction + BeginPlay)
              -> FGroup_Gameplay_Chaos (GeometryCollection destruction)
                -> FGroup_Physics
                  -> FGroup_Transform_SyncFrom (SyncFromActor, SyncFromMeshSocket, Interpolation)
                    -> FGroup_Transform        (HandleRequests, SceneNode, Tween)
                      -> FGroup_Transform_Finalize (SyncToActor, FireSignals)
                        -> FGroup_Gameplay_Camera  (compose/POV/apply — reads anchors AFTER they
                                                    are synced this frame)
                          -> FGroup_PostTransform  (OverlapBody, RaySense, UI, ...)
                            -> FGroup_DeferredApply (replicated-fragment + save-load hydration
                                                     dispatch — applies payloads after composition,
                                                     before replication-complete fires)
                              -> FGroup_Replication
                            -> FGroup_EntityLifecycle (entity create + DestructionPhase_Endplay —
                                                       adds the EndPlay tag)
                              -> FGroup_EndPlay       (all feature-level *_EndPlay processors +
                                                       EntityScript_EndPlay; CK_IF_END_PLAY matches
                                                       here because EndPlay is set and Teardown is not)
                                -> FGroup_Teardown    (DestructionPhase_Teardown — adds the Teardown
                                                       tag, ending the EndPlay window)

FGroup_Overlap (TG_PostPhysics)
```

### Destruction pipeline, tick by tick

- **Tick N** (the tick `Destroy()` is called): the entity gets `FTag_DestroyEntity_Initiate` during the tick; `FGroup_EntityLifecycle`'s DestructionPhase_Endplay adds `FTag_DestroyEntity_EndPlay`; `FGroup_EndPlay` runs every feature `*_EndPlay` processor (they see EndPlay + !Teardown), including EntityScript_EndPlay; `FGroup_Teardown`'s DestructionPhase_Teardown adds `FTag_DestroyEntity_Teardown`.
- **Tick N+1:** `FGroup_DestructionPipeline`'s DestructionPhase_Await adds `FTag_DestroyEntity_Await`.
- **Tick N+2:** `FGroup_DestructionPipeline`'s DestructionPhase_Finalize adds `FTag_DestroyEntity_Finalize` and DestroyEntity destroys the entity.

Attribute pipeline ordering (Recompute → Compute → Clamp → FireSignals → Refill) is NOT expressed by groups — the composite processor classes manage it internally in their `Tick()`.

---

## EntityScript

`UCk_EntityScript_UE` is a `UObject` base that attaches gameplay logic to an entity. It has a lifecycle: `Construct → BeginPlay → EndPlay`.

```cpp
UCLASS(Abstract, Blueprintable, BlueprintType)
class UCk_EntityScript_UE : public UCk_GameWorldContextObject_UE
{
    virtual auto Construct(FCk_Handle&, const FInstancedStruct& InSpawnParams)
        -> ECk_EntityScript_ConstructionFlow;
    virtual auto ContinueConstruction(FCk_Handle) -> void;
    virtual auto BeginPlay() -> void;
    virtual auto EndPlay() -> void;

    // BP-callable self-access:
    FCk_Handle DoGet_ScriptEntity() const; // UFUNCTION
};
```

- `ECk_EntityScript_ConstructionFlow::Finished` — construction done, `BeginPlay` fires.
- `ECk_EntityScript_ConstructionFlow::Continue` — defer; you must call `DoFinishConstruction()` when ready.
- `Get_AssociatedEntity()` — the entity this script is attached to (from C++).
- `DoGet_ScriptEntity()` — same, exposed as a Blueprint callable (use from BPs/AS).
- Instancing policy: `InstancedPerEntity` (default) creates one instance per spawned entity; `NotInstanced` shares the CDO — use the CDO mode only for stateless scripts.

### Spawn processor rules (`FProcessor_EntityScript_SpawnEntity_HandleRequests`)

- **Owner-liveness cancel.** A spawn whose lifetime owner is destroyed or pending-destroy is cancelled. The owner's destroy cascade marks only the lifetime dependents that EXIST at destroy time, so a child materializing after it would be a zombie outliving its owner (observed: an SmTask ticking against a destroyed StateMachine). Cancelling is the owner's cascade applied late, not an error. The check must run BEFORE the mid-construction defer, or a request whose owner dies while still constructing defers forever; and `ck::Is_NOT_Valid` must short-circuit the `Has_Any` queries, because a finalized owner is a tombstone handle and any `Has` query on it would itself ensure.
- **Editor drag-drop churn.** Editor-preview spawns (`ACk_EntitySpawner_UE`) pass the spawner's INSTANCED script as the archetype; runtime spawns pass a CDO, which cannot die. Drag-drop destroys the preview actor on drop — taking its instanced script with it AFTER the request was enqueued — and the entity-destroy cascade races the spawn processor and loses. That staleness is expected churn (the dropped actor enqueues its own fresh request), so the processor drops the request silently and cleans up the orphaned entity-under-construction, gated on `FTag_EditorOnlyEntity` so the runtime ensure on an invalid archetype stays reachable.
- **Replication-driver ordering.** The driver is added in two places, one per entity shape. **Pre-Construct:** `UCk_Utils_EntityReplicationDriver_UE::TryAdd` walks the lifetime chain for an OwningActor and uses that actor as the driver UObject's Outer, so non-actor-bearing replicated entities (children under a replicated parent) have their driver before Construct — utilities called during Construct (e.g. an attribute `Add` that registers a container fragment) need to find it. Entities that receive their own OwningActor later during Construct (WithActor and similar) have no actor in the chain yet: `TryAdd` fails gracefully (`NotAdded`) and `UCk_Utils_OwningActor_UE::Add` supplies the driver when the actor links. **Post-Construct:** the processor only enables actor-side replication and enqueues the replicate request, and only on a networked authority (NM_Standalone has no net driver; on NM_Client authority-side replication is not our concern). `FProcessor_EntityScript_Replicate` defers (retry next frame, keeping the dirty marker) until the driver exists — that retry bookkeeping is processor-specific and deliberately not in the shared helper. `Request_ReplicateEntityScript` is the single establishment path: validation, dependent-count accounting, `Request_Replicate`, and the FireOnDependent tag all live there, and the ContextOwnerOverride is threaded through so spawn-time context-owner preservation survives.
- **`Net_Params` derivation** (`UCk_Utils_EntityScript_UE::Add`): a new script entity's `Net_Params` comes from the archetype's effective replication intent combined with the TransientEntity's NetMode/NetRole context. `Get_EffectiveReplication` is the virtual hook subclasses override to reconcile the CDO default with runtime state (e.g. WithActor returns `DoesNotReplicate` when its OwningActor isn't replicated). This covers the transient-owner case, where `Request_CreateEntity` skips Net_Params inheritance, and direct `Add()` callers (SM condition/state/transition attach — any non-spawn flow attaching a script to an existing transient-owned entity). Net_Params already inherited from a non-transient lifetime owner is respected.

### EntityScript across save/load (v3 rebuild+hydrate)

- **`UPROPERTY(SaveGame)` fields** (`CkEntityScript_SaveFields`): the load rebuilds every EntityScript by re-Constructing a FRESH UObject from its recipe class-path, so SaveGame fields would reset to class defaults. ONE framework handler (in CkEcs, registered once by a filename-namespaced static registrar — the reflect walk over `CPF_SaveGame` FProperties is generic, so it is not per-script) round-trips them: `Produce` serializes via `FObjectAndNameAsStringProxyArchive` with `ArIsSaveGame = true` into `_FieldBytes`; `HydrationApply` (the only path this type takes) replays them onto the re-Constructed instance. Registered `Register_SaveOnly` — the payload never enters a replicated container, so it stays off the wire and `FProcessor_Hydration_Dispatch` is its sole caller. The `FCk_SaveData_` prefix is deliberately not `FCk_RepData_` so it stays off the census ratchet (`Ck.Snapshot.Meta.RepDataRestoreCoverage` enumerates `FCk_RepData_*`). **Scope limit:** `FCk_Handle`-typed SaveGame fields are NOT saved-id-remapped — the inner byte blob is opaque to the outer handle walker.
- **SpawnRecipe GC ownership.** A RuntimeSpawned entity's construction recipe (EntityScript class + spawn params) is retained from spawn until the save capture reads it. It cannot live directly in the plain `ck::FFragment_SpawnRecipe`: CkFoundation fragments are not GC-traced, so a bare `FInstancedStruct` member (and its `UScriptStruct` type ptr) would dangle — worst for `NotInstanced` scripts (no per-entity object exists) and AS-defined params structs (reinstanced on script reload). A UObject's UPROPERTYs ARE traced (`FInstancedStruct` traces its inner refs via `AddStructReferencedObjects`), so the fragment pins a `UCk_EntityScript_SpawnRecipe_UE` holder via `TStrongObjectPtr`; the holder's UPROPERTY shape mirrors `FCk_EntityReplicationDriver_ReplicationData_EntityScript` (same `TSubclassOf` + `FInstancedStruct`). The recipe is always stamped at spawn (cheap); the capture filters by provenance, so ConstructSpawned/EngineOwned entities that also carry one never consult it.
- **Restore relink.** `UCk_Utils_EntityScript_UE::Relink_AssociatedEntities_AfterRestore` re-derives every script's `_AssociatedEntity` back-pointer from its owning entity after a CkSnapshot restore, mirroring the spawn processor's assignment (friend access via the Utils class). The field is Transient and set only at spawn, so restore — which recreates each script UObject — leaves it a tombstone and the next teardown's EndPlay would read it and ensure.

### `UCk_EntityScript_Subsystem_UE` — UE 5.7 loading-pipeline hardening

- `Initialize` and `OnFilesLoaded` populate the SpawnParams cache via asset-registry query + `FindObject` (memory-only) and never `GetAsset()` / `LoadObject` / `FindOrLoadAssetsByPath` — sync package loading there cascades into Blueprint regeneration and re-entrant `QueueForCompilation` crashes (`ScanForExistingEntityParamsStructInPath` was moved out of `Initialize` for this reason).
- While `GCompilingBlueprint`, cached structs are returned as-is and no new struct is created: `UpdateStructProperties` calls `CompileStructure`/`OnStructureChanged` and `CreateUserDefinedStruct` fires `OnStructureChanged`, both re-entering compilation. The compilation ticker re-runs the update afterwards.
- An asset that exists on disk but is not in memory must NEVER be recreated — a fresh struct gets fresh variable GUIDs, invalidating `FInstancedStruct` data in Blueprints that reference the old ones. During async loading return null (it arrives as a Blueprint dependency and `OnFilesLoaded` discovers it); after startup use `LoadObject` so K2Node compilation can find it.

---

## Signals

Signals are the CkFoundation event system. They carry typed payloads, support BP delegates, have binding policies for "fire if payload already in flight this frame," and can auto-unbind after first fire.

**Define a signal:**

```cpp
// C++-only (no delegate, no BP binding):
CK_DEFINE_SIGNAL_AND_UTILS(MYMODULE_API, OnFoo, FCk_Handle, int32);

// With BP delegate (binds from BP/AS, fires to delegate):
DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_Delegate_MyFeature_OnFoo,
    FCk_Handle, InHandle, int32, InValue);

CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(MYMODULE_API, OnFoo,
    FCk_Delegate_MyFeature_OnFoo, FCk_Handle, int32);
```

**Bind:**

```cpp
// Standard bind (retain after fire):
CK_SIGNAL_BIND(UUtils_Signal_OnFoo, InHandle, InDelegate,
    ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
    ECk_Signal_PostFireBehavior::DoNothing);

// Promise bind (unbind after first fire):
CK_SIGNAL_BIND_PROMISE(UUtils_Signal_OnFoo, InHandle, InDelegate);

// Request-fulfilled bind (ignore in-flight payload, unbind after first fire):
CK_SIGNAL_BIND_REQUEST_FULFILLED(UUtils_Signal_OnFoo, InHandle, InDelegate);
```

**Broadcast:**

```cpp
UUtils_Signal_OnFoo::Broadcast(InHandle, Value);
```

**Unbind:**

```cpp
CK_SIGNAL_UNBIND(UUtils_Signal_OnFoo, InHandle, InDelegate);
```

Binding policies:

| Policy | Meaning |
|---|---|
| `FireIfPayloadInFlight` | If a payload was broadcast before this bind, fire immediately. |
| `IgnorePayloadInFlight` | Ignore in-flight payloads; only fire on future broadcasts. |

Post-fire behaviors:

| Behavior | Meaning |
|---|---|
| `DoNothing` | Delegate stays bound after fire. |
| `Unbind` | Auto-unbind after first fire. |

### Broadcast payload ordering (invariant)

`TUtils_Signal::Broadcast` must store the payload tuple BEFORE publishing, and `publish()` must source its arguments from the STORED tuple, never from the local `InArgs`. `forward<T_Args>()` moves for value-type `T_Args` deduced from rvalue call-site arguments, leaving `InArgs` moved-from. Repro: the GOAP autotests received empty plans because `FCk_Goap_Payload_OnPlanComplete` was broadcast as a temporary, which made `T_Args[N]` a value type, which made `forward()` move.

### Per-listener profiling

`ck.Signal.StatListeners 1` times each bound delegate invoked during a broadcast under `stat CkSignals_Listeners`, named `"ClassName::FunctionName"` — answers "WHICH listener of a hot signal (e.g. OnTimerUpdate) is expensive", which the per-signal-TYPE stat cannot. Off by default because the dynamic stat-id construction is not free.

---

## Entity lifetime

All entity creation and destruction goes through `UCk_Utils_EntityLifetime_UE`:

```cpp
// Create — use Request_SpawnEntity or the EntityScript spawn flow
static FCk_Handle Request_SpawnEntity(...);

// Destroy
static void Request_DestroyEntity(FCk_Handle& InHandle,
    ECk_EntityLifetime_DestructionBehavior = ECk_EntityLifetime_DestructionBehavior::ForceDestroy);

// World-context (needed to resolve the UWorld)
static UWorld* Get_WorldForEntity(const FCk_Handle& InHandle);
```

Destruction is **deferred** until end of frame by default. Never delete fragments or components in a `ForEachEntity` body — fire a signal and let an EndPlay processor handle cleanup (see root `CLAUDE.md` section "Component Lifetime Management in ECS").

### `Request_SetupEntityWithLifetimeOwner` — what it stamps

- **`ck::FTag_ConstructSpawned` (save/load provenance).** Stamped when the lifetime owner is inside a construction window, meaning the child is part of the owner's deterministic build and is re-created by the owner's replayed construction on load — so the v3 capture ADOPTS it by identity (owner + label) rather than respawning a recipe. Two windows qualify: (1) an EntityScript still constructing — carries `FFragment_EntityScript_Current` but not `FTag_EntityScript_HasBegunPlay`; (2) a definition-built entity mid-`Request_BuildAndReplicate`, marked `FTag_DefinitionBuild_InProgress` for the synchronous span of its ConstructionInfo execution. A definition-built entity has NO EntityScript fragment, so window (1) never fires for it; window (2) is what lets a Stackable item's labeled stack-count attribute child persist and re-hydrate instead of reverting to definition defaults. Owner already begun-play / build complete (child spawned by later runtime logic, e.g. an SM task) → no stamp → RuntimeSpawned by default. Both tags are TRANSIENT: the writer reads the LIVE tag at capture and records provenance as entity-table metadata, so the tags themselves must never round-trip.
- **The reverse dependent link — SAME-registry only.** A cross-registry child (e.g. a 2dGridSystem cell, which lives in the grid's private nested `FEcsWorld` yet is lifetime-owned by the main-registry grid) must NOT be added to the owner's `FFragment_LifetimeDependents`: that fragment and its serialization assume same-registry handles. Serialized, a foreign handle keeps only its bare entity-id; on restore it re-homes onto the load registry where its nested id aliases an unrelated entity — including the owner itself, forming a self-cycle that stack-overflows the lifetime-dependents walk. Such children are lifetime-managed by their own nested registry/world, so the forward owner link alone suffices.
- **Inherited context stamps.** `ck::FFragment_EditorSelectionOwner` and `ck::FTag_EditorOnlyEntity` are inherited by every lifetime-descendant at creation, the same strategy as ContextOwner.

### `ck::FFragment_EditorSelectionOwner`

Stamped (via `UCk_Utils_EditorSelectionOwner_UE::Request_SetupEntityWithEditorSelectionOwner`) on the root entity of an editor-world preview spawned on behalf of a placed actor (e.g. `ACk_EntitySpawner_UE`'s editor entity), then inherited by descendants. Consumers read it directly off their own entity — there is no chain walk. Editor-world visuals (ISM/ISKM instances, hosted scene components, world-space widgets, opted-in PMG shapes) use it to host on a per-owner proxy actor (`ACk_EditorSelectionProxyHost_Actor_UE`, spawned by `UCk_EditorEcsWorld_Subsystem_UE::Get_SelectionProxyHostActor`), so a viewport click on the visual redirects selection to the placed actor via the engine's selection-parent mechanism (`AActor::GetSelectionParent` — the pattern `ALevelInstanceEditorInstanceActor` uses). Helper actors hosting preview visuals are NOT attached to the owner, so the engine's attached-actor propagation cannot reach them: they register via `RegisterProxyActor` and the owner's `PushSelectionToProxies` override forwards the highlight through `PushOwnerSelectionToProxies`.

`FTag_EditorOnlyEntity` is consumed by editor-only processor view filters (the `CK_IF_EDITOR_ONLY_ENTITY` shorthand) and by `TIgnoreInEditor<>` dispatch in `TProcessor::DoTick` (the runtime view EXCLUDES the tag, the editor view REQUIRES it). It is stamped on the editor subsystem's transient entity in `CkEcsEditor_Subsystem.cpp`.

### Destroy-entity replicated-object cleanup

`FProcessor_EntityLifetime_DestroyEntity` requires BOTH `FTag_Replicated` and `UCk_Utils_ReplicatedObjects_UE::Has`. The tag alone is not enough: a restored husk (e.g. an orphan SM-graph entity cascade-destroyed when its restored sub-SM is reconciled) can carry the tag without `FFragment_ReplicatedObjects_Params`, and a husk with nothing to clean up would trip the `Get<...>` ensure.

### Save/load entity tags

- **`FTag_Snapshot_JustRestored`** is stamped by the load (`UCk_Snapshot_Subsystem::DoHydrate_Enqueue`) on every restored (saved-id-mapped) entity before the load gate opens; game-side rebind processors key off it. Transient, so never captured, but it survives for the entity's whole lifetime — consumers must pair it with their own once-per-feature dedup.
- **`FTag_Snapshot_SaveTransient`** marks DERIVED state whose owner's construction/redrive recreates it on load; the capture must never persist it as a respawnable row. Canonical case: the SM graph (states/tasks/conditions/transitions/sub-SMs, recreated by the SM hydration redrive). Without the stamp such entities are captured via their SpawnRecipe (RuntimeSpawned) and respawned as top-level duplicates that re-run their lifecycle outside the owning feature's context — the zombie-SmTask-with-destroyed-SM incident.

### Private ECS worlds — `ck::FEcsWorld`

`CkEcs/World/CkEcsWorld.h` — RAII owner of an entt registry plus its `ck::registry_table` slot; `Get_Registry()` returns a non-owning `FCk_Registry` view bound to that slot. Used by editor-only engine subsystems that need a private ECS world (`UCk_EditorAssetLoader_SubSystem_UE`, `UCk_EditorToolbar_*`). The game-time `UCk_EcsWorld_Subsystem_UE` / `UCk_EditorEcsWorld_Subsystem_UE` do NOT use it — they own their registry directly. Its destructor frees the table slot BEFORE destroying the entt registry, so any outstanding handle fails safe (resolves to nullptr) instead of UAFing.

---

## Registry

`FCk_Registry` is a **non-owning view** over a slot-table `FCk_RegistryHandle`. Provenance worth knowing:

- The transient entity used to be a per-view `_TransientEntity` field, which made `*Handle` return a view with no transient entity (a footgun). It now lives in `entt::registry::ctx()` as `FCtx_TransientEntity`, so the registry is the single source of truth.
- Slot resolution failures were previously a hard crash through a `TSharedPtr` deref; they are now `CK_ENSURE_IF_NOT` on mutating paths and silent defaults on read-only paths.
- The registry's `CK_DEFINE_CUSTOM_FORMATTER_INLINE` braces must stay escaped (`{{` → `'{'`, `}}` → `'}'`). An earlier `TEXT("{slot={},gen={}}")` made fmt parse `{slot=...}` as a NAMED field it could not resolve and throw `fmt::format_error`, crashing every ensure that formats a registry — observed on the tombstone-handle ensures fired during a CkSnapshot load.

### Slot table — why a module-static phoenix singleton

`ck::registry_table` (`CkRegistry_SlotTable.cpp`) is deliberately NOT a UEngineSubsystem.

1. **UObject lifecycle inversion.** Subsystems are torn down DURING editor shutdown, BEFORE the final GC purge that destroys the long tail of unreachable UObjects. Those late-purged UObjects can hold `FCk_Handle` fields whose destructors call into the slot table — a subsystem-owned table would already be gone. That is the exact bug the slot-table migration was built to fix; do not reintroduce it one level up.
2. **Module-static storage outlives the whole UObject lifecycle** (DLL unload happens after all UObjects are gone). The phoenix sentinel makes `Free()`/`Resolve()`/`TryResolve()` safe through DLL teardown regardless of caller order.
3. **Accepted trade-off:** the `FRegistryTable_State` bytes are intentionally leaked at process exit — `ShutdownTable()` flips the sentinel and never runs the destructor. A finite, process-lifetime leak, traded against a far worse class of crash. There is deliberately no `atexit` handler; `FCkEcsModule::ShutdownModule` calls `ShutdownTable()` explicitly.

- **Live Coding caveat.** Live-Coding-patching `CkEcs.dll` while a PIE session is live can leave the slot table inconsistent (the new DLL's table is empty; existing handles still point at the old DLL's storage). After such a patch, fully restart the editor before the next PIE session — there is no automatic recovery.
- **Symbol naming.** File-scope statics in that TU carry the `FRegistryTable_*` / `GRegistryTable_*` prefix so unity builds cannot collide them with same-named symbols in unrelated CkEcs .cpp files.
- **The slot's registry pointer is `std::atomic`** to give the Free-vs-Resolve race window explicit acquire/release ordering instead of relying on transitive publication via the Generation read (Free release-stores nullptr; Resolve acquire-loads). The parallel-region invariant still applies — no concurrent Allocate/Free with workers — but the atomic also defines behaviour for the cross-tick window where a game-thread Allocate/Free could overlap with a worker still draining a prior parallel region.
- **`Generation` is `int32`, not `uint32`,** to match the reflected `FCk_RegistryHandle::Generation` (UHT/BP does not reflect `uint32`). It is an opaque token; the only operation is increment-with-skip-zero on alloc/free, since 0 is the never-allocated sentinel.

---

## Archetypes

An **Archetype** is a NAMED amalgamation of features. CkEcs owns the storage + data model only; resolution is split by consumer:

- **FeatureIds** match through the DebugFeatureFlags bit cache when enabled — `(bits & required) == required`.
- **`RequiredLabel` / `NamePattern`** are carried as data and matched consumer-side by systems that can see them (the ECS debugger) — CkEcs cannot depend on CkLabel.
- **Native registrations** may supply a direct matcher (`CK_DEFINE_ARCHETYPE` wires its generated `TryCast` in).

Bulk matching over many entities should resolve the required mask once via `debug_feature_flags::Get_BitIndex` and test rows directly; `archetype_registry::Get_Matches` is the convenience path for BP/AS single queries.

### Typed archetypes (`CkArchetype_Typed.h`)

```cpp
struct FCk_Archetype_Shelf
{
    FCk_Handle_Transform Transform;
    FCk_Handle_Inventory Inventory;
    CK_ARCHETYPE_BODY(FCk_Archetype_Shelf);
};
CK_DEFINE_ARCHETYPE(FCk_Archetype_Shelf, "Rewind99.Shelf", Transform, Inventory);
```

Each listed name `X` is simultaneously the member name, the typesafe handle type `FCk_Handle_X`, and the utils class `UCk_Utils_X_UE`. The macros are purely textual — the utils classes resolve at the EXPANSION site (game code links its own features), so CkEcs stays feature-agnostic. `CK_DEFINE_ARCHETYPE` generates `TryCast(Handle) -> TOptional<StructType>` (all-or-nothing member casts) plus deferred `EndOfEngineInit` registration of the equivalent descriptor with `TryCast` wired in as the registry's native matcher. `USTRUCT`/`UPROPERTY` on the struct is optional (add it only for BP visibility). Max 8 features per archetype.

### DebugFeatureFlags cache

A per-registry bit table — one `uint64` row per entity index — maintained by EnTT `on_construct`/`on_destroy` sinks on each registered feature's MARKER fragment (the stable Params/Current fragment, never a request/transient tag). Rows self-correct on entity destruction because a feature's `on_destroy` fires for fragment removal AND entity destruction alike. Zero cost until `Enable()` connects the sinks (the debugger opening); consumers then get O(1) per-entity feature queries and archetype matching compiles to `(bits & required) == required`. Feature→fragment registration is deliberately NOT in CkEcs (it must not see T4 feature modules): consumers that link the fragment types call `RegisterFlag<TFragment>(FeatureId)` at startup, BEFORE `Enable()`.

---

## Context ownership

Every entity has an optional context owner — another entity that "owns" it for scoping purposes (e.g., an ability's entity is owned by the character entity).

```cpp
UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle)          // -> FCk_Handle
UCk_Utils_ContextOwner_UE::Request_Override(InEntity, InOwner) // change owner
UCk_Utils_ContextOwner_UE::Request_OverrideToSelf(InEntity)    // self-owned
```

Access pattern from EntityScript:

```cpp
auto SelfHandle  = Get_AssociatedEntity();               // on the EntityScript
auto OwnerHandle = UCk_Utils_ContextOwner_UE::Get_ContextOwner(SelfHandle);
```

(Root `CLAUDE.md` section 9 shows `ck::SelfEntity(this)` / `ck::GetOwnerEntity()` as shorthand — use the above UCk_Utils pattern if you can't find those symbols.)

---

## Networking / replication

Replication fragments and the Iris driver live in `Net/`. `CkNet_Fragment_Data.h` declares the replication enum and per-entity net config. `FFragment_Net_Current` holds authority/client flags. A separate processor (`Scheduler/`) orders net processors relative to gameplay processors.

Flag: `ECk_Replication` — set per-EntityScript's `_Replication` property. Defaults to `Replicates`.

Use `ECk_Processor_NetModePolicy` (see `CkProcessor_NetModePolicy.h`) to gate a processor to server-only, client-only, or both.

### Replicated fragment containers — deferred dispatch

Server-side writes go through `UCk_Utils_Net_UE::TryAddContainerFragment` / `TryUpdateContainerFragment` (host-gated) into a FastArray on the entity's `UCk_Fragment_EntityReplicationDriver_Rep`. Client-side application is fully deferred:

1. **Net receive and driver link are pure bookkeeping.** `PostReplicatedAdd/Change` and `PostLink` only mark entries pending and tag the associated entity with `ck::FTag_RepFragments_PendingApply`; `PreReplicatedRemove` queues the removed entry. No handler runs inline during net receive.
2. **One dispatch site.** `FProcessor_ReplicatedFragments_Dispatch` (`FGroup_DeferredApply`, ClientOnly — moved there in Phase 2 from `FGroup_Gameplay_Script`) drains pending entries each tick by resolving `FCk_PersistenceHandlerRegistry` handlers. It skips entities tagged `FTag_EntityScript_ConstructedThisFrame` (so a just-composed entity's pending Setup drains first, no stomp); the sibling `FProcessor_Hydration_Dispatch` runs last in the group and clears that tag. `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` fire-gates the completion signal on any undrained replicated fragment across the lifetime-dependents tree.
3. **The handler contract is `NetApply(Entity, New, TOptional Old) -> Applied | NotReady`** (plus optional `NetRemove`), with a load-path twin `HydrationApply` of the same signature. `Old` is unset on the first application and otherwise holds the last APPLIED data — coalesced receives diff against what was actually applied (the load path never coalesces, so `HydrationApply`'s `Old` is always unset). Return `NotReady` while the targeted feature is not composed yet; the dispatcher retries each tick and, past a timeout (5s dev / 2s shipping), drops the entry with an ensure naming the type and entity. Never compose the feature from inside `NetApply`/`HydrationApply` — composition belongs to construction / OnConstructed.

Registration lives in the feature's `_Fragment.cpp` on `FCk_PersistenceHandlerRegistry` (`CkEcs/Persistence/CkPersistenceHandlerRegistry.h`) — **prefer a named participation shape** (`Register_NetOnly` / `Register_SaveOnly` / `Register_NetAndSave_SharedApply` / `Register_NetAndSave_SplitApply`) over hand-building an `FHandler`. Each takes a **designated-init args struct** so every lambda is labeled at the call site (`.Produce =`, `.NetApply =`, `.HydrationApply =`), with required slots compile-enforced (an omitted required lambda does not compile). The primitives `RegisterLazyTyped<T>` / `RegisterLazy` / `RegisterFallback` (the last for runtime-typed payloads, e.g. dynamic fragments) are the low-level surface those forms wrap. The reference handler is Team (`CkRelationship/Team/CkTeam_Fragment.cpp`, `Register_NetAndSave_SharedApply<FCk_RepData_Team>`): `Has` check → `NotReady`, else assign → `Applied`. Save/load authoring recipe: [`../CkSnapshot/Claude.md`](../CkSnapshot/Claude.md).

### Two-signal client lifecycle contract

- **`OnConstructed`** (entity script) — the entity is COMPOSED. Replicated container values are NOT applied yet (the dispatcher runs after FinishConstruction in the same frame). Compose features here; do not read replicated values (team, attributes, SM state).
- **`OnReplicationComplete`** (`UCk_Utils_EntityReplicationDriver_UE::Promise_OnReplicationComplete`) — replicated values are applied. The dispatcher's group (`FGroup_DeferredApply`) precedes `FGroup_Replication`, where `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` broadcasts — so by the time the callback runs, the initial container data has been dispatched. Read replicated values here. The promise fires retroactively if bound late (payload-in-flight semantics).

Pinned by `Ck.Attribute.Net.Values_AppliedBefore_OnReplicationComplete`, `Float_InitialBakedValue_Replicates`, and `Float_PreComposition_StashedValue_Applies` in CkTests.

### Actor-side unified promise

For actor-linked entities (`EntityScript_WithActor`), consumers should not juggle the two signals
directly. `UCk_Utils_OwningActor_UE::Promise_OnActorEcsReady(Actor, Delegate, Policy)` is the one
actor-side hook that works on every world:

- `ECk_ActorEcsReady_Policy::ValuesReplicated` (default) — fires once the Actor↔Entity link exists
  AND `OnReplicationComplete` has fired for the linked entity. Collapses to link-time on authority
  and for non-replicated entities. Replicated values are readable inside the callback.
- `ECk_ActorEcsReady_Policy::LinkEstablished` — fires at link time (OnConstructed-equivalent on
  clients; replicated values may not be applied yet).

The promise fires immediately when bound after the actor is already ready, queues on the
(non-replicated, auto-added) `UCk_EntityOwningActor_ActorComponent_UE` otherwise, and is discarded
if the actor is destroyed before ever becoming ready. To spawn the actor-linked entity in the first
place, use `UCk_Utils_EntityScript_WithActor_UE::Request_SpawnEntityScript_OnActor` (CkEcsExt) or
add the `Ck Entity Script (With Actor)` component in the editor — both are authority-gated and
replication-correct.

**Host collapse is mandatory, not an optimisation.** `DoGet_ShouldDeferUntilReplicationComplete` must NOT defer on the host: replicated values are written locally there, so there is nothing to await and OnReplicationComplete timing is purely a client-convergence concern. Deferring anyway coupled every authority-side consumer (e.g. a HUD's context injection) to the fire processor's schedule, which a v3 load starves for the restored pawn — its OnReplicationComplete never re-fires in the loaded world, so the promise hung forever awaiting a signal with nothing left to signal. Netmode gotcha: gate with `Get_IsEntityNetMode_Host`, NOT `Get_HasAuthority` (which admits clients).

### EntityScript replication pipeline

**Server:** `Request_SpawnEntity` → `Request_CreateEntity(Owner)` → SpawnProcessor (`Construct()` fires; for WithActor the net params are set, `EntityOwningActor` enables replication, the ReplicationDriver is created and `FRequest_Replicate` added; non-WithActor follows the same shape) → ReplicateProcessor populates the driver's replicated properties and marks them dirty.

**Wire:** `UCk_Fragment_EntityReplicationDriver_Rep` is a replicated UObject registered as a sub-object on the EntityOwningActor component via `AddReplicatedSubObject()`.

**Client:** the spawn call detects Replicated + Client net mode, creates a PENDING entity on the owner and returns a PendingEntityScript handle the caller binds `Promise_OnConstructed()` to. On receive, `OnRep_ReplicationData_EntityScript()` picks the lifetime owner (self-referencing → the transient entity, otherwise the replicated parent), adds `TWeakObjectPtr<UWorld>` directly (the ownership chain may not resolve to a World yet) and calls `UCk_Utils_EntityScript_UE::Add()`. The client SpawnProcessor then runs with the replication block skipped, and the FinishConstruction processor broadcasts OnConstructed on the real entity, checks the lifetime owner for `FFragment_PendingReplication`, consumes the matching pending entity (FIFO by class), re-broadcasts OnConstructed on it with the real handle as payload, and destroys it.

**Add the OwningActor BEFORE any path that can add a driver.** A ReplicationDriver's Outer is chain-walked to a replicated actor AT ADD TIME, so if the spawn processor's pre-Construct pass added the driver before this entity owned an actor, it got Outer'd to an ancestor's actor. Adding the driver from `UCk_Utils_OwningActor_UE::Add` (with this entity's actor available) gives the tightest Outer resolution.

### Replicated-object strong ownership

`FCk_ReplicatedObjects` is the GC anchor for its replicated objects, mirroring how `FFragment_EntityScript_Current` owns its `UCk_EntityScript_UE`. The actor's `FSubObjectRegistry` holds them weakly (`UE_NET_SUBOBJECTLIST_WEAKPTR`) and the entity-fragment back-ref is GC-untraced, so without the strong ref GC reclaims the object whenever there is no active netdriver+channels (standalone / `-nullrhi`), cascading `Request_DestroyEntity` into actor teardown. It is not a UPROPERTY because `TStrongObjectPtr` is not UHT-reflectable; the wire format (`FCk_EntityReplicationDriver_ReplicateObjects_Data::_Objects`) stays weak and converts at the boundary via `ToWeak`/`ToStrong`.

### Replication-driver failure modes worth naming

1. **Driver constructed with no UWorld.** `UCk_Fragment_EntityReplicationDriver_Rep::_AssociatedEntity` is created ONLY in the constructor, so a driver built on a client before its outer Actor had a UWorld stays permanently invalid; every child driver naming it as owner parks in `_PendingChildEntityConstructions` / `_PendingChildAbilityEntityConstructions`, which are never drained. Symptom: a silently-missing replicated subtree followed ~10s later by PendingReplicationRetry timeouts. Investigated 2026-06 — never reproduced across same-actor + cross-actor burst stress (see the EntityReplicationDriver.Net tests). The three OnRep park sites therefore log loudly rather than treating the state as a wait.
2. **OnReplicationComplete fire-gate stall.** The dispatcher's 5s dev / 2s shipping timeout only bounds entries some dispatcher on this net mode actually drains. An entry NOTHING drains (e.g. `FTag_RepFragments_PendingApply` stamped on a world whose ClientOnly dispatcher never runs) held the gate FOREVER and silently hung every OnReplicationComplete consumer, including `Promise_OnActorEcsReady/ValuesReplicated` — found via the post-v3-load HUD hang, 2026-07-14. `FFragment_RepDriver_FireGateStall` now accumulates the held time and, past 10s, ensures naming the blocking entity.

### Net ↔ Persistence split

The load-path hydration dispatcher (`FProcessor_Hydration_Dispatch`) and `ck::persistence_apply::ApplyOne` were split out of `Net/ReplicatedFragmentContainer/` into `CkEcs/Persistence/`. `Net/` keeps ONLY the net-side `FProcessor_ReplicatedFragments_Dispatch`; the hydration dispatcher forward-declares it and `RunAfter`s it (scheduling-order dependency only). **Net → Persistence is the only permitted dependency direction.** `ck::PendingApplyTimeoutSeconds` is defined in `CkPersistenceHydration_Processor.h` so one header definition serves both dispatcher TUs (unity-build safe); it mirrors the EntityScript pending-replication timeout.

Scheduling contract: `FProcessor_ReplicatedFragments_Dispatch` lives in `FGroup_DeferredApply`, which runs after `FGroup_Gameplay_Script` (home of `FProcessor_EntityScript_FinishConstruction`), so OnConstructed-driven composition exists before the first dispatch — no per-processor `RunAfter` needed. `FGroup_DeferredApply` precedes `FGroup_Replication` globally, so applied values are visible before `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` broadcasts in the same frame (fire-gating additionally waits on the drain).

### Participation-shape design notes

- `SharedApply` / `SplitApply` are two distinct names rather than an overload set: the variants differ only by which lambdas they carry, which would make overload resolution on the args struct fragile.
- `FRequired*` wrappers are implicitly constructible from any compatible callable (raw lambda, `FApplyFn`/`FProduceFn`, or a named local) as a single user-defined conversion. Optional `NetRemove` defaults to an empty `TFunction`; a save-only handler's args struct cannot even name `.NetApply`.
- A `Produce` with no `HydrationApply` is excluded from `Get_SaveHandlerTypes` (the registration ensure has already fired), keeping the save free of state that could never load back. The capture writes one payload per (entity, type).

---

## Anti-patterns

1. **Don't put game logic outside processors.** Timers, attribute math, state transitions — all belong in `ForEachEntity`.
2. **Don't hold `FCk_Entity` in fragments or outside the processor tick.** Always store `FCk_Handle` or `FCk_Handle_TypeSafe`. Raw entities are invalidated on entity destruction.
3. **Don't call `ck::IsValid` with UE's `IsValid`.** Handle validators are registered in `CkEcs`; only `ck::IsValid` routes to them.
4. **Don't destroy components or fragments in `ForEachEntity`.** Use the EndPlay processor pattern.
5. **Don't ignore deferred creation.** Entity creation inside `ForEachEntity` must use the deferred API (`DeferredEntity/`), not synchronous spawn — the registry is locked during iteration.
6. **Don't put TypeSafe handle declarations in `_Fragment.h`.** They belong in `_Fragment_Data.h` to avoid UHT circular dependency issues.

---

## See also
- `CkEcsExt/Claude.md` — higher-level ECS utilities (EntityHolder, SceneNode, Transform helpers, EntityScript extensions).
- `CkRecord/Claude.md` — entity-to-entity parent/child record system.
- `CkProvider/Claude.md` — data-asset-driven value providers.
- `CkLabel/Claude.md` — per-entity named label fragments.
- `CkCore/Claude.md` — the Validation, Ensure, Format, Chrono, and Time utilities used throughout processors.
- Root `/Source/CLAUDE.md` section 9 "ECS Framework Patterns" — naming conventions and signal binding boilerplate.
