# PROGRESS — scheduler settle pass

## Status
| Phase | State | Session | Notes |
|---|---|---|---|
| Approval (Saad) | PENDING | — | PROMPT.md status line is the gate |
| 0 — red spec | not started | — | |
| 1 — plumbing (toggle off) | not started | — | |
| 2 — whitelist + green | not started | — | |
| Validation | not started | — | |

## Baselines (fill during Phase 0)
- Full-suite counts + failing names at campaign start: _(record here; 2026-08-11 reference:
  1553 total / 9 red, all attributed — see BusterBlock memory `alt_worktree_suite_reds_2026_08_11`)_
- Phase-0 spec failure line, verbatim: _(record here)_

## Decisions made for the executor (do not re-litigate)
- Settle = once after pump convergence; no outer loop; opt-in whitelist of 7 (+ layer template);
  default-off setting; `Pump()` (dt=0) dispatch; emitters/FireSignals/replication-tail deferred.
- `LastPushedTransform` stays.

## Blockers
_(Executor: append here and END THE SESSION instead of improvising. Include: phase, step,
exact command, exact output, what you expected.)_

## Session log
_(One line per session: date, phase, result.)_
