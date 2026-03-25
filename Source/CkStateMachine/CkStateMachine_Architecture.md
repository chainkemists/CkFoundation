# CkStateMachine — Architecture

## Design Philosophy

### Everything Is an Entity
Every participant in the state machine — the machine itself, states, tasks, transitions, and conditions — is an ECS entity with an EntityScript. This gives maximum flexibility: any piece can carry fragments, bind signals, and participate in the ECS world naturally.

### Dynamic Graph
Unlike traditional state machines with static definitions, the graph is built at runtime by walking states. When a state is entered, its `DefineState()` method registers transitions and tasks as child entities. The graph emerges from state→transition→state references.

This enables:
- **Overriding states in derived classes** — subclass a state, override `DefineState()`, completely change its transitions
- **Runtime graph modification** — conditionally add/remove transitions in `DefineState()` based on game state
- **No schema/asset dependency** — pure code definitions, works in C++ and AngelScript

### Flat States, Composable Hierarchy
States are flat by design. Hierarchical state machines are achieved by creating a Task that spawns a sub-StateMachine in its state. This avoids first-class hierarchy complexity while enabling it through composition.

### Editing vs Viewing
- **Editing** (defining states, transitions, conditions): C++ and AngelScript only
- **Viewing** (debugging an entity's SM at runtime): Slate widget (V2)

---

## Entity Hierarchy

```
Game Entity (owns the SM)
└── StateMachine Entity (FCk_Handle_StateMachine)
    └── Current State Entity (FCk_Handle_SmState)
        ├── Task Entity 0..N (FCk_Handle_SmTask)
        └── Transition Entity 0..N (FCk_Handle_SmTransition)
            └── Condition Entity 0..N (FCk_Handle_SmCondition)
```

Lifetime ownership cascades: destroying the SM destroys everything. Destroying a state destroys its tasks, transitions, and conditions.

---

## Module Structure

```
Source/CkStateMachine/
├── CkStateMachine.Build.cs                          Build rules
├── CkStateMachine_Module.h/.cpp                     Module boilerplate
├── CkStateMachine_Log.h/.cpp                        Log category (ck::sm)
├── CkStateMachine_Architecture.md                   This file
├── CkStateMachine_Progress.md                       Implementation tracker
└── Public/CkStateMachine/
    ├── CkStateMachine_Fragment_Data.h               Handles, enums, requests, payloads, delegates
    ├── CkStateMachine_Fragment.h/.cpp               Fragments, tags, signals
    ├── CkStateMachine_Processor.h/.cpp              5 processors
    ├── CkStateMachine_Utils.h/.cpp                  BPFL public API
    ├── CkStateMachine_ProcessorInjector.h/.cpp      Processor registration
    └── EntityScripts/
        ├── CkSmState_EntityScript.h/.cpp            State base class + builder API
        ├── CkSmTask_EntityScript.h/.cpp             Task base class (tick / enter-exit)
        ├── CkSmTransition_EntityScript.h/.cpp       Event-driven transition base class
        └── CkSmCondition_EntityScript.h/.cpp        Condition base class
```

---

## Data Model

### Handles
| Handle | Entity Type |
|--------|-------------|
| `FCk_Handle_StateMachine` | The state machine entity |
| `FCk_Handle_SmState` | A state entity (child of SM) |
| `FCk_Handle_SmTask` | A task entity (child of state) |
| `FCk_Handle_SmTransition` | A transition entity (child of state) |
| `FCk_Handle_SmCondition` | A condition entity (child of transition) |

### Enums
| Enum | Values | Purpose |
|------|--------|---------|
| `ECk_SmRunStatus` | Stopped, Running, Paused | SM lifecycle state |
| `ECk_SmAutoStart` | Disabled, OnSetup | Whether SM auto-starts |
| `ECk_SmTaskResult` | Running, Succeeded, Failed | Tick task result |
| `ECk_SmTaskMode` | EnterExitOnly, Tick | Task behavior mode |
| `ECk_SmTransitionMode` | Polled, EventDriven | Transition evaluation mode |

### Tags
| Tag | On Entity | Purpose |
|-----|-----------|---------|
| `FTag_Sm_RequiresSetup` | SM | One-time setup pending |
| `FTag_Sm_Running` | SM | SM is running |
| `FTag_Sm_Paused` | SM | SM is paused |
| `FTag_SmTask_Tick` | Task | Task ticks every frame |
| `FTag_SmTask_EnterExit` | Task | Task only fires on enter/exit |
| `FTag_SmTransition_Polled` | Transition | Conditions evaluated every frame |
| `FTag_SmTransition_EventDriven` | Transition | Triggered by external event |

### Fragments
| Fragment | On Entity | Fields |
|----------|-----------|--------|
| `FFragment_Sm_Params` | SM | `_InitialStateClass`, `_AutoStart` |
| `FFragment_Sm_Current` | SM | `_RunStatus`, `_CurrentStateHandle`, `_CurrentStateClass` |
| `FFragment_Sm_Requests` | SM | `TArray<variant<Start,Stop,Pause,Resume,Transition>>` |
| `FFragment_SmTransition_Params` | Transition | `_OwnerStateMachine`, `_TargetStateClass`, `_TransitionPayload` |
| `FFragment_SmTask_Current` | Task | `_LastResult` |
| `FFragment_SmCondition_Current` | Condition | `_IsSatisfied` |

### Signals
| Signal | Payload | Fires When |
|--------|---------|------------|
| `OnSmStateChanged` | `_PreviousStateClass`, `_NewStateClass`, `_NewStateHandle` | State transition completes |
| `OnSmStarted` | (empty) | SM starts |
| `OnSmStopped` | (empty) | SM stops |

---

## Processor Pipeline

Execution order within each phase matters.

### Requests Phase
1. **`FProcessor_Sm_Setup`** — Removes `FTag_Sm_RequiresSetup`, auto-queues Start if configured
2. **`FProcessor_Sm_HandleRequests`** — Processes all queued requests via `std::variant` dispatch

### Update Phase
3. **`FProcessor_SmTask_Tick`** — Ticks all task entities with `FTag_SmTask_Tick`, stores result
4. **`FProcessor_Sm_EvalPolledTransitions`** — Evaluates polled transitions, queues transition requests
5. **`FProcessor_Sm_EndPlay`** — Cleans up SM state on entity destruction

---

## EntityScript Base Classes

### UCk_SmState_EntityScript

The core user-facing class. Users subclass this to define states.

**Lifecycle:**
- `Construct` → finds owning SM, calls `DefineState()`
- `BeginPlay` → calls `OnStateEnter()`
- `EndPlay` → calls `OnStateExit()`

**Builder API (called from `DefineState()`):**
```cpp
// Add a task to this state
auto AddTask(TSubclassOf<UCk_SmTask_EntityScript>, FInstancedStruct = {}) -> FCk_Handle_SmTask;

// Add a polled transition (conditions checked every frame)
auto AddTransition_Polled(TSubclassOf<UCk_SmState_EntityScript>, FInstancedStruct = {}) -> FCk_Handle_SmTransition;

// Add an event-driven transition (user subclass handles triggering)
auto AddTransition_EventDriven(TSubclassOf<UCk_SmState_EntityScript>, TSubclassOf<UCk_SmTransition_EntityScript>, FInstancedStruct = {}) -> FCk_Handle_SmTransition;

// Add a condition to a transition
static auto AddCondition(FCk_Handle_SmTransition&, TSubclassOf<UCk_SmCondition_EntityScript>, FInstancedStruct = {}) -> FCk_Handle_SmCondition;
```

### UCk_SmTask_EntityScript

Base class for tasks that run within a state.

**Modes:**
- `EnterExitOnly` — `OnStateEnter()` / `OnStateExit()` callbacks only
- `Tick` — additionally `Tick(float)` called every frame, returns `ECk_SmTaskResult`

### UCk_SmTransition_EntityScript

Base class for event-driven transitions. Users subclass, bind to signals in `BeginPlay()`, call `TriggerTransition()` when the event fires.

`TriggerTransition()` evaluates all child conditions and queues a transition request if all pass.

### UCk_SmCondition_EntityScript

Base class for conditions. Override `Evaluate() const -> bool`.

---

## State Transition Flow

### Starting the SM
1. `UCk_Utils_StateMachine_UE::Add(Owner, InitialStateClass)` creates the SM entity
2. `FProcessor_Sm_Setup` removes setup tag, auto-queues Start if `OnSetup`
3. `FProcessor_Sm_HandleRequests` processes Start → calls `DoEnterState`

### Entering a State
1. `DoEnterState` calls `UCk_Utils_EntityScript_UE::Add(SmHandle, StateClass, Payload, PostConstructionFunc)`
2. State EntityScript's `Construct` → finds owning SM → calls `DefineState()`
3. `DefineState()` spawns child entities (tasks, transitions, conditions)
4. `PostConstructionFunc` sets `_CurrentStateHandle` on SM
5. `BeginPlay()` fires → `OnStateEnter()` on state and all tasks

### Polled Transition
1. `FProcessor_Sm_EvalPolledTransitions` iterates polled transitions each frame
2. For each: check SM is running + not paused, evaluate all child conditions
3. If all conditions pass → queue `FCk_Request_Sm_Transition` on SM
4. Request handler exits current state, enters target state

### Event-Driven Transition
1. Transition EntityScript binds to a signal in `BeginPlay()`
2. Signal fires → callback calls `TriggerTransition()`
3. Evaluates conditions → queues transition request if all pass

### Exiting a State
1. `DoExitCurrentState` destroys the current state entity
2. Lifetime cascade: state EndPlay → tasks EndPlay → transitions/conditions destroyed
3. `OnStateExit()` fires on state and all tasks

---

## Usage Examples

### Basic State Machine (C++)

```cpp
UCLASS()
class UEnemyNearbyCondition : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()
    auto Evaluate() const -> bool override { /* game logic */ return true; }
};

UCLASS()
class UPatrolTask : public UCk_SmTask_EntityScript
{
    GENERATED_BODY()
    UPatrolTask() { _TaskMode = ECk_SmTaskMode::Tick; }
    auto OnStateEnter() -> void override { /* start patrol */ }
    auto OnStateExit() -> void override  { /* stop patrol */ }
    auto Tick(float InDeltaSeconds) -> ECk_SmTaskResult override { return ECk_SmTaskResult::Running; }
};

UCLASS()
class UIdleState : public UCk_SmState_EntityScript
{
    GENERATED_BODY()
    auto DefineState(FCk_Handle& InHandle) -> void override
    {
        AddTask(UPatrolTask::StaticClass());
        auto Transition = AddTransition_Polled(UCombatState::StaticClass());
        AddCondition(Transition, UEnemyNearbyCondition::StaticClass());
    }
};

// Setup
auto SmHandle = UCk_Utils_StateMachine_UE::Add(GameEntity, UIdleState::StaticClass());
```

### Listening to State Changes

```cpp
UCk_Utils_StateMachine_UE::BindTo_OnStateChanged(SmHandle, OnStateChangedDelegate);
```

### Hierarchical SM via Task

```cpp
UCLASS()
class USubSmTask : public UCk_SmTask_EntityScript
{
    GENERATED_BODY()
    USubSmTask() { _TaskMode = ECk_SmTaskMode::EnterExitOnly; }
    auto OnStateEnter() -> void override
    {
        auto StateHandle = DoGet_ScriptEntity();
        UCk_Utils_StateMachine_UE::Add(StateHandle, USubInitialState::StaticClass());
    }
};
```

---

## Edge Cases & Safeguards

1. **Multiple transitions same frame** — First polled transition that passes wins. Consider adding `FTag_Sm_TransitionPending` guard.
2. **Transition during construction** — Polled transitions only evaluate entities with `FTag_EntityScript_HasBegunPlay`.
3. **Condition accessing game state** — Walk ownership chain: condition → transition → state → SM → game entity. Provide `Get_GameEntity()` helper.

---

## Future Evolution

### V2: Debug Slate Widget
- Text-based status panel: current state, active tasks, transition history
- Selectable per-entity in editor
- Graph visualization (states as nodes, transitions as arrows, current state highlighted)

### V2+: Additional Features
- Transition priority ordering
- Transition cooldowns
- Task result-driven transitions (e.g., transition when a tick task returns `Succeeded`)
- State history / breadcrumb trail for debugging
- AngelScript example library
