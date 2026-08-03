# How CkEcs wraps EnTT

Reference for `ckecs-domain-reference`. Registry views, handle validity, the TProcessor CRTP, MarkedDirtyBy, and the real tick flow.

### 2.1 FCk_Registry — a generational view, not the registry

`FCk_Registry` owns nothing. It is a trivially-copyable `FCk_RegistryHandle` — `{int32 SlotIndex,
int32 Generation}` (`CkEcs/Registry/CkRegistry_Handle.h:16-46`) — resolved on demand through
`ck::registry_table`, a process-lifetime slot table mapping (slot, generation) → live
`entt::basic_registry*` (`CkEcs/Registry/CkRegistry_SlotTable.h:15-99`; registry type alias
`CkRegistry_SlotTable.h:17`; hard cap 16384 slots `:30`). Freeing a slot bumps its generation, so
every outstanding `FCk_Registry`/`FCk_Handle` pointing at a torn-down world resolves to `nullptr`
instead of a use-after-free (`CkRegistry_SlotTable.h:38-46`). `Resolve` fires a non-shipping ensure
on stale handles; `TryResolve` is the silent probe used by validity checks
(`CkRegistry_SlotTable.h:48-69`).

On top of raw EnTT, `FCk_Registry` (`CkEcs/Registry/CkRegistry.h:76-326`) adds:

| Wrapper behavior | Where | Delta vs raw EnTT |
|---|---|---|
| `Add<T>` rejects duplicates with `CK_ENSURE_IF_NOT` | `CkRegistry.h:441-487` | raw `emplace` would be UB/assert |
| Tags must derive `ck::TTag` (static_assert) | `CkRegistry.h:449` | enforces `CK_DEFINE_ECS_TAG` usage |
| Counted tags (`ck::FTag_CountedTag`) nest Add/Remove | `CkRegistry.h:59-71, 463-472, 610-619` | remove only deletes at count 0 |
| `Get<T>` on missing fragment returns a **shared static** `Invalid_Fragment` after ensuring | `CkRegistry.h:723-746` | never assume the ref is entity-owned on the failure path |
| Every mutation bumps a per-fragment-type version counter | `CkRegistry.h:237-260` | feeds the scheduler's pump short-circuit (§2.5) |
| `AddOrGet` bumps the version even when returning the existing fragment | `CkRegistry.h:502-511` | so in-place request-queue appends still wake the pump |
| Mutations assert not-in-parallel-region (non-shipping) | `CkRegistry.h:437-439`; `CkRegistry_SlotTable.h:71-80` | catches worker-thread structural mutation |
| `TExclude<T>` marks exclusion in `View<...>` | `CkRegistry.h:52-55` | maps to `entt::exclude` (`CkRegistry.h:162`) |
| `View<...>().ForEach(lambda)` | `CkRegistry.h:104-172` | wraps `entt view.each`; lambda gets `FCk_Entity` + **non-empty** fragments only |

### 2.2 FCk_Handle — what makes it valid or invalid

`FCk_Handle` = `FCk_Entity _Entity` + `FCk_RegistryHandle _RegistryHandle` (+ a weak
replication-driver pointer and a debug mapper) (`CkEcs/Handle/CkHandle.h:326-342`). All fragment
ops (`Add/Get/Has/Remove/Try_Remove/CopyAndRemove/View`, `CkHandle.h:183-226`) forward to the
resolved registry. `Get_RegistryView()` returns **by value** — never bind it to a reference
(`CkHandle.h:273-278`).

The validity ladder (always via `ck::IsValid`, never raw checks):

1. **Registry live?** slot table `TryResolve` ≠ nullptr.
2. **Entity live?** `registry.valid(entity)` — index present AND version matches (§1.1).
   1+2 = `IsValid(IsValid_Policy_IncludePendingKill{})` (`CkHandle.cpp:213-226`).
3. **Not being torn down?** default policy additionally requires the entity is NOT in the
   Teardown/Destroyed destruction phases (`CkHandle.cpp:202-211`).

So during the destroy pipeline (§3.4) a handle is *default-invalid but pending-kill-valid*; the
idiom `CK_IF_HANDLE_IS_PENDING_KILL(Handle)` tests exactly that window (`CkHandle.h:395-396`).

**Typesafe layering (one-liner):** `FCk_Handle_[Feature]` subclasses add ZERO data
(`static_assert(sizeof ==)`, `CkHandle_TypeSafe.h:76-80`); the base→derived constructor is private
(`CkHandle_TypeSafe.h:101-102`) so a typed handle can only be minted by `ck::StaticCast` /
the feature's `Cast`, i.e. after its identifying fragments were verified. Full contract and rules:
`ckecs-architecture-contract`.

### 2.3 The TProcessor CRTP

Two generations, both in `CkEcs/Processor/CkProcessor.h`; new code uses `ck_exp`:

- `ck::TProcessorBase<Derived>` (`CkProcessor.h:27-72`) — owns an `FCk_Registry` copy and
  `_TransientEntity` (`CkProcessor.h:131`, the handle it builds views from). `Tick(InDeltaT)`
  supports a fixed `_TickRate` via an accumulator loop (multiple `DoTick`s per frame when behind,
  `CkProcessor.h:134-161`); `Pump()` = one `DoTick` with zero DeltaT (`CkProcessor.h:163-171`).
- `ck::TProcessor<Derived, Fragments...>` (`CkProcessor.h:76-117`) — legacy: `ForEachEntity`
  receives a plain `FCk_Handle`.
- `ck_exp::TProcessor<Derived, T_HandleType, Fragments...>` (`CkProcessor.h:237-337`) — current:
  first parameter after Derived is the **typed handle** (`requires` base-of `FCk_Handle`,
  `CkProcessor.h:240`); aliases `TimeType = FCk_Time`, `HandleType = T_HandleType`
  (`CkProcessor.h:273-279`).

**How the fragment pack maps to a view and to your signature** (`CkProcessor.h:353-420`): `DoTick`
builds `_TransientEntity.View<UnwrapAccessPolicy_T<Fragments>...>()` and calls
`This()->ForEachEntity(InDeltaT, TypedHandle, Components&...)` per entity, minting the handle with
`ck::StaticCast<HandleType>(ck::MakeHandle(InEntity, _TransientEntity))` (`CkProcessor.h:412`).
Rules that fall out:

- Every data fragment MUST be wrapped `ck::TReadOnly<F>` or `ck::TReadWrite<F>` (static_assert,
  `CkProcessor.h:262-266`). `TReadOnly` arrives as `const F&` (const restored via
  `TResolveConstness`, `CkProcessor.h:414`); the wrappers also feed the scheduler's access metadata
  (`Scheduler/CkProcessorTraits.inl.h:76-120`).
- Tags and `TExclude<...>` entries **filter but do not appear as parameters** (empty types carry no
  payload, §1.4; stripped by `FragmentsOnly`, `CkRegistry.h:137-144`).
- Parameter order after `(TimeType, HandleType)` mirrors the declaration order of the non-tag,
  non-exclude fragments.

Real exemplar (`CkTimer/CkTimer_Processor.h:13-36`):

```cpp
class CKTIMER_API FProcessor_Timer_Setup : public ck_exp::TProcessor<
    FProcessor_Timer_Setup,
    FCk_Handle_Timer,
    ck::TReadOnly<FFragment_Timer_Params>,
    ck::TReadWrite<FFragment_Timer_Current>,
    FTag_Timer_NeedsSetup,
    CK_IGNORE_PENDING_KILL>
{
public:
    using Group = FGroup_Gameplay_TimeDelta;
    using MarkedDirtyBy = FTag_Timer_NeedsSetup;

public:
    using TProcessor::TProcessor;

public:
    static auto
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InTimerEntity,
        const FFragment_Timer_Params& InParams,
        FFragment_Timer_Current& InCurrentComp)
        -> void;
};
```

`CK_IGNORE_PENDING_KILL` expands to four `TExclude`s over the destroy-pipeline tags
(`CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h:37-41`) — the standard "skip dying entities"
filter. Cleanup passes instead match them: `CK_IF_END_PLAY` / `CK_IF_TEARING_DOWN`
(`CkEntityLifetime_Fragment.h:43-50`). Note the `Initiate` tag is deliberately NOT excluded —
regular processors still complete the frame's work on a just-marked entity
(`CkEntityLifetime_Fragment.h:35-36`).

### 2.4 MarkedDirtyBy — what marks, what clears, what pumps

`using MarkedDirtyBy = <FragmentOrTag>;` on a processor does nothing inside the processor — it is
**scheduler metadata**, harvested at registration into the processor descriptor
(`Scheduler/CkProcessorTraits.inl.h:164-175`; multi-marker `MarkedDirtyByAnyOf = TDepList<...>`
`:45-60,176-184`). "Dirty" is defined as: **any entity in the registry currently has the marker
fragment** (`_IsDirtyChecker` = `Has_AnyEntityWith<Marker>`, `CkProcessorTraits.inl.h:171-174`).

- **Marking**: whatever adds the marker — `Add<FTag_Timer_NeedsSetup>` during composition, a Utils
  `Request_*` doing `AddOrGet<FFragment_X_Requests>` + append. Every mutation also bumps the
  per-type version counter in the slot table (`CkRegistry.h:254-260`) — that counter is only a
  change detector for the pump short-circuit, not the dirty definition itself.
- **Clearing**: the processor body must **consume the marker**. Verified in CkTimer:
  `FProcessor_Timer_Setup::ForEachEntity` starts with `InTimerEntity.Remove<MarkedDirtyBy>();`
  (`CkTimer_Processor.cpp:28`); `FProcessor_Timer_HandleRequests` copies + resets the request
  array, visits, then removes the `Requests` fragment only if nothing re-enqueued during handling
  (`CkTimer_Processor.cpp:48-66`). A processor that declares `MarkedDirtyBy` but never removes the
  marker keeps reporting dirty and gets re-pumped to the frame cap.
- **Pumping**: after the main pass, the scheduler runs extra `Pump()` passes (DeltaT = 0) over
  dirty processors so cascading reactive work (setup → request → signal → new request) drains in
  ONE frame (§2.5). Opt out with `static constexpr auto PumpPolicy =
  ECk_ProcessorPumpPolicy::SkipPump;` when the body integrates DeltaT or re-applies cached state
  (`Scheduler/CkProcessorDescriptor.h:80-84`; honored `CkProcessorScheduler.cpp:216-220`).

### 2.5 Registration and the real tick flow

Processors self-register at static-init: `CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_Setup);` in
the feature's `_Processor.cpp` (`CkTimer_Processor.cpp:10-13`) declares a file-scope
`ck::FAutoProcessorRegistrar<T>` whose ctor pushes a descriptor (name, `Group`, `RunAfter`/
`RunBefore`, dirty markers, RO/RW fragment hashes, net/world/tick-group requirements, pump policy —
all `if constexpr`-probed off the type, `CkProcessorTraits.inl.h:125-222`) plus a factory lambda
into the global `FProcessorRegistry` (`Scheduler/CkProcessorRegistration.h:12-50`; macros
`:82-95`). Registration is process-global; worlds instantiate from it. (Any doc mentioning
"ProcessorInjector" is stale — root `CLAUDE.md`.)

Tick flow, confirmed end-to-end:

1. `UCk_EcsWorld_Subsystem_UE` (a `UWorld` subsystem) **owns** the `entt::basic_registry`
   (`TUniquePtr` + slot registration, `Subsystem/CkEcsWorld_Subsystem.h:181-186`).
2. `OnWorldBeginPlay` → `DoBuildGraphAndSpawnActors`: broadcasts `Get_OnPreBuildProcessorGraph()`
   (the hook CkDynamic uses to inject AngelScript/Blueprint-defined script processors as
   descriptors — `CkEcsWorld_Subsystem.h:98-104`, consumer
   `CkDynamic/CkDynamic_ScriptProcessor_Host.cpp:192`), then `ck::FProcessorGraphBuilder` builds
   the dependency graph and partitions it per `ETickingGroup`
   (`CkEcsWorld_Subsystem.cpp:348-389`).
3. One `ACk_EcsWorld_Actor_UE` (an `AInfo`) is spawned per non-empty ticking group, each holding
   one `ck::FProcessorScheduler`; the actor's `Tick(DeltaSeconds)` calls
   `_Scheduler->Tick(FCk_Time{DeltaSeconds}, _Registry)` (`CkEcsWorld_Subsystem.h:31-82`,
   `CkEcsWorld_Subsystem.cpp:49`). **That actor tick is where every processor actually runs.**
4. `FProcessorScheduler::Tick`: main pass — every node once, in the dependency-resolved
   `_ExecutionOrder` (`Scheduler/CkProcessorScheduler.cpp:103-141`) — then **pump passes**: up to
   `_MaxPumpIterations` (30) rounds where each dirty, pump-eligible node gets `Pump()`
   (`CkProcessorScheduler.cpp:143-155, 196-307`). A version short-circuit skips the
   `Has_AnyEntityWith` scan when the marker type's mutation counter hasn't moved; the PRE-pump
   version is stored so work a pump adds re-fires next pass instead of slipping a frame
   (`CkProcessorScheduler.cpp:289-298`). Hitting the cap logs the still-dirty processor names
   (`CkProcessorScheduler.cpp:157-173`).
5. Graph rebuilds (`Request_RebuildProcessorGraph`) defer to `FCoreDelegates::OnEndFrame`
   (`CkEcsWorld_Subsystem.h:117-121`). `Request_PumpToQuiescence` drives all schedulers at
   DeltaT=0 for snapshot capture (`CkEcsWorld_Subsystem.h:122-131`).

Side note: `UCk_ProcessorScript_Subsystem_UE` (`Processor/CkProcessorScript_Subsystem.h:12-28`)
holds BP/AS `UCk_Ecs_ProcessorScript_Base_UE` objects, but `Request_CreateNewProcessorScript` has
zero call sites inside CkFoundation (rg-verified 2026-07-02) — the live script-processor path is
the CkDynamic pre-build hook above. Treat the subsystem as dormant plumbing.

