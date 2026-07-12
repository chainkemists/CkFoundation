# CkCore/ObjectPooling

Object pooling is a **CkCore framework intrinsic**, hoisted into
`UCk_Utils_Object_UE::Request_CreateNewObject`. Callers optionally pass pool params; whether the
returned instance is freshly created or recycled is invisible to them. A world subsystem
(`UCk_ObjectPooling_Subsystem_UE`) owns the lifetime of **every** instance it hands out.

## The one load-bearing idea

The subsystem **pins** every instance it hands out (in `TObjectPtr` UPROPERTY storage). That is why
holders keep `TWeakObjectPtr`, never `TStrongObjectPtr`: UE GC does **not** trace fragment members,
and an object whose only reference is a fragment would be collected mid-life regardless of its
Outer. The pin is the root.

## Two policies (`ECk_ObjectPooling_RecyclePolicy`)

| Policy | On release | Use it for |
|---|---|---|
| `Recycle` | Park the instance in its pool; the next acquire of the same **(class, archetype)** re-issues it with all reflected properties reset to the archetype | Genuinely expensive-to-construct things reused with churn |
| `DestroyOnRelease` | Unpin only (GC then collects) — never recycled | "Force create new every time" **plus** subsystem-owned lifetime, so a fragment can hold the instance weakly. This is the sweep's mode for `UActorComponent`/widget holders |

There is **no** per-instance "can be pooled" veto. A class that must never recycle uses
`DestroyOnRelease`; per-use safety is the participant's `OnReleasedToPool` quiescence job.

## API

- **Create (pooled):** `UCk_Utils_Object_UE::Request_CreateNewObject(Outer, Class, Archetype, PoolParams, InitFunc)`
  (C++ template) / `Request_CreateNewObject_Pooled(...)` (BP/AS). Resolves the subsystem from
  `Outer`'s world; a no-world Outer ensures + falls back to a plain caller-owned create.
- **Release:** `TryReleaseToPool(Object)` — Recycle parks, DestroyOnRelease unpins. A **benign
  no-op** (returns `Failed`, no ensure) for objects the subsystem never handed out, so teardown
  paths call it unconditionally.
- **Query/tooling:** `Get_IsPoolTrackedObject(Object)`, `Get_ObjectPoolStats(WCO, Class, Archetype)`;
  subsystem-direct `ForEach_Pool`, `Get_PoolStats`, `Get_NumPinnedUnique`, and the public
  `AcquireFromPool` / `TryReleaseToPool` for code that already holds the subsystem.
- **Config precedence:** a per-class Project Settings entry OVERRIDES the acquire-site params when
  that class's Recycle pool is created (read once, not live-reactive); no entry → the acquire-site
  params apply as-is. `DestroyOnRelease` acquires never create a pool, so settings never touch them
  — force-new is decided at the call site (for EntityScripts: by the asset's instancing policy).

## Participant opt-in — `FCk_Handle_ObjectPoolingParticipant`

Declare a property of this type on any UObject-derived class to receive per-use hooks. The
subsystem reflection-scans for it (`TFieldIterator<FStructProperty>`, IncludeSuper), exactly like
`FCk_Handle_ContextReceiver`. AngelScript can't implement UInterfaces (Hazelight fork), so this
property route is the single opt-in surface for C++/BP/AS.

- `OnAcquiredFromPool` / `OnReleasedToPool` — payload-free notifications (per-use data flows through
  the caller, or an EntityScript's spawn-params + Construct).
- Binds are **idempotent** per `(object, function)` — `Construct` re-runs on every acquire of a
  recycled EntityScript, and the recycle reset **skips** this property so bound delegates survive.

## EntityScript integration

`ECk_EntityScript_InstancingPolicy::InstancedPerEntity_Poolable` recycles the script instance across
spawn/destroy cycles (pool config in the CDO's `_PoolParams`); plain `InstancedPerEntity` is
pinned-unique (DestroyOnRelease). Both vend through this subsystem; `EndPlay` releases.
`Construct`/`BeginPlay` re-run per acquire — pooling saves the `NewObject` + property-init cost, not
feature composition.

## Steal semantics & lifecycle edges (everything stays benign)

The subsystem never fights external destruction — "dev does not care whether the object is pooled"
also means teardown code may destroy or release in any order:

- **External destroy of a tracked instance ("steal")** — GC nulls the pinned slot; a post-GC sweep
  (plus a lazy per-acquire sweep) drops dead slots, reverse-map entries, pinned-unique entries, and
  zombie pools whose class/archetype died. Counts stay honest.
- **Destroy-then-release** — `TryReleaseToPool` on an already-destroyed object is a benign no-op
  (`Failed`, Verbose log) that reconciles tracking immediately. Only a NULL argument ensures.
- **Double release** — the second call is a benign no-op (the instance is no longer tracked as
  in-use).
- **Release quiesces like death** — releasing a tracked instance clears its pending world timers
  and latent actions (the `UActorComponent::EndPlay` pair). Pre-pooling, GC of the dead instance
  silenced those for free; pooling keeps the instance alive, so without this a lingering
  `SetTimer` would fire post-release against dead associations.
- **Recycled ≠ leaked native state** — the recycle reset covers REFLECTED properties only (plus
  re-instancing of `Instanced` subobjects so the recycled instance never aliases the archetype's).
  Non-reflected native members survive recycling by design — the participant handle relies on
  exactly this; any other native-state class must quiesce itself via `OnReleasedToPool`.

## World coverage

The subsystem exists in `Game`, `PIE`, **and `Editor`** worlds — the editor ECS world spawns
EntityScripts/components through the same pooled path, and those instances need the subsystem pin
as their GC root exactly like runtime ones.

## When NOT to pool

EnTT already recycles entity ids and fragment storage — pooling buys nothing there. Pool only when
per-instance (re)construction cost dominates. Measure first (`ck-performance-and-analysis`).

## Debugger

`ck.ObjectPoolingDebugger` (CkGameplayDebugger → `CkObjectPoolingDebugger`) opens a Slate tab
listing every pool with live stats + the pinned-unique count.
