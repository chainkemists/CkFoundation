# Gate125 BusterBlock NPC-AI compatibility continuation

> Historical Gate125 evidence. Gate126 subsequently corrected the liveness fixture and passed NpcAI 27/27; see [Gate126](GATE126_NPCAI_LIVENESS_FIXTURE.md). The remaining-failure statements below describe Gate125.

## Scope and status

This record continues the bounded effort to return the focused BusterBlock `NpcAI` gate to its unchanged-game **22/27** baseline. It follows the verified isolated A/B result in [Gate124 runbook](GATE124_BUSTERBLOCK_AB_RUNBOOK.md) and its [continuation handoff](CONTINUATION_PROMPT_BusterBlockCompatibilityAndABReadiness.md). It does not widen the verified isolated Recast/GroundNav A/B result into general gameplay compatibility or readiness.

The candidate source change and native EQS regression are present locally but remain uncommitted. Gate125d validated the regression, and Gate125e returned the focused `NpcAI` gate to **26/27**. The sole remaining row is the pre-existing `LivenessWatchdogAccrual` live-line exemption failure; this is not broader readiness. No cook, package, push, or dev advancement was performed. Gate125 makes no BusterBlock production-policy edit; it changes only the narrow EQS admission repair, its native regression, and four AngelScript test/diagnostic fixtures.

**Current rebase debt:** BusterBlock tracked `origin/dev` advanced independently after Gate124 to root `c314b1ed7d81b9f7461afb7900e85dc8f425de0d`, Foundation `369d6766d6fd62537a61886f391cb5aa2b5660fc`, and Tests `b8087d6eff8fdaa3e93452630db16568e2268d98`; none is an ancestor of the current candidate HEAD. Host tracked refs are unchanged. The candidate’s local-dev refs, working HEADs, index gitlinks, protected 13 files, and final-run identity remain unchanged, so Gate125e measures the current local candidate only. The six prior ancestry checks cover Gate124’s fetched dev/entry pins, not these new BusterBlock tips. Before any further rebase, establish a controlled preservation checkpoint for the uncommitted implementation in the shared checkout. Gate125 did not attempt a rebase or make additional commits.

## Evidence ledger

| Gate | Result | Meaning |
|---|---:|---|
| Gate125a | AngelScript compile failure, exit 76, 0 tests | A temporary diagnostic soft-class conversion was invalid. It was removed; this run is not behavior evidence. |
| Gate125b | AngelScript compile failure, exit 76, 0 tests | The second temporary diagnostic attempt also ran no tests. It was removed; this run is not behavior evidence. |
| Gate125c | 19/27, one boot, 6m3s | Diagnostic-only focused result. Raw runtime log: `D:\Repos\BusterBlock\Saved\Logs\Nav-Gate125c-NpcAI-Runtime.log`; SHA-256 `e1104a038ab90731a3fafe0071d62d0affc08ee9858a75e430c382e4f79e7830`. |
| Gate125d | C++ build 61.72s; `CkEqs` 2/2, exit 0 | Two boots: fresh discovery plus test. `NoDataFailsClosed` passed its controls/callback counts and sibling `ObbContainment` passed; 0 skipped/contaminated, in 3m19s. Raw archive: `D:\Repos\BusterBlock\Saved\Logs\Nav-Gate125d-EqsProjection-Runtime.log`. It contains no ensures or AngelScript errors/warnings; the FText self-test error is absent. |
| Gate125e | `NpcAI` 26/27, one boot, 5m26s, Toolbox 1 / editor 255 | All 27 rows ran; 0 skipped/contaminated. Only `LivenessWatchdogAccrual` failed its live-line exemption at 1.066707 s, matching the unchanged-game baseline about 1.066671 s. The five new Gate124j/Gate125c failures, `BelowFloorRecovery`, and `SidewalkOwnedRoute` passed. Raw runtime log: `D:\Repos\BusterBlock\Saved\Logs\Nav-Gate125e-NpcAI-Runtime.log`; SHA-256 `ae488e3063a5079964801165d1403cc69eea2d6bd096894328118b0cedbb90fa`. |

Gate125c does not establish a production regression. It converts several earlier suspicions into specific fixture and admission-path observations.

## Pre-fix Gate125c state-machine and fixture observations

In Gate125c, the three initial-idle assertions reported the actual state and leaf. The observed state was `Bb_NpcAI_SmState_Locomotion` with `Bb_NpcAction_TravelToPOI`, rather than Idle.

`StuckRecoverySuppression` releases and initializes at zero stall as intended, then accrues **1.133338 s** before POI blacklisting and a five-second `NoNavData` result reset the NPC into a fresh `TravelToPOI` episode before the assertion observes it. That explains the observed zero value at the assertion point; it does not prove a suppression-policy defect.

The `SidewalkForeignFollower` temporary fixture override and no-existing/None-owner preconditions removed the duplicate ensure. The actor was already automatically in `TravelToPOI`, so the fixture recorded no new leaf entry and zero routes. The fixture needs a real new locomotion entry before it can test foreign-follower ownership. Do not infer an ownership regression from Gate125c.

`SidewalkOwnedRoute` was an inconclusive pre-fix observation, not a retained blocker: it now passes with unchanged test expectations and unchanged production tuning. Its Recast adapter still delegates to the existing `FCk_Nav_Algorithm`; no GroundNav-default attribution is supported.

## Source-grounded EQS admission cause

Projection was already a required candidate-admission step for projected generators. The neutral navigation migration at `21aa19aa` accidentally made `NoData` or `Error` skip the projection/filter branch and retain raw generated candidates. The earlier NavSys-present/NoData behavior projected and rejected those candidates. That difference can make POI selection admit a route that the former path rejected.

The narrow repair moves snapped-candidate allocation and the final assignment outside the provider-health conditional. An unavailable provider now leaves the snapped array empty and rejects the candidates, matching required projection admission. It retains the existing exclusions: `EntitiesWithTag` bypasses projection, and disabled `_ProjectOntoNav` retains raw candidates. It does not change provider selection, query filters, path algorithms, or NPC policy. The temporary suppression observer was removed with an exact empty diff.

## Final validation and remaining boundary

Gate125d built in 61.72 seconds and passed `CkEqs` 2/2; Gate125e then ran all 27 `NpcAI` rows in one boot. Across Gate125a-e there were six editor boots: a/b each compile-failed before tests, c was the red diagnostic run, d used discovery plus test boots, and e was the final focused run. The final source/artifact identity matched before and after Gate125e, including the CkEqs DLL. `GATE125_VALIDATION_EVIDENCE.json` is authoritative for eight repository heads/local-dev refs unchanged, protected files 13/13 exact, mirrored files 16/16 exact, and six ancestry checks.

The two known external-actor paths each appeared twice in Gate125e. There were no ensures or AngelScript errors/warnings; ordinary startup warnings remain in the raw log. The Gate124 A/B passes remain historical evidence after this EQS rebuild; no fresh A/B result is claimed. The only remaining focused NPC row is the baseline `LivenessWatchdogAccrual` live-line exemption at 1.066707 seconds. All Gate124 deferred work remains open: direct async GroundNav markup-rejection rollback, installed-filter-policy mutation, AccessZone/Entryway production lifecycle, cook/package, matched performance, a production-map migration/rollback, and the Gate102 queue debt. Protected/unrelated work is retained, and the seven approved gyms remain closed.
