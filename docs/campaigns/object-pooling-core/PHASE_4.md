# Phase 4 — Test suite (CkTests companion branch)

> **Status:** ⏳ Pending
> **Depends on:** Phase 2 ✅ (Phase 3 parallel-safe)

## Goal

After this phase: a CkTests branch (`feature/object-pooling-autotests`) carries the pooling
suite; all tests green on the final CkFoundation binary. Old
`origin/feature/pool-receiver-autotests` is mined for subjects, not merged.

## Entry criteria

- [ ] CkTests current branch clean; new branch off its dev.
- [ ] `reference_toolbox_test_discovery_gotchas`: new tests need `--discover-fresh`; net tests need
      explicit `--test-pattern`.

## Work items (AS autotests unless noted)

1. ObjectPooling core: acquire→release→acquire pointer identity; reset-to-archetype
   (authored non-default property values on the archetype; recycled instance matches); participant
   property SKIPPED by reset (delegates still bound and fire).
2. Exhaustion/capacity/grow-batch policies (port surviving logic tests from the old branch).
3. Veto path: `_CanBePooled = false` → release destroys instead of storing.
4. EntityScript poolable round-trip: spawn→destroy→spawn same instance; Construct/BeginPlay re-ran;
   participant signals ordered (OnReleased at EndPlay, OnAcquired before Construct on reuse).
5. Force-new GC pin: force full GC mid-life; script alive; after entity destroy + GC, script gone
   (unpin proof — use weak ptr observation, not IsValid on a recycled handle).
6. Replicated poolable script under PIE net session (decision 3): spawn/destroy/respawn on
   authority; client sees consistent entity scripts; no `[REP_DEBUG]` flood. (Net spec — explicit
   `--test-pattern`.)
7. C++ unit test (plain `FAutomationTestBase`, CkTests C++ side): reset-to-archetype property sweep
   on a hand-built UObject — pure logic, no world.

## Exit criteria

- [ ] All new tests green on final binaries (fresh build AFTER last source edit — stale-green trap).
- [ ] Full CkTests suite diff vs campaign-start baseline recorded.
- [ ] PROGRESS.md dated entry with counts.
