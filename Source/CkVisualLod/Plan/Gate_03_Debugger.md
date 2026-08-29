# Gate 3 — Debugger surface

> **Status:** ✅ Done (2026-08-27) — CkGameplayDebugger 880c822, compile-green; [EDITOR-VERIFY]
> is the gym PIE pass in PROGRESS.md
> **Depends on:** Gate 1 ✅ (368651f5e); ran alongside Gate 2's remaining test item
> **Estimate:** 0.5 session — actual: same session

## Goal

After this gate: aiming at any VisualLod entity with the on-screen debugger enabled
(`ck.DebugOverlay 1`) shows its LOD state live — representation + slot, mid-fade alpha, promote
locks, hidden — and aiming at an arbiter shows the three budget counters and the observer mode.

## Work items

1. **Gen 3 overlay providers** (CkGameplayDebugger / `CkEntityDebugOverlay` — Runbook A, Aggro
   exemplar): `FCk_DebugOverlay_Provider_VisualLod` (member: Representation [slot] (+HIDDEN),
   fade alpha when mid-fade, lock count at Warning severity; compact token `LOD:P/F/-`) and
   `FCk_DebugOverlay_Provider_VisualLodArbiter` (budgets near/locked/unbudgeted, observer vs
   local-view discovery). Both registered + added to the hard-coded "All" layout +
   `CkVisualLod` Build.cs dep.
2. Deferred (follow-up, not this gate): view-cone/ring PMG world-draw, and a Gen 2 ECS-debugger
   inspector section — add when PIE use shows the card rows aren't enough.

## Expected observations at the gate

| I will run | I expect | If instead | Response |
|---|---|---|---|
| toolbox `--build` | green | errors | fix |
| [EDITOR-VERIFY] (maintainer): gym PIE + `ck.DebugOverlay 1` | member cards show the VisualLod section (priority 24, near Aggro's 22); arbiter card shows budgets; `LOD:P/F/-` pills | missing section | check layout membership first (registered ≠ rendered) |

## Exit criteria — same commit as the last work item

- [x] Build green (`=== Build succeeded ===`, 0 error lines; two fixes: severity enumerator is
      `Warn` not `Warning`; direct CkIskmRenderer link dep for the AS signal-payload thunks);
      PLAN row + Status header updated; PROGRESS entry appended
