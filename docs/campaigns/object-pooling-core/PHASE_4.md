# Phase 4 — Test suite (CkTests companion branch)

> **Status:** 🟡 Tests green; full-suite regression diff running (2026-07-11)
> **Depends on:** Phase 2 ✅ (Phase 3 parallel-safe)
> **Branch:** `feature/object-pooling-autotests` off CkTests dev (ac4f00d)
> **Drift notes (design changes this session, code wins):**
> - Per-use `FInstancedStruct` dropped from the whole acquire chain (user call) — participant
>   delegates are payload-free; the veto test's subject method is gone.
> - `_CanBePooled` veto REMOVED entirely (user call) — no per-instance opt-out; "never recycle" is
>   the force-new policy. Veto autotest + subject deleted.
> - `Get_ScriptInstance` NOT added (user call) — tests observe via public pool-stats + the direct
>   plain-object API only. Instance-level proofs (identity/reset/participant/pin+GC) live on plain
>   pooled objects; EntityScript tests prove policy wiring via `Get_ObjectPoolStats`.
> - Subsystem methods renamed public: `AcquireFromPool`, `TryReleaseToPool`. Predicate renamed
>   `Get_IsPoolTrackedObject`. "Vend" terminology eliminated repo-wide.
> - Final suite = 4 tests (was 7 planned): recycle+reset+delegates (plain), pinned+GC+unpin (plain),
>   poolable-script-recycles (stats), force-new-script-never-pools (stats). Grow-batch/capacity/perf
>   ports and the net-replicated-poolable test deferred (see open items).

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

- [x] All new tests green on final binary — VERIFIED 2026-07-11: `--test-pattern ObjectPooling
      --discover-fresh` → 4/4 Success, 0 AS errors, EXIT CODE 0. Build was fresh (build_p4_try4)
      AFTER the last C++ edit; AS temporary-chain fix was AS-only (recompiles at boot).
- [ ] Full CkTests suite diff vs campaign baseline — RUNNING (suite_p4_full).
- [ ] Net-replicated poolable test (decision 3 safety net) — DEFERRED to a follow-up; needs a PIE
      net harness. Logged in open items.
- [x] PROGRESS.md dated entry.

## Deferred (logged, not silently dropped)
- Grow-batch / capacity / exhaustion-policy tests (portable from the old branch's logic).
- Perf A/B (spawn-vs-recycle) — belongs with `ck-performance-and-analysis`, not correctness.
- Replicated poolable EntityScript under a PIE net session (decision 3 ALLOWED replicated poolable;
  this is its safety net — the current suite covers only DoesNotReplicate subjects).
