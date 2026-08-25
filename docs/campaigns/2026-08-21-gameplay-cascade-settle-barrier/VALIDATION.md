# Validation — acceptance protocol

## Headless (toolbox, editor closed, `-DisablePlugins=RiderLink` env)
| Gate | Command | Expected |
|---|---|---|
| Spec | `--test --test-pattern CascadeWriteReaches` | 1/1 pass |
| Livelock guard | `--test --test-pattern PolledTransitionDoesNotTripBarrier` | 1/1 pass (no `Local settle after group` warning) |
| Floor intact | `--test --test-pattern OneShotPushReaches` | 1/1 pass — ONLY if BB #2706 + CkF #717 are checked out/merged (not on `dev` as of 2026-08-21); otherwise record "skipped — branches absent" |
| Drift contract | `--test --test-pattern Transform` | at baseline by name incl. `DirtyOwnersOnly` (record the count; the "47/47" figure came from the #717 PR run, re-measure) |
| SM suite | `--test --test-pattern StateMachine` | at baseline by name; any timing-shift listed for review Q8 |
| Rewind / Tween | respective patterns | baseline by name |
| Full suite | `--test` | baseline by name; `Local settle ... reached` = 0; `Pump limit [` = 0; `Dirty marker conflict` = 0 |
| A/B | revert the trait lines (git stash the campaign diff), rebuild, spec | RED again — proves the barrier is the cause |

The spec is also the standing tripwire for the plan's all-or-nothing validity: if any participant
ever gains `SkipPump` the whole barrier is silently disabled at registration, and the spec goes red.

## Performance (measure, don't assert)
- `stat CkScheduler`: record `Scheduler::LocalSettle` and `Scheduler::Pump` on a busy gym with
  and without the barrier (the expectation is "moved earlier, not added" — confirm the sum is ~flat).
  Expected per barrier-converged transition ≈4–5 passes; a chained transition ≈9–10 (trips the ≥8
  `High pump count` advisory, which the tail pump already trips today for the same chain).
- Full-suite wall time with vs without (note lane count; <5% is noise).

## [EDITOR-VERIFY] (human, PIE)
1. Rewind station, barrier on: outbox case appears on the table the instant rewinding completes;
   no inbox-cover table-flash. (Already true via #717 next-frame; with the barrier it is same-frame —
   the observable difference is only visible with #717 stashed, which is an optional extra check.)
2. Place-item giant-scale flash: with barrier OFF then ON. If ON removes it → second confirmed
   victim of the cascade-after-push shape. If not → not this mechanism; record and stop.
3. Any polled-condition HFSM (player Engaged/IsGrounded, claw machine, bowling) for 60 s: no
   `Local settle after group` in the log.

## Definition of done (route through ck-change-control)
- All gates at baseline; A/B proves causality; perf recorded in PROGRESS.md.
- Review questions 1–8 answered in PROGRESS.md (maintainer's calls on 3 and 7 recorded verbatim).
- Docs updated (CkStateMachine + CkEcs Claude.md; group comment); `.claude/reports/DECISIONS.md`
  entry (next number is 111) pointing here.
- Ship per `ck-ship-pr`: CkFoundation branch → PR; BusterBlock branch (spec tests) → PR with the
  gitlink bump as its own commit; #717 / #2706 remain separate PRs (merge order independent).
