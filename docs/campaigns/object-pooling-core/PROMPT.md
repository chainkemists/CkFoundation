# Object pooling as a CkCore framework intrinsic — mission brief (PROMPT.md)

> **Written:** 2026-07-11. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** the campaign ships and `Source/CkCore` ships an ObjectPooling section in its
> Claude.md/README set. On death: delete, or tombstone ("Superseded by CkCore docs — kept for history").

## Goal

Object pooling stops being a standalone module (`feature/pool-module`'s CkPool) and becomes an
intrinsic CkCore capability hoisted into `UCk_Utils_Object_UE::Request_CreateNewObject`. Callers
optionally pass pooling params (or use an overload); whether the returned object is fresh or
recycled is invisible to them. A CkCore world subsystem owns the lifetime of every object vended
through this path (pinned in `TObjectPtr` UPROPERTY storage); fragments that used to be the sole
GC root via `TStrongObjectPtr` become `TWeakObjectPtr`. EntityScript gains a poolable instancing
policy: poolable scripts are released to the pool at EndPlay and recycled on the next spawn of the
same class+archetype, with all reflected properties reset to the archetype (participant property
skipped so bound delegates survive).

## Success criteria

1. `Request_CreateNewObject` with pool params returns a recycled instance on the second
   acquire-after-release of the same class+archetype; caller code is identical either way.
2. A recycled instance's reflected properties match the archetype it was created against
   (verified property-by-property in an autotest), EXCEPT `FCk_Handle_ObjectPoolingParticipant`
   properties, whose bound delegates still fire after recycling.
3. An EntityScript with the poolable policy: spawn → destroy entity → spawn again reuses the same
   UObject instance (pointer-identity assert in an autotest); `OnAcquiredFromPool` /
   `OnReleasedToPool` participant signals fire at the right moments.
4. A force-new (`InstancedPerEntity`) script survives a forced full GC mid-life with
   `FFragment_EntityScript_Current::_Script` being a `TWeakObjectPtr` (subsystem pin holds it).
5. Old CkPool module surface (EntityPool promise API, `ICk_ObjectPool_Poolable`) does not exist on
   this branch; `FCk_Handle_ObjectPoolingParticipant` is the single opt-in hook surface.
6. All six swept `TStrongObjectPtr` members (see Phase 3) are `TWeakObjectPtr` with subsystem-pinned
   lifetime; their features' existing test suites stay green vs the captured baseline.
7. A CkGameplayDebugger inspector lists live pools with stats (free/in-use/hits/misses/high-water)
   and per-instance state.
8. Public API verified in C++, Blueprint, AND AngelScript (root non-negotiable #4).

## Constraints & locked decisions

| # | Decision | Choice | Why |
|---|---|---|---|
| 1 | GC pinning model | Subsystem pins EVERY instance vended through the pooling-aware path — poolable AND force-new. `TryReleaseToPool` recycles poolable, unpins+destroys non-poolable. | UE outers do NOT root inners; a weak-only fragment ref to a force-new instance would be GC'd mid-life. One ownership model. **User-confirmed 2026-07-11.** |
| 2 | EntityPool (promise-based entity pooling, ~1.9k lines on `feature/pool-module`) | DROPPED — not ported. | Object-level pooling re-runs Construct/BeginPlay per acquire; construct-once entity pooling contradicts the pivot. Revive from the branch later if composition cost proves dominant. **User-confirmed 2026-07-11.** |
| 3 | Replicated EntityScripts + poolable policy | ALLOWED (no v1 rejection). | User call, overriding the reviewer lean to reject-in-v1. Consequence: a net autotest must cover recycling under replication (Phase 5). **User-confirmed 2026-07-11.** |
| 4 | Sweep scope (point 8) | Convert all six candidates (CkUnrealComponent `_Component`, CkPmg `_MeshComponent` ×2, CkAudio `_AudioComponent`, CkUI `_WidgetComponent`+`_WrapperWidget`) to subsystem-pinned + weak, as non-pooled vends (no recycling yet). | Uniform ownership now; per-feature pooling opt-in later. **User-confirmed 2026-07-11.** |
| 5 | Participant struct | `FCk_Pool_PoolableReceiver` → `FCk_Handle_ObjectPoolingParticipant`, ContextReceiver-shaped, lives in CkCore. Reflection-scanned (TFieldIterator, IncludeSuper), no cache. `ICk_ObjectPool_Poolable` UInterface deleted. | User-specified rename; interface is unreachable from AS and now redundant. |
| 6 | Recycle reset | Property sweep copying from the creation archetype (CDO when none), skipping properties of type `FCk_Handle_ObjectPoolingParticipant`. | Struct-property copy would clobber the non-reflected multicast delegates; skipping preserves binds (user point 10). |
| 7 | Participant bind semantics | AddUnique-style (re-bind of same object+method is a no-op). | Construct re-runs on recycled scripts; preserved delegates + re-bind must not double-fire. |
| 8 | Instancing enum | Keep `InstancedPerEntity` (= force-new) and ADD `InstancedPerEntity_Poolable`. No rename of existing values. | Additive avoids CoreRedirects on serialized assets. Decision made, not asked — say if you want a rename instead. |
| 9 | Actor pooling | EXCLUDED from this campaign. Subsystem vends plain UObjects only (`Request_CreateNewObject` never spawns actors). Old ObjectPool's `Acquire_Actor` / freeze-thaw actor paths are not ported. | The hoist is Request_CreateNewObject-centric; actors go through SpawnActor. Decision made, not asked — revisit when actor churn needs pooling. |
| 10 | Pool key | (UClass, archetype instance). Archetype pinned strong by the subsystem for pool lifetime. CDO used when no archetype supplied. | Reset-on-recycle resets to the archetype the instance was created against (user point 9). |
| 11 | Pool config source | Explicit params at the acquire site win; else the EntityScript CDO's pool-params property (for script pools); else per-class project settings (ported from CkPool_Settings into CkCore); else built-in defaults. | Mirrors the branch's precedence rule. |
| 12 | Branch strategy | New `feature/object-pooling-core` off `origin/dev` (b3894d535); `feature/pool-module` stays intact as the port source, never merged. | dev never contained CkPool — this is a port, not a refactor. |

## Non-goals

- **Entity pooling (construct-once)** — dropped per decision 2.
- **Actor pooling** — excluded per decision 9.
- **`NotInstanced` policy changes** — CDO-sharing path untouched (weak ptr to a CDO is trivially safe).
- **Retrofits of gameplay features to poolable policy** (e.g. VfxCue AutoPool) — separate follow-up.
- **CkTests/BB adoption beyond the test suite** — game-side adoption is the host project's business.

## Reading list

- Port source: `origin/feature/pool-module` — `Source/CkPool/**` (ObjectPool subsystem, PoolableReceiver, settings, Claude.md).
- Hoist target: `Source/CkCore/Public/CkCore/Object/CkObject_Utils.h` (`Request_CreateNewObject` family, `Request_CopyAllProperties`, `Request_ResetAllPropertiesToDefault`).
- Pattern to mimic for the participant: `Source/CkEcs/Public/CkEcs/ContextReceiver/CkContextReceiver.h` + `_Utils` (reflection-scan injection).
- EntityScript lifecycle: `CkEcs/Public/CkEcs/EntityScript/CkEntityScript.h`, `_Fragment.h(:70 _Script)`, `_Processor.cpp` (:92-122 spawn switch, :469 EndPlay), `_Utils.cpp`.
- Subsystem base: `CkCore/Public/CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h`.
- Sweep audit (deref sites for the weak conversion): PROGRESS.md 2026-07-11 entry.
- Old tests to mine: `CkTests origin/feature/pool-receiver-autotests` (9 autotests; most are EntityPool-shaped and die with decision 2 — the receiver round-trip and veto subjects are reusable).

## Things ruled out — do not re-investigate

| Ruled out | Why | Evidence |
|---|---|---|
| "Outer keeps non-pooled objects alive" (original point 3 wording) | UE GC does not trace outer→inner; fragment strong ptrs were the sole root | Sweep 2026-07-11; plugin incident history (root CLAUDE.md, GC-untraced fragments) |
| Keeping `ICk_ObjectPool_Poolable` alongside the participant | AS can't implement UInterfaces; two surfaces for one hook = drift | Branch Claude.md; user point 10 |
| Construct-once recycling (skip Construct on acquire) | Entity is fresh each spawn — features MUST recompose on it; skipping Construct orphans composition | Pivot discussion 2026-07-11 |
| Continuing `feature/pool-module` in place | Pivot deletes ~half the branch (EntityPool, interface, actor paths) and relocates the rest across modules | Plan decision 12 |
| ECS registry entities for pools (branch's RecordOfObjectPools) | CkCore cannot depend on CkEcs; the debugger inspector enumerates the subsystem directly | Module tier table (CkCore = T1) |
