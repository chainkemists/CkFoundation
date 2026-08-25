# Gameplay-cascade settle barrier — SM cascades converge before the transform phases

**Status: APPROVED 2026-08-24** — go-ahead relayed by Neil (implementation authorized; Q3 scope =
the corrected 16-trigger set, Q7 = #717 stays as floor, per the reviewed package).

Supersedes `docs/campaigns/2026-08-11-scheduler-settle-pass/` (marked SUPERSEDED: it proposed a
parallel post-pump mechanism; upstream landed the right primitive on 2026-08-20).

## Problem (one paragraph; full evidence in the BusterBlock digests)

A state-machine transition is a multi-step cascade: task result → state evaluate → transition
commit → new state EntityScript spawns → its tasks spawn → task `BeginPlay` → `EnterTask` →
`DoEnterTask` enqueues work (e.g. a transform request). Each step is a different consumed-marker
processor. The main pass dispatches each once; the rest of the chain converges in the **global
tail pump, after the transform phases and the component push slot**. So a transform request born
in that cascade drains after the push slot; its `FTag_Transform_Updated` is cleared by next
frame's `Transform_Cleanup` before the next push slot — the tag-gated push never sees it
(rewind-station: outbox case at origin, inbox cover table-flash). PR #717 makes the push
tolerate late drains (next-frame delivery). This campaign fixes the *producer* timing for the
SM-cascade class so the ordinary drain/push deliver **same frame, one road, pump or no pump** —
the symmetric shape the team asked for — using the group-local settle barrier primitive that
landed in `e4cf54edf`.

## The primitive (landed 2026-08-20, `e4cf54edf`; read `Source/CkEcs/Claude.md` § "Group-local settle barriers")

- `using LocalSettleAfter = FGroup_X;` — replay this processor at X's end, in topological order, `dt=0`.
- `static constexpr auto LocalSettleTrigger = true;` — activate the barrier from THIS processor's
  **consumed** dirty marker (never a sticky tag like `FTag_Transform_Updated`).
- A participant must precede the barrier in the main graph (topological position — its own `Group`
  may be earlier, as `Transform_HandleRequests` already is for the `Transform_Derived` barrier), must
  not be `SkipPump`, shares the per-frame pump budget (`_MaxPumpIterations`), and the barrier runs in
  the load kernel only if every participant is `RunsDuringLoad`.
- **Two semantics the doc under-states (verified `CkProcessorScheduler.cpp:388-470`):**
  (a) a participant WITHOUT a dirty marker is replayed on EVERY barrier pass (only marker-bearing
  nodes are skipped when clean) — so a marker-less participant that re-dirties a trigger is a
  livelock; (b) trigger dirtiness is fragment PRESENCE (`Has_AnyLiveEntityWith`), not
  version-compare — a trigger whose consumer drains-but-retains its fragment pins the barrier at the
  pass limit every frame. Every trigger below was checked to REMOVE its marker.
- Declared today: `Transform_HandleRequests` (trigger) + `SceneNode_Update` layers (replay) after
  `FGroup_Transform_Derived`.

## Chosen approach

Declare a barrier after **`FGroup_Gameplay_Script`** (NOT `Gameplay_AI`: `EnterTask` fires from
the task EntityScript's `BeginPlay` — `CkSmTask_EntityScript.cpp:47` — states/tasks are per-transition
entities created on entry — `CkSmState_Utils.cpp:97-136` — so the cascade's second half lives in the
EntityScript spawn pipeline in `Gameplay_Script`). Participants — exact set, nothing else:

**Triggers (consumed markers) — CkStateMachine, `FGroup_Gameplay_AI`:**
| Processor | Marker | Role in the cascade |
|---|---|---|
| `FProcessor_Sm_HandleRequests` | `FFragment_Sm_Requests` | Request_Transition / Start / Stop |
| `FProcessor_Sm_CommitPendingTransition` | `FFragment_Sm_PendingTransition` | commit → `DoEnterState` (waits, in-pass, for the old state's exit) |
| `FProcessor_SmState_Evaluate` | `FTag_SmState_NeedsEvaluation` | walk transitions |
| `FProcessor_SmTransition_Evaluate` | `FTag_SmTransition_Evaluating` | arm/evaluate conditions |
| `FProcessor_SmTask_FireFinishedSignal` | `FTag_SmTask_ResultDirty` | task result → parent (sync broadcast → event-driven condition → transition wake) |
| `FProcessor_SmState_Exit` / `FProcessor_SmTask_Exit` / `FProcessor_SmTransition_Exit` / `FProcessor_SmCondition_Exit` | `*_PendingExit` | exit lifecycle of the leaving state |
| `FProcessor_Sm_Setup` / `FProcessor_Sm_FirstSyncInitialState` | `FTag_Sm_RequiresSetup` / `FTag_Sm_NeedsInitialStateEntry` | fresh SMs enter their initial state in-frame |

**Triggers — `FGroup_Gameplay_Script` (CkStateMachine + CkEcs EntityScript pipeline):**
| Processor | Marker | Note |
|---|---|---|
| `FProcessor_SmScript_CommitPendingAttach` (CkStateMachine) | `FTag_SmScript_PendingAttach` | **every SM task/condition created in `DefineState` defers its EntityScript attach through this** (`CkSmTask_Utils.cpp:73-76`, `CkSmCondition_Utils.cpp:96-98`); without it the barrier enters the state but never reaches `EnterTask`. Not `RunsDuringLoad`. |
| `FProcessor_EntityScript_SpawnEntity_HandleRequests` | `FFragment_EntityScript_RequestSpawnEntity` | `RunsDuringLoad` |
| `FProcessor_EntityScript_ContinueConstruction` | `FTag_EntityScript_ContinueConstruction` | `RunsDuringLoad` |
| `FProcessor_EntityScript_FinishConstruction` | `FTag_EntityScript_FinishConstruction` | `RunsDuringLoad` |
| `FProcessor_EntityScript_BeginPlay` | `FTag_EntityScript_BeginPlay` | `RunsDuringLoad`; this is the "enter" node (`EnterState`/`EnterTask`/`EnterCondition` are script `BeginPlay`) |

**Replay-only participants: NONE.** (`FProcessor_SmCondition_Polled` was proposed as replay-only
and is rejected — see EXCLUDED and review Q4: marker-less ⇒ replays every pass ⇒ unconditionally
re-dirties `FTag_SmTransition_Evaluating` ⇒ 2-pass livelock for any state with a non-firing polled
transition.) 16 triggers total.

**Scope — what the barrier converges.** The **event-driven** cascade only: task result /
event-driven condition → transition → commit → exit lifecycle → new state spawn → `BeginPlay`
(`EnterState`) → `CommitPendingAttach` → task/condition spawn → `BeginPlay` (`EnterTask` /
`EnterCondition`). A state entered inside the barrier whose transitions are polled or vacuous is
armed by `FProcessor_SmState_Update` in the NEXT frame's main pass — exactly as today (unchanged).
The rewind cascade is fully event-driven (`UBb_SmCondition_AllTasksSucceeded` is a
`UCk_SmCondition_TaskResults`, event-driven); the Phase-0 spec must use an event-driven condition.

**Explicitly EXCLUDED (fences), with the verified reason for each:**
- `FProcessor_SmTask_Tick` — passes real `DeltaT` into user `Tick` (`CkSmTask_Processor.cpp:74`);
  it is the main-pass *ignition* of a Tick-mode task's result, and the barrier drains what the main
  pass ignited — it never ignites.
- `FProcessor_SmCondition_Polled` — marker-less (would replay every pass) and ends with an
  unconditional `AddOrGet<FTag_SmTransition_Evaluating>` (`CkSmCondition_Processor.cpp:125-129`);
  with `SmState_Evaluate`'s Fail → `Request_ResetTransition` → `Request_ResetCondition` re-arm this
  is a guaranteed non-convergent 2-cycle. Also passes `dt=0` into user predicates.
- `FProcessor_SmState_Update` — marker-less arm (`Request_Evaluate` every tick for
  non-FullyEventDriven states; body has no DeltaT). Pointless without Polled, and as a marker-less
  participant it would re-arm every pass. Stays main-pass-only (the polled half of the SM is
  next-frame by design).
- `FProcessor_SmCondition_ResetEveryFrame` — frame-scoped reset; replaying mid-cascade wipes armed
  state.
- `FProcessor_Sm_ApplyReplicatedHistory` — its marker `FFragment_Sm_ReplayQueue` is never removed
  (`AddOrGet`, empty ⇒ plain return) ⇒ presence-checked trigger would spin to the limit every frame on
  every client forever; and it is deliberately one-event-per-tick (`CkStateMachine_Processor.cpp:876-878`).
  Client convergence is NOT a frame behind (review Q6): it runs in the main pass before the barrier and
  feeds `FFragment_Sm_PendingTransition`, which IS a trigger.
- `FProcessor_Sm_FlushPendingReplication_Drain` — convergent but feeds only `ReplayQueue`; pointless
  without the above.
- `FProcessor_EntityScript_Replicate` — defers keeping its marker until the replication driver
  exists (`CkEntityScript_Processor.cpp:425`) ⇒ same presence hazard. `FProcessor_EntityScript_PendingReplicationRetry`
  has no marker.
- `FProcessor_Sm_HydrationResume` (`FGroup_Gameplay`, load ladder — converges via the load kernel) and
  all `*_Debug*` processors.

No scheduler code changes. No new groups. `#717` (`LastPushedTransform`) stays as the floor for
late producers this barrier cannot reach (phase/tail-triggered writes, timer-driven harness
writes) — the tail pump is intentionally global.

## What this changes / does not
- Converges before `Gameplay_Chaos` → Physics → transforms → PostTransform → replication: all of
  those see settled SM state and cascade-born transforms same frame. **This is a frame-structure
  change:** every SM transition in the game converges before the phases instead of after — the full
  suite is the detector (review Q8).
- `Gameplay_Audio` / `Gameplay_Rendering` run BEFORE `Gameplay_Script` — they still see pre-cascade
  state (one frame late), exactly as today. Unchanged; noted.
- Load kernel: SM participants are not `RunsDuringLoad` → the barrier is silent during loads —
  load behavior unchanged (EntityScript construction still converges via `_LoadPumpOrder` +
  `Request_PumpToQuiescence`).
- Budget: shares `_MaxPumpIterations` (default 30, cvar `ck.Scheduler.MaxPumpIterations`) with the
  tail pump — barrier passes reduce the tail pump's remaining passes. One barrier-converged transition
  costs ≈4–5 passes (transition evaluate → state evaluate → requests+exits+commit+state spawn/BeginPlay
  → attach/task spawn/BeginPlay); the same count the tail pump spends today — moved earlier, not added.
  Diagnostics: `High pump count this frame` fires at ≥8 on the shared count (suppressed by the
  AutoTest runner, 5 s throttle in-game); `Local settle after group [...] reached the [N]-pass limit`
  (barrier) and `Pump limit [N] reached` (tail pump, whose budget the barrier consumed) are BOTH gates
  (must be 0). `Local settle …` is NOT in the AutoTest runner's suppression list and must stay out —
  it is the livelock tripwire.
- Fragility to know: any participant later gaining `SkipPump` sets the whole plan invalid at
  registration (one ensure at boot, then the barrier is silently absent). The Phase-0 spec is the
  standing tripwire for that.

## Rejected approaches (recorded so they aren't re-litigated)
1. Post-pump settle pass (the superseded campaign) — a parallel mechanism; the landed primitive
   is placed at group ends and must not trigger on sticky tags; redundant now.
2. `MarkedDirtyBy = FTag_Transform_Updated` on the push — dirty-marker-conflict diagnostics, 7
   unorderable co-consumers, destabilized a BB test (2026-08-11).
3. Ungated per-tick push — broke `TransformPropagation.DirtyOwnersOnly`.
4. Barrier after `Gameplay_AI` — converges only the first half of the cascade; task `EnterTask`
   happens in `Gameplay_Script`'s spawn pipeline.
5. Moving/disabling the tail pump — the maintainers' doc states it is intentionally global.
6. `SmCondition_Polled` as a replay-only participant — livelock (see EXCLUDED).
7. #717 alone, accepting one-frame latency as the contract — fixes the drop class at the consumer
   but leaves the asymmetry the team asked to remove and needs a per-consumer memory for every future
   tag-gated consumer of cascade-born writes.

## File inventory
| File | Change |
|---|---|
| `Source/CkStateMachine/Public/CkStateMachine/StateMachine/CkStateMachine_Processor.h` | `LocalSettleAfter`/`LocalSettleTrigger` on Setup, HandleRequests, FirstSyncInitialState, CommitPendingTransition, **SmScript_CommitPendingAttach** |
| `Source/CkStateMachine/Public/CkStateMachine/State/CkSmState_Processor.h` | on Evaluate, Exit |
| `Source/CkStateMachine/Public/CkStateMachine/Task/CkSmTask_Processor.h` | on FireFinishedSignal, Exit |
| `Source/CkStateMachine/Public/CkStateMachine/Transition/CkSmTransition_Processor.h` | on Evaluate, Exit |
| `Source/CkStateMachine/Public/CkStateMachine/Condition/CkSmCondition_Processor.h` | on Exit ONLY (Polled gets nothing) |
| `Source/CkStateMachine/Public/CkStateMachine/Condition/CkSmCondition_Processor.cpp` | optional hardening (maintainer-confirmable): `SmCondition_Exit` consumes `FTag_SmCondition_PendingExit` before its invalid-script early return, mirroring `SmTask_Exit` |
| `Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript_Processor.h` | on SpawnEntity_HandleRequests, ContinueConstruction, FinishConstruction, BeginPlay |
| `Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorGroups.h` | one why-comment on `FGroup_Gameplay_AI` ("decision-logic tier: state machines/trees/planners") and on the barrier placement after `Gameplay_Script` |
| `Source/CkStateMachine/Claude.md`, `Source/CkEcs/Claude.md` | one paragraph each: the cascade now converges at the Gameplay_Script barrier; CkEcs doc gains the two under-stated semantics (marker-less replay-every-pass; presence-based trigger check) |
| BusterBlock `Plugins/BusterBlockTests/Script/Tests/StateMachine/` (new) | the red spec test (Phase 0) + the polled-no-livelock guard spec |

## Executable spec (Phase 0 authors it)
`Bb_AutoTest_Sm_CascadeWriteReachesComponentSameFrame`: a minimal SM on a component-bearing entity;
StateA has a **Tick-mode** task that returns Succeeded once a go entity-tag appears (the production
ignition shape — `SmTask_Tick` in the AI main pass, like the rewind station); transition on
`AllTasksSucceeded` (event-driven — required, see Scope) to StateB, whose task's `DoEnterTask`
enqueues a one-shot `Request_SetLocation` on the entity. A WaitUntil predicate polls "SM is in
StateB"; on the FIRST tick that is true it asserts the live component is ALREADY at the target,
failing immediately otherwise. RED without the barrier (the post-ignition cascade drains in the
tail pump; the write is swallowed — or next-frame via #717), GREEN with it.

**Measured harness constraint (2026-08-24, frame-interleave probe):** AutoTest step callbacks (the
per-frame step timer) execute in the frame's TAIL region — after the transform phases and BOTH
settle barriers — not in `Gameplay_TimeDelta` as the SM-start slot. A spec that starts/ignites the
SM directly from a step callback is therefore a tail-born producer the barrier can never reach
(the #717 floor class), and stays red with the barrier on. Hence the Tick-mode gate: the step only
flips a tag; ignition happens in the NEXT frame's AI main pass. The predicate also runs in the tail
(after that frame's push slot), which keeps the first-observation assert sound: with the barrier,
StateB's first observation is in the same frame as the push (component already moved); without it,
the write is pump-drained and swallowed, so the component reads stale on every observation.

Companion guard: `Bb_AutoTest_Sm_PolledTransitionDoesNotTripBarrier` — an SM sitting in a state with
a polled condition that returns false for N frames; asserts no `Local settle after group` warning
(the runner escalates it). Pins the Polled/State_Update exclusions.

## Glossary
- **Cascade**: the multi-processor chain from task result to the next state's task `DoEnterTask`.
- **Barrier**: group-local settle barrier (replay of opted-in processors at a group's end while a
  consumed trigger stays dirty).
- **Tail pump**: the global end-of-tick pump; intentionally global, runs after the push slot.
- **Floor**: #717's per-owner push memory — next-frame delivery for anything born after the push.

## Skills to load, and when
Before code: `ck-macros-and-codegen` (trait conventions), `ckecs-architecture-contract`. Before
Phase 0: `ck-game-testing-discipline` + BusterBlock `Script/CLAUDE.md` AutoTest section and the
existing SM AutoTests (copy `CkTests/Script/CkStateMachine/CkAutoTest_StateMachine_BasicTransition.as`
and `CkStateMachine_TestStates.as` authoring shape). Before gates: `ck-change-control`. On any
failure: `ck-debugging-playbook`, then `ck-failure-archaeology`.

## Review questions — answered in PROGRESS.md (2026-08-21); 3 and 7 are the maintainer's calls
1. **Completeness:** trace the rewind station's `Rewinding_Manual → BackInCase_Manual` cascade
   end-to-end against the participant table. Does any link run in a group outside AI/Script, or
   fire from a signal broadcast in a later group?
2. **Replay safety at dt=0** for every trigger, especially `Sm_Setup`, `FirstSyncInitialState`,
   `ContinueConstruction`, `BeginPlay`.
3. **Maintainer intent:** the primitive's doc says barriers are for "canonical processors" and warns
   against "replaying an entire group of unrelated/time-sensitive systems". This set is 16 triggers
   (one mechanically coupled chain, zero replay-only) across two modules; the shipped precedent is
   1 trigger + 8 replay-only in one module. Within intent? Narrower / broader?
4. **`SmCondition_Polled` replay** — benign or not.
5. **Budget/perf:** measure the warning counts and `stat CkScheduler` `LocalSettle` on a busy gym.
6. **Net paths excluded:** is client-side convergence a frame behind the server's?
7. **Does #717 stay?** Yes per this plan. Confirm the maintainer agrees the push should tolerate
   late drains rather than the framework forbidding them (#717 and BB #2706 are still OPEN).
8. **Test-timing migration:** any AutoTest asserting "SM reacts within exactly N ticks" may shift
   by one (earlier). The full suite is the detector; list any shifted test and decide keep/adjust.
