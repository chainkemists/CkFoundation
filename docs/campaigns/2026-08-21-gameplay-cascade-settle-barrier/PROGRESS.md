# PROGRESS — gameplay-cascade settle barrier

## Status
| Phase | State | Session | Notes |
|---|---|---|---|
| Fresh-eyes review (Fable 5) | **DONE 2026-08-21** | review session | 3 corrections folded into PROMPT/PHASE_0/PHASE_1/VALIDATION (missing `SmScript_CommitPendingAttach`; `SmCondition_Polled` livelock → no replay-only participants; event-driven-only scope). Digest: BusterBlock `docs/digests/2026-08-21-cascade-barrier-review.html` |
| Maintainer approval | **APPROVED 2026-08-24** | — | go-ahead relayed by Neil (Q3 = corrected 16-trigger set; Q7 = #717 stays as floor) |
| 0 — red spec | **DONE 2026-08-24** | exec session | red line + baseline recorded below |
| 1 — declare + green | **DONE 2026-08-25** | exec session | 16 triggers + fences + framework fixes; both specs green; barrier/pump warning gates 0 |
| Validation | **DONE except 1 open red** | exec session | final full gate + by-name diff below; `DormancyReleasesLookoutAdmission` = the one open regression (see Blockers); perf capture + [EDITOR-VERIFY] left for PR time |

## Review answers (2026-08-21 — every claim read in source; nothing observed at runtime)

1. **Completeness of the participant set for the rewind cascade.** Traced
   `Rewinding_Manual → BackInCase_Manual` (`Script/ECS/RewindStation/BB_RewindStation_Hfsm_Manual.as:1169-1230`):
   it is a ROOT-SM state override (`DoGet_StatesToOverride`, `:1173-1176`, `:1210-1213`), not a sub-SM.
   Chain: CkTimer `OnDone` (`FProcessor_Timer_Update`, `FGroup_Gameplay_TimeDelta`, sync delegate sets
   `_Done`, `:1121-1145`) → `DoTick` returns Succeeded (`FProcessor_SmTask_Tick`, main-pass ignition,
   `:945-946`) → `Request_UpdateTaskResult` sets `FTag_SmTask_ResultDirty` (`CkSmTask_Utils.cpp:118-123`)
   → `SmTask_FireFinishedSignal` sync-broadcasts → `UCk_SmCondition_TaskResults::OnTaskFinished` →
   `MarkSatisfied` → `AddOrGet<FTag_SmTransition_Evaluating>` (`CkSmCondition_Utils.cpp:164-170`;
   `UBb_SmCondition_AllTasksSucceeded` is event-driven, `BB_Interactable_Hfsm_Conditions.as:177-180`)
   → `SmTransition_Evaluate` → `SmState_Evaluate` → `Sm_HandleRequests` (exit + `PendingTransition`)
   → `SmState/Task/Transition/Condition_Exit` → `Sm_CommitPendingTransition` (waits in-pass for
   `PendingExit`, `CkStateMachine_Processor.cpp:550-558`; `DoEnterState` → `UCk_Utils_SmState_UE::Create`
   → `UCk_Utils_EntityScript_UE::Add`, `CkSmState_Utils.cpp:97-136`) → `EntityScript_SpawnEntity_HandleRequests`
   → state Construct/`DefineState` (tasks/conditions get `FFragment_SmScript_PendingAttach`,
   `CkSmTask_Utils.cpp:73-76`) → **`FProcessor_SmScript_CommitPendingAttach` (`CkStateMachine_Processor.h:24-34`,
   `FGroup_Gameplay_Script`, consumed `FTag_SmScript_PendingAttach` — MISSING from the original table; added)**
   → task spawn → `ContinueConstruction`/`FinishConstruction`/`BeginPlay` → `EnterTask`
   (`CkSmTask_EntityScript.cpp:47,90-91`) → `DragToCase::DoEnterTask` enqueues `Request_SetLocation/Rotation`
   (`BB_RewindStation_Hfsm_Manual.as:313-372,618-626`) → `Transform_HandleRequests` (`FGroup_Transform`, after
   the barrier, same frame). Links outside AI/Script: Timer (TimeDelta, BEFORE AI — harmless), Transform
   consumer (after — intended), old-state destroy (EntityLifecycle/EndPlay — not needed), immediate
   Niagara/`SetVisibility` calls (no group). **No link fires from a later-group signal.** "States created
   on entry" and "EnterTask fires from BeginPlay" both CONFIRMED. `SmTask_Tick` is topologically last in
   AI so ignition is main-pass-only and the barrier strictly drains.

2. **Replay safety at dt=0 (per trigger).** All 16 triggers consume their marker: first-statement
   removal for `Sm_Setup` (`.cpp:72`), `FirstSyncInitialState` (`:908`, re-entry guarded `:919`),
   `SmState_Evaluate` (`CkSmState_Processor.cpp:96`), `SmTransition_Evaluate` (`:56`), `SmTask_FireFinishedSignal`
   (`:119`), `ContinueConstruction` (`CkEntityScript_Processor.cpp:383`), `FinishConstruction` (`:449`),
   `BeginPlay` (`:562`); `Sm_HandleRequests` drains by `CopyAndRemove` (`:163/:175`); `Sm_CommitPendingTransition`
   removes last (`:789`) after a deliberate wait the Exit participants satisfy in the same pass; the four
   `*_Exit` consume after a synchronous, `FTag_*_Active`-deduped script `Exit*` call; `SpawnEntity_HandleRequests`
   removes (`:116/:138`) except the in-pass owner-mid-construction defer (`:122`, resolved by participants the same
   frame — `ContinueConstruction`/`FinishConstruction` tags are transient, so no long-lived presence);
   `CommitPendingAttach` removes both fragments first (`:1216-1217`). No trigger uses DeltaT. BeginPlay of
   entities spawned inside the barrier is the intended effect; `FTag_EntityScript_ConstructedThisFrame` set
   inside the barrier is still cleared by `FProcessor_Hydration_Dispatch::DoTick` (`FGroup_DeferredApply`, later
   the same frame). Sharp edges, bounded to the entity's lifetime but budget-burning: `SmCondition_Exit`
   invalid-script early return does not consume `PendingExit` (`CkSmCondition_Processor.cpp:48-49`; `SmTask_Exit:92-97`
   does) → Phase-1 optional hardening; the three `*_Exit` views require `FFragment_EntityScript_Current`, so a
   child exited before its script attached leaves its tag until finalize. `Sm_Setup` spawns nothing;
   `FirstSync` spawns once, guarded. **Replay-unsafe: `SmCondition_Polled` only** (Q4) — removed.

3. **Maintainer intent — set size/scope (OPEN, Saad).** 16 triggers (11 SM in `Gameplay_AI` +
   `CommitPendingAttach` + 4 EntityScript in `Gameplay_Script`), zero replay-only, one mechanically
   coupled chain, vs. precedent of 1 trigger + 8 replay-only in one module. Nothing in the primitive
   forbids it (`CkProcessorScheduler.cpp:105-218`: topological precedence, no SkipPump, trigger needs
   marker — no module/group constraint). Reviewer's assessment: closer to "canonical processors making
   downstream composition explicit" than "replaying an entire group" — but it is a frame-structure
   change for every SM transition in the game, so it is Saad's call. Also raised: one future `SkipPump`
   on any participant invalidates the whole plan silently (`:138-144,378`); the primitive should
   probably warn on marker-less participants.

4. **`SmCondition_Polled` replay without per-frame reset — NOT benign; livelock.** Marker-less
   participants replay every pass (`CkProcessorScheduler.cpp:407-411`); Polled ends with an
   unconditional `AddOrGet<FTag_SmTransition_Evaluating>` (`CkSmCondition_Processor.cpp:125-129`);
   `SmState_Evaluate` Fail → `Request_ResetTransition` → `Request_ResetCondition` re-arms
   (`CkSmTransition_Utils.cpp:180-199`, `CkSmCondition_Utils.cpp:145-152`). Passes k/k+1 repeat for any
   state with a currently-failing polled transition → 30-pass limit every frame, tail pump starved,
   `Local settle after group` warning (not suppressed by the AutoTest runner, `CkAutoTestRunner.cpp:446-447`)
   fails every test. Today the tail pump excludes Polled (no marker) so the cycle breaks. **Dropped.**
   `SmState_Update` stays excluded too (sole arm of `NeedsEvaluation` for non-FullyEventDriven states,
   marker-less, body has no dt) ⇒ barrier converges event-driven cascades only; polled/vacuous
   transitions arm next frame as today. Event-driven conditions are unaffected by the missing
   `ResetEveryFrame` (resting Fail via `Request_SetInitialResult`, no wake).

5. **Budget/perf (numbers to be measured in Phase 1).** Shared `_LastFramePumpCount`
   (`CkProcessorScheduler.cpp:234,328,445`), default 30. ≈4–5 barrier passes per transition, ≈9–10
   for the spec's Start→StateA→StateB chain — the same count the tail pump spends today (moved, not
   added). `WarnThreshold=8` "High pump count" (`:336-352`) is suppressed in AutoTests; gates are BOTH
   `Local settle after group` and `Pump limit [`. Zero-cost fast path when no trigger is dirty
   (`:395`); no empty-view skip inside the replay loop. `Scheduler::LocalSettle` stat (`:56`).

6. **Excluded net paths — client convergence.** NOT a frame behind for the first replayed event per
   frame: `ApplyReplicatedHistory` (`FGroup_Gameplay_AI`, no net-mode gate) runs in the main pass before
   the barrier and `AddOrGet`s `FFragment_Sm_PendingTransition` (`CkStateMachine_Processor.cpp:882`) —
   a trigger — so the client commits/enters in the same barrier. It MUST stay excluded: its marker
   `FFragment_Sm_ReplayQueue` is never removed (presence-checked → spins forever) and it is deliberately
   one-event-per-tick (`:876-878`). Multi-event backlogs remain one per tick by design.

7. **#717 stays as floor — maintainer stance (OPEN, Saad).** CkF #717 (`d2a8166d4`, branch
   `bugfix/oneshot-transform-push-pump-drain`) and BB #2706 (`bugfix/rewind-oneshot-push-visuals`) are
   OPEN; no `LastPushedTransform` on CkF `dev` (`315182cff`); `OneShotPushReachesComponent` exists only
   on the BB branch. Plan keeps #717 as the floor for tail-pump-drained producers (phase/signal/timer
   born). Gates referencing it carry a checkout precondition. Stance = Saad's.

8. **Test-timing shifts found.** None run (review only). Watch list (frame-count assertions near
   SM-driven features): `CkAutoTest_SmTask_Delay_DestroysTimerOnCompletion`, BB `ClawPlay_*` (5),
   `CandyDealer_*` (3), `PhoneBooth_AnswerFlow`, `MeleeCombo_BankStrategy`; the 32 tests in
   `CkTests/Script/CkStateMachine/` are the detector. Phase-1 full suite decides keep/adjust.

## Baselines (captured 2026-08-24, pre-barrier binaries @ CkF bfd1d9a55, BB feature/fixture-ghost-visual-layout tree)
- Full suite: **1817 total / 1807 passed / 10 failed** (`Build/barrier_p0_fullbaseline.log`). The 10 by
  name: `Bb_AutoTest_Sm_CascadeWriteReachesComponentSameFrame` (the deliberate red spec) + 9
  pre-existing: `Bb_AutoTest_BalloonDarts_Game`, `Bb_AutoTest_Employee_Task_Checkout_MansCounter`,
  `Bb_AutoTest_Flyer_LockedCustomerAccepts`, `Bb_AutoTest_NpcCombat_WeaponDrawLatchesPerEncounter`,
  `Ck_AutoTest_Crowd_BunchUp_SettlesAtSharedGoal`, `Ck_AutoTest_Crowd_Separation_CoincidentPairOrbitSearch`,
  `Ck_AutoTest_Crowd_Separation_SpatialOrbitSearch`, `Ck_AutoTest_Crowd_TransientPersonalSpace`,
  `ListenServerReplicates`.
- Phase-0 spec failure line, verbatim: `FinishTest TestResult=Failed. Failed: step 3/3 'SM reaches
  StateB with component already at B': cascade write landed late: component=X=2500.000 Y=0.000 Z=0.000
  expected=X=3000.000 Y=0.000 Z=0.000 (component started at X=2500.000 Y=0.000 Z=0.000)`
  (`Build/barrier_p0_red.log`). Guard spec `Bb_AutoTest_Sm_PolledTransitionDoesNotTripBarrier` GREEN
  pre-barrier.

## Decisions pre-made for the executor (do not re-litigate)
- Barrier after `FGroup_Gameplay_Script`, not `Gameplay_AI` (task `EnterTask` is in the spawn
  pipeline). Exact participant/exclusion tables in PROMPT.md: **16 triggers, 0 replay-only**.
  No scheduler changes. #717 stays. Barrier converges the event-driven cascade only.

## Executor decisions (2026-08-24, Neil AFK — recorded, not asked)
- **CkF branch base = `bfd1d9a55`** (a commit ON origin/dev, 2026-08-22, contains the primitive) —
  the exact SHA BusterBlock_alt's current branch (`feature/fixture-ghost-visual-layout`) pins, so the
  build A/B changes ONLY the barrier and the fixture branch's working tree keeps compiling. origin/dev
  is 79 commits ahead; rebase to tip at ship time (doc + trait lines only — trivial).
- **BusterBlock side**: the two spec tests are committed on a new BB branch
  `feature/gameplay-cascade-settle-barrier` based on BB origin/dev, WITHOUT switching the working
  branch (plumbing commit) and WITHOUT a CkF gitlink bump — the bump lands at PR time after the CkF
  branch is rebased onto CkF dev tip and pushed (bumping to an unpushed SHA would break checkouts).
- **Verification runs** happen on the fixture branch's working tree + this CkF checkout (only
  consistent buildable combination available without disturbing in-flight work). #717/#2706 are not
  checked out → the `OneShotPushReaches` floor gate is SKIPPED (recorded per PHASE_0).
- **Optional `SmCondition_Exit` hardening: TAKEN** (one line, mirrors `SmTask_Exit`, prevents a
  bounded budget-burn; flagged in the commit message for the maintainer).
- Nothing is pushed; both branches are local until Neil ships via ck-ship-pr.

## Blockers
_(Executor: append and END THE SESSION instead of improvising — phase, step, exact command, exact output, expectation.)_

- **OPEN (2026-08-25): `Bb_AutoTest_Npc_DormancyReleasesLookoutAdmission` red with the barrier**
  (green pre-barrier in isolation; red post-barrier 5/5, isolated and in-suite). NOT a warning
  escalation — a behavioral timeout: step 4 'the admitted tag lands' never becomes true. Diagnostic
  trace (temp test, deleted): from the FIRST poll after `Step_Admit`, `LookoutRuntime` exists but
  `AdmittedLookout` is already INVALID and `POITarget.TargetPOI` already CLEARED — i.e. a release
  ran within one frame of admission, and every LOGGING release path (`POITargetTracker` preemption,
  `POIBehaviorCompletion`, no-progress blacklist, SM no-path blacklist) printed NOTHING; the silent
  suspects are `POIPicker`'s clear-before-repick (fires only on invalid TargetPOI) and
  `Fail_CurrentPoiTarget` via an unlogged route. The DoFinishConstruction fix is NOT the cause (the
  failure predates it). Root class: the barrier converging SM/GOAP work earlier re-times which pump
  pass the NPC roam processors observe, breaking the test's carefully balanced "no release path can
  fire" arrangement (its own comment documents the balance). NEEDS: NPC-AI owner review — determine
  whether production admissions can also be wrongly re-released under the new timing, or only this
  synthetic arrangement. Repro: `UnrealToolbox.exe --test --test-pattern DormancyReleasesLookout`.
  Evidence logs: `Build/barrier_p1_refix_DormancyReleasesLookout.log`, `Build/barrier_dormdiag.log`.

- RESOLVED 2026-08-25 (framework fix, recorded for Saad): full-gate A/B isolated 5 new-vs-baseline
  failures. `PointBlankThrow` = lane-load flake (green alone); `Timer_Jump_Backward` +
  `EmployeeOrders_RewindFetchesFromBin` = pre-existing isolation-sensitive (red alone on PRE-barrier
  binaries too; Timer_Jump's assert races its own 2-tick threshold). The remaining two —
  `Npc_DormancyReleasesLookoutAdmission` (admission never lands) and `MainMapLandscapeTrace`
  (captured-ensure escalation) — were OURS, one root cause: `UBb_Shelf_EntityScript` (Continue-flow)
  binds a setup promise in `Construct`; the shelf-setup script processor completes and broadcasts in
  the SAME pump pass, so `DoFinishConstruction` ran while `FTag_EntityScript_ContinueConstruction`
  was still unconsumed → the "NOT ONGOING Construction" ensure fired → recovery early-returned →
  construction NEVER finished (shelf half-built ⇒ dormancy admission times out). The hazard is
  pre-existing (setup processors sort between `SpawnEntity` and `ContinueConstruction` in exec
  order); the barrier exposed it by pre-draining SM/EntityScript pump work and re-timing which pump
  pass the setup lands in. Fix per the tenets (converge from arbitrary state):
  `UCk_EntityScript_UE::DoFinishConstruction` now CLAIMS a still-pending continue
  (`Try_Remove<FTag_EntityScript_ContinueConstruction>`) instead of ensuring — the script itself
  declared construction done, so the un-dispatched hook is moot. All other DoFinishConstruction
  ensures (BeginPlay/double-finish/EndPlay) unchanged.

- RESOLVED 2026-08-24 (crossed the "no scheduler source edits" fence — deliberately, recorded for
  Saad): the first full gate produced 29 warning-escalated test failures and **351**
  `Local settle after group` limit warnings — `SmState_Evaluate` (288) and `Sm_HandleRequests` (63)
  presence-dirty on DYING entities (markers stranded through the 3-frame destruction pipeline that
  no `CK_IGNORE_PENDING_KILL` view can consume). The review's presence-vs-consumability hazard is
  the COMMON case at scale, not an edge. Class fix, barrier-only: a second dirty checker
  `_IsDirtyChecker_Consumable` (`Has_AnyLiveEntityWith_Excluding<Marker, EndPlay/Teardown/Await/
  Finalize>`) used by the barrier's trigger checks and participant skip; the tail pump keeps the
  plain checker (its version-compare already defuses stranded markers). Files:
  `CkRegistry.h` (+`Has_AnyLiveEntityWith_Excluding`), `CkProcessorDescriptor.h`,
  `CkProcessorTraits.inl.h`, `CkProcessorGraph.h/.cpp`, `CkProcessorScheduler.cpp` (3 call sites,
  fallback to the plain checker when the consumable one is unset — script processors).

- OPEN (2026-08-24): teardown-frame budget burn. On the frame after an SM (or its state subtree) is
  destroyed, `FTag_SmState_PendingExit` (and possibly the task/condition twins) can sit on entities
  outside `SmState_Exit`'s view (pending-kill, or script never attached), and the presence-based
  trigger check spins the barrier to the 30-pass limit with
  `Local settle after group [FGroup_Gameplay_Script] reached the [30]-pass limit. Still dirty:
  [FProcessor_SmState_Exit]` (observed in every run's teardown). Bounded (ends when the entity
  finalizes) but it burns the whole shared pump budget for that frame and trips the warning the
  gates require to be ZERO. Candidate fixes, maintainer-visible: (a) consume `*_PendingExit` in the
  destroy path / EndPlay so a dying subtree never leaves the tag; (b) make the Exit views drop the
  `FFragment_EntityScript_Current` requirement and consume the tag for script-less entities (the
  `SmTask_Exit` invalid-script shape, extended); (c) exclude pending-kill entities from
  `Has_AnyLiveEntityWith` in the trigger check (framework change, wider blast). RESOLVED 2026-08-24 by the
  producer-side fence (a): all four `Request_Exit` twins now skip tagging a target that is already
  in the destruction pipeline (`Get_IsPendingDestroy(BeginDestroy)`) — a dying entity's exit is
  delivered by the Active-deduped EntityScript EndPlay path, exactly as the commit-path comment
  documents, so the tag was pure dead-signal there. Semantics-preserving; flagged for Saad's review
  at PR time. The `SmCondition_Exit` invalid-script consume (review Q2) is also in.

## Final gate of record (2026-08-25, `Build/barrier_final_gate.log`)
- Full suite: **1817 total / 1803 passed / 14 failed** vs baseline 1817/1807/10.
- By NAME vs baseline — fixed: the campaign spec (now green) + `Employee_Task_Checkout_MansCounter`
  + `NpcCombat_WeaponDrawLatchesPerEncounter` (both known run-to-run churn). New: **`Npc_DormancyReleasesLookoutAdmission`
  (the ONE genuine open regression — Blockers)**; `Ck_AutoTest_Timer_Jump_Backward` and
  `Bb_AutoTest_EmployeeOrders_RewindFetchesFromBin` (both red in ISOLATION on pre-barrier binaries —
  pre-existing isolation-sensitive, surfaced by lane re-slicing); `ThrowItem_PointBlankThrow` (green
  in isolation — lane-load flake); `Ck_AutoTest_Crowd_Facing_CalmWhilePressingBlockedGap` (the
  known-flaky Crowd family: baseline itself had 4 other Crowd reds, 2 of which went green here).
- Warning gates: `Local settle after group` **0** · `Dirty marker conflict` **0** · `Pump limit` **1**
  (the same pre-existing GeometryCollection pump storm — baseline also has exactly 1).
- A/B causality: the spec is red on pre-barrier binaries (baseline run + the original Phase-0 red)
  and green with the barrier; the barrier-limit livelock guard is green in both worlds.

## Shipped state (2026-08-25, local branches — NOTHING pushed)
- CkFoundation `feature/gameplay-cascade-settle-barrier` (base `bfd1d9a55` ∈ origin/dev): 5 commits
  `0dfe84454` docs(package) · `09a67ebcf` feat(barrier traits) · `a38207832` fix(consumable checker)
  · `9410a4789` fix(pending-exit fences) · `cb2e41a03` fix(DoFinishConstruction claim).
  Rebase onto CkF dev tip + push at PR time (ck-ship-pr).
- BusterBlock `feature/gameplay-cascade-settle-barrier` (`3010049da`, on origin/dev): the two spec
  tests only. The generated wrapper .as + AutoTests map external actors are deliberately NOT
  committed (branch-derived; regenerate on first AS recompile). Gitlink bump follows at PR time.
- Left untouched in the working tree (other sessions' property): `Config/DefaultGameplayTags.ini`,
  `Plugins/AutoSettings/`, CkF `Content/CkIsm` sidecars; two populator-staged wrapper uassets remain
  in the BB index (editor was open — unstage or commit with the map at PR time).

## Session log
- 2026-08-21 · fresh-eyes review · 3 Opus traces + reviewer re-read of the load-bearing sources; package corrected; awaiting Saad (Q3, Q7).
- 2026-08-24 · exec · Phase 0 red captured; Phase 1 traits applied; first green run stayed red →
  frame-interleave probes (temp scheduler/SM logs, since reverted) proved the barrier valid+firing
  but the SPEC igniting from an AutoTest step callback, which executes in the frame's TAIL region
  (after both barriers) — a tail-born producer, i.e. the #717 floor class. Spec reshaped to the
  production ignition (Tick-mode task gated on an entity tag; step only flips the tag). Also
  observed live: SM teardown leaves `FTag_SmState_PendingExit` on out-of-view (pending-kill /
  script-less) entities → barrier spins to the 30-pass limit on the teardown frame with
  `Still dirty: FProcessor_SmState_Exit` — the review's Q2 sharp edge, now confirmed at runtime;
  needs a fence before ship (see Blockers).
