# CkStateMachine Viewer — Implementation Plan

## Overview

A **read-only, real-time visual viewer** for CkStateMachine instances. Renders inside an Unreal Slate window using ImGui (via `D:\Repos\Venus\Plugins\ImGui`). No graph mutation — users build state machines in AngelScript/Blueprints/C++ and watch the viewer update live.

**Philosophy**: Unlike traditional UE state machine editors that couple editing and viewing, this is a pure **observation tool**. The graph is derived entirely from ECS entity data at runtime.

---

## Architecture

```
┌─ SWindow (Slate) ─────────────────────────────────┐
│  ┌─ SImGuiOverlay ──────────────────────────────┐  │
│  │  ┌─ FImGuiContext ────────────────────────┐   │  │
│  │  │                                         │   │  │
│  │  │  ┌─ Left Panel ─┐  ┌─ Graph Canvas ──┐ │   │  │
│  │  │  │ SM Selector   │  │                  │ │   │  │
│  │  │  │ SM Info       │  │  [Idle] ──────►  │ │   │  │
│  │  │  │ State Details │  │  [Patrol] ────►  │ │   │  │
│  │  │  │ Condition     │  │  [Alert] ──────► │ │   │  │
│  │  │  │   Details     │  │                  │ │   │  │
│  │  │  └───────────────┘  └──────────────────┘ │   │  │
│  │  │                                         │   │  │
│  │  └─────────────────────────────────────────┘   │  │
│  └────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────┘
```

### Module: `CkStateMachineViewer` (new module)

**Location**: `D:\Repos\Venus\Plugins\CkFoundation\Source\CkStateMachineViewer\`

**Dependencies**: `CkStateMachine`, `CkEcs`, `CkCore`, `ImGui`, `Slate`, `SlateCore`, `Engine`

**Rationale for separate module**: Keeps ImGui dependency out of the runtime `CkStateMachine` module. The viewer is a development-only tool.

---

## Data Model (What the viewer reads)

The viewer queries ECS entity hierarchy in real-time. No caching — every frame reads live data.

### Entity Hierarchy Traversal

```
SM Entity (FCk_Handle_StateMachine)
  │ Has: FFragment_Sm_Params, FFragment_Sm_Current, FFragment_Sm_Context
  │ Tags: FTag_Sm_Running, FTag_Sm_Paused, FTag_Sm_TransitionQueued
  │
  ├─► State Entity (FCk_Handle_SmState) — via Get_LifetimeDependents
  │     Has: EntityScript (class name = node label)
  │     │
  │     ├─► Task Entity (FCk_Handle_SmTask)
  │     │     Has: FFragment_SmTask_Current (_LastResult)
  │     │     Tags: FTag_SmTask_Tick or FTag_SmTask_EnterExit
  │     │
  │     └─► Transition Entity (FCk_Handle_SmTransition)
  │           Has: FFragment_SmTransition_Params (_TargetStateClass, _Order)
  │           │
  │           └─► Condition Entity (FCk_Handle_SmCondition)
  │                 Has: FFragment_SmCondition_Current (_IsSatisfied, _ResetBehavior)
  │                 Tags: FTag_SmCondition_Polled or FTag_SmCondition_EventDriven
  │                 EntityScript class name = condition label
```

### Key Query APIs

```cpp
// Hierarchy traversal
UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(Handle)  // children
UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Handle)       // parent

// SM state
UCk_Utils_StateMachine_UE::Get_RunStatus(SmHandle)
UCk_Utils_StateMachine_UE::Get_CurrentStateClass(SmHandle)
UCk_Utils_StateMachine_UE::Get_CurrentStateHandle(SmHandle)
UCk_Utils_StateMachine_UE::IsInState(SmHandle, StateClass)

// Fragment reads (direct ECS access)
Handle.Get<ck::FFragment_Sm_Current>()
Handle.Get<ck::FFragment_SmTransition_Params>()
Handle.Get<ck::FFragment_SmCondition_Current>()
Handle.Has<ck::FTag_Sm_Running>()
// etc.
```

---

## File Structure

```
CkStateMachineViewer/
├── CkStateMachineViewer.Build.cs
└── Public/
    └── CkStateMachineViewer/
        ├── CkSmViewer_Module.h              // Module startup, registers console command
        ├── CkSmViewer_Module.cpp
        ├── CkSmViewer_Window.h              // Slate window + ImGui context owner
        ├── CkSmViewer_Window.cpp
        ├── CkSmViewer_GraphRenderer.h       // Node graph rendering with ImGui
        ├── CkSmViewer_GraphRenderer.cpp
        ├── CkSmViewer_DataCollector.h       // Reads ECS data into view model
        ├── CkSmViewer_DataCollector.cpp
        └── CkSmViewer_Types.h              // View model structs
```

---

## Detailed Component Design

### 1. `CkSmViewer_Types.h` — View Model

Lightweight structs that mirror ECS data for rendering. Rebuilt every frame from live ECS queries.

```cpp
struct FCkSmViewer_ConditionInfo
{
    FCk_Handle Handle;
    FString ClassName;              // Short class name (e.g. "AfterDelay")
    bool IsSatisfied = false;
    ECk_SmConditionMode Mode;       // Polled vs EventDriven
    ECk_SmConditionResetBehavior ResetBehavior;
};

struct FCkSmViewer_TransitionInfo
{
    FCk_Handle Handle;
    int32 Order = 0;
    TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;
    FString TargetStateName;        // Short class name
    TArray<FCkSmViewer_ConditionInfo> Conditions;
    bool AreAllConditionsSatisfied = false;   // Derived: all conditions satisfied
};

struct FCkSmViewer_TaskInfo
{
    FCk_Handle Handle;
    FString ClassName;
    ECk_SmTaskMode Mode;            // Tick vs EnterExitOnly
    ECk_SmTaskResult LastResult;
};

struct FCkSmViewer_StateInfo
{
    FCk_Handle Handle;
    TSubclassOf<UCk_SmState_EntityScript> StateClass;
    FString StateName;              // Short class name
    bool IsCurrentState = false;
    TArray<FCkSmViewer_TransitionInfo> Transitions;   // Sorted by Order
    TArray<FCkSmViewer_TaskInfo> Tasks;
    ImVec2 NodePosition;            // Layout position (calculated)
    ImVec2 NodeSize;                // Layout size (calculated after first render)
};

struct FCkSmViewer_SmInfo
{
    FCk_Handle_StateMachine Handle;
    FCk_Handle GameEntity;          // Owning game entity
    FString DebugName;              // From debug name utils or entity label
    ECk_SmRunStatus RunStatus;
    TSubclassOf<UCk_SmState_EntityScript> InitialStateClass;
    TSubclassOf<UCk_SmState_EntityScript> CurrentStateClass;
    bool IsTransitionQueued = false;
    TArray<FCkSmViewer_StateInfo> States;
};
```

### 2. `CkSmViewer_DataCollector` — ECS → View Model

Reads ECS entities and populates the view model structs every frame.

**Key Responsibilities**:
- Discover all SM entities in the world (iterate entities with `FFragment_Sm_Current`)
- Walk entity hierarchy to build `FCkSmViewer_SmInfo`
- Classify children as State vs non-State, Transition vs Task vs Condition
- Sort transitions by `_Order`
- Compute derived state (e.g. `AreAllConditionsSatisfied`)

**Approach**: Use ECS query to find all entities with `FFragment_Sm_Current`, then traverse via `Get_LifetimeDependents`.

```cpp
class FCkSmViewer_DataCollector
{
public:
    // Rebuild the full view model from the ECS world.
    // Called once per frame.
    auto Collect(UWorld* InWorld) -> void;

    auto Get_AllStateMachines() const -> const TArray<FCkSmViewer_SmInfo>&;

private:
    auto CollectStateMachine(FCk_Handle_StateMachine InSmHandle) -> FCkSmViewer_SmInfo;
    auto CollectState(FCk_Handle InStateHandle, FCk_Handle_StateMachine InSmHandle) -> FCkSmViewer_StateInfo;
    auto CollectTransition(FCk_Handle InTransHandle) -> FCkSmViewer_TransitionInfo;
    auto CollectCondition(FCk_Handle InCondHandle) -> FCkSmViewer_ConditionInfo;
    auto CollectTask(FCk_Handle InTaskHandle) -> FCkSmViewer_TaskInfo;

    TArray<FCkSmViewer_SmInfo> _StateMachines;
};
```

### 3. `CkSmViewer_GraphRenderer` — ImGui Node Graph

Renders the state machine as a visual node graph using ImGui's `ImDrawList` API.

**Visual Design**:

```
┌─────────────────────────┐
│ ● Idle                  │  ← Green dot = current state, Grey = inactive
│   Tasks: [Tick]         │
│                         │
│   → Patrol  [1/1 ✓]    │  ← Transition arrow, condition count
│   → Alert   [0/2 ✗]    │
└─────────────────────────┘
         │
         │ ──── Transition line (green if satisfied, grey if not)
         ▼
┌─────────────────────────┐
│   Patrol                │
│   ...                   │
└─────────────────────────┘
```

**State Node**:
- Rectangle with rounded corners
- Header: state class name (bold)
- Active state: highlighted border (green), pulsing glow
- Paused: amber border
- Body: list of tasks (with mode icon), list of outgoing transitions (with condition summary)
- Clickable to select and show details in side panel

**Transition Lines**:
- Bezier curve from source state to target state
- Color: green if all conditions satisfied, grey otherwise, amber if transition is queued
- Arrow head at target
- Label: order number, condition count summary (e.g. "2/3")
- Clickable to select and show condition details

**Condition Detail** (shown in side panel when transition selected):
- List of conditions with:
  - Class name
  - Mode (Polled/EventDriven) icon/badge
  - Reset behavior (EveryFrame/Manual) icon/badge
  - Satisfied status: green checkmark or red X
  - If Manual reset: shows "latched" indicator when satisfied

**Layout Algorithm**:
- Simple automatic layout: states arranged in a circle or grid
- Current state centered/highlighted
- Auto-layout on first open, positions cached

**Key Implementation Details**:
```cpp
class FCkSmViewer_GraphRenderer
{
public:
    auto Render(
        const FCkSmViewer_SmInfo& InSmInfo,
        FCkSmViewer_Selection& InOutSelection) -> void;

private:
    auto RenderStateNode(
        ImDrawList* InDrawList,
        const FCkSmViewer_StateInfo& InState,
        bool InIsSelected) -> void;

    auto RenderTransitionLine(
        ImDrawList* InDrawList,
        const FCkSmViewer_StateInfo& InSource,
        const FCkSmViewer_TransitionInfo& InTransition,
        const FCkSmViewer_StateInfo& InTarget) -> void;

    auto CalculateLayout(
        TArray<FCkSmViewer_StateInfo>& InOutStates) -> void;

    auto HandleInteraction(
        const FCkSmViewer_SmInfo& InSmInfo,
        FCkSmViewer_Selection& InOutSelection) -> void;

    // Canvas state
    ImVec2 _CanvasOffset = {0, 0};   // Pan offset
    float _CanvasZoom = 1.0f;         // Zoom level
    bool _NeedsRelayout = true;
};
```

### 4. `CkSmViewer_Window` — Slate + ImGui Host

Creates a Slate window, owns the `FImGuiContext`, and orchestrates data collection + rendering.

```cpp
class FCkSmViewer_Window
{
public:
    auto Open(UWorld* InWorld) -> void;
    auto Close() -> void;
    auto Tick(float InDeltaTime) -> void;

private:
    auto RenderImGui() -> void;
    auto RenderSmSelector() -> void;     // Dropdown/list of all SMs in world
    auto RenderSmOverview() -> void;     // Run status, current state, entity info
    auto RenderDetailPanel() -> void;    // Selected node/transition/condition details
    auto RenderGraph() -> void;          // The visual graph

    TSharedPtr<SWindow> _SlateWindow;
    TSharedPtr<FImGuiContext> _ImGuiContext;

    FCkSmViewer_DataCollector _DataCollector;
    FCkSmViewer_GraphRenderer _GraphRenderer;
    FCkSmViewer_Selection _Selection;

    TWeakObjectPtr<UWorld> _World;
    int32 _SelectedSmIndex = 0;
};
```

**Window Layout** (ImGui panels inside the window):
```
┌──── SM Viewer ──────────────────────────────────────┐
│ [SM Selector ▼] [Status: Running ●] [Pause] [Stop]  │
├─────────────────┬───────────────────────────────────┤
│  Detail Panel   │                                   │
│                 │          Graph Canvas              │
│  State: Idle    │                                   │
│  Tasks:         │     ┌───────┐    ┌────────┐      │
│   - IdleAnim    │     │ Idle  │───►│ Patrol │      │
│                 │     └───────┘    └────────┘      │
│  Transition:    │          ▲            │           │
│   → Patrol      │          │            ▼           │
│   Order: 0      │     ┌────────┐                    │
│   Conditions:   │     │ Alert  │                    │
│   ✓ AfterDelay  │     └────────┘                    │
│     Mode: Event │                                   │
│     Reset: Man  │                                   │
├─────────────────┴───────────────────────────────────┤
│  [History: Idle → Patrol → Alert → Idle → ...]       │
└─────────────────────────────────────────────────────┘
```

### 5. `CkSmViewer_Module` — Module + Console Command

Registers the viewer and provides a console command to open it.

```cpp
// Console command: "Ck.SmViewer" opens the window
// Can also be opened programmatically
class FCkSmViewer_Module : public IModuleInterface
{
public:
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;

private:
    auto OpenViewer() -> void;
    TSharedPtr<FCkSmViewer_Window> _Window;
    TSharedPtr<IConsoleObject> _ConsoleCommand;
};
```

---

## Visual Design Details

### Color Palette

| Element | Color | Hex |
|---------|-------|-----|
| Current state border | Green | `#4CAF50` |
| Inactive state border | Grey | `#607D8B` |
| Paused state border | Amber | `#FFC107` |
| Satisfied condition | Green | `#66BB6A` |
| Unsatisfied condition | Red-grey | `#EF5350` |
| Transition line (ready) | Bright green | `#4CAF50` |
| Transition line (not ready) | Dark grey | `#455A64` |
| Queued transition | Amber pulse | `#FF9800` |
| State node background | Dark | `#1E1E2E` |
| State node header | Slightly lighter | `#2D2D3D` |
| Canvas background | Very dark | `#0D0D14` |

### Node Sizing

- State node: ~180px wide, height auto from content
- Task rows: ~20px each
- Transition rows: ~20px each
- Padding: 8px
- Corner radius: 6px
- Border thickness: 2px (current state: 3px)

### Interaction

- **Pan**: Middle mouse drag on canvas
- **Zoom**: Scroll wheel on canvas
- **Select state**: Left click on state node
- **Select transition**: Left click on transition line or transition row in state node
- **Details**: Selected element shows details in left panel
- **Focus**: Double-click state to center it in view

---

## State History Timeline (Bottom Panel)

A horizontal timeline showing state transitions over time:

```
Time ──────────────────────────────────────────────►
│ Idle ████████│ Patrol █████│ Alert ████│ Idle ██ │
```

- Color-coded bars per state
- Timestamps at transition points
- Scrollable/zoomable

**Implementation**: ring buffer of `{StateClass, Timestamp}` entries, rendered as colored rectangles.

---

## Implementation Order

### Phase 1 — Foundation (get something on screen)
1. Create `CkStateMachineViewer` module with `Build.cs`
2. `CkSmViewer_Types.h` — all view model structs
3. `CkSmViewer_Module` — module init + console command
4. `CkSmViewer_Window` — Slate window with ImGui context, basic ImGui::Text output
5. Verify: console command opens window, ImGui renders text

### Phase 2 — Data Collection
6. `CkSmViewer_DataCollector` — collect all SMs, states, transitions, conditions, tasks
7. Wire into window: show SM selector dropdown, display collected data as text
8. Verify: real SM data appears in window

### Phase 3 — Graph Rendering
9. `CkSmViewer_GraphRenderer` — canvas with pan/zoom
10. Render state nodes as rectangles with class names
11. Render transition lines between states (bezier curves + arrows)
12. Auto-layout algorithm (circular or force-directed)
13. Highlight current state, color transition lines by satisfaction

### Phase 4 — Interaction & Details
14. Click-to-select states and transitions
15. Detail panel showing selected element info
16. Condition details when transition selected
17. Task details when state selected

### Phase 5 — Polish
18. State history timeline (bottom panel)
19. Visual polish (colors, icons, glow effects)
20. Edge cases (SM not started, no states, paused, transitioning)
21. Multiple SM support (tab bar or dropdown)

---

## Open Questions / Decisions Made

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Module type | Separate module `CkStateMachineViewer` | Keeps ImGui dep out of runtime |
| UI framework | ImGui inside Slate window | User requirement — Slate host, ImGui rendering |
| Cog framework | **NOT used** (deprecated) | User directive |
| Data approach | Direct ECS query every frame | Simple, no stale data, SM graphs are small |
| Layout | Auto-layout (circle/grid) | Read-only viewer, no manual placement needed |
| Window type | Standalone `SWindow` | Independent of editor, works in PIE |

---

## Dependencies Summary

```
CkStateMachineViewer.Build.cs:
  PublicDependencyModuleNames:
    - Core
    - CoreUObject
    - Engine
    - Slate
    - SlateCore
    - ImGui
    - CkCore
    - CkEcs
    - CkStateMachine
```

---

## Key Reference Files

| File | Purpose |
|------|---------|
| `D:\Repos\Venus\Plugins\ImGui\Source\ImGui\Public\ImGuiModule.h` | `FImGuiModule::CreateWindowContext()` |
| `D:\Repos\Venus\Plugins\ImGui\Source\ImGui\Public\ImGuiContext.h` | `FImGuiContext::Create()`, `BeginFrame()`, `EndFrame()` |
| `D:\Repos\Venus\Plugins\ImGui\Source\ImGui\Private\SImGuiOverlay.h` | Slate widget that renders ImGui |
| `D:\Repos\Venus\Plugins\CkFoundation\Source\CkStateMachine\Public\CkStateMachine\CkStateMachine_Fragment.h` | All SM fragments and tags |
| `D:\Repos\Venus\Plugins\CkFoundation\Source\CkStateMachine\Public\CkStateMachine\CkStateMachine_Fragment_Data.h` | Enums, handles |
| `D:\Repos\Venus\Plugins\CkFoundation\Source\CkStateMachine\Public\CkStateMachine\CkStateMachine_Utils.h` | Query API |
| `D:\Repos\Venus\Plugins\CkFoundation\Source\CkStateMachine\Public\CkStateMachine\CkStateMachine_Processor.h` | Processor declarations |
| `D:\Repos\Venus\Plugins\CkFoundation\Source\CkEcs\Public\CkEcs\EntityLifetime\CkEntityLifetime_Utils.h` | `Get_LifetimeDependents`, `Get_LifetimeOwner` |
