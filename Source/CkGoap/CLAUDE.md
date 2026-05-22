# CkGoap

**Purpose:** Goal-Oriented Action Planning built on top of `CkAStar`'s time-sliced A* search. The model is a tree of **Action** entities grouped under **Planner** entities; each active Planner runs a regressive A* search over its registered child Actions to produce a multi-step plan. World state is classical boolean (`TMap<FGameplayTag, bool>`). The planner is **search only** — executing the resulting plan is the consumer's job.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkAStar`, `CkLog`. Private deps: `CkLabel`, `CkRecord`.
**Used by:** Tactical / strategic AI on entities that need multi-step planning. Pairs naturally with `CkEqs` (pick the destination) and `CkStateMachine` (drive plan execution).

---

## Two typesafe handles, fragment-discriminated

| Handle | Discriminator fragment | Meaning |
|---|---|---|
| `FCk_Handle_Goap_Planner` | `FFragment_Goap_Planner_Params` (+ `FFragment_Goap_Planner_Current`) | This entity runs a goal-directed A* search over its registered child Actions. |
| `FCk_Handle_Goap_Action` | `FFragment_Goap_Action_Definition` | This entity is a unit of work: CDO-extracted preconditions, effects, cost — offered as a candidate operator to some parent Planner. |

`FCk_Handle_Goap` (the old root container) **does not exist**. There is no dedicated root entity type.

An entity may carry **one or both** clusters:

| Position in tree | Action role | Planner role |
|---|---|---|
| Top-level Planner (nothing picks it as a step) | — | ✓ |
| Leaf Action (no children) | ✓ | — |
| Mid-tier composite (picked by parent AND has children of its own) | ✓ | ✓ |

Both casts succeed for a mid-tier composite:

```cpp
auto EngageAsAction  = UCk_Utils_Goap_Action_UE::Cast(EngageEntity);   // valid
auto EngageAsPlanner = UCk_Utils_Goap_Planner_UE::Cast(EngageEntity);  // valid (same entity)
```

---

## Add vs Create — two Planner installation paradigms

```cpp
// Add — stamps Planner fragments onto InOwner directly.
// Returns the typesafe Planner handle.
auto PlannerHandle = UCk_Utils_Goap_Planner_UE::Add(InOwner, Params);

// Create — spawns a named child entity that hosts the Planner.
// Use when one owner needs multiple independent Planners (e.g., combat + dialogue).
auto PlannerHandle = UCk_Utils_Goap_Planner_UE::Create(InOwner, FGameplayTag{"Goap.Tactical"}, Params);

// Find a named Planner added via Create:
auto PlannerHandle = UCk_Utils_Goap_Planner_UE::Find_Planner(InOwner, Tag);
```

`PromoteActionToPlanner` converts an existing Action entity to also carry the Planner role:

```cpp
// Engage is already an Action; now it also becomes a Planner with its own goal.
auto EngagePlanner = UCk_Utils_Goap_Planner_UE::PromoteActionToPlanner(EngageAction, PlannerParams);
// After this, AddAction calls on EngagePlanner register children under Engage.
```

---

## Architecture in one diagram

```
        DESIGN TIME (CDO)                    ECS PIPELINE (runtime)
   ─────────────────────────              ────────────────────────────
   UCk_GoapAction_EntityScript            FProcessor_Goap_Action_Setup
     DefineAction                           scans CDOs of registered
       AddPrecondition                      Action classes, extracts
       AddEffect                            ActionDefs, registers WS
       SetCost                              keys, builds DependencyCycles
                                                       ↓
                                            FProcessor_Goap_Action_AutoReplan
                                              consumes dirty tags per Action,
                                              enqueues Plan requests per policy
                                                       ↓
                                            FProcessor_Goap_Action_HandleRequests
                                              drains per-Action request queue,
                                              builds A* graph from child Actions,
                                              seeds AStar SearchState
                                                       ↓
                                            TProcessor_AStar_Execute<...>
                                              time-sliced regressive A*,
                                              budget = _SearchBudgetMicroseconds
                                                       ↓
                                            FProcessor_Goap_Action_HandleResult
                                              converts A* path to Action entity
                                              list (PlanState._Plan), fires
                                              OnPlanComplete / OnPlanFailed;
                                              sets FTag_Goap_Planner_RequiresChainUpdate
                                                       ↓
                                            FProcessor_Goap_Planner_UpdateActivation
                                              per-Planner: compare Plan[0] to
                                              _LastActivatedPlan0; fire
                                              OnPlannerActivated / Deactivated;
                                              walk Plan[0]s to derive active chain
```

All processors live in `FGroup_Gameplay_AI`. Order within the group:

```
Setup → AutoReplan → HandleRequests → AStar_Execute → HandleResult → UpdateActivation
```

UpdateActivation runs last so every Planner has a fresh `Plan[0]` before activation decisions.

---

## Entity hierarchy (illustrative)

```
Owner entity (NPC)
  └── Planner_Alive                   [Planner role]
        ├── Action_Engage             [Action + Planner roles]
        │     ├── Action_LightAttacks [Action + Planner roles]
        │     │     ├── Action_Light1 [Action role]
        │     │     └── Action_Light2 [Action role]
        │     └── Action_WalkToEnemy  [Action + Planner roles]
        │           ├── Action_Walk   [Action role]
        │           └── Action_Run    [Action role]
        └── Action_Idle               [Action + Planner roles]
              └── ...
```

- `Planner_Alive` carries only the Planner role — nothing picks it as a step, so it has no effects or cost.
- Every node beneath it carries the Action role. Nodes with children are also promoted to the Planner role.
- "Top-level" is emergent: nothing references `Planner_Alive` as a child. The framework does not declare it as root.

---

## Public API surface

### `UCk_Utils_Goap_Planner_UE`

| Group | Function | Notes |
|---|---|---|
| **Construction** | `Add(Owner, Params)` | Stamps Planner role onto Owner. `Params._WorldStateSource` is required for top-level Planners. |
| | `Create(Owner, Tag, Params)` | Spawns named child Planner entity. |
| | `Find_Planner(Owner, Tag)` | Lookup by tag for Create-spawned Planners. |
| | `AddAction(Planner, ActionParams)` | Canonical construction verb for Actions. On a top-level Planner: first call = implicit-root Action (runs A*); subsequent calls become tree children of the implicit root. On a promoted mid-tier Planner: every call adds a direct tree child of the host (which itself runs A*). Returns `FCk_Handle_Goap_Action`. |
| | `PromoteActionToPlanner(Action, PlannerParams)` | Stamps Planner fragments onto an existing Action entity. After promotion, `AddAction` on the promoted handle adds direct tree children. Returns `FCk_Handle_Goap_Planner`. |
| **Query** | `Has(Handle)` | True if the entity has the Planner role. |
| | `Find_Action(Planner, Tag)` | Catalog lookup by class-derived tag. |
| | `Find_ActionByClass(Planner, Class)` | Catalog lookup by EntityScript class. |
| | `Get_ActiveChain(Planner)` | Walks Plan[0] links from this Planner downward; returns ordered handles. |
| | `Get_EnableToggle(Planner)` | Current enable/disable state. |
| | `Get_DependencyCycles(Planner)` | Setup-time cycle diagnostics (Tarjan SCC). |
| | `Get_RootAction(Planner)` | The `_RootAction` from `FFragment_Goap_Planner_Current`. |
| **Requests** | `Request_SetEnableToggle(Planner, Toggle)` | Enable or disable; disabled Planners skip planning and activation. |
| | `Request_ResetActiveChain(Planner)` | Collapses active chain; fires `OnPlannerDeactivated` per removed node. |
| | `Request_SetGoal(Planner, Goal)` | Set this Planner's goal. Triggers a replan. |
| **Signals** | `BindTo_OnActiveChainChanged(Planner, Delegate, ...)` | Fires whenever the chain mutates. |
| | `UnbindFrom_OnActiveChainChanged(...)` | Counterpart unbind. |

### `UCk_Utils_Goap_Action_UE`

| Group | Function | Notes |
|---|---|---|
| **Query** | `Has(Handle)` | True if the entity has the Action role. |
| | `Get_PlanStatus(Action)` | `ECk_GoapPlanStatus`: Idle / Planning / PlanFound / PlanFailed / CostThresholdReached. |
| | `Get_Plan(Action)` | Ordered child Action **classes** from the last plan (convenience mapping of `PlanState._Plan` entity handles to their EntityScript classes). |
| | `Get_PlanCost(Action)` | Cost of the last plan. |
| | `Get_WorldStateSource(Action)` | The resolved WS handle this Action consumes (`FFragment_Goap_Planner_WorldStateSource._Resolved`). |
| | `Get_ActiveParentAction(Action)` | Class of the parent Action that injected the current goal; null for top-level / dormant. |
| | `Get_InvalidGoal(Action)` | Effects referencing unregistered WS keys (populated at Setup time). |
| **Requests** | `Request_Plan(Action)` | Force an immediate replan. |
| | `Request_CancelPlan(Action)` | Abort in-flight A* search. |
| | `Request_SetActionCost(Action, ChildClass, Cost)` | Adjust a child Action's cost. |
| | `Request_SetReplanInterval(Action, Seconds)` | Throttle replans. |
| | `Request_SetReplanPolicy(Action, Policy)` | Change the replan trigger. |
| | `Request_SetSearchBudget(Action, Microseconds)` | Time slice for A* search. |
| | `Request_SetCostThreshold(Action, Threshold)` | Early-out when best frontier FScore exceeds threshold. |
| **Signals** | `BindTo_OnPlanComplete(Action, Delegate, ...)` | Fires when `PlanFound`; payload = plan entities + cost. |
| | `BindTo_OnPlanFailed(Action, Delegate, ...)` | Fires when planning cannot satisfy the goal. |
| | `BindTo_OnPlannerActivated(Action, Delegate, ...)` | Fires when this Action (in its Planner role) is activated by a parent selecting it as Plan[0]. Source handle is `FCk_Handle_Goap_Action`. |
| | `BindTo_OnPlannerDeactivated(Action, Delegate, ...)` | Fires when it is deactivated. |
| | Unbind counterparts | `UnbindFrom_On*` for each signal above. |

---

## Replan policy

`ECk_Goap_ReplanPolicy` is **per-Planner** (set via `Request_SetReplanPolicy` or `_ReplanPolicy` on `FCk_Fragment_Goap_PlannerParamsData`):

| Policy | Triggers replan when |
|---|---|
| `Explicit` | Only an explicit `Request_Plan` fires a replan. |
| `OnWorldStateDirty` | A registered WS key changes value. |
| `OnCostDirty` | A child Action's cost changes. |
| `OnEitherDirty` | Either of the above. |

Dirty events within `_MinReplanIntervalSeconds` coalesce into one replan at window end. Default interval is `0.0` (no throttle).

`_PlanOnStart` (default `true`) fires an initial `Request_Plan` after the Planner is activated — saves an explicit kick in `DoConstruct`.

---

## Multi-step plans at every tier

Each Planner produces a possibly-multi-step plan (`PlanState._Plan`). `Plan[0]` is the Action this Planner activates next. If that Action is itself a Planner, `UpdateActivation` walks downward — `Plan[0]` of `Plan[0]` — recursively, producing the full active chain.

`Get_ActiveChain(Planner)` derives this chain on demand:

```
Planner_Alive         → Plan[0] = Action_Engage (active)
  Action_Engage       → Plan[0] = Action_LightAttacks (active)
    Action_LightAttacks → Plan[0] = Action_Light2 (active, leaf)
```

The deepest active node's `OnPlanComplete` payload is what the action-runner subscribes to. There is no stored chain fragment — `Get_ActiveChain` walks Plan[0] links live.

---

## Per-Planner goal — no goal=effects rule

Each Planner has its own `_Goal` in `FFragment_Goap_Planner_Goal`, set independently at construction via `FCk_Fragment_Goap_PlannerParamsData._Goal` and mutable at runtime via `Request_SetGoal`. This goal is **completely independent** of:

- Any Action-role effects this same entity may carry.
- Any parent or descendant Planner's goal.

The Action-role effects are only what the parent Planner consumes when deciding "should I include this Action in my plan?" — a different layer of meaning.

A Planner with an empty `_GoalAuthored` has an empty `_Goal`; the planner emits an empty plan (`PlanFound` immediately). This is valid for top-level Planners that plan once at startup.

---

## Active chain — implicit derivation

The active chain is derived by `UpdateActivation` walking Plan[0] links each frame. It is not stored in a fragment. Planners that have not yet produced a plan, or whose `_IsActive` flag is `false`, do not participate in the walk.

`FFragment_Goap_Planner_Activation` holds two fields per Planner:

- `_LastActivatedPlan0` — the Plan[0] handle seen on the previous tick. Used to detect changes.
- `_IsActive` — whether a parent selected this Planner as its Plan[0] (or, for top-level Planners, always true). Inactive Planners skip the activation walk.

`OnGoap_Planner_Activated` fires when `_IsActive` flips to true. `OnGoap_Planner_Deactivated` fires when it flips to false.

---

## WorldState resolution

For each Action at activation time:

```
_Resolved =
    _WorldStateSource_Override (on this Action's FFragment_Goap_Planner_WorldStateSource)
    ELSE parent Action's _Resolved
    ELSE Planner's _WorldStateSource (supplied via PlannerParams._WorldStateSource on Add/Create)
```

Top-level Planners must supply a WS source; sub-Planners may inherit.

---

## Parent-plan gating

`FTag_Goap_Action_PlanInFlight` — set on an Action while its own A* search is in progress. Child Actions whose parent has this tag defer their own Plan requests one frame. Root-role Planners (no parent) are never gated. This prevents child planners from replanning during a parent's search and thrashing the plan state.

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

- `AddPrecondition` / `AddEffect` / `SetCost` are the only builder methods.
- Identity is class-derived — no `SetActionTag` call.
- **Boolean only.** Project numeric/enum state to boolean tags before use.

To register an Action under a Planner, call `UCk_Utils_Goap_Planner_UE::AddAction(Planner, Params)` where `Params._ActionClass` is the EntityScript subclass.

---

## Fragment table

#### Action-role fragments

| Fragment / tag | Lives on | Contents |
|---|---|---|
| `FFragment_Goap_Action_Definition` | Action entity | CDO-extracted `_Preconditions`, `_Effects`, `_Cost`; `_GoalFromEffects`; `_InvalidGoal`; `_CachedActionDef`. **Discriminator for Action role.** |
| `FFragment_Goap_Action_Params` (`FCk_Fragment_Goap_ActionParamsData`) | Action entity | `_ActionClass`, `_WorldStateSource_Override` |
| `FFragment_Goap_Action_Tree` | Action entity | `_ParentAction`, `_ChildActions` |
| `FFragment_Goap_Action_Current` | Action entity | `_ActiveParentAction` (the parent class that injected the current goal) |
| `FFragment_Goap_Action_Requests` | Action entity | `std::variant` request queue |
| `FFragment_Goap_Action_ReplanThrottle` | Action entity | `_SecondsSinceLastReplan` |
| `FFragment_Goap_Action_PlanContext` | Action entity | `_Graph` — `FGoapGraph` kept alive between search and result phases |
| `FFragment_Goap_Action_SearchState` | Action entity | Underlying A* state (aliased `TFragment_AStar_SearchState`) |
| `FFragment_Goap_Action_Result` | Action entity | Underlying A* result (aliased `TFragment_AStar_Result`) |
| `FFragment_Goap_Action_ActionClasses` | Action entity | Registered EntityScript classes (legacy catalog, preserved through U11) |
| `FTag_Goap_Action_RequiresSetup` | Action entity | One-shot setup gate |
| `FTag_Goap_Action_RequiresInitialPlan` | Action entity | Added when `_PlanOnStart`; drives first plan |
| `FTag_Goap_Action_PlanRequested` | Action entity | Request-flow gate |
| `FTag_Goap_Action_PlanInFlight` | Action entity | Parent-plan gating; child Planners defer while parent has this tag |

#### Planner-role fragments

| Fragment / tag | Lives on | Contents |
|---|---|---|
| `FFragment_Goap_Planner_Params` (`FCk_Fragment_Goap_PlannerParamsData`) | Planner entity | `_Goal`, `_WorldStateSource`, `_ReplanPolicy`, `_MinReplanIntervalSeconds`, `_SearchBudgetMicroseconds`, `_CostThreshold`, `_PlanOnStart` |
| `FFragment_Goap_Planner_Current` | Planner entity | `_EnableToggle`, `_DependencyCycles`, `_RootAction` |
| `FFragment_Goap_Planner_Activation` | Planner entity | `_LastActivatedPlan0`, `_IsActive` |
| `FFragment_Goap_Planner_ActionCatalogIndex` | Planner entity | `_TagToAction` map for O(1) tag lookup |
| `FFragment_Goap_Planner_WorldStateSource` | Planner entity (also Action entities) | `_WorldStateSource` (default), `_Resolved` (eager-resolved at activation) |
| `FFragment_Goap_Planner_PlanState` | Planner entity (also Action entities) | `_PlanStatus`, `_Plan` (TArray<FCk_Handle_Goap_Action>), `_PlanCost`, `_PlanAttemptCount` |
| `FFragment_Goap_Planner_Goal` | Planner entity (also Action entities) | `_GoalAuthored`, `_Goal` (resolved), `_InvalidGoal` |
| `FTag_Goap_Planner_RequiresSetup` | Planner entity | One-shot setup gate |
| `FTag_Goap_Planner_RequiresChainUpdate` | Planner entity | Set on plan-complete; consumed by `UpdateActivation` |

Note: `FFragment_Goap_Planner_PlanState`, `FFragment_Goap_Planner_Goal`, and `FFragment_Goap_Planner_WorldStateSource` live on every **Action** entity too (Action entities run their own planner). The Planner-role discriminator fragments (`Params`, `Current`, `ActionCatalogIndex`, `Activation`) are what distinguish a bare Action from a dual-role entity.

---

## Anti-patterns

- **Calling `Add` on an owner that already has standalone `CkAStar`.** GOAP stamps `FFragment_AStar_Params` per Action; the two collide. Use `Create` (child entity) or remove the standalone AStar feature.
- **Setting world state with a tag no Action references.** The key registry is sealed after Setup; writes to unregistered tags are silent no-ops. Reference the key in at least one precondition or effect to register it.
- **Trying to make a leaf Action also plan.** Leaf Actions have no children registered, so there is nothing to plan over. If you want a leaf to plan, `PromoteActionToPlanner` it first, then `AddAction(PromotedPlanner, ...)` to register children under the promoted host.
- **Expecting goal = effects on a composite.** This rule no longer exists. Set `_Goal` on `FCk_Fragment_Goap_PlannerParamsData` independently of the effects the Action-role declares.
- **Setting `_PlanOnStart = true` on a sub-Planner that should only plan when activated.** Eager planning fires before the first activation. Set `_PlanOnStart = false` to get "plan only when activated" semantics.
- **Calling `Request_ResetActiveChain` and expecting the chain to stay collapsed.** `UpdateActivation` re-extends the chain on the next frame if the Planner's plan still has a composite Plan[0]. Disable the Planner first via `Request_SetEnableToggle(Planner, Disable)`.
- **Reading `Get_Plan()` while `Get_PlanStatus() == Planning`.** The plan is only populated after `HandleResult` runs. Wait for `OnPlanComplete` or poll status.
- **Skipping `CK_REGISTER_PROCESSOR` when adding a new GOAP processor.** An unregistered processor compiles silently and is never scheduled.
- **Numeric world state.** Classical boolean GOAP only. Project to booleans (`HasEnoughX`, `IsAtY`, `IsLowZ`).

---

## See also

- `CkAStar/CLAUDE.md` — underlying time-sliced A* (search budgets, search-state lifecycle, parallel execute).
- `CkEqs/CLAUDE.md` — natural pairing for "where" decisions feeding "what" decisions.
- `CkStateMachine/CLAUDE.md` — plan execution / orchestration.
- `CkEntityScript/CLAUDE.md` (via `CkEcs`) — base class semantics for the Action EntityScript.
- Design spec: `docs/superpowers/specs/2026-05-21-CkGoap-PlannerActionCollapse-design.md` — full data model, processor pseudocode, lifecycle invariants, migration notes.
