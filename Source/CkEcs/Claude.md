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
- `CK_PROPERTY(_TickRate)` — set a fixed tick rate (0 = every frame).
- `_TransientEntity` — a scratch entity the processor owns; use for deferred commands.

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

Processor scripts (`CkProcessorScript_UE`) are a Blueprint/AS-scriptable wrapper around a processor. Use them when artists or designers need to author behaviour without writing C++.

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
2. **One dispatch site.** `FProcessor_ReplicatedFragments_Dispatch` (`FGroup_Hydration`, ClientOnly — moved there in Phase 2 from `FGroup_Gameplay_Script`) drains pending entries each tick by resolving `FCk_ReplicatedFragmentHandlerRegistry` handlers. It skips entities tagged `FTag_EntityScript_ConstructedThisFrame` (so a just-composed entity's pending Setup drains first, no stomp); the sibling `FProcessor_Hydration_Dispatch` runs last in the group and clears that tag. `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` fire-gates the completion signal on any undrained replicated fragment across the lifetime-dependents tree.
3. **The handler contract is `Apply(Entity, New, TOptional Old) -> Applied | NotReady`** (plus optional `Remove`). `Old` is unset on the first application and otherwise holds the last APPLIED data — coalesced receives diff against what was actually applied. Return `NotReady` while the targeted feature is not composed yet; the dispatcher retries each tick and, past a timeout (5s dev / 2s shipping), drops the entry with an ensure naming the type and entity. Never compose the feature from inside Apply — composition belongs to construction / OnConstructed.

Registration lives in the feature's `_Fragment.cpp` via `RegisterLazy` (per-type) or `RegisterFallback` (runtime-typed payloads, e.g. dynamic fragments). The reference handler is Team (`CkRelationship/Team/CkTeam_Fragment.cpp`): `Has` check → `NotReady`, else assign → `Applied`.

### Two-signal client lifecycle contract

- **`OnConstructed`** (entity script) — the entity is COMPOSED. Replicated container values are NOT applied yet (the dispatcher runs after FinishConstruction in the same frame). Compose features here; do not read replicated values (team, attributes, SM state).
- **`OnReplicationComplete`** (`UCk_Utils_EntityReplicationDriver_UE::Promise_OnReplicationComplete`) — replicated values are applied. The dispatcher's group (`FGroup_Hydration`) precedes `FGroup_Replication`, where `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` broadcasts — so by the time the callback runs, the initial container data has been dispatched. Read replicated values here. The promise fires retroactively if bound late (payload-in-flight semantics).

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
