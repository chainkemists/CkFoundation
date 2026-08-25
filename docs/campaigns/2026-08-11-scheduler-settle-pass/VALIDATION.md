# Validation — acceptance protocol (final phase)

## Headless gates (all via the toolbox, editor closed, `-DisablePlugins=RiderLink` env)
| Gate | Command | Expected |
|---|---|---|
| Spec | `--test --test-pattern SettlesSameFrame` | 1/1 pass |
| Regression pin | `--test --test-pattern OneShotPushReaches` | 1/1 pass |
| Drift contract | `--test --test-pattern Transform` | 47/47 incl. `DirtyOwnersOnly` |
| Tween/Rewind | `--test-pattern Tween`, `--test-pattern Rewind` | baseline counts (known reds by name only) |
| Full suite | `--test` | baseline counts from PROGRESS.md; 0 `Dirty marker conflict` |
| Toggle-off A/B | set `_EnableSchedulerSettlePass=False`, rerun spec | spec RED again (proves the setting truly gates it); flip back per maintainer's default decision |

## Performance (non-negotiable #7 — measure before claiming)
- Idle overhead: in a quiet gym, `stat CkScheduler` — record `Scheduler::Settle` avg with the
  toggle on vs off. Claim numbers only from this capture. The design expectation (empty-view
  short-circuits ≈ version-sum checks) must be CONFIRMED, not asserted.
- Suite duration: full-suite wall time on vs off (1 run each; note lane count; treat <5% as noise).

## [EDITOR-VERIFY] (human, PIE)
1. Rewind station, toggle ON: outbox case appears on the table the moment rewinding completes;
   inbox cover never table-flashes. (Should already hold via #717; settle makes it same-frame.)
2. The giant-item prediction: reproduce the place-item one-frame-large flash with the toggle
   OFF, then ON. If ON eliminates it → record as confirmed second victim of the pump/phase gap.
   If it persists → record as NOT this mechanism (valuable either way; do not chase in this campaign).

## Three environments
No new public API surface (trait + setting only) — C++/BP/AS check reduces to: settings row
visible in Project Settings (BP-facing), AS compile clean (no bindings added).

## Definition of done (route through ck-change-control)
- All gates above green/at-baseline; perf numbers recorded in PROGRESS.md.
- Docs updated: `CkEcs/CLAUDE.md` scheduler section gains a Settle-pass paragraph (two-phase →
  three-phase tick description + the whitelist trait); `.claude/reports/DECISIONS.md` entry
  linking this campaign folder.
- Ship per `ck-ship-pr` (maintainer preference): CkFoundation feature branch → PR; BusterBlock
  branch (test + ini flip if kept) → PR with gitlink bump as its own commit.
- The maintainer's default-on/off decision recorded in PROGRESS.md; if default-on, a follow-up
  note listing the deferred campaigns (emitters, tail reorder, fixpoint, FireSignals).
