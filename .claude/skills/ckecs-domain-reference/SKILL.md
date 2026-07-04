---
name: ckecs-domain-reference
description: Use when reasoning about EnTT/CkEcs internals — registry/sparse-set/view/group mechanics, entity index+version staleness, in_place_delete vs swap-and-pop, why a stale FCk_Handle fails ck::IsValid, MarkedDirtyBy/pump passes, where processors tick from, Request_DestroyEntity teardown order, actor↔entity mapping (Get_ActorEntityHandle, ck::ToEntity), TransientEntity, or UE GC collecting UObjects held only by fragments. Not for architecture-why (ckecs-architecture-contract) or add-a-fragment how-to (ck-macros-and-codegen).
---

# CkEcs Domain Reference — the ECS mental model

Theory pack for CkFoundation's ECS: what EnTT (the vendored C++ ECS library, **v3.16.0**,
`Source/CkThirdParty/Public/CkThirdParty/entt-3.16.0/`) actually does, how CkEcs wraps it, and how
entity lifetime maps onto UE worlds, actors, and the garbage collector. Every claim below cites the
vendored source or CkEcs source (file:line, verified 2026-07-02). Lingo (Fragment = component,
Processor = system, Handle, Request, Signal): root `CLAUDE.md` — this file assumes those words.

## When NOT to use this skill

| You want | Load instead |
|---|---|
| WHY the architecture is shaped this way; invariants, signal lifecycle, typesafe-handle contract | `ckecs-architecture-contract` |
| Mechanical how-to: add a fragment/processor/request/handle; macro expansions | `ck-macros-and-codegen` |
| Debug a build/UHT/link failure | `ck-debugging-playbook` |
| Incident history (what broke before) | `ck-failure-archaeology` |

## Layer map

```
UWorld
 └─ UCk_EcsWorld_Subsystem_UE              owns TUniquePtr<entt registry> + slot registration
     ├─ ck::registry_table                 (slot, generation) → entt registry*   [global table]
     ├─ ACk_EcsWorld_Actor_UE (per ETickingGroup)  ticks one FProcessorScheduler
     └─ TransientEntity                    per-registry root entity (world fragment lives here)
entt::basic_registry<FCk_Entity::IdType>   one sparse-set storage pool PER FRAGMENT TYPE
FCk_Registry                               trivially-copyable VIEW = FCk_RegistryHandle (slot+gen)
FCk_Handle                                 FCk_Entity (index+version) + FCk_RegistryHandle
FCk_Handle_[Feature]                       same bytes, typed; minted only via ck::StaticCast
```

---

## 1. EnTT as vendored (3.16.0)

### 1.1 Entity = index + version

`entt::entity` is `enum class entity : uint32` (`entity/fwd.hpp:14`; `ENTT_ID_TYPE` = `std::uint32_t`,
`config/config.h:39-41`). The 32 bits split into a **20-bit index** (`entity_mask 0xFFFFF`) and a
**12-bit version** (`version_mask 0xFFF`) (`entity/entity.hpp:31-40`). The version is the staleness
detector: destroying an entity bumps its version, so an old copy of the id — same index, old
version — no longer matches what the registry stores. `to_entity`/`to_version` split an id
(`entity.hpp:94-109`); `next()` increments the version, skipping the reserved all-ones value
(`entity.hpp:116-119`).

Two sentinels (`entity.hpp:203-384`): `entt::null` compares by **index** part, `entt::tombstone`
compares by **version** part — both encode as all-ones. `FCk_Entity` (the Ck wrapper,
`CkEcs/Entity/CkEntity.h:12-67`) default-constructs to `entt::tombstone` (`CkEntity.h:27`) and is
statically asserted to be nothing more than the id in non-editor builds (`CkEntity.h:86-92`).
12 bits of version ⇒ a slot's version wraps after 4095 destroys; a wrapped id can false-match. Low
practical risk, but it is why handles are re-validated, never trusted long-term.

### 1.2 Registry = per-type sparse-set pools

`entt::basic_registry` holds a `dense_map<id_type, std::shared_ptr<basic_sparse_set>>` — one
**pool per fragment type**, keyed by compile-time type hash (`entity/registry.hpp:243`), plus a
dedicated entity storage. `assure<Type>()` lazily creates a pool on first touch
(`registry.hpp:248-277`). There is no "archetype" anywhere: an entity is just an index; which
fragments it "has" is answered per-pool ("is this index in that pool?").

A **sparse set** is two arrays (`entity/sparse_set.hpp:138-165`): a paged *sparse* array indexed by
entity index (pages of `ENTT_SPARSE_PAGE` = 4096, `config.h:46-47`) holding positions into a dense
*packed* array of entity ids. Membership test = one indexed load; iteration = walk the packed array
(contiguous). `basic_storage` extends the sparse set with paged payload storage — pages of
`ENTT_PACKED_PAGE` = 1024 elements (`config.h:50-51`), allocated page-by-page
(`entity/storage.hpp:241-261`). Because payload lives in **independently allocated pages**, growing
a pool never relocates existing fragments: a `FFragment_X&` stays valid across *adds* of the same
type. Whether *deletion* moves fragments depends on the deletion policy — and CkEcs pins that
globally (§1.3).

`create()` recycles destroyed indices with bumped versions (`registry.hpp:503-505`).
`destroy(entt)` walks every pool in reverse pool-creation order, removing the entity from each,
then releases the id (`registry.hpp:544-551`). `valid(entt)` checks the id (index **and** version)
is live in the entity storage (`registry.hpp:485-487`); `current(entt)` reports the live version
for an index (`registry.hpp:495-497`). `emplace<T>` on an entity that already has `T` is UB
(assert) (`registry.hpp:597-610`) — the Ck wrapper turns this into an ensure (§2.1).

### 1.3 Deletion policies: swap-and-pop vs in_place (tombstones)

Enum `entt::deletion_policy` (`entity/fwd.hpp:17-26`): `swap_and_pop` (default), `in_place`,
`swap_only` (entity storage only).

- **swap_and_pop** — EnTT's upstream default, NOT what CkEcs pools use (see below)
  (`sparse_set.hpp:264-276`; payload side `storage.hpp:347-352`): the last packed element is moved
  into the erased slot. The packed array stays **dense** — iteration touches only live elements —
  but the moved fragment's address changes and iteration order is not stable.
- **in_place** (`sparse_set.hpp:282-286`; payload destroyed in place `storage.hpp:343-345`): the
  slot becomes a **tombstone** threaded into a free list (`head`). Fragment addresses are stable
  ("pointer stability") and erasing moves nothing, but the packed array accumulates holes that
  iteration must skip — density and cache locality degrade until slots are reused or the pool is
  compacted.
- **swap_only**: used by the entity storage itself (`storage.hpp:978-1042`) — destroyed ids stay in
  the packed array past the free-list boundary with a bumped version; that boundary is what
  `registry.valid()` tests.

Which policy a type gets: `component_traits<T>::in_place_delete` (`entity/component.hpp:42-55`) —
a per-type `static constexpr bool in_place_delete = true;` member (`component.hpp:20-22`),
**automatically** if the type is not move-constructible-and-assignable (`component.hpp:14-15`), or
a `component_traits` specialization.

**CkEcs forces in_place for EVERY fragment type.** `CkHandle.h:71-77` specializes
`entt::component_traits<Type>` for all types with `in_place_delete = true` (keeping the standard
page-size formula). Consequences:

- Fragment addresses are stable for the fragment's lifetime; erases move nothing.
- Every pool carries tombstones after churn. Views skip them silently — single-storage views via an
  explicit check (`view.hpp:23`, applied `view.hpp:459`), multi-storage views because a tombstone
  id matches no other pool. Tombstone slots are recycled by later emplaces through the free list;
  nothing in `Source/` ever calls `compact()` (rg-verified 2026-07-02).
- `FCk_Registry::Sort` on a pool that currently has tombstones trips EnTT's "Sorting with
  tombstones not allowed" assert (`sparse_set.hpp:1000,1054`) — sort only pools without pending
  in-place holes.
- The unconditional form is deliberate design, established by git (DECISIONS.md §45): introduced
  debug-gated (`745507381`, 2024-03-07), deliberately ungated in `06938bba3` (2026-02-17, "feat:
  fragments are always pointer stable"). The mechanical motive, INFERRED from the code shape:
  pointer stability for fragments whose members are referenced from outside the pool — signal
  fragments' bound-delegate state (`CkSignal_Fragment.h:44,99`) and the handle-debug mapper's raw
  fragment pointers (`CkHandle_Debugging.h:67`) both ALSO opt in with the per-type member form,
  which holds even in a TU where the CkHandle.h specialization isn't visible — plus erase-safety
  during processor iteration.

### 1.4 Empty types (tags) cost nothing

`component_traits<T>::page_size` is 0 for empty classes (`component.hpp:24-25`, via
`ENTT_ETO_TYPE`, `config.h:71-75`); Ck's global `component_traits` specialization reproduces the
same formula directly (`!std::is_empty_v<Type> * ENTT_PACKED_PAGE`, `CkHandle.h:76`), so the result
holds either way. A `page_size == 0` storage specialization stores **no payload at all** — the pool
is just the sparse set of member entities (`storage.hpp:789-819`; warning at `storage.hpp:220-222`).
This is why Ck tags (`CK_DEFINE_ECS_TAG`) are free-ish to add/remove, why `FCk_Registry::Get` on an
empty type returns a shared static instead of touching storage (`CkRegistry.h:737-741`), and why
tags never appear as `ForEachEntity` parameters (§2.4).

### 1.5 The storage_type specialization mechanism

`registry.storage<T>()` resolves the pool type through `storage_for` → `storage_type`
(`entity/fwd.hpp:225-262`): by default `sigh_mixin<basic_storage<T>>` — a mixin that adds
`on_construct`/`on_update`/`on_destroy` signals, surfaced on the registry at
`registry.hpp:1013-1071`. Customize a type's storage by specializing `storage_type`/
`component_traits` — CkEcs's blanket `component_traits` specialization (§1.3) is exactly this
mechanism in production. Two more facts about this codebase:

- **Nothing in CkFoundation subscribes to those storage signals or creates groups** (rg over
  `Source/` minus ThirdParty: zero `on_construct()`/`.group<` call sites, 2026-07-02). Reactivity
  is done with tag fragments + processors instead.
- **Named storages ARE used**: `registry.storage<T>(id)` creates a *separate pool of the same C++
  type per runtime id* (`registry.hpp:449-456`). CkEcs exposes this as
  `FCk_Registry::Storage<T>(Hash)` (`CkRegistry.h:298-302`, friend-gated) and CkEntityTag builds
  its per-FName tag pools on it (`CkEntityTag/CkEntityTag_Utils.cpp:24`).

### 1.6 Views — lazy intersection

`registry.view<A, B>(entt::exclude<C>)` builds a lightweight object holding **pool pointers only**
— nothing is computed or cached at construction (`registry.hpp:1081-1094`). At iteration time the
view picks the **smallest** pool as the *leading* pool (`view.hpp:244-254`, doc `view.hpp:419-425`
— NOT the first template argument; `use<T>()` overrides, `view.hpp:516-525`), walks its packed
array, and for each entity membership-checks every other pool and the exclude list
(`view.hpp:456-467`). Cost per step = one packed load + (N-1) sparse lookups. Views are cheap to
create every frame — that is exactly what processors do (§2.4).

The const overload of `registry.view` does **not** create missing pools — a never-touched fragment
type yields an empty view (`registry.hpp:1081-1087`); the non-const overload `assure`s pools into
existence (`registry.hpp:1089-1094`).

**Iteration invalidation contract** (verbatim rules, `view.hpp:203-219`): view iterators survive
(a) new entities/fragments being added, (b) the *currently returned* entity being modified or
destroyed. **Everything else invalidates** — in particular, removing the viewed fragment types
from *other* entities mid-loop. This is the mechanical reason for house non-negotiable #5
("requests are deferred; processors mutate"): a request-drain processor touches only the current
entity, which the contract explicitly permits. Deferred destruction (§3.4) exists for the same
reason. For cross-entity mutation from parallel code, `ck::FDeferredCommandBuffer` queues
Add/Remove/Replace closures per worker thread and flushes them single-threaded
(`CkEcs/Processor/CkDeferredCommands.h:13-137`).

### 1.7 Groups — owning, reordered storage (available, unused here)

`registry.group<Owned...>(get<...>, exclude<...>)` is the eager alternative: a `group_handler`
subscribes to `on_construct`/`on_destroy` of every involved pool (`entity/group.hpp:144-149`) and
**physically reorders the owned pools** so all group members sit contiguously at the front
(`group.hpp:103-152`) — iteration is then a straight walk of aligned arrays with no per-entity
checks. The prices: each pool can be owned by at most one group ("Conflicting groups" assert,
`registry.hpp:1119`), owned pools can no longer be user-sorted ("Cannot sort owned storage",
`registry.hpp:1173-1186`), and every add/remove of an involved type pays the bookkeeping.
CkFoundation uses **views only** (§1.5); know groups exist so you read upstream EnTT docs
correctly, but do not reach for them here without a maintainer conversation. Owning groups are in
fact statically forbidden today — the global `in_place_delete` trait (§1.3) trips `group.hpp:697`'s
static_assert. The adoption question (measure the tombstone cost, then decide) is
`ck-feature-frontier` candidate 5 — the decision procedure and constraints live there.

---

## 2. How CkEcs wraps EnTT

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

---

## 3. CkEcs ↔ UE lifetime mapping

### 3.1 World ↔ registry

One game registry per `UWorld`, owned by `UCk_EcsWorld_Subsystem_UE` (§2.5). Private extra worlds
(editor tooling) use `ck::FEcsWorld`, an RAII registry+slot owner (`CkEcs/World/CkEcsWorld.h:11-53`).
From any entity: `UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle)`
(`CkEntityLifetime_Utils.h:124-129`) — reads a `TWeakObjectPtr<UWorld>` fragment if the entity has
one, else walks the LifetimeOwner chain up to the TransientEntity, which always carries it
(`CkEntityLifetime_Utils.cpp:209-238`).

### 3.2 TransientEntity — the per-registry root

Every registry has ONE transient entity, stored in the underlying registry's context storage
(`entt::registry::ctx()`, a type-keyed value store) as `ck::FCtx_TransientEntity` so every
`FCk_Registry` view resolves the same one (`CkRegistry.h:36-48,191-196`). It is the root of the
lifetime-ownership tree — every other entity has a `FFragment_LifetimeOwner`; the transient entity
alone has none (by-design comment `CkEntityLifetime_Utils.h:79-84`). Get it via
`UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(World)` (`CkEcsWorld_Subsystem.h:197-213`) or
`UCk_Utils_EntityLifetime_UE::Get_TransientEntity(AnyHandleOrRegistry)`
(`CkEntityLifetime_Utils.h:230-239`). Use it as (a) the lifetime owner for world-scoped entities
(`Request_CreateEntity_TransientOwner`, `CkEntityLifetime_Utils.h:71-77`), (b) the handle
processors build views from (`CkProcessor.h:131`), (c) the world/subsystem access point when all
you have is "some handle".

### 3.3 Actor ↔ entity bridge

`UCk_Utils_OwningActor_UE` (`CkEcs/OwningActor/CkOwningActor_Utils.h`) — verified function names:

| Direction | C++ | header line |
|---|---|---|
| Actor → entity | `Get_ActorEntityHandle(InActor)` / `TryGet_ActorEntityHandle(InActor)` | `:97-111` |
| Entity → actor | `Get_EntityOwningActor(InHandle)` / `TryGet_EntityOwningActor(InHandle)` | `:57-71` |
| Entity → actor, walk owners | `TryGet_EntityOwningActor_Recursive(InHandle)` | `:73-79` |
| Bridge readiness | `Get_IsActorEcsReady(InActor)`, `Promise_OnActorEcsReady(InActor, InDelegate)` | `:113-128` |

The three environments (root non-negotiable #4):

```cpp
// C++
auto Entity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(InActor);
```
Blueprint: node **[Ck][OwningActor] Get Actor To Entity** (compact title `ActorToEntity`,
category `Ck|Utils|OwningActor`); reverse **[Ck][OwningActor] Get Entity To Actor**.
```angelscript
// AngelScript (Script/CkUtils_Common.as:5-20)
FCk_Handle Entity = ck::ToEntity(Actor);
FCk_Handle Self   = ck::ToEntity(EntityScript);
AActor Actor      = ck::ToActor(Handle);   // Checked by default — ensures if no OwningActor;
                                           // pawn-less ECS entities: TryGet_EntityOwningActor
```

Inside an EntityScript: `Get_AssociatedEntity()` (C++) / `DoGet_ScriptEntity()` (BP/AS).
**ContextOwner** is the DI-style scope root, distinct from LifetimeOwner:
`UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle)` (BP node **[Ck][Context] Get Entity
Context Owner**, compact `CTX`), `Request_Override`, `Request_OverrideToSelf`
(`CkEcs/ContextOwner/CkContextOwner_Utils.h:25-48`). LifetimeOwner answers "who destroys me";
ContextOwner answers "whose context/config do I resolve against".

### 3.4 Destroy flow — deferred, staged, leaf-first

`UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle)` does NOT destroy anything
synchronously (`CkEntityLifetime_Utils.cpp:26-72`): after validity + dedup + orphan-policy checks
it adds `ck::FTag_DestroyEntity_Initiate`, recursively requests destruction of all
`FFragment_LifetimeDependents` (children marked before the parent broadcast — leaf-to-root), then
broadcasts the `OnEntityBeginDestroy` signal. From there a tag pipeline advances stage by stage
through the scheduler's group order (`CkEcs/EntityLifetime/CkEntityLifetime_Processor.h:36-158`;
how many stages complete within one frame depends on group ordering and pump passes — INFERRED:
at least the remainder of the current frame elapses before the actual destroy):

```
Initiate → +EndPlay → +Teardown → +Await → +Finalize → registry.destroy()
           (FGroup_EntityLifecycle → FGroup_Teardown → FGroup_DestructionPipeline)
```

While `EndPlay`/`Teardown` tags are present, feature cleanup processors filtered by
`CK_IF_END_PLAY` / `CK_IF_TEARING_DOWN` run (destroy owned UObjects, unbind, disconnect records);
regular processors skip the entity via `CK_IGNORE_PENDING_KILL`. The final
`FProcessor_EntityLifetime_DestroyEntity` collects `Finalize`-tagged entities and calls the actual
EnTT destroy in its `DoTick` (`CkEntityLifetime_Processor.h:134-158`) — which erases the entity
from every pool and bumps the version (§1.2), flipping every outstanding handle to invalid.
Consequences: there is always a window where the entity is destroy-marked but still alive —
default-invalid yet pending-kill-valid (§2.2); never `registry.destroy` directly; cleanup logic
belongs in EndPlay/Teardown-filtered processors, not in signal callbacks. Regular processors keep
running on an `Initiate`-tagged entity for the rest of the frame by design
(`CkEntityLifetime_Fragment.h:35-36`).

---

## 4. UHT limitations that shaped the API

UHT (UnrealHeaderTool, the reflection codegen) parses UFUNCTION/USTRUCT declarations with a
restricted grammar. Each restriction below produced a house pattern (style itself: root
`CLAUDE.md`):

| UHT limitation | Ck pattern it produced | Evidence |
|---|---|---|
| No UFUNCTION overloads (one reflected name each) | suffix vocabulary: `_ByName`, `_ByTag`, `_Simple`, `AddOrReplace`, `TryGet_*`; a plain C++ overload MAY shadow a UFUNCTION name | root CLAUDE.md Naming; e.g. `Get_EntityOwningActor` vs `TryGet_EntityOwningActor` (`CkOwningActor_Utils.h:57-71`) |
| No trailing return types on UFUNCTION declarations | split declaration: concrete return type on its own line, `static FCk_Handle\nRequest_CreateEntity(...)`; trailing returns everywhere else | `CkEntityLifetime_Utils.h:67-69`; root CLAUDE.md function shapes |
| Historically no `TOptional` in reflected surfaces | enum-mode + value pair (`ECk_...` + payload). UE 5.5+ UHT now accepts TOptional UPROPERTYs and the 3 newest files use it — open fork, see ADJUDICATIONS **A1** before copying either way | `.claude/reports/ADJUDICATIONS.md` A1 |
| UFUNCTION parameter defaults must be `()`-constructible expressions, not `{}` | `{}` construction everywhere EXCEPT UFUNCTION param defaults, which use `()`; no `= {}` in UFUNCTION signatures | root CLAUDE.md; live: `FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f)` (`CkPmg/CkPmg_Utils_SymbolShapes.h:28`) |
| UHT owns UObject construction (GENERATED_BODY declares the ctor path; CDO factory) | `CK_DEFINE_CONSTRUCTORS` on value structs ONLY, never UObjects; UObjects configure via UPROPERTY defaults | root CLAUDE.md macro table; mechanism: `ck-macros-and-codegen` |

---

## 5. GC interaction rules

**The one-sentence law: UE's garbage collector traces UPROPERTY graphs only — EnTT fragment
members are INVISIBLE to it.** A fragment is a plain C++ struct living in an EnTT storage page;
no reflection, no `AddReferencedObjects`. A `UPROPERTY()` inside a USTRUCT-shaped fragment does
NOT help — nothing walks it once the struct lives in a pool. A UObject whose only reference is a
fragment member **will be collected** mid-play. This is not theoretical: the replication-driver
UObject was collected ~1.2s after possession whenever no netdriver happened to also reference it,
cascading into PlayerController destruction (fix `56b344310`; a follow-up audit swept 82 fragment
UObject fields). Full incident: `ck-failure-archaeology`.

**Ownership split (the rule that follows):**

- `TStrongObjectPtr<T>` — the entity OWNS the object (spawned component, render target, driver):
  roots it against GC; the feature's EndPlay/Destructor processor must Reset/destroy it.
- `TWeakObjectPtr<T>` — observation only: auto-nulls when the owner-of-record collects it; always
  re-validate with `ck::IsValid` before use.
- `TObjectPtr<T>` — only in real reflected contexts (UPROPERTY on a UClass/USTRUCT that UE
  actually traverses); inside fragments it is GC-invisible decoration.

**The audit question for ANY `UObject*`-family member in a fragment: "who roots this?"** Acceptable
answers: this fragment via TStrongObjectPtr; a UPROPERTY on a live outer (subsystem, actor,
component); the engine (asset in memory, rooted CDO). "The net connection replicator chain
happened to reference it" is how the incident above shipped — a masking reference, not ownership.

**"Disregard for GC" pool (one paragraph):** UE stamps every UObject created before engine init
completes into a permanent pool the GC never traverses (`gUObjectArray`'s disregard-for-GC set) —
they are immortal, AND their outbound references are never traced. In this codebase, AngelScript
`asset ... of ...` owners and AS CDOs are created during AS InitialCompile, *before* that set
closes. When post-boundary code attaches normal-pool objects under such an owner, the first real
GC collects the children out from under the untraversed parent → dangling pointer → packaged-only
0xC0000005 (PIE never exercises this timing; cooked Development client does). Root fix: a pre-GC
delegate feeds AS disregard objects through `CollectReferences` and `AddToRoot`s unrooted targets
(`feb08ee94`, diagnostics `Ck.Diag.VerifyGCAssumptions` / `Ck.Diag.DumpAngelscriptAssets`). The
boundary lesson: an object created during AS InitialCompile has DIFFERENT GC semantics than the
identical object created one frame later. Incident detail: `ck-failure-archaeology`.

---

## Common mistakes

1. **Holding `FCk_Entity` instead of `FCk_Handle`.** A raw entity has no registry and no staleness
   protection beyond its embedded version; store handles (typed where possible), re-check
   `ck::IsValid` at use time.
2. **Caching a fragment reference across mutations.** In this codebase addresses are stable
   (paged storage §1.2 + in_place pools §1.3), so the ref only dies when THAT entity's fragment is
   removed — but it dies silently: the object is destroyed in place and the slot may later be
   refilled for an unrelated entity. Re-`Get` per tick; never cache across frames.
3. **Structural mutation of OTHER entities inside `ForEachEntity`.** Violates the view
   invalidation contract (§1.6). Queue a request fragment, use `FDeferredCommandBuffer`, or tag
   and let another processor act.
4. **`MarkedDirtyBy` without consuming the marker.** The scheduler re-pumps you to the 30-pass cap
   and logs your processor as still-dirty every frame (§2.4). Symmetrically: a time-integrating
   processor without `PumpPolicy = SkipPump` re-applies work at DeltaT=0.
5. **Treating `Get_RegistryView()` / `operator*` results as long-lived references.** Returned by
   value; binding to a reference is a dangling temporary (`CkHandle.h:251-262`).
6. **Assuming default-invalid == destroyed.** During the destroy pipeline the entity still exists
   and EndPlay/Teardown processors still run; use `IncludePendingKill` /
   `CK_IF_HANDLE_IS_PENDING_KILL` when cleanup code must address it (§2.2, §3.4).
7. **Expecting the first View type to drive iteration.** The smallest pool leads (§1.6); don't
   "optimize" by reordering template arguments.
8. **A UObject reference parked in a fragment with no root.** It will vanish at an arbitrary GC;
   ask "who roots this?" (§5).
9. **Sorting a pool with pending tombstones.** `FCk_Registry::Sort` asserts "Sorting with
   tombstones not allowed" if the pool has un-reused in-place holes (§1.3); non-movable fragments
   additionally trip the pinned-type assert in swap paths (`storage.hpp:318-321`).

---

## Provenance and maintenance

Campaign date **2026-07-02**; verified against submodule HEAD `7330c1bab`, EnTT vendored 3.16.0.
Re-verification (PowerShell/Git Bash, cwd `d:\Repos\BusterBlock\Plugins\CkFoundation`):

- EnTT version: `ls Source/CkThirdParty/Public/CkThirdParty/ | grep entt` (expect `entt-3.16.0`);
  masks in `.../src/entt/entity/entity.hpp:31-40`.
- Cited EnTT internals move between minor versions — on an EnTT bump re-check:
  `deletion_policy` (`entity/fwd.hpp:17-26`), `in_place_delete` trait (`entity/component.hpp:14-22`),
  view invalidation doc (`entity/view.hpp:203-219`), smallest-pool pick (`view.hpp:244-254`),
  group conflict assert (`entity/registry.hpp:1119`).
- Groups/sigh still unused: `rg -n "\.group<|on_construct\(\)" Source -g '!*ThirdParty*'` (expect 0).
- Global in_place specialization still present: `rg -n "struct entt::component_traits" Source/CkEcs`
  (expect `CkHandle.h:72`); per-type members: `rg -n "in_place_delete" Source -g '!*ThirdParty*'`
  (expect CkSignal_Fragment.h ×2, CkHandle_Debugging.h, CkHandle.h).
- Slot table shape: `rg -n "kRegistryTable_MaxSlots|EnttRegistryType" Source/CkEcs/Public/CkEcs/Registry/CkRegistry_SlotTable.h`.
- Handle validity ladder: `rg -n "IsValid_Policy_IncludePendingKill\) const" Source/CkEcs/Public/CkEcs/Handle/CkHandle.cpp`.
- Processor CRTP: `rg -n "namespace ck_exp" Source/CkEcs/Public/CkEcs/Processor/CkProcessor.h`
  (expect :237, :342); marker consumption `rg -n "Remove<MarkedDirtyBy>" Source/CkTimer`.
- Tick flow: `rg -n "_Scheduler->Tick|DoBuildGraphAndSpawnActors" Source/CkEcs/Public/CkEcs/Subsystem/CkEcsWorld_Subsystem.cpp`;
  pump cap `rg -n "_MaxPumpIterations" Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorScheduler.h`.
- Destroy pipeline tags: `rg -n "CK_IGNORE_PENDING_KILL" Source/CkEcs/Public/CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h`.
- ProcessorScript subsystem still dormant:
  `rg -ln "Request_CreateNewProcessorScript" Source -g '!**/CkProcessorScript_Subsystem.*'` (expect 0
  — a single `*` glob does not cross `/`, so the old `!*CkEcs/...` form excluded nothing).
- A1 status (TOptional): `.claude/reports/ADJUDICATIONS.md` — re-read before teaching optionality.
- GC incident SHAs: `git log --oneline --no-walk feb08ee94 56b344310` (both exist in this repo's history).
- Tooling caveat: Grep/Glob tools are blind under `Script/`, `docs/`, `Content/` here — use
  `rg --no-ignore` (root CLAUDE.md provenance notes).
