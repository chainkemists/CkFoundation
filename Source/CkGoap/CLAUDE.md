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

`AddAction` always creates a direct tree child of the Planner it is called on. There is no longer any "first AddAction is special" rule — every Action handle returned from `AddAction` is registered in the Planner's `FFragment_Goap_Planner_ActionCatalogIndex` and offered as a candidate operator. Hierarchical decompositions use `PromoteActionToPlanner` to make a child Action also a Planner with its own subtree.

---

## Architecture in one diagram

```
        DESIGN TIME (CDO)                    ECS PIPELINE (runtime)
   ─────────────────────────              ────────────────────────────
   UCk_GoapAction_EntityScript            FProcessor_Goap_Action_Setup
     DefineAction                           per-Action CDO extraction:
       AddPrecondition                      Preconditions / Effects /
       AddEffect                            Cost → _CachedActionDef;
       SetCost                              registers WS keys
                                                       ↓
                                            FProcessor_Goap_Planner_Setup
                                              per-Planner: builds dependency
                                              graph over child Action defs,
                                              detects cycles, resolves
                                              _GoalAuthored → _Goal
                                                       ↓
                                            FProcessor_Goap_Planner_AutoReplan
                                              per-Planner: consumes WS / cost
                                              dirty flags, enqueues Plan
                                              requests per ReplanPolicy
                                                       ↓
                                            FProcessor_Goap_Planner_HandleRequests
                                              per-Planner: drains request queue,
                                              builds A* graph from child Actions,
                                              seeds the Planner's AStar
                                              SearchState
                                                       ↓
                                            TProcessor_AStar_Execute<...>
                                              per-Planner: time-sliced
                                              regressive A*; budget =
                                              PlannerParams._SearchBudgetMicroseconds
                                                       ↓
                                            FProcessor_Goap_Planner_HandleResult
                                              per-Planner: converts A* path to
                                              Action entity list
                                              (PlanState._Plan), fires
                                              OnPlanComplete / OnPlanFailed;
                                              sets FTag_Goap_Planner_RequiresChainUpdate
                                                       ↓
                                            FProcessor_Goap_Planner_UpdateActivation
                                              per-Planner: compare Plan[0] to
                                              _LastActivatedPlan0; fire
                                              OnPlannerActivated / Deactivated;
                                              walk Plan[0]s to derive active chain
```

All processors live in `FGroup_Gameplay_AI`. Order within the group (registered via `RunAfter` dep lists):

```
Action_Setup → Planner_Setup → Planner_AutoReplan → Planner_HandleRequests → AStar_Execute → Planner_HandleResult → Planner_UpdateActivation
```

`Planner_Setup` runs after `Action_Setup` so the WS key registry is fully populated before the Planner resolves `_GoalAuthored → _Goal`. `Planner_UpdateActivation` runs last so every Planner has a fresh `Plan[0]` before activation decisions.

`FProcessor_Goap_Action_Setup` is the only Action-matched processor remaining — its sole job is per-Action CDO extraction. Everything downstream of that is Planner-matched.

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
| | `AddAction(Planner, ActionParams)` | Canonical construction verb for Actions. Always creates a direct tree child of the Planner; the child is registered in the Planner's `_TagToAction` catalog and offered as a candidate operator. Returns `FCk_Handle_Goap_Action`. |
| | `PromoteActionToPlanner(Action, PlannerParams)` | Stamps Planner fragments onto an existing Action entity. After promotion, `AddAction` on the promoted handle adds direct tree children. Returns `FCk_Handle_Goap_Planner`. |
| **Query** | `Has(Handle)` | True if the entity has the Planner role. |
| | `Find_Action(Planner, Tag)` | Catalog lookup by class-derived tag. |
| | `Find_ActionByClass(Planner, Class)` | Catalog lookup by EntityScript class. |
| | `Get_ActiveChain(Planner)` | Walks Plan[0] links from this Planner downward; returns ordered handles. |
| | `Get_EnableToggle(Planner)` | Current enable/disable state. |
| | `Get_DependencyCycles(Planner)` | Setup-time cycle diagnostics (Tarjan SCC). |
| | `Get_PlanStatus(Planner)` | `ECk_GoapPlanStatus`: Idle / Planning / PlanFound / PlanFailed / CostThresholdReached. |
| | `Get_Plan(Planner)` | Ordered child Action **entity handles** from the last plan. |
| | `Get_PlanClasses(Planner)` | Same as `Get_Plan` but mapped to EntityScript classes. |
| | `Get_PlanCost(Planner)` | Cost of the last plan. |
| | `Get_PlanAttemptCount(Planner)` | Replan attempt counter. |
| | `Get_WorldStateSource(Planner)` | The resolved WS handle this Planner consumes. |
| | `Get_InvalidGoal(Planner)` | Goal conditions referencing unregistered WS keys (populated at Setup time). |
| **Requests** | `Request_SetEnableToggle(Planner, Toggle)` | Enable or disable; disabled Planners skip planning and activation. |
| | `Request_ResetActiveChain(Planner)` | Collapses active chain; fires `OnPlannerDeactivated` per removed node. |
| | `Request_SetGoal(Planner, Goal)` | Set this Planner's goal. Triggers a replan. |
| | `Request_Plan(Planner)` | Force an immediate replan. |
| | `Request_CancelPlan(Planner)` | Abort in-flight A* search. |
| | `Request_SetReplanInterval(Planner, Seconds)` | Throttle replans. |
| | `Request_SetReplanPolicy(Planner, Policy)` | Change the replan trigger. |
| | `Request_SetSearchBudget(Planner, Microseconds)` | Time slice for A* search. |
| | `Request_SetCostThreshold(Planner, Threshold)` | Early-out when best frontier FScore exceeds threshold. |
| | `Request_SetChildActionCost(Planner, ChildClass, Cost)` | Adjust a child Action's cost. |
| | `Request_RemoveAction(Planner, ChildClass)` | Runtime catalog mutation: remove a previously-registered child Action; replans afterward. |
| **Signals** | `BindTo_OnActiveChainChanged(Planner, Delegate, ...)` | Fires whenever the chain mutates. |
| | `BindTo_OnPlanComplete(Planner, Delegate, ...)` | Fires when `PlanFound`; payload = plan entities + cost. |
| | `BindTo_OnPlanFailed(Planner, Delegate, ...)` | Fires when planning cannot satisfy the goal. |
| | `BindTo_OnPlannerActivated(Planner, Delegate, ...)` | Fires when this Planner is activated by a parent selecting it as Plan[0]. Source handle is `FCk_Handle_Goap_Planner`. |
| | `BindTo_OnPlannerDeactivated(Planner, Delegate, ...)` | Fires when this Planner is deactivated. |
| | Unbind counterparts | `UnbindFrom_On*` for each signal above. |

### `UCk_Utils_Goap_Action_UE`

The Action utility class is intentionally thin. Most planner-tier verbs (Plan / CancelPlan / SetReplanPolicy / SetSearchBudget / SetCostThreshold / SetReplanInterval) now live on `UCk_Utils_Goap_Planner_UE`. The Action-facing verbs that remain here either query Action-role state directly or operate per-Action and **delegate to the owning Planner** internally.

| Group | Function | Notes |
|---|---|---|
| **Query** | `Has(Handle)` | True if the entity has the Action role. |
| | `Get_PlanStatus(Action)` | Reads through to the **owning Planner**. Any Action-side `PlanState` stamp left over from the old dual-stamp model is not authoritative. |
| | `Get_Plan(Action)` | Plan **classes**, read from the owning Planner. |
| | `Get_PlanCost(Action)` | Cost of the owning Planner's last plan. |
| | `Get_WorldStateSource(Action)` | The resolved WS handle this Action consumes (`FFragment_Goap_Planner_WorldStateSource._Resolved`). |
| | `Get_ActiveParentAction(Action)` | Class of the parent Action that injected the current goal; null for top-level / dormant. |
| | `Get_InvalidGoal(Action)` | Effects referencing unregistered WS keys (populated at Setup time); read from the owning Planner. |
| | `Get_IsSetupComplete(Action)` | True once `FProcessor_Goap_Action_Setup` has cached this Action's operator def. Wait on this between a runtime `AddAction` and `Request_Plan`. |
| **Requests** | `Request_Plan(Action)` | Delegates to the owning Planner — enqueues a Plan request on its queue. |
| | `Request_CancelPlan(Action)` | Delegates to the owning Planner. |
| | `Request_SetActionCost(Action, ChildClass, Cost)` | Delegates: routes to the owning Planner's `Request_SetChildActionCost`. |
| | `Request_SetReplanInterval(Action, Seconds)` | Delegates to the owning Planner. |
| | `Request_SetReplanPolicy(Action, Policy)` | Delegates to the owning Planner. |
| | `Request_SetSearchBudget(Action, Microseconds)` | Delegates to the owning Planner. |
| | `Request_SetCostThreshold(Action, Threshold)` | Delegates to the owning Planner. |

Signal bindings have moved to the Planner utility. If you held an `FCk_Handle_Goap_Action` to a node that also carries the Planner role, cast it to `FCk_Handle_Goap_Planner` and bind via `UCk_Utils_Goap_Planner_UE::BindTo_OnPlan*`.

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

### World State override stack

Every WS entity carries a stack of named override layers (`FFragment_Goap_WorldState_OverrideStack`). Reads walk the stack top-down — `Get_Value(WS, Key)` returns the value from the topmost layer that contains the key, falling through to the base store (`FFragment_Goap_WorldState_Values`) if no layer covers it. Writes via `Set_Value` always mutate the base; override layers are read-overlay only.

Push and pop fire `FTag_Goap_Planner_Dirty_WorldState` only for keys whose effective view (the topmost winning value) actually changes. A re-push of an existing layer name with identical contents is a no-op — no dirty fire, no replan. A* snapshots the flattened view at seed time in `FProcessor_Goap_Planner_HandleRequests`; the inner search loop never walks the stack.

API on `UCk_Utils_Goap_WorldState_UE`:

| Function | Purpose |
|---|---|
| `Push_Override(WS, Name, Map)` | Push (or replace) a named override layer. |
| `Pop_Override_ByName(WS, Name)` | Pop a named layer. No-op if the name is absent. |
| `Clear_Overrides(WS)` | Remove all override layers; restores base-only reads. |
| `Push_Override_SingleKey(WS, Name, Key, Value)` | Push or update a single key into the named layer; creates the layer on first call. |
| `Get_OverrideDepth(WS)` | Number of layers currently on the stack. |
| `Get_OverrideLayerNames(WS)` | Layer names bottom-to-top. Useful for debugger tooltips. |
| `Has_KeyOverride(WS, Key)` | True if any override layer is currently shadowing this key. |

Layers store **raw `FGameplayTag` keys** and resolve them lazily at read time, so a push never has to be ordered against key registration. Layers are **named** so the debugger's fixed `DebugUI` layer and ad-hoc AI-deliberation scopes coexist independently.

**Multi-Planner reuse:** layers pushed on a shared WS entity affect all Planners subscribed to it — both replan when the effective view changes. This is intentional; document it as a feature when sharing a WS across Planners.

### World State fragment internals

| Fragment | Contents / rationale |
|---|---|
| `FFragment_Goap_WorldState_KeyRegistry` | Exactly one `goap::FKeyRegistry` per WS entity. Every Planner pointing at that WS shares it, so key indices are consistent across the whole layer of Planners observing the state. `FProcessor_Goap_Action_Setup` populates it via `FindOrRegister` while scanning action/goal CDOs. |
| `FFragment_Goap_WorldState_Values` | Wraps `goap::FWorldState` — a `TStaticArray`-backed fixed-size boolean store, sized for O(1) compare/hash. That fixed sizing is where the `WorldState_MaxKeys` = 64 ceiling and the registry-full drop path come from. |
| `FFragment_Goap_WorldState_Subscribers` | `TArray<FCk_Handle>`, not `FCk_Handle_Goap_Action`: stamping `FTag_Goap_Dirty_WorldState` is a generic-handle operation, and non-Action systems may want to react to WS changes. Entries are lazy-pruned — destroyed subscribers drop out as a walk encounters them. Actions subscribe at activation (ActionSet ChainUpdate) and unsubscribe at deactivation; the root Action subscribes at `AddAction` time. |
| `FFragment_Goap_WorldState_ChangeLog` | Bounded ring (Capacity 32, oldest drops first) of *effective*-value changes — base `Set_Value` writes AND override push/pop/clear deltas — each stamped with mutator + frame. Feeds the debugger's timeline WS lane and the Planner's replan-cause attribution. |

`Get_MutableRegistry()` / `Get_MutableValues()` are public **on purpose**: the GOAP Setup processor lives in CkGoap proper, outside these fragments' friend lists, and registers keys on demand; the A* effect-application path mutates values through `FWorldState`'s own typed setters. Neither is a friend. Read-only callers must keep using the const `Get_Registry` / `Get_Values`.

Four WS queries exist primarily for the GOAP debugger, not for gameplay code: `Get_TopOverrideLayerForKey` ("shadowed by \<layer\>" row tooltips), `Get_LayerValues` / `Get_LayerKeyCount` (per-layer drilldown inspector), `Get_RecentChanges` (timeline + replan-cause display), `Get_SubscriberCount` (blast radius — a shared WS replans every subscribed entity when a key flips).

---

## Parent-plan gating

`FTag_Goap_Planner_PlanInFlight` — set on a Planner while its own A* search is in progress. Child Planners whose parent has this tag defer their own Plan requests one frame. Top-level Planners (no parent) are never gated. This prevents child planners from replanning during a parent's search and thrashing the plan state.

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
- Identity is class-derived — no `SetActionTag` call. `UCk_GoapAction_EntityScript::Get_ActionTagForClass` derives it via `UCk_Utils_Object_UE::Get_TagFromClassName`, mirroring `UCk_SmState_EntityScript::Get_StateTagForClass` in `CkStateMachine`.
- **Boolean only.** Project numeric/enum state to boolean tags before use.

To register an Action under a Planner, call `UCk_Utils_Goap_Planner_UE::AddAction(Planner, Params)` where `Params._ActionClass` is the EntityScript subclass.

---

## Fragment table

#### Action-role fragments

| Fragment / tag | Lives on | Contents |
|---|---|---|
| `FFragment_Goap_Action_Definition` | Action entity | CDO-extracted `_Preconditions`, `_Effects`, `_Cost`; `_GoalFromEffects`; `_InvalidGoal`; `_CachedActionDef`. **Discriminator for Action role.** |
| `FFragment_Goap_Action_Params` (`FCk_Fragment_Goap_ActionParamsData`) | Action entity | `_ActionClass`, `_WorldStateSource_Override`, `_SearchBudgetMicroseconds`, `_CostThreshold` (the budget/threshold on ActionParams are vestigial overrides at the Action tier; the authoritative copies live on `FFragment_Goap_Planner_Params` and are what the A* pipeline reads) |
| `FFragment_Goap_Action_Tree` | Action entity | `_ParentAction`, `_ChildActions` |
| `FFragment_Goap_Action_Current` | Action entity | `_ActiveParentAction` (the parent class that injected the current goal) |
| `FTag_Goap_Action_RequiresSetup` | Action entity | One-shot setup gate consumed by `FProcessor_Goap_Action_Setup` |

#### Planner-role fragments

| Fragment / tag | Lives on | Contents |
|---|---|---|
| `FFragment_Goap_Planner_Params` (`FCk_Fragment_Goap_PlannerParamsData`) | Planner entity | `_PlannerTag`, `_InitialToggle`, `_Goal`, `_WorldStateSource`, `_SearchBudgetMicroseconds`, `_CostThreshold`, `_ReplanPolicy`, `_MinReplanIntervalSeconds`, `_PlanOnStart` |
| `FFragment_Goap_Planner_Current` | Planner entity | `_EnableToggle`, `_DependencyCycles` |
| `FFragment_Goap_Planner_Activation` | Planner entity | `_LastActivatedPlan0`, `_IsActive` |
| `FFragment_Goap_Planner_ActionCatalogIndex` | Planner entity | `_TagToAction` map for O(1) tag lookup |
| `FFragment_Goap_Planner_WorldStateSource` | Planner entity (also Action entities) | `_WorldStateSource` (default), `_Resolved` (eager-resolved at activation) |
| `FFragment_Goap_Planner_PlanState` | Planner entity (also Action entities) | `_PlanStatus`, `_Plan` (TArray<FCk_Handle_Goap_Action>), `_PlanCost`, `_PlanAttemptCount` |
| `FFragment_Goap_Planner_Goal` | Planner entity (also Action entities) | `_GoalAuthored`, `_Goal` (resolved), `_InvalidGoal` |
| `FFragment_Goap_Planner_Requests` | Planner entity | `std::variant` request queue over `FCk_Request_Goap_Planner_{Plan,CancelPlan,SetGoal,SetActionCost,SetReplanInterval,SetReplanPolicy,SetSearchBudget,SetCostThreshold}` |
| `FFragment_Goap_Planner_ReplanThrottle` | Planner entity | `_SecondsSinceLastReplan` |
| `FFragment_Goap_Planner_PlanContext` | Planner entity | `_Graph` — `goap::FGoapGraph` kept alive between search and result phases |
| `FFragment_Goap_Planner_SearchState` | Planner entity | A* search state alias `TFragment_AStar_SearchState<int32, goap::FGoapGraph>` |
| `FFragment_Goap_Planner_Result` | Planner entity | A* result alias `TFragment_AStar_Result<int32>` |
| `FTag_Goap_Planner_RequiresSetup` | Planner entity | One-shot setup gate |
| `FTag_Goap_Planner_RequiresChainUpdate` | Planner entity | Set on plan-complete; consumed by `UpdateActivation` |
| `FTag_Goap_Planner_RequiresInitialPlan` | Planner entity | Added when `_PlanOnStart`; drives first plan |
| `FTag_Goap_Planner_PlanRequested` | Planner entity | Request-flow gate |
| `FTag_Goap_Planner_PlanInFlight` | Planner entity | Parent-plan gating; child Planners defer while parent has this tag |

**Actions are lean, with one exception.** `DoCreateOrFindActionEntity` stamps `_Definition`, `_Params`, `_Tree`, `_Current` — plus `FFragment_Goap_Planner_WorldStateSource`, which even an atomic leaf needs because `FProcessor_Goap_Action_Setup` resolves its preconditions/effects against the registry that fragment points at (and `UCk_Utils_Goap_Action_UE::Get_WorldStateSource` reads it). The rest of the Planner-role cluster — `PlanState`, `Goal`, `Activation`, `Requests`, `ReplanThrottle`, `SearchState`, `Result`, `PlanContext`, `AStar_Params`, `AStar_Debug` — arrives only via `PromoteActionToPlanner`'s AddOrGet pass. The Planner-role discriminator fragments are what distinguish a bare Action from a dual-role entity.

`FFragment_Goap_Action_Current` is **residual** Action-role state: `_Plan` / `_PlanCost` / `_PlanStatus` / `_PlanAttemptCount` moved to `FFragment_Goap_Planner_PlanState`, `_Goal` / `_InvalidGoal` to `FFragment_Goap_Planner_Goal`, and the resolved WS source to `FFragment_Goap_Planner_WorldStateSource._Resolved`. Only `_ActiveParentAction` remains.

---

## Design tenets

### Every Planner must always produce a valid plan

**`PlanFailed` is a misconfiguration in a game, never a normal operational state.** Every Planner you author for a real game must have a fallback Action that guarantees `PlanFound` from every world-state. The recommended pattern is a no-precondition, very-high-cost Action whose effect satisfies the Planner's goal:

```cpp
class UCk_<Feature>_Idle : public UCk_GoapAction_EntityScript
{
    virtual auto DefineAction() -> void override
    {
        // No preconditions — always selectable.
        AddEffect(<the Planner's goal key>, true);
        SetCost(999.0);  // Very high — only wins when nothing else is viable.
    }
};
```

Real-game examples: a combat NPC's `WaitForEnemy` (satisfies `EnemyNeutralized` by attrition / time-out), a worker AI's `Idle` (satisfies `TaskComplete` by waiting for the next assignment), a patrol bot's `StandWatch` (satisfies `AreaPatrolled` by holding position).

**The framework enforces this tenet.** Two checks fire automatically:

1. **Setup-time static check (`FProcessor_Goap_Planner_Setup`)**: walks the Planner's catalog and asserts at least one Action has empty preconditions AND effects covering every goal condition. If no fallback exists, fires `CK_ENSURE_IF_NOT` with a clear message + suggested fix. Result is cached on `FFragment_Goap_Planner_Current._HasUnconditionalFallback`.
2. **Runtime check on `PlanFailed`**: when `FProcessor_Goap_Planner_HandleResult` (or `HandleRequests` on the WS-unresolved path) would set status to `PlanFailed`, fires `CK_ENSURE_IF_NOT` unless `_HasUnconditionalFallback || _AllowPlanFailed`. Belt-and-suspenders: catches cases the static check might miss.

**Opt-out via `FCk_Fragment_Goap_PlannerParamsData._AllowPlanFailed = true`**. Only for framework tests, research catalogs, or gym stations that intentionally demonstrate `PlanFailed` (e.g. `CkAutoTest_Goap_Planner_InvalidGoal` — exercises the unregistered-key diagnostic; `CkGoapGym_MakeTea_Station` — demos PlanFailed when ingredients are missing). **Game-content Planners must never set this true.** The flag is greppable in code review.

**`CostThresholdReached` is exempt from the runtime ensure.** It's a deliberate budget-cap signal (the user explicitly set a CostThreshold and the planner respected it), not a catalog misconfiguration. Consumers can still react via `OnPlanFailed`.

**Fallback cost picks itself.** Any cost much higher than the cheapest real Action wins automatically when no other plan is viable. `999.0` is the convention used in `CkGoapFEARGym_Actions::UCk_GoapFEARGym_WaitForEnemy` — the canonical example. Pick anything well above your real-Action cost ceiling.

**Why gameplay code should also check `PlanFailed`:** even with the framework ensure, shipping builds may suppress ensures. Plan-consuming code (state-machine drivers, behaviour trees, animation graphs) should defensively `CK_ENSURE_IF_NOT (status != PlanFailed)` to catch issues in builds that ship with ensure suppression. The framework ensure is the first line; consumer ensure is the second.

---

## Anti-patterns

- **Letting a game-authored Planner reach `PlanFailed`.** See *Design tenets / Every Planner must always produce a valid plan*. The framework fires `CK_ENSURE_IF_NOT` at Setup (no fallback found) and at runtime (PlanFailed actually reached) when `_AllowPlanFailed=false`. Game-content Planners must include a fallback Action (e.g. `WaitForEnemy`, `StandWatch`, `Idle`) and never set `_AllowPlanFailed=true`.
- **Calling `Add` on an owner that already has standalone `CkAStar`.** GOAP stamps `FFragment_AStar_Params` per Planner; the two collide. Use `Create` (child entity) or remove the standalone AStar feature.
- **Expecting a WS key to influence a plan when no Action references it.** `Set_Value` lazily registers any tag (`FindOrRegister`) and stores it — writes are *not* silent no-ops, and the registry is *not* sealed after Setup. The only drop conditions are the per-WS registry already holding `WorldState_MaxKeys` (64) keys, or an invalid tag — both logged at `Verbose`, not dropped silently. The real caveat: a stored value only changes a plan if some Action precondition/effect (or the Planner goal) references the key. Reference the key in at least one Action to make it plan-relevant; registration itself is automatic.
- **Trying to make a leaf Action also plan.** Leaf Actions have no children registered, so there is nothing to plan over. If you want a leaf to plan, `PromoteActionToPlanner` it first, then `AddAction(PromotedPlanner, ...)` to register children under the promoted host.
- **Expecting goal = effects on a composite.** This rule no longer exists. Set `_Goal` on `FCk_Fragment_Goap_PlannerParamsData` independently of the effects the Action-role declares.
- **Setting `_PlanOnStart = true` on a sub-Planner that should only plan when activated.** Eager planning fires before the first activation. Set `_PlanOnStart = false` to get "plan only when activated" semantics.
- **Calling `Request_ResetActiveChain` and expecting the chain to stay collapsed.** `UpdateActivation` re-extends the chain on the next frame if the Planner's plan still has a composite Plan[0]. Disable the Planner first via `Request_SetEnableToggle(Planner, Disable)`.
- **Reading `Get_Plan()` while `Get_PlanStatus() == Planning`.** The plan is only populated after `Planner_HandleResult` runs. Wait for `OnPlanComplete` or poll status.
- **Skipping `CK_REGISTER_PROCESSOR` when adding a new GOAP processor.** An unregistered processor compiles silently and is never scheduled.
- **Writing to override layers.** `Set_Value(WS, Key, NewValue)` always mutates the base store, never an override layer. There is no API to write into a layer directly. To express a transient "what if" mutation push an override layer; for a permanent change write to the base. Trying to use override layers as a write target produces the wrong semantics — the base store will be stale relative to the layer until the layer is popped.
- **Numeric world state.** Classical boolean GOAP only. Project to booleans (`HasEnoughX`, `IsAtY`, `IsLowZ`).
- **Calling `Request_Plan` immediately after a runtime `AddAction`.** The new child Action's `_CachedActionDef` (Preconditions/Effects/Cost extracted from the CDO) is populated by `FProcessor_Goap_Action_Setup` on the next group tick, and the Planner's catalog rebuild runs in `FProcessor_Goap_Planner_Setup` after that. A `Request_Plan` issued in the same frame sees a default-constructed candidate (zero cost, empty effects) and the planner silently sticks with the pre-existing operator set. Symptom: tests that mutate the operator catalog at runtime appear to ignore the new Action. Fix: poll `UCk_Utils_Goap_Action_UE::Get_IsSetupComplete(NewChild)` and issue the `Request_Plan` once it returns true. (An earlier revision of this doc referenced an `OnGoapAction_SetupComplete` signal that was never implemented; the query is the real mechanism.)

---

## Implementation notes

### The regressive search graph (`goap::FGoapGraph`)

`Algorithm/CkGoap_Graph.h` — the planning graph adapter satisfying `AStarGraph<FGoapGraph, int32>`.

- **Nodes** are `FConstraintSet`s (the set of typed constraints a predecessor state must satisfy). `NodeId = int32 = index into _StatePool`.
- **Edges** are actions applied in reverse: an action's effects resolve constraints; its preconditions become new constraints on the predecessor.
- **Direction is BACKWARD.** Start = the goal's conditions as an `FConstraintSet` (index 0); Goal = "every constraint satisfied by the current world state".
- Plan extraction reverses the edge actions along the A* path to get execution order.

**Encodings.** WorldState: `uint8{0 = false, 1 = true}`, initially all-zero. ConstraintSet: `uint8{0 = unconstrained, 1 = must-be-false, 2 = must-be-true}` (mirrors `ck::goap::EConstraint`). The must-be-false/must-be-true split is distinct from "key set vs unset" — WorldState treats unset as false (classical GOAP), but a constraint must be able to say *I don't care about this key*. Values are bools only; project numeric economies, distances and enum machines down before writing:

```cpp
utils_goap_world_state::Set_Value(WS, Tag_HasEnoughFood, ActualFood >= Threshold);
```

### Cycle detection (`FProcessor_Goap_Planner_Setup`)

Each Planner runs an **iterative** Tarjan SCC over its *direct children* — a promoted mid-tier Planner's own `_ChildActions`, or a top-level Planner's `ActionCatalogIndex` entries. The recursive textbook formulation is deliberately avoided: a deep Action catalog would consume the native call stack. The work-stack form carries an explicit `FFrame{Node, ChildIdx}` to resume each node's child iteration.

**Edges are precondition/effect, not tree edges.** For sibling Actions A and B, an effect `(Key,Value)` of A matching a precondition `(Key,Value)` of B adds `A -> B` ("B depends on A"). A tree-edge model would be a no-op — a tree has no cycles by construction. A non-trivial SCC (size > 1, or a self-loop) means those candidate operators mutually require each other's effects; it is recorded in `FFragment_Goap_Planner_Current._DependencyCycles` (with the union of participating WS keys) as a **diagnostic only** — the planner does not refuse a cyclic catalog; designers fix them via the debugger surface.

### Catalog mutation goes through the index mutator

`Request_RemoveAction` uses `ActionCatalogIndex::RemoveEntry`, not a direct `_TagToAction` write. `UCk_Utils_Goap_Planner_UE` is a friend of the fragment so the direct write would compile, but the mutator keeps symmetry with `AddEntry` and leaves room for removal side effects later. (Fragment friendship is class-scoped and does not reach namespace-level free helpers — those get access via explicit friend declarations, e.g. `DoResolveChildWorldStateFromParent` on `FFragment_Goap_Planner_WorldStateSource`.)

---

## See also

- `CkAStar/CLAUDE.md` — underlying time-sliced A* (search budgets, search-state lifecycle, parallel execute).
- `CkEqs/CLAUDE.md` — natural pairing for "where" decisions feeding "what" decisions.
- `CkStateMachine/CLAUDE.md` — plan execution / orchestration.
- `CkEntityScript/CLAUDE.md` (via `CkEcs`) — base class semantics for the Action EntityScript.
- Design spec: `docs/superpowers/specs/2026-05-21-CkGoap-PlannerActionCollapse-design.md` — full data model, processor pseudocode, lifecycle invariants, migration notes.
