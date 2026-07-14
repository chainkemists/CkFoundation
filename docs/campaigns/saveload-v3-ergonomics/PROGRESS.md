# PROGRESS — saveload-v3-ergonomics

Living doc. The EXECUTOR updates this at every phase boundary and whenever reality diverges from a phase
doc. Planner: Fable session 2026-07-14 (design discussion with Adam + 3-agent terrain survey). Branch:
`feature/save-load-improvements` (CkFoundation submodule; CkTests changes ride its same-named branch).

Adam's constraints: **harness first; minimize builds (one per phase, 4 total); each phase independently
shippable; quick.** Nothing is pushed by the executor (Class-4 → Adam review).

## Baseline (recorded at Phase 1 entry, 2026-07-14 — executor filled)

- CkFoundation HEAD at entry: `993f6323c` (clean tree: Y). After the campaign-docs commit `2f45437fa`
  (docs-only, `git add -f`; binary unaffected) that is the current tip; the baseline gates below ran against
  the `993f6323c` binary (parity 5B build; no source changed since).
- CkTests HEAD at entry: `4d55ec34a` (clean tree: Y).
- Parity campaign Phase 5 committed: **Y** — parity STOPPED at end of Phase 5 per its PROGRESS.md (CkF `993f6323`,
  CkTests `4d55ec34`). Parity PHASE_6 is absorbed by this campaign's Phase 2; PHASE_7/VALIDATION stays parity's.
- **Ck.Snapshot: 35 pass / 0 fail** (pattern `Ck.Snapshot`, exit 0, 6m38s). No failing names. Full name list:
  scratchpad `baseline-snapshot-names.txt` + `CkAuto/logs/erg-baseline-snapshot.log`. (Planner expectation 30/30;
  parity Phase 5 added tests → 35, all green.)
- **Ck.Net: 90 pass / 0 fail** (pattern `Ck.Net`, exit 0, 15m30s). No failing names. Full name list:
  scratchpad `baseline-net-names.txt` + `CkAuto/logs/erg-baseline-net.log`. (Planner expectation 90/90 — matched.)
- Both suites all-Success → no pre-existing reds to attribute a regression to. **THE by-name delta baseline** for
  every later phase = these two name lists (delta-zero measured with the identical patterns).

## Phase status

| Phase | Status | Commit(s) | Gate result vs baseline | Notes |
|---|---|---|---|---|
| 1 — harness + cast sweep + Jump absolute | DONE (2026-07-14) | CkF `3439ac756` (cast) + `f780dcf2d` (Timer Jump); CkTests `6237fa5` (harness+conversion); docs `2f45437fa` (pkg) | build exit 0; **Ck.Snapshot 35/35 (delta-0 by name), Ck.Net 90/90 (delta-0 by name), Ck.Timer 21/21** (planner expected 0 — see D4) | Timer_MPReload converted, both cycles green. Deviations D1–D6. |
| 2 — rename bundle (absorbs parity PHASE_6) | NOT STARTED | | | parity PROGRESS 6A/6B rows annotated: Y/N |
| 3 — symmetry class (a), 7 features | NOT STARTED | | | |
| 4 — symmetry class (b), fold | NOT STARTED | | | |
| 5 — CkEcs/Persistence header split | NOT STARTED | | | RunAfter seam outcome (fwd-decl vs include): ____ |
| VALIDATION | NOT STARTED | | | |

## Blockers (STOP-and-record; do not improvise past these)

_(none yet)_

Format per entry: date / phase+step / what was expected / what was observed (verbatim error or grep
output) / what you did NOT do / question for the planner or Adam.

## Deviations from plan (executed differently than written, with reason)

- **D1 (Phase 1 / Timer conversion) — dropped old Stage 3b, folded old Stage 8.** The harness shape
  (Spawn/SubjectReady/Mutate/ReloadSettled/Assert) has no pre-save assert slot, so the old Stage 3b pre-save
  sanity ("server holds elapsed 7 pre-save") is dropped; its sole failure mode (the deferred Jump never took)
  surfaces identically in the post-reload Assert ("elapsed round-tripped (7, not Construct default 10)" reads 10).
  Old Stage 8 ("no re-fire after +60 ticks") is folded into `NumCycles=2` — re-running the whole round-trip and
  re-asserting is a structurally STRONGER stability check. Coverage-neutral; the 7 Stage-7 parity checks are
  preserved verbatim in Assert.
- **D2 (Phase 1 / commit split) — CkTimer_Fragment.cpp's cast change rides commit 3, not commit 2.** The plan's
  commit 2 ("cast") and commit 3 ("Jump") both touch CkTimer_Fragment.cpp, but its Jump-hydration change depends
  on Fragment_Data.h's new `_JumpMode` field, so a commit 2 containing Fragment.cpp would not compile alone.
  Split for atomicity: commit 2 = AnimPlan+attribute cast; commit 3 = Timer Jump + Timer's own cast. Message on
  commit 2 says "AnimPlan/attribute", not "Timer/AnimPlan/attribute".
- **D3 (Phase 1 / Step 4.2) — fixed a CountUp-absolute Done no-op the plan's prescribed code introduced.** PHASE_1
  Step 4.2 prescribes `TimerChrono.Tick(FCk_Time{DeltaToApply})` for the CountUp branch. `FCk_Chrono::Tick`
  (CkChrono.cpp:34) early-returns when `Get_IsDone()` (`_CurrentValue >= _GoalValue`) WITHOUT mutating, so a
  BACKWARD absolute jump on an already-Done CountUp chrono silently no-ops (stays at GoalValue instead of Target).
  Found by the pre-build adversarial review (subagent workflow), independently CONFIRMED against the chrono source.
  NOT reachable by the save/load caller (HydrationApply jumps a CountUp chrono from its post-Setup baseline of 0,
  never Done unless GoalValue==0 where the delta is 0) — so the Timer parity test is unaffected and stays green.
  But `_JumpMode` is a new public BlueprintReadWrite field, and a silent-wrong-result on a valid public call
  violates the no-silent-failure non-negotiable. Fix: CountUp absolute now `Reset()` then `Tick(target)`, landing
  from any prior position. CountDown is unaffected (Consume has no Done early-out).
- **D4 (Phase 1 / Step 5.4) — `Ck.Timer` matched 21 tests, not 0.** The planner expected zero tests for the
  `Ck.Timer` pattern; the substring tokens `Ck`+`Timer` actually match 21 Timer-related tests. All 21 pass (exit 0,
  2m11s) — extra green Timer coverage, no concern.
- **D5 (Phase 1 / harness) — added an explicit `CkEcs/Registry/CkRegistry.h` include to the harness .cpp.** The
  pre-build review flagged that `FCk_Registry` completeness (for `Get_RegistryHandle()`/`ck::MakeHandle`) was only
  transitive via `CkEcsWorld_Subsystem.h`. Added the explicit include for IWYU robustness (matches the proven
  `CkEntityScript_Utils.cpp` mirror). Non-behavioral.
- **D6 (Phase 1 / skills) — `ck-tests-authoring-and-running` was NOT in this session's Skill registry.** PROMPT
  names it for session start; only `ck-change-control` was loadable. Relied on exemplar-mimicry per PHASE_1
  (M2b / TransformParity / TimerParity spec files + CkNetAutomation_Common) as the phase doc mandates anyway.

### Behavior notes (not deviations, flagged for review)
- **Spurious `OnTimerJump(amount 0)` on a same-position restore.** Timer HydrationApply now issues an
  UNCONDITIONAL absolute `Request_Jump` (the old `IsNearlyZero` guard is gone — that guard's job moved INTO the
  handler as the single source of truth). When saved elapsed == post-Setup baseline (CountUp saved at 0), the
  handler applies a 0-delta jump and broadcasts `OnTimerJump(0)` + `OnTimerUpdate` — signals the old guarded path
  suppressed. Never broadcasts `OnTimerDone` (the no-re-fire guarantee holds). Benign for the load path; only
  matters if game code binds those signals in BeginPlay and reacts to load-time hydration. The Timer test uses
  non-zero deltas so it never hits the 0 case.

## The one claim most likely to be wrong (Phase 1)

The CountUp-absolute Done fix (D3): I claim `Reset()` then `Tick(target)` on a CountUp chrono has no side effect
beyond setting elapsed to `target` (Reset only writes `_CurrentValue=0`, verified CkChrono.cpp:79). If a future
CountUp timer relied on Reset clearing some other state, or on monotonic elapsed, that assumption could bite — but
no such coupling exists in the chrono today. Verified against source; the gate (Snapshot 35/35 incl. Timer_MPReload
two-cycle) exercises the CountUp save/load path and stays green.
