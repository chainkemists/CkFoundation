# Phase 2 — EntityScript integration

> **Status:** ⏳ Pending
> **Depends on:** Phase 1 ✅

## Goal

After this phase: an EntityScript class marked poolable is recycled across spawn/destroy cycles
(same UObject, properties reset to archetype, delegates preserved); force-new scripts are
subsystem-pinned; `FFragment_EntityScript_Current::_Script` is a `TWeakObjectPtr`.

## Entry criteria

- [ ] Phase 1 exit re-verified on current HEAD (hash recorded here: ______).
- [ ] Baseline captured: full CkTests suite counts + failing names (EntityScript/EntityLifetime/Net
      groups at minimum) on the Phase-1 binary.
- [ ] Re-read `CkEntityScript_Processor.cpp` spawn switch + EndPlay on current HEAD (plans go stale).

## Work items

1. `CkEntityScript.h` — add `ECk_EntityScript_InstancingPolicy::InstancedPerEntity_Poolable`
   (DisplayName "Instanced Per Entity (Poolable)"); add
   `FCk_ObjectPooling_PoolParams _PoolParams` UPROPERTY(EditDefaultsOnly,
   EditCondition = poolable value, EditConditionHides) + `CK_PROPERTY_GET`.
   → verify: compiles; property shows/hides in editor per policy [EDITOR-VERIFY].
2. `CkEntityScript_Processor.cpp` spawn switch (:92-122 pre-change) — `InstancedPerEntity` AND
   `InstancedPerEntity_Poolable` both vend via the pooling-aware `Request_CreateNewObject`
   (archetype = `EntityScriptClassArchetype`, pool params from CDO property for poolable /
   force-new marker for plain). `NotInstanced` untouched (CDO).
   → verify: spawn works for all three policies in an autotest.
3. `CkEntityScript_Fragment.h/.cpp` — `_Script` → `TWeakObjectPtr<UCk_EntityScript_UE>`; ctor +
   `SerializeSnapshot` load path vends through the subsystem (pin) instead of bare
   `NewObject`+strong. Audit ALL deref sites (list in PROGRESS.md 2026-07-11 entry:
   Processor :275,295,328,352,361,363,374,409,411,436,455,476; Utils :83-87,115; Fragment :96).
   → verify: compile + full-suite diff vs baseline; forced-GC autotest (success criterion 4).
4. `CkEntityScript_Processor.cpp` EndPlay (:469 pre-change) — after `EntityScript->EndPlay()`,
   `TryReleaseToPool(Script)`; subsystem recycles poolable / unpins force-new. Fragment weak ptr
   left as-is (dies naturally).
   → verify: poolable round-trip autotest (pointer identity across spawn→destroy→spawn).
5. Replication interplay (decision 3: ALLOWED) — confirm `FRequest_EntityScript_Replicate`'s
   existing weak `_Script` and the pending-replication matcher (`ConsumeFirst`) behave when the
   matched script instance is a recycled one. No guard added; net test in Phase 5.
   → verify: existing `Ck.*.Net` EntityScript specs green vs baseline.

## Expected observations at the gate

| I will run | I expect | If instead | Response |
|---|---|---|---|
| Poolable round-trip autotest | Same instance, reset properties, participant signals, Construct+BeginPlay re-ran | Double-fired delegates | AddUnique bind semantics broken — fix Utils, not the test |
| Forced full GC mid-life (force-new script) | Script alive (subsystem pin) | Script collected | Pin path missed a vend route (check snapshot + replicate paths) |
| Full CkTests suite | Counts == baseline | New reds | A/B-stash to prove ownership; fix or revert before exit |

## Exit criteria — same commit as last work item

- [ ] All expected observations confirmed; evidence in PROGRESS.md.
- [ ] Suite diff vs Phase-2-entry baseline recorded ("baseline N failing {names} → still N {names}").
- [ ] [EDITOR-VERIFY] items listed with exact steps (pool-params property visibility).
- [ ] PROGRESS.md dated entry.
