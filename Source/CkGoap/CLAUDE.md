# CkGoap

**Purpose:** Goal-Oriented Action Planning built on top of `CkAStar`'s time-sliced A* search. The model is a tree of **Actions** grouped under an **ActionSet**; each active Action has its own planner running a regressive A* search to pick the best child Action that satisfies its goal. World state is classical boolean (`TMap<FGameplayTag, bool>`). The planner is **search only** — executing the resulting plan is the consumer's job.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkAStar`, `CkLog`. Private deps: `CkLabel`, `CkRecord` (intentionally not public — `FFragment_RecordOfGoapActionSets` / `FFragment_RecordOfGoapActions` live inside implementation files to avoid forcing every consumer to link `CkEntityExtension`).
**Used by:** Tactical / strategic AI on entities that need multi-step planning. Pairs naturally with `CkEqs` (pick the destination) and `CkStateMachine` (drive plan execution).

---

## Add vs Create — two installation paradigms

```cpp
// Add — owner IS the Goap root. One Goap root per owner. Use when the entity
// you already manage needs a single Goap root and downstream code will cast
// the owner handle to FCk_Handle_Goap directly.
// CAUTION: owner must NOT already have standalone CkAStar — GOAP stamps its
// own FFragment_AStar_Params per Action and the two will collide.
auto GoapHandle = UCk_Utils_Goap_UE::Add(InOwner, Params);

// Create — spawns a NAMED child entity that hosts the Goap root. The owner
// gets a RecordOfGoapPlanners (private to the implementation). Use when one
// owner needs multiple Goap roots, when the owner already has standalone
// AStar, or when the Goap root's lifetime should be independent of the
// owner's other features.
auto GoapHandle = UCk_Utils_Goap_UE::Create(InOwner, FGameplayTag{"Goap.Tactical"}, Params);
```

Find them later with:
- `Find_Goap(InHandle)` — returns the Goap root if InHandle itself has the feature (Add case), otherwise the first valid entry from the record (Create case).
- `Find_GoapByName(InHandle, FGameplayTag)` — explicit lookup for multi-root owners.

---

## Architecture in one diagram

```
        DESIGN TIME (CDO)                    ECS PIPELINE (runtime)
   ─────────────────────────              ────────────────────────────
   UCk_GoapAction_EntityScript            FProcessor_Goap_Action_Setup
     DoDefineAction                         scans CDOs of registered
       AddPrecondition                      Action classes, extracts
       AddEffect                            ActionDefs, registers WS
       SetCost                              keys, builds _DependencyCycles
                                                       ↓
                                            FProcessor_Goap_Action_AutoReplan
                                              consumes per-Action dirty tags,
                                              enqueues Plan requests per policy
                                                       ↓
                                            FProcessor_Goap_Action_HandleRequests
                                              drains per-Action request queue,
                                              builds A* graph from _ChildActions,
                                              seeds AStar SearchState
                                                       ↓
                                            TProcessor_AStar_Execute<...>
                                              time-sliced regressive A*,
                                              budget = _SearchBudgetMicroseconds
                                                       ↓
                                            FProcessor_Goap_Action_HandleResult
                                              converts A* path to child-Action
                                              class list, fires OnPlanComplete /
                                              OnPlanFailed
                                                       ↓
                                            FProcessor_Goap_ActionSet_ChainUpdate
                                              walks each ActionSet's ActiveChain,
                                              applies truncate/extend rule
```

All processors live in `FGroup_Gameplay_AI`. Order within the group:

```
Setup → AutoReplan → HandleRequests → AStar_Execute → HandleResult → ChainUpdate
```

ChainUpdate runs last so every Action in the chain has a fresh `Plan[0]` before chain mutation decisions.

---

## Entity hierarchy

```
FCk_Handle (owner — NPC, pawn, etc.)
  └── FCk_Handle_Goap (Goap root — one per entity)
        └── FCk_Handle_Goap_ActionSet (one per decision domain)
              ├── _RootAction: FCk_Handle_Goap_Action
              ├── _ActiveChain: [root, child, grandchild, ...]
              └── FCk_Handle_Goap_Action (catalog entry — registered action)
                    ├── _ParentAction: FCk_Handle_Goap_Action (invalid for root)
                    ├── _ChildActions: TArray<FCk_Handle_Goap_Action>
                    └── own A* planner state (SearchState, Result, PlanContext)
```

The ActionSet keeps a **flat catalog** of all registered Actions regardless of tree depth. The hierarchy lives in each Action's `_ParentAction` / `_ChildActions`.

---

## Public API surface

### `UCk_Utils_Goap_ActionSet_UE`

| Group | Function | Notes |
|---|---|---|
| **Construction** | `AddActionSet(Goap, Params)` | Creates a decision domain on the Goap root. `Params` carries `_ActionSetTag` and `_InitialToggle`. |
| | `SetRootAction(ActionSet, ActionParams, InitialWorldState)` | Designates the entry-point Action; `InitialWorldState` becomes the ActionSet's default WS source. Returns the root `FCk_Handle_Goap_Action`. |
| | `AddAction_ToActionSet(ActionSet, ActionParams)` | Adds a top-level Action (sibling of root). Uncommon — most authoring puts everything in the root's subtree. |
| **Query** | `Has(Handle)` | True if the entity has a Goap ActionSet feature. |
| | `Find_Action(ActionSet, ActionTag)` | Catalog lookup by class-derived tag. |
| | `Find_ActionByClass(ActionSet, ActionClass)` | Catalog lookup by class. |
| | `Get_ActiveChain(ActionSet)` | Ordered active Action list; `[0]` = root. |
| | `Get_EnableToggle(ActionSet)` | Current enable/disable state. |
| | `Get_DependencyCycles(ActionSet)` | Setup-time cycle diagnostics (Tarjan SCC over child-edge graph). |
| | `Get_RootAction(ActionSet)` | Current root Action handle. |
| **Requests** | `Request_SetEnableToggle(ActionSet, Toggle)` | Enable or disable the ActionSet. Disabled ActionSets skip planning and chain-update. |
| | `Request_SetRootAction(ActionSet, ActionParams, InitialWorldState)` | Swap the root at runtime; truncates the active chain, creates the new root. |
| | `Request_ResetActiveChain(ActionSet)` | Collapses the chain back to root-only; fires `OnActionDeactivated` per removed Action. |
| **Signals** | `BindTo_OnActiveChainChanged(ActionSet, Delegate, ...)` | Fires whenever the chain mutates. Payload includes old chain; read new chain via `Get_ActiveChain`. |
| | `UnbindFrom_OnActiveChainChanged(...)` | Counterpart unbind. |

### `UCk_Utils_Goap_Action_UE`

| Group | Function | Notes |
|---|---|---|
| **Construction** | `AddAction_ToAction(ParentAction, ActionParams)` | Registers a child Action under the parent; parent becomes composite. |
| **Query** | `Has(Handle)` | True if the entity has a Goap Action feature. |
| | `Get_PlanStatus(Action)` | `ECk_GoapPlanStatus`: Idle / Planning / PlanFound / PlanFailed / CostThresholdReached. |
| | `Get_Plan(Action)` | Ordered child Action classes chosen by the last plan. |
| | `Get_PlanCost(Action)` | Cost of the last plan. |
| | `Get_WorldStateSource(Action)` | The resolved WS handle this Action consumes. |
| | `Get_ActiveParentAction(Action)` | Class of the parent that currently has this Action in its active sub-plan; null if root or dormant. |
| | `Get_InvalidGoal(Action)` | Effects that reference unregistered WS keys (populated at Setup time). |
| **Requests** | `Request_Plan(Action)` | Force an immediate replan. |
| | `Request_CancelPlan(Action)` | Abort in-flight A* search. |
| | `Request_SetGoalWorldState(Action, Goal)` | Override the Action's goal conditions at runtime. |
| | `Request_SetActionCost(Action, ChildClass, Cost)` | Adjust a child Action's cost; flagged dirty for `OnCostDirty` / `OnEitherDirty` policies. |
| | `Request_SetReplanInterval(Action, Seconds)` | Throttle replans to at most once per interval. |
| | `Request_SetReplanPolicy(Action, Policy)` | Change the replan trigger (see table below). |
| | `Request_SetSearchBudget(Action, Microseconds)` | Time slice for this Action's A* search per frame. |
| | `Request_SetCostThreshold(Action, Threshold)` | Early-out when best frontier FScore exceeds threshold; fires `CostThresholdReached`. |
| **Signals** | `BindTo_OnPlanComplete(Action, Delegate, ...)` | Fires when `PlanFound`; payload = chosen child class list + plan cost. |
| | `BindTo_OnPlanFailed(Action, Delegate, ...)` | Fires when planning cannot satisfy the Action's goal. |
| | `BindTo_OnActionActivated(Action, Delegate, ...)` | Fires once when this Action enters the active chain. |
| | `BindTo_OnActionDeactivated(Action, Delegate, ...)` | Fires once when this Action leaves the active chain. |
| | Unbind counterparts | `UnbindFrom_On*` for each signal above. |

---

## Replan policy

`ECk_Goap_ReplanPolicy` is **per-Action** (set via `Request_SetReplanPolicy` or `_ReplanPolicy` on `FCk_Fragment_Goap_ActionParamsData`):

| Policy | Triggers replan when |
|---|---|
| `Explicit` | Never — only an explicit `Request_Plan` does. |
| `OnWorldStateDirty` | A registered key in the resolved WS changes value. |
| `OnCostDirty` | A child Action's cost changes. |
| `OnEitherDirty` | Either of the above. |

Dirty events within `_MinReplanIntervalSeconds` coalesce into one replan at window end. Default interval is `0.0` (no throttle).

`_PlanOnStart` (default `true`) fires an initial `Request_Plan` after Setup completes for the Action — saves an explicit kick in `DoConstruct`.

---

## Composite vs atomic Actions

- **Composite Action** — has at least one child registered via `AddAction_ToAction`. When active, its own A* planner searches among its children to satisfy its goal. `Plan[0]` is the chosen next child. If that child is also composite, `ChainUpdate` extends the active chain to include it.
- **Atomic Action** — no registered children. When in a parent's `Plan[0]`, it terminates the active chain at the parent's depth. Executed by gameplay code (state machine, action runner) that subscribes to `OnPlanComplete` on the deepest chain link.

Composite vs atomic is determined at registration time and does not change at runtime.

---

## Active chain

`Get_ActiveChain(ActionSet)` returns the ordered list of currently-active Actions:

```
[0] root Action
[1] root's chosen child (Plan[0] of the root, if composite)
[2] that child's chosen grandchild, etc.
```

`OnActionActivated` fires when an Action is appended to the chain. `OnActionDeactivated` fires when it is removed (truncation or reset). The deepest Action in the chain is the one whose `OnPlanComplete` payload the action-runner subscribes to.

---

## WorldState resolution

For each Action at activation time:

```
_WorldStateSource_Resolved =
    _WorldStateSource_Override (if set on this Action via FCk_Fragment_Goap_ActionParamsData)
    ELSE ParentAction._WorldStateSource_Resolved (if Action has a parent)
    ELSE ActionSet's WS (supplied to SetRootAction as InInitialWorldState)
```

The eager resolve happens when an Action is appended to the chain by `ChainUpdate`. Root Actions and top-level ActionSet Actions (siblings of root) must supply a WS source via `_WorldStateSource_Override` — there is no parent to inherit from.

---

## Authoring an Action

Override `DoDefineAction` (Blueprint/AngelScript) or `DefineAction` (C++) on a subclass of `UCk_GoapAction_EntityScript`:

```cpp
UCLASS(Blueprintable)
class UCk_GoapAction_OpenDoor : public UCk_GoapAction_EntityScript
{
    GENERATED_BODY()
public:
    virtual auto DefineAction() -> void override
    {
        AddPrecondition(Tag_HasKey,       true);
        AddEffect      (Tag_DoorUnlocked, true);
        SetCost(1.0f);
    }
};
```

- `AddPrecondition(Tag, Value)` / `AddEffect(Tag, Value)` / `SetCost(float)` are the only builder methods.
- **No `SetActionTag` call** — that builder is removed. Identity is class-derived.
- Goals are not separate scripts. A Root Action's `_InitialGoal_RootOnly` (in its `FCk_Fragment_Goap_ActionParamsData`) IS the goal. Non-root Actions use their own declared `_Effects` as the goal when activated.
- **Boolean only.** Numeric / enum state must be projected to a boolean tag before use (`HasEnoughFood = (Food >= Threshold)`).

---

## Fragment table (brief)

| Fragment / tag | Lives on | Role |
|---|---|---|
| `FFragment_RecordOfGoapActionSets` | Goap root | Record of ActionSet entities |
| `FFragment_Goap_ActionSet_Params` | ActionSet | `_ActionSetTag`, initial toggle |
| `FFragment_Goap_ActionSet_Current` | ActionSet | Enable state, `_RootAction` handle, `_DependencyCycles` |
| `FFragment_Goap_ActionSet_ActiveChain` | ActionSet | Ordered active Action handles |
| `FFragment_RecordOfGoapActions` | ActionSet | Flat catalog of all registered Actions |
| `FFragment_Goap_Action_Params` | Action | `_ActionClass`, WS override, search budget, replan policy/interval, cost threshold, `_PlanOnStart` |
| `FFragment_Goap_Action_Definition` | Action | CDO-extracted: preconditions, effects, cost |
| `FFragment_Goap_Action_Tree` | Action | `_ParentAction`, `_ChildActions` |
| `FFragment_Goap_Action_Current` | Action | `_WorldStateSource_Resolved`, `_Goal`, `_InvalidGoal`, `_Plan`, `_PlanCost`, `_PlanStatus`, `_ActiveParent` |
| `FFragment_Goap_Action_Requests` | Action | std::variant request queue |
| `FFragment_Goap_Action_ReplanThrottle` | Action | Per-Action throttle accumulator |
| `FFragment_Goap_Action_SearchState` / `_Result` / `_PlanContext` | Action | Underlying A* state |
| `FTag_Goap_Action_RequiresSetup` | Action | One-shot setup gate |
| `FTag_Goap_Action_RequiresInitialPlan` | Action | Added when `_PlanOnStart`; drives first plan |
| `FTag_Goap_Action_PlanRequested` | Action | Request-flow gate |
| `FTag_Goap_Dirty_WorldState` / `FTag_Goap_Dirty_Cost` | Action | Per-Action value-change dirty tracking |

See the design spec (§2.2) for the full table.

---

## Anti-patterns

- **Calling `Add` on an owner that already has standalone `CkAStar`.** GOAP stamps its own `FFragment_AStar_Params` per Action; the two collide. Use `Create` (child entity) or remove the standalone AStar feature.
- **Setting world state with a tag no Action references.** The key registry is sealed after Setup; writes to unregistered tags are silent no-ops (Verbose-logged). Reference the key in at least one precondition or effect to register it.
- **Forgetting to add a child Action to a Mid-level Action.** A Mid-level Action with no registered children is atomic — the chain stops at its parent. Chain doesn't extend past it. Always call `AddAction_ToAction(Mid, LeafClass)` for every intended composite.
- **Setting `_PlanOnStart = true` (the default) on a child Action that should only plan when activated.** Eager-resolve causes pre-activation planning. Set `_PlanOnStart = false` on `FCk_Fragment_Goap_ActionParamsData` if you need strict "plan only when activated" semantics.
- **Calling `Request_ResetActiveChain` and expecting the chain to stay at length 1.** The next `ChainUpdate` frame will re-extend it if the root's plan contains a composite child. To prevent re-extension, disable the ActionSet first via `Request_SetEnableToggle(ActionSet, Disable)`.
- **Action's effects reference unregistered WS keys.** These land in `_InvalidGoal` at Setup time; the planner can't satisfy the goal. Check via `Get_InvalidGoal(Action)`.
- **Trying to make the planner *execute* the plan.** GOAP is a planner. After `OnPlanComplete` fires, the consumer (state machine / hand-written runner) walks `Get_Plan()` and drives behaviour. Re-planning while executing is fine — `Request_CancelPlan` aborts in-flight search; `Request_Plan` queues a fresh one.
- **Numeric world state.** Classical boolean GOAP only. Project to booleans (`HasEnoughX`, `IsAtY`, `IsLowZ`).
- **Reading `Get_Plan()` while `Get_PlanStatus() == Planning`.** The plan is only populated after `HandleResult` runs. Wait for `OnPlanComplete` or poll `Get_PlanStatus()` for a terminal status.
- **Skipping `CK_REGISTER_PROCESSOR` when adding a new GOAP processor.** An unregistered processor compiles silently and is never scheduled.

---

## See also

- `CkAStar/Claude.md` — underlying time-sliced A* (search budgets, search-state lifecycle, parallel execute).
- `CkEqs/Claude.md` — natural pairing for "where" decisions feeding "what" decisions.
- `CkStateMachine/Claude.md` — plan execution / orchestration.
- `CkEntityScript/Claude.md` (via `CkEcs`) — base class semantics for the Action EntityScript.
- Design spec: `docs/superpowers/specs/2026-05-19-CkGoap-ActionSetUnification-design.md` — full data model, processor pseudocode, lifecycle invariants, diagnostics.
