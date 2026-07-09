# CkPool

**Purpose:** Object pooling — recycle expensive-to-construct things instead of destroy/respawn churn.
Two flavors: **EntityPool** (EntityScript-spawned entities, deferred/promise-based) and **ObjectPool**
(arbitrary UObjects/AActors, synchronous). Shared policy vocabulary (prewarm, capacity, exhaustion)
and a shared per-instance opt-in hook mechanism (interface for C++/BP, receiver property for everyone
including AngelScript).

**Depends on:** `CkCore`, `CkEcs`, `CkLabel`, `CkLog`, `CkSettings`.
**Used by:** gameplay code with high spawn churn (projectiles, VFX carriers, pickups, transient NPCs).

---

## When to pool (and when not to)

EnTT already recycles entity identifiers (version-bumped free list) and fragment storage — an
explicit pool buys you **nothing** for id/memory churn. Pool only when per-instance
**(re)construction cost** dominates: EntityScript composition (child entities, fragments,
components), actor spawning (a top hitch source engine-wide), physics/rendering proxy setup.

Do NOT pool:
- Replicated EntityScripts / replicated actor classes — rejected at pool creation (v1). Client
  channel/NetGUID bookkeeping does not survive reuse. Route replicated churn through dormancy or a
  replicated-manager + FastArray pattern instead.
- Anything cheap to construct — measure first (`ck-performance-and-analysis` skill).

---

## EntityPool — `UCk_Utils_EntityPool_UE`

Pools are ECS entities (`FCk_Handle_EntityPool`), keyed by `TSubclassOf<UCk_EntityScript_UE>`
(+ optional `FGameplayTag` pool name for multiple configs of one class), tracked by
`UCk_EntityPool_Subsystem_UE`.

**The core contract — Construct ONCE, per-use data via hooks:**

1. Instances are spawned at prewarm/grow via `EntityScript::Add` with the pool's
   `ConstructionSpawnParams`. `Construct`/`BeginPlay` run **once per instance lifetime**, not per acquire.
2. `Request_Acquire(WorldCtx, Class, PerUseParams)` returns `FCk_Handle_PendingEntityPoolAcquire` —
   bind `Promise_OnAcquired` **the same frame**. Fulfillment is deferred to the pool's
   HandleRequests processor.
3. Per-use data (`FInstancedStruct`) arrives through the acquired-entity hooks, never Construct.
4. `Request_ReleaseToPool(PooledEntity)` returns the instance; the pool parks it dormant (tags:
   `FTag_EntityPool_Dormant` / `FTag_EntityPool_InUse`) or re-vends it to a parked acquire.
5. The pool owns instance lifetime (instances are its lifetime children). Destroying an in-use
   pooled entity externally is legal "steal" semantics; destroying a dormant one is a caller bug
   (ensure fires).
6. Stored handles across acquires: pooled entities keep the same EnTT id+version, so `ck::IsValid`
   cannot detect recycling — cache `Get_UseGeneration` next to any stored handle and compare on read.

**Policies** (`FCk_Fragment_EntityPool_ParamsData`): `_PrewarmCount` + `_PrewarmBudgetPerTick`
(amortized warm-up), `_CapacityPolicy` (Unbounded/Bounded+`_MaxSize`), `_ExhaustionPolicy`
(Grow — parks the acquire at bounded capacity; Fail — fulfills the promise with Failed).
Auto-created pools (bare `Request_Acquire`) read the class's entry in **Project Settings → Pool**,
else built-in defaults; an explicit `Request_CreatePool` always wins.

**Quiescence is the pooled script's job.** On release: deactivate visuals/audio, cancel per-use
timers, unbind per-use signals. The pool never touches feature state.

---

## ObjectPool — `UCk_Utils_ObjectPool_UE`

Synchronous pooling for UObjects/AActors, keyed by class, hosted by `UCk_ObjectPool_Subsystem_UE`
(tickable only for amortized prewarm). `Acquire`/`Acquire_Actor` return immediately (pooled
instance, fresh spawn under Grow, or null at Fail/capacity). Instances are GC-rooted by the
subsystem's UPROPERTY arrays.

- Generic freeze/thaw is automatic: release = hidden + collision/tick off + timers cleared;
  acquire = CDO-default visibility/collision/tick (+ optional transform).
- Externally destroyed in-use instances are swept lazily ("steal"); destroying a FREE instance
  fires an ensure.
- Per-use `FInstancedStruct` params ride `Acquire`/`Acquire_Actor` into the receiver hook (below).

---

## Poolable opt-in hooks — interface vs receiver property

Deep per-use reset (physics velocities, AI state, bound delegates, per-use timers) is the
implementer's job. Two equivalent surfaces:

| Surface | Who can use it | Hooks |
|---|---|---|
| `ICk_ObjectPool_Poolable` (UInterface) | C++ / Blueprint only | `PrepareForUse`, `PrepareForPool`, `Get_CanBePooled` |
| `FCk_Pool_PoolableReceiver` (struct property) | **anyone, incl. AngelScript** | `OnAcquiredFromPool(FInstancedStruct)`, `OnReleasedToPool`, `_CanBePooled` |

**Why the receiver exists:** the Hazelight AngelScript fork cannot implement UInterfaces (no
interface support in the AS class generator). Declaring a `FCk_Pool_PoolableReceiver` UPROPERTY on
any class opts the instance in — the pools reflection-scan for the struct type
(`TFieldIterator<FStructProperty>`, `IncludeSuper`), exactly like `FCk_Handle_ContextReceiver`.
No cache: AS hot-reload regenerates class layouts, so properties are re-scanned per call.

**Firing order:**
- ObjectPool acquire: generic thaw → interface `PrepareForUse` → receiver `OnAcquiredFromPool`.
- ObjectPool release: veto check (interface OR receiver; false ⇒ destroy instead of store) →
  interface `PrepareForPool` → receiver `OnReleasedToPool` → generic freeze.
- EntityPool deliver: entity signal `OnAcquiredFromPool` → receiver on the EntityScript instance.
- EntityPool release: receiver veto (false ⇒ entity destroyed, steal-path reconciliation) →
  entity signal `OnReleasedToPool` → receiver.

**Where to bind:** actors — `BeginPlay` (fires once, at hidden pool-spawn); EntityScripts —
`Construct`. Plain `NewObject` UObjects have no self-init hook in AS — known v1 limitation
(the interface path is equally unreachable there).

### AngelScript example (EntityScript)

```angelscript
class UMy_Projectile_EntityScript : UCk_EntityScript_UE
{
    default _Replication = ECk_Replication::DoesNotReplicate; // EntityPool v1 requirement

    UPROPERTY()
    FCk_Pool_PoolableReceiver _Poolable;

    // Construct runs ONCE per instance (prewarm/grow) — compose features, then bind the hooks
    // ... in Construct:
    //     _Poolable.BindTo_OnAcquiredFromPool(FCk_Delegate_PoolableReceiver_OnAcquired(this, n"OnAcquired"));
    //     _Poolable.BindTo_OnReleasedToPool(FCk_Delegate_PoolableReceiver_OnReleased(this, n"OnReleased"));

    UFUNCTION()
    private void OnAcquired(FInstancedStruct InPerUseParams)
    {
        // per-use init: teleport, re-activate visuals, apply InPerUseParams
    }

    UFUNCTION()
    private void OnReleased()
    {
        // quiescence: deactivate visuals/audio, cancel per-use timers, unbind per-use signals
    }
}
```

Consumers acquire/release via the `utils_entity_pool` / `utils_object_pool` namespaces (generated
from the BPFLs); the pending-acquire promise is `Promise_OnAcquired` on the returned handle.

---

## Snapshot / replication notes

- Pool bookkeeping is deliberately NOT snapshotable (transient tags, no `CK_REGISTER_SNAPSHOTABLE`).
  Pooled entities that round-trip a snapshot come back as plain entities — destroy them normally;
  never Release into a pool that no longer tracks them.
- v1 rejects `Replicates` EntityScripts and replicated actor classes at pool creation, loudly.

## Anti-patterns

1. **Don't pass per-use data via ConstructionSpawnParams** — Construct runs once, not per acquire.
2. **Don't bind `Promise_OnAcquired` a frame late** — the fulfillment ticket does not outlive its
   fulfillment.
3. **Don't Release an entity you already destroyed** — steal is legal, steal-then-Release is a bug.
4. **Don't store a pooled-entity handle without `Get_UseGeneration`** — same id+version across uses.
5. **Don't add an `IsPoolable` interface to AS classes** — impossible; use `FCk_Pool_PoolableReceiver`.
6. **Don't pool replicated things** — rejected in v1 by design; see Snapshot/replication notes.

## See also

- `CkEcs/Claude.md` — EntityScript lifecycle (Construct/BeginPlay/EndPlay), signals.
- `CkEcs/ContextReceiver/` — the reflection-scan opt-in precedent the receiver mirrors.
- Engine precedents: `FWorldPSCPool` (Niagara), `IMassActorPoolableInterface` (Mass).
