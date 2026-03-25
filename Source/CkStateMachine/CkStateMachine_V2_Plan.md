# CkStateMachine V2 — Improvement Plan

## Overview

V2 restructures the condition/transition architecture, replaces FInstancedStruct payloads with dynamic fragments, removes unnecessary complexity, and fixes bugs from V1.

Core philosophy: **lean state machine for games with thousands of enemies.**

---

## Architectural Changes

### 1. Conditions Own the Evaluation Mode (Not Transitions)

**V1 (current):** Transitions are typed as `Polled` or `EventDriven`. Polled transitions have a processor that evaluates their child conditions. Event-driven transitions have an EntityScript that binds signals and calls `TriggerTransition()`.

**V2 (new):** Transitions are pure data entities. Conditions are typed as `Polled` or `EventDriven`:
- **Polled condition:** Processor ticks it every frame, calls `Evaluate()` on the EntityScript.
- **Event-driven condition:** User subclasses `UCk_SmCondition_EntityScript`, binds signals in BeginPlay, calls `MarkSatisfied()` / `MarkUnsatisfied()`.

A transition evaluates to true when **ALL** its conditions are satisfied.

**OR logic:** Use multiple transitions to the same target state, each with different conditions.

### 2. Remove UCk_SmTransition_EntityScript

Transitions become pure data entities — fragments + child conditions, no EntityScript.

**Remove:**
- `UCk_SmTransition_EntityScript` class (header + cpp)
- `FTag_SmTransition_Polled`, `FTag_SmTransition_EventDriven`
- `ECk_SmTransitionMode` enum
- `TriggerTransition()` method

**Keep:**
- `FCk_Handle_SmTransition`
- `FFragment_SmTransition_Params` (modified — see below)

### 3. Condition Reset Behavior

New enum:
```cpp
UENUM(BlueprintType)
enum class ECk_SmConditionResetBehavior : uint8
{
    ResetEveryFrame,  // Default. Auto-resets to unsatisfied at start of each frame.
    Manual            // Stays satisfied until MarkUnsatisfied() or state exits.
};
```

- `ResetEveryFrame`: For event-driven conditions where the triggering event is transient. Condition must be re-satisfied every frame it should be true.
- `Manual` (latching): For event-driven conditions where the event means "this is now permanently true until I say otherwise." Resets when state exits or `MarkUnsatisfied()` is called explicitly.

### 4. Transition Ordering

Transitions evaluate in the order they are added via `AddTransition()` in `DefineState()`. First added = highest priority.

**Implementation:** `FFragment_SmTransition_Params` gets an `_Order` field (int32), set incrementally by the builder API. The evaluation processor sorts/iterates by order. First fully-satisfied transition wins.

**V2 behavior:** All transitions evaluate every frame (no early-out optimization). This is intentional — optimization where we skip lower-priority transitions after a winner is found is deferred to a future iteration.

### 5. Payload → Dynamic Fragments

**Remove all direct `FInstancedStruct` usage** from the SM public API. Payloads live as dynamic fragments on entities.

**What changes:**
- `FCk_Request_Sm_Transition._TransitionPayload` → removed
- `FFragment_SmTransition_Params._TransitionPayload` → removed
- `AddTransition()` builder API no longer takes `FInstancedStruct InTransitionPayload`
- `AddTask()` builder API no longer takes `FInstancedStruct InSpawnParams`
- `AddCondition()` builder API no longer takes `FInstancedStruct InSpawnParams`
- `DoEnterState()` no longer takes `const FInstancedStruct& InPayload`

**New convenience utilities** (on `UCk_Utils_StateMachine_UE` or a new small utility, TBD during implementation):
- `AddPayload(FCk_Handle& InEntity, const FInstancedStruct& InPayload)` — wraps `UCk_Utils_DynamicFragment_UE::Add_Fragment`
- `GetPayload<T>(const FCk_Handle& InEntity)` — wraps dynamic fragment getter
- Blueprint/AngelScript equivalents

Users add payloads to transition/task/condition entities via the returned handles:
```cpp
auto Transition = AddTransition(StateB);
AddPayload(Transition, MyPayloadStruct{});
```

### 6. Remove const_cast

Add a `const` overload to `DoGet_ScriptEntity()` on the base `UCk_EntityScript_UE`.

Remove all 4 const_casts in:
- `UCk_SmState_EntityScript::DoGet_GameEntity()`
- `UCk_SmTask_EntityScript::DoGet_GameEntity()`
- `UCk_SmTransition_EntityScript::DoGet_GameEntity()` (file being deleted, but the pattern applies to Condition)
- `UCk_SmCondition_EntityScript::Evaluate()`
- `UCk_SmCondition_EntityScript::DoGet_GameEntity()`

### 7. Deduplicate Condition Evaluation

V1 has the condition evaluation loop copy-pasted in `FProcessor_Sm_EvalPolledTransitions::DoEvaluateConditions` and `UCk_SmTransition_EntityScript::TriggerTransition`.

V2: Single static utility function. Since transitions no longer have an EntityScript, this just lives in the processor or a shared utility.

### 8. Fix FFragment_Sm_Requests Encapsulation

Change `CK_PROPERTY(_Requests)` to `CK_PROPERTY_GET(_Requests)`.

Only friend classes (processor, utils) should modify the requests array directly.

---

## New/Modified Files Summary

### Delete
| File | Reason |
|------|--------|
| `CkSmTransition_EntityScript.h` | Transitions no longer have EntityScripts |
| `CkSmTransition_EntityScript.cpp` | Transitions no longer have EntityScripts |

### Modify (significant changes)
| File | Changes |
|------|---------|
| `CkStateMachine_Fragment_Data.h` | Remove `ECk_SmTransitionMode`. Add `ECk_SmConditionMode`, `ECk_SmConditionResetBehavior`. Remove `FCk_Handle_SmCondition` if unused (keep if conditions still need handles). |
| `CkStateMachine_Fragment.h` | Remove `FTag_SmTransition_Polled`, `FTag_SmTransition_EventDriven`. Add `FTag_SmCondition_Polled`, `FTag_SmCondition_EventDriven`. Add `_Order` to `FFragment_SmTransition_Params`. Remove `_TransitionPayload` from `FFragment_SmTransition_Params`. Add `_ResetBehavior` to `FFragment_SmCondition_Current`. |
| `CkStateMachine_Processor.h` | Remove `FProcessor_Sm_EvalPolledTransitions`. Add `FProcessor_SmCondition_Polled` (ticks polled conditions). Add `FProcessor_Sm_EvalTransitions` (evaluates all transitions in order, first winner fires). Add `FProcessor_SmCondition_ResetEveryFrame` (resets non-latching conditions at frame start). |
| `CkStateMachine_Processor.cpp` | Implement new processors. Shared condition evaluation utility. |
| `CkStateMachine_Request_Data.h` | Remove `_TransitionPayload` from `FCk_Request_Sm_Transition`. |
| `CkStateMachine_Utils.h` | Add `Request_Transition` overload with payload convenience. Add `IsInState` query. Add payload convenience utilities. |
| `CkStateMachine_Utils.cpp` | Implement additions. |
| `CkSmState_EntityScript.h` | Remove `AddTransition_EventDriven` (no more transition scripts). `AddTransition` becomes a single method (just target state class). Remove `FInstancedStruct` params from builder API. Return handles for payload attachment. |
| `CkSmState_EntityScript.cpp` | Implement simplified builder API. Transition ordering via incremental counter. |
| `CkSmCondition_EntityScript.h` | Add `ECk_SmConditionMode _ConditionMode`. Add `MarkSatisfied()`, `MarkUnsatisfied()`. Add reset behavior property. Virtual `Evaluate()` stays for polled conditions. |
| `CkSmCondition_EntityScript.cpp` | Implement `MarkSatisfied/MarkUnsatisfied`. |
| `CkStateMachine_ProcessorInjector.cpp` | Update processor registration for new processors. |

### Modify (minor changes)
| File | Changes |
|------|---------|
| `CkSmTask_EntityScript.h` | Remove `FInstancedStruct` from `AddTask` params. |
| `CkSmTask_EntityScript.cpp` | Remove const_cast in `DoGet_GameEntity`. |

### Modify (outside SM module)
| File | Changes |
|------|---------|
| `UCk_EntityScript_UE` (base class) | Add `const` overload of `DoGet_ScriptEntity()`. |

---

## Processor Execution Order

```
Phase: Requests
  1. FProcessor_Sm_Setup                    — One-time init, auto-start
  2. FProcessor_Sm_HandleRequests           — Process Start/Stop/Pause/Resume/Transition

Phase: Update
  1. FProcessor_SmCondition_ResetEveryFrame — Reset non-latching conditions to unsatisfied
  2. FProcessor_SmCondition_Polled          — Tick polled conditions (call Evaluate())
  3. FProcessor_Sm_EvalTransitions          — For each SM, evaluate transitions in order, queue first winner
  4. FProcessor_SmTask_Tick                 — Tick tasks
  5. FProcessor_Sm_EndPlay                  — Cleanup
```

Note: Event-driven conditions call `MarkSatisfied()` from signal callbacks, which can happen at any point. The evaluation processor reads the current satisfied state.

---

## Builder API (DefineState)

### V2 usage in AngelScript:
```
void DoDefineState(FCk_Handle& InHandle)
{
    // Transition 1 (highest priority): go to Combat if health is low AND damage received
    auto TransitionToCombat = AddTransition(UCk_State_Combat);
    AddCondition(TransitionToCombat, UCk_Condition_HealthBelow50);       // Polled
    AddCondition(TransitionToCombat, UCk_Condition_OnDamageReceived);    // EventDriven

    // Transition 2: go to Flee if health is critical (any time)
    auto TransitionToFlee = AddTransition(UCk_State_Flee);
    AddCondition(TransitionToFlee, UCk_Condition_HealthBelow10);         // Polled

    // Tasks
    AddTask(UCk_Task_PatrolWaypoints);
}
```

### V2 C++ signature changes:
```cpp
// Before (V1):
auto AddTransition_Polled(TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass,
    FInstancedStruct InTransitionPayload = {}) -> FCk_Handle_SmTransition;

auto AddTransition_EventDriven(TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass,
    TSubclassOf<UCk_SmTransition_EntityScript> InTransitionClass,
    FInstancedStruct InTransitionPayload = {}) -> FCk_Handle_SmTransition;

// After (V2):
auto AddTransition(TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass) -> FCk_Handle_SmTransition;
```

---

## Backlog (Not in V2 scope)

### Task Accelerant Conditions
- Built-in condition: "Task X returned Succeeded/Failed"
- Built-in condition: "All tasks in current state succeeded"
- These are pre-built condition subclasses users can add without subclassing

### Transition Evaluation Optimization
- Early-out: stop evaluating lower-priority transitions once a winner is found
- Only re-evaluate transitions whose conditions have changed (dirty flag)

### Other
- Slate debug widget
- Transition cooldowns
- State history breadcrumb
- Graph visualization
- AngelScript example library
- Replication verification

---

## Test Plan

### Must verify after V2:
- [ ] Polled condition evaluates every frame, transition fires when all conditions true
- [ ] Event-driven condition: `MarkSatisfied()` → transition fires
- [ ] Non-latching condition resets every frame
- [ ] Latching condition persists across frames, resets on state exit
- [ ] Transition ordering: first added transition wins when multiple are satisfied
- [ ] Multiple transitions to same state (OR logic) works
- [ ] Mixed conditions on one transition (polled + event-driven) works
- [ ] Payloads via dynamic fragments work on transitions, tasks, conditions
- [ ] No const_cast anywhere in SM module
- [ ] Pause/Resume still works
- [ ] Start/Stop still works
- [ ] SM destruction cascades correctly
- [ ] All signals fire correctly
- [ ] Gym tests updated and passing
