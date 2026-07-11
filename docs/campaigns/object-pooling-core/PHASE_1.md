# Phase 1 — CkCore pooling core + Request_CreateNewObject hoist

> **Status:** ✅ Done (2026-07-11) — round-trip autotest [DEFERRED-TO-P5]
> **Depends on:** nothing (campaign start)
> **Drift note (code wins):** planned `CkObjectPooling_Common.h` was merged into
> `CkObjectPooling_Params.h` (single consumer, no shared-flavor split anymore); a third enum
> `ECk_ObjectPooling_RecyclePolicy` (Recycle / DestroyOnRelease) carries the pin-everything model —
> DestroyOnRelease IS the force-new vend, so there is no separate vend API.

## Goal

After this phase: `UCk_Utils_Object_UE::Request_CreateNewObject` overloads accepting pool params
vend subsystem-owned instances; a second acquire-after-release of the same class+archetype returns
the recycled instance with properties reset to the archetype; participant signals fire on
acquire/release. All in CkCore, compiling with zero CkEcs dependency.

## Entry criteria

- [x] Branch `feature/object-pooling-core` off `origin/dev` b3894d535 (created 2026-07-11).
- [x] Port source read: `origin/feature/pool-module` `Source/CkPool/**`.
- [x] Baseline: dev has NO CkPool — nothing to delete; this is additive to CkCore.

## Work items

New directory `Source/CkCore/Public/CkCore/ObjectPooling/`:

1. `CkObjectPooling_Common.h` — `ECk_ObjectPooling_CapacityPolicy`, `ECk_ObjectPooling_ExhaustionPolicy`
   (port of branch `CkPool_Common.h`, renamed; drop pool-name tag machinery — key is class+archetype).
   → verify: compiles; formatters registered (`CK_DEFINE_CUSTOM_FORMATTER_ENUM`).
2. `CkObjectPooling_Params.h` — `FCk_ObjectPooling_PoolParams` (class-less port of
   `FCk_ObjectPool_ParamsData`: prewarm count/budget, capacity, max size, exhaustion, grow batch) +
   `FCk_ObjectPooling_PoolStats` (port of `FCk_ObjectPool_Stats`).
   → verify: compiles; `CK_DEFINE_CONSTRUCTORS` shape matches house style.
3. `CkObjectPoolingParticipant.h` — `FCk_Handle_ObjectPoolingParticipant`: `_CanBePooled` UPROPERTY +
   non-reflected MC delegates `_OnAcquiredFromPool(FInstancedStruct)` / `_OnReleasedToPool`.
   Pattern: `CkContextReceiver.h` + branch `CkPoolableReceiver.h`.
   → verify: compiles; struct is BlueprintType; delegates NOT UPROPERTYs.
4. `CkObjectPoolingParticipant_Utils.h/.cpp` — `UCk_Utils_ObjectPoolingParticipant_UE`: BindTo/UnbindFrom
   (AddUnique semantics — decision 7), `Broadcast_AcquiredFromPool_OnObject`, `Broadcast_ReleasedToPool_OnObject`,
   `Get_CanBePooled_OnObject` (reflection scan: `TFieldIterator<FStructProperty>`, IncludeSuper, no cache).
   Port of branch `CkPoolableReceiver_Utils`, renamed, minus interface interop.
   → verify: compiles; BP-callable; scan finds the property on a test subclass.
5. `CkObjectPooling_Subsystem.h/.cpp` — `UCk_ObjectPooling_Subsystem_UE`
   (base `UCk_Game_TickableWorldSubsystem_Base_UE`, tick = amortized prewarm only). Port of branch
   `CkObjectPool_Subsystem` MINUS: ECS registry entity, actor spawn/freeze/thaw paths, interface calls.
   PLUS: (a) pool key = (UClass, archetype) with archetype pinned in a UPROPERTY; (b) `_VendedNonPooled`
   pinned set for force-new vends (decision 1); (c) recycle reset = property sweep from archetype
   skipping `FCk_Handle_ObjectPoolingParticipant`-typed properties (decision 6); (d) `TryReleaseToPool`:
   pooled → veto-check → participant OnReleased → store free; non-pooled vend → unpin (GC collects);
   unknown object → ensure-fail, returns Failed.
   → verify: compiles; no CkEcs include anywhere in the folder.
6. `CkObjectPooling_Settings.h/.cpp` — per-class default pool params project settings (port of branch
   `CkPool_Settings`, ObjectPool half only).
   → verify: compiles; appears under Project Settings.
7. `CkObject_Utils.h/.cpp` — new overloads: `Request_CreateNewObject(Outer, Class, Archetype, PoolParams, PerUseParams, InitFunc)`
   (template + BP surface); resolves `UWorld` from Outer → subsystem; no resolvable world =
   `CK_ENSURE_IF_NOT` with non-pooled create as the documented recovery. `TryReleaseToPool(UObject*)`
   BPFL forwarding to the subsystem. TransientPackage variants stay non-pooled.
   → verify: compiles; existing call sites unaffected (additive overloads only).

## Expected observations at the gate

| I will run | I expect | If instead | Response |
|---|---|---|---|
| Toolbox `--build` (BB editor target) | Clean compile, CkCore + all downstream | UHT/link errors | `ck-debugging-playbook`; fix before proceeding |
| Minimal CkTests AS autotest: acquire → release → acquire | Same pointer both acquires; properties reset; participant signals fired in order | Fresh instance second time | Debug pool keying (class+archetype identity) before Phase 2 |

## Exit criteria — same commit as last work item

- [x] Full toolbox build green on final binaries — VERIFIED 2026-07-11: `UnrealToolbox --build --target
      Editor --config Development` → 436 actions, `Result: Succeeded`; all 4 new .cpps compiled
      explicitly (adaptive-build excluded from unity), 0 errors.
- [x] Zero CkEcs references under `Source/CkCore/ObjectPooling/` — VERIFIED: `rg --no-ignore "CkEcs"`
      over the folder + CkObject_Utils.h/.cpp → no matches.
- [ ] Round-trip autotest green — [DEFERRED-TO-P5] (lands on the CkTests companion branch).
- [x] PROGRESS.md dated entry with build evidence.
