# CkStateMachine — Architecture

## Design Philosophy

- **Everything is an entity.** The SM, each state, task, transition, and condition is an independent ECS entity. Any of them can carry fragments, bind signals, and participate in the ECS world.
- **Dynamic graph.** The state graph is built at runtime: entering a state calls `DefineState()`, which creates child transition and condition entities. No static asset or schema required.
- **Conditions own the evaluation mode.** A transition is pure data — it has no EntityScript. Whether a transition evaluates via polling or via external events is determined by its child conditions.
- **Pump-driven evaluation.** State/transition/condition evaluation does not poll the full graph every frame. Instead, dirty tags gate each processor. A transition only runs when `FTag_SmTransition_Evaluating` is set; a state only evaluates when `FTag_SmState_NeedsEvaluation` is set. The ECS pump re-runs processors within the same frame as tags change, so the full Pass/Fail resolution happens in a single frame without manual loops.

---

## Entity Hierarchy

```
Game Entity (owner of the SM)
└── StateMachine Entity  (FCk_Handle_StateMachine)
    └── Active State Entity  (FCk_Handle_SmState)
        ├── Task Entity 0..N  (FCk_Handle_SmTask)
        └── Transition Entity 0..N  (FCk_Handle_SmTransition)
            └── Condition Entity 0..N  (FCk_Handle_SmCondition)
```

Lifetime ownership cascades downward. Destroying the SM destroys everything. Destroying a state destroys its tasks, transitions, and conditions. This is the exit path — condition/transition entities are torn down when the SM transitions to a new state.

---

## Tags Reference

### StateMachine tags
| Tag | Meaning |
|-----|---------|
| `FTag_Sm_RequiresSetup` | One-time init not yet run |
| `FTag_Sm_Running` | SM is actively running |
| `FTag_Sm_Paused` | SM is paused (no ticking, no transitions) |
| `FTag_Sm_TransitionQueued` | A transition request is pending this frame — guards against double-fire |

### State tags
| Tag | Meaning |
|-----|---------|
| `FTag_SmState_Active` | This state is the current active state of a running SM |
| `FTag_SmState_FullyEventDriven` | All conditions on all transitions are event-driven. Added on creation. Removed automatically when any Polled condition is created on a child transition. |
| `FTag_SmState_NeedsEvaluation` | Signals `FProcessor_SmState_Evaluate` to walk transitions this pump cycle. Acts as the dirty key. |

### Transition tags
| Tag | Meaning |
|-----|---------|
| `FTag_SmTransition_FullyEventDriven` | All conditions on this transition are event-driven. Added on creation. Removed when a Polled condition is added; also cascades removal to the parent state. |
| `FTag_SmTransition_Evaluating` | Signals `FProcessor_SmTransition_Evaluate` to run this pump cycle. Acts as the dirty key. |

### Condition tags
| Tag | Meaning |
|-----|---------|
| `FTag_SmCondition_Polled` | Evaluated by processor every frame when not paused |
| `FTag_SmCondition_EventDriven` | Evaluated by the condition's EntityScript reacting to external signals |
| `FTag_SmCondition_EvaluationPaused` | Condition is frozen — neither the Reset nor the Polled processor will touch it |
| `FTag_SmCondition_ResetsEveryFrame` | Polled condition: result resets to Undetermined every frame (applied to the currently active condition) |

---

## Processor Pipeline

Processors run in dependency order within each ECS tick. Dirty-tag processors (`MarkedDirtyBy`) can be re-triggered within the same frame by the pump.

```
── Per-frame linear pass ──────────────────────────────────────────────────────

[1] FProcessor_Sm_Setup
      Runs on: SM with FTag_Sm_RequiresSetup
      Does:    Removes setup tag. If AutoStart configured, queues Start request.

[2] FProcessor_Sm_HandleRequests       (RunAfter: Setup | MarkedDirtyBy: FFragment_Sm_Requests)
      Runs on: SM with pending requests
      Does:    Processes Start / Stop / Pause / Resume / Transition requests.
               Start/Transition: exits current state (destroying child entities),
               enters new state (spawning child entities via DefineState()).

[3] FProcessor_SmTask_Tick
      Runs on: Task with FTag_SmTask_Tick
      Does:    Calls EntityScript Tick(). Calls Request_UpdateTaskResult() which
               sets _LastResult and adds FTag_SmTask_ResultDirty if Running→non-Running.

[4] FProcessor_SmCondition_ResetEveryFrame    (RunAfter: HandleRequests)
      Runs on: Condition with FTag_SmCondition_ResetsEveryFrame,
               EXCLUDES FTag_SmCondition_EvaluationPaused
      Does:    Sets _Result = Undetermined on the currently active (unpaused) condition.
               Only hits the one condition that is currently being evaluated this frame.

[5] FProcessor_SmCondition_Polled             (RunAfter: ResetEveryFrame)
      Runs on: Condition with FTag_SmCondition_Polled,
               EXCLUDES FTag_SmCondition_EvaluationPaused
      Does:    Calls EntityScript Evaluate() → sets _Result to Pass or Fail.
               Bumps FTag_SmTransition_Evaluating on parent transition (Remove + Add)
               to wake FProcessor_SmTransition_Evaluate in the pump.

[6] FProcessor_SmTransition_Evaluate          (RunAfter: Polled | MarkedDirtyBy: FTag_SmTransition_Evaluating)
      Runs on: Transition with FTag_SmTransition_Evaluating
      Does:    Removes FTag_SmTransition_Evaluating.
               Walks conditions (AND logic):
                 Undetermined → unpause condition (Request_StartOrResumeEvaluating) → return (wait)
                 Pass         → pause condition → continue to next
                 Fail         → pause condition → set transition Fail → wake parent state → return
               All Pass → set transition Pass → wake parent state.
               "Wake parent state" = Request_Evaluate → adds FTag_SmState_NeedsEvaluation.

[7] FProcessor_SmState_Update                 (EXCLUDES FTag_SmState_FullyEventDriven)
      Runs on: Active state that is NOT fully event-driven
      Does:    Calls Request_Evaluate (adds FTag_SmState_NeedsEvaluation) every frame.
               This drives the evaluation loop for states that have at least one polled condition.

[8] FProcessor_SmState_Evaluate               (RunAfter: TransitionEvaluate + StateUpdate | MarkedDirtyBy: FTag_SmState_NeedsEvaluation)
      Runs on: Active state with FTag_SmState_NeedsEvaluation
      Does:    Removes FTag_SmState_NeedsEvaluation.
               Guards: SM must be Running, not Paused, no TransitionQueued.
               Walks transitions in record order (= insertion/priority order):
                 Undetermined → Request_StartEvaluating (sets Undetermined + adds FTag_SmTransition_Evaluating) → break
                 Fail         → continue to next transition
                 Pass         → queue transition request on SM → break
               Queuing fires FProcessor_Sm_HandleRequests in the pump to execute the transition.

[9] FProcessor_SmTask_FireFinishedSignal
      Runs on: Task with FTag_SmTask_ResultDirty
      Does:    Fires OnSmTaskFinished signal. Removes FTag_SmTask_ResultDirty.

[10] FProcessor_Sm_EndPlay
      Runs on: SM during entity destruction (CK_IF_END_PLAY)
      Does:    Cleans up SM state on teardown.
```

---

## Evaluation Modes

### Fully Event-Driven State
All transitions on the state have only event-driven conditions.

- `FProcessor_SmState_Update` **does not run** (excluded by `FTag_SmState_FullyEventDriven`).
- The state waits silently until an event-driven condition calls `Request_UpdateConditionResult`.
- That wakes the parent transition (`FTag_SmTransition_Evaluating`), which wakes the state (`FTag_SmState_NeedsEvaluation`).
- The state evaluator then walks transitions and fires if one passes.

### Non-Fully-Event-Driven State (has at least one Polled condition)
`FProcessor_SmState_Update` adds `FTag_SmState_NeedsEvaluation` every frame, driving the evaluation loop continuously.

---

## Auto-Detection of Evaluation Mode

States and transitions start as `FullyEventDriven`. The mode is inferred automatically:

1. A Polled condition is added to a transition → `UCk_Utils_SmCondition_UE::Create` calls `UCk_Utils_SmTransition_UE::Request_MarkTransition_AsNotFullyEventDriven`.
2. That removes `FTag_SmTransition_FullyEventDriven` from the transition and calls `UCk_Utils_SmState_UE::Request_MarkState_AsNotFullyEventDriven` on the parent state.
3. `FTag_SmState_FullyEventDriven` is removed from the state.

No authoring required. The state machine author only specifies condition types.

---

## Steady-State Frame Trace (Non-Fully-Event-Driven, Condition N Active)

```
Frame tick linear pass:
  [ResetEveryFrame]  Condition N (unpaused) → _Result = Undetermined
  [StateUpdate]      State → NeedsEvaluation added
  [Polled]           Condition N (unpaused) → Evaluate() → _Result = Pass/Fail
                     → bumps FTag_SmTransition_Evaluating on parent transition

Pump cycle 1 (triggered by FTag_SmTransition_Evaluating bump):
  [TransitionEvaluate]
    Condition N = Pass → pause it → continue to next condition (N+1)
    Condition N+1 = Undetermined → unpause it → return (wait for next tick)
    OR: Condition N = Fail → pause it → transition Fail → NeedsEvaluation added

Pump cycle 1 (if NeedsEvaluation added by transition):
  [StateEvaluate]
    transition Fail → Continue to next transition
    (no transition passes → nothing happens this frame)

Next frame: Condition N+1 is now unpaused → ResetEveryFrame hits it → cycle repeats
```

---

## Condition Reset Between Frames

**Polled conditions** use a "one at a time" active window:
- The transition processor unpauses exactly one condition (`Request_StartOrResumeEvaluating`) and waits.
- Next frame, `FProcessor_SmCondition_ResetEveryFrame` resets that unpaused condition to Undetermined.
- `FProcessor_SmCondition_Polled` evaluates it and writes Pass/Fail.
- Once a condition passes, it is paused with its Pass result retained.
- Subsequent frames advance to the next Undetermined condition.

**Event-driven conditions** are never touched by the Reset or Polled processors. They call `Request_UpdateConditionResult` directly from their EntityScript in response to a signal.

---

## Known Deficiencies

### Transition result not reset between evaluation cycles

When all conditions on a transition fail in frame N, the transition result is set to `Fail` and all conditions are paused. In frame N+1:
- `FProcessor_SmState_Update` adds `NeedsEvaluation` again.
- `FProcessor_SmState_Evaluate` walks transitions and sees `Fail` → continues to next.
- No conditions are unpaused. Nothing re-evaluates. The transition stays failed permanently until the state is exited and re-entered.

**Root cause**: Transition results are never reset back to `Undetermined` between evaluation cycles. In the reference project (RelicSimGameplay), `FUtilsTransition::Reset` was called on failed transitions (via the exit flow), fully reinitialising them. We have no equivalent.

**What needs to change**: When `FProcessor_SmState_Evaluate` consumes `NeedsEvaluation` and encounters a `Fail` transition, it should reset the transition to `Undetermined` (and reset/unpause all its conditions) so evaluation restarts from the beginning next cycle. Or: `Request_StartEvaluating` on a transition should unpause all child conditions.

---

## File Map

```
Source/CkStateMachine/
├── CkStateMachine.Build.cs
├── CkStateMachine_Architecture.md          ← this file
├── CkStateMachine_Log.h/.cpp               Log category + ck::sm:: helpers
└── Public/CkStateMachine/
    ├── StateMachine/
    │   ├── CkStateMachine_Fragment_Data.h  Handles, enums, requests, payloads, delegates
    │   ├── CkStateMachine_Fragment.h/.cpp  SM fragments, tags, signals, records
    │   ├── CkStateMachine_Processor.h/.cpp Setup / HandleRequests / EndPlay processors
    │   ├── CkStateMachine_Utils.h/.cpp     Public BPFL (create, start/stop, signals, cast)
    │   └── CkStateMachine_ProcessorInjector.h/.cpp
    ├── State/
    │   ├── CkSmState_Fragment.h            FTag_SmState_*, FFragment_RecordOfSmStates
    │   ├── CkSmState_Processor.h/.cpp      FProcessor_SmState_Update + _Evaluate
    │   ├── CkSmState_Utils.h/.cpp          Create, Request_Evaluate, Is_FullyEventDriven, ...
    │   └── EntityScripts/
    │       └── CkSmState_EntityScript.h/.cpp  DefineState() builder API base class
    ├── Task/
    │   ├── CkSmTask_Fragment.h             FTag_SmTask_*, FFragment_SmTask_Current
    │   ├── CkSmTask_Processor.h/.cpp       FProcessor_SmTask_Tick + _FireFinishedSignal
    │   ├── CkSmTask_Utils.h/.cpp           Create, Request_UpdateTaskResult
    │   └── EntityScripts/
    │       └── CkSmTask_EntityScript.h/.cpp   Tick / OnStateEnter / OnStateExit base class
    ├── Transition/
    │   ├── CkSmTransition_Fragment.h       FTag_SmTransition_*, FFragment_SmTransition_*
    │   ├── CkSmTransition_Processor.h/.cpp FProcessor_SmTransition_Evaluate
    │   └── CkSmTransition_Utils.h/.cpp     Create, Request_StartEvaluating, Is_FullyEventDriven, ...
    └── Condition/
        ├── CkSmCondition_Fragment.h        FTag_SmCondition_*, FFragment_SmCondition_Current
        ├── CkSmCondition_Processor.h/.cpp  FProcessor_SmCondition_ResetEveryFrame + _Polled
        ├── CkSmCondition_Utils.h/.cpp      Create, Request_UpdateConditionResult, ...
        └── EntityScripts/
            ├── CkSmCondition_EntityScript.h/.cpp  Base class
            ├── CkSmCondition_Polled.h/.cpp        Polled base (override Evaluate() → bool)
            └── CkSmCondition_EventDriven.h/.cpp   Event-driven base (MarkSatisfied/Unsatisfied)
```
