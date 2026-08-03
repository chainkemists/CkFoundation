# CkEcs ↔ UE lifetime mapping

Reference for `ckecs-domain-reference`. Worlds, the transient root, the actor bridge, and the deferred leaf-first destroy flow.

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

