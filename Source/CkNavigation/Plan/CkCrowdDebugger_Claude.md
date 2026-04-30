# CkCrowdDebugger

> ⚠️ **NOTE:** This file is the planning placeholder. The real `Claude.md` lives at
> `Plugins/CkGameplayDebugger/Source/CkCrowdDebugger/Claude.md` once that module is created in
> [Gate 0](Gate_00_Foundation.md). After the module's `Claude.md` is committed, **delete this
> file** — it exists only because the module folder doesn't exist yet at planning time.

> **Module status:** ⏳ Not yet created — first lands in [Gate 0](Gate_00_Foundation.md), grows through [Gate 6](Gate_06_StressTuning.md). This file describes the *target* shape.

**Purpose:** Real-time diagnostic UI for the new navigation + crowd stack. Replaces the wiped CkNavDebugger module. MVVM Slate window, console-toggled, mirrors the CkGoapDebugger architecture.

**Depends on:** `CkNavigation`, `CkCrowd`, `CkPmg` (for in-world overlays), `CkDebuggerCommon`, plus Slate / WorkspaceMenuStructure / EditorStyle / AppFramework / ToolMenus.

**Plugin location:** `Plugins/CkGameplayDebugger/Source/CkCrowdDebugger/`. Sibling to `CkGoapDebugger`.

---

## How to open

```
ck.CrowdDebugger 1   // open
ck.CrowdDebugger 0   // close
ck.CrowdDebugger     // toggle (with no arg)
```

`FAutoConsoleCommand` registered in `CkCrowdDebugger_Module.cpp`. The window is a nomad tab via `FGlobalTabmanager::TryInvokeTab`.

The full UX contract is in [Plan/Debugger_Mockup/](Debugger_Mockup/index.html). When this module's real Claude.md is written, those mockup pages should be screenshotted (or rebuilt) at the right places — until ship, the HTML mockup is canonical.

---

## Architecture (mirrors CkGoapDebugger)

```
ck.CrowdDebugger 1
       │
       ▼
FCkCrowdDebuggerModule::OpenDebugger()
       │
       ▼
FGlobalTabmanager::TryInvokeTab → SCkCrowdDebuggerWindow
       │
       │  owns
       ▼
FCkCrowdDebugger_ViewModel  (plain C++, not UObject)
       │
       │  owns
       ▼
FCkCrowdDebugger_DataCollector  (plain C++, not UObject)
       │  per tick
       │  Collect(UWorld*)
       ▼
ECS world reads:
  - Navmesh status (UNavigationSystemV1)
  - Per-agent state (FCk_Handle_CrowdAgent + fragments via DataCollector helper)
  - Per-event log entries (signals' history buffers, capped)
```

ViewModel exposes 5 multicast delegates:

```
TMulticastDelegate<void()> OnAgentListChanged;       // entity added/removed
TMulticastDelegate<void()> OnAgentDataRefreshed;     // selected agent's data changed
TMulticastDelegate<void()> OnSelectedAgentChanged;
TMulticastDelegate<void()> OnPausedChanged;
TMulticastDelegate<void()> OnViewModeChanged;
```

Each panel binds to the relevant delegates in its `Construct`, rebuilds on invalidation (dirty-bit hash check in Tick).

---

## Panels

| Panel | Mockup ref | Purpose |
|---|---|---|
| `SCkCrowdDebugger_NavmeshStatusPanel` | [§1 top-left](Debugger_Mockup/01_main.html) / [§4](Debugger_Mockup/04_health_fail.html) | NavSystem / NavData / Filter / Bounds rows + Health Check button + last-run summary |
| `SCkCrowdDebugger_AgentListPanel` | [§1 left](Debugger_Mockup/01_main.html) / [§3](Debugger_Mockup/03_filter.html) | Live agent list with filter + status badges. Click to select. |
| `SCkCrowdDebugger_AgentDetailPanel` | [§1 center](Debugger_Mockup/01_main.html) / [§2](Debugger_Mockup/02_idle.html) | Identity, Transform, Goal & Path, Steering Forces, Neighbors, Sleep & Replan |
| `SCkCrowdDebugger_StatsPanel` | [§1 right](Debugger_Mockup/01_main.html) | Total / Awake / Asleep / Replanning / Failed counts + frame-timing breakdown + sparkline |
| `SCkCrowdDebugger_EventLogPanel` | [§1 right bottom](Debugger_Mockup/01_main.html) | Reverse-chronological event log: Path / Sleep / Pierce / Proxy categories |

All panels live in `Public/CkCrowdDebugger/Window/`. Each is a `SCompoundWidget` taking a `TSharedPtr<FCkCrowdDebugger_ViewModel>` as argument.

---

## Selection model

Selection lives in the ViewModel, **not as a tag on the agent entity** (the wiped module did the latter; experience showed it added implicit fragment churn for every panel click).

```cpp
class FCkCrowdDebugger_ViewModel
{
public:
    void Set_SelectedHandle(FCk_Handle_CrowdAgent InHandle);
    auto Get_SelectedHandle() const -> FCk_Handle_CrowdAgent;
    auto Get_SelectedSnapshot() const -> const FCkCrowdDebugger_AgentSnapshot*;
    // ...
};
```

When the AgentListPanel's row is clicked, it calls `ViewModel->Set_SelectedHandle(handle)`, which fires `OnSelectedAgentChanged`, which causes AgentDetailPanel to rebuild from `Get_SelectedSnapshot()`.

The selected agent gets a special render in PMG: brighter / orange-tinted capsule + path. That's done by a small CkCrowdDebugger processor that watches the ViewModel's selected handle and writes a tag-fragment toggle on the matching agent's path-viz child entity. Single agent at a time.

---

## DataCollector

`FCkCrowdDebugger_DataCollector` is a plain C++ class. Per-tick `Collect(UWorld*)`:

1. Iterate all entities with `FFragment_CrowdAgent_Params` (the cheap-to-find marker).
2. For each, build a `FCkCrowdDebugger_AgentSnapshot` from the various fragments.
3. Detect list change (count or set membership delta), set dirty bit.
4. For the selected agent, refresh detailed snapshot.
5. Sample navmesh status (UNavigationSystemV1, NavData, etc.) once per tick.
6. Drain event signal buffers into a capped event log (200 entries max, ring-buffer).

`AgentSnapshot` shape:

```cpp
struct FCkCrowdDebugger_AgentSnapshot
{
    FCk_Handle_CrowdAgent Handle;
    FString OwnerName;
    FGameplayTagContainer Tags;
    FVector Position, Velocity, DesiredVelocity, Goal;
    FVector PathFollowForce, SeparationForce;
    ECk_CrowdAgent_Status Status;
    int32 WaypointIndex, WaypointCount;
    TArray<FCkCrowdDebugger_NeighborSnapshot> Neighbors;
    bool IsAsleep, IsPiercing, IsReplanning;
    float IdleSeconds;
    int32 ReplanCount;
    // ...
};
```

Snapshots are copies of read-only data. The DataCollector never holds references into the live ECS world.

---

## Anti-patterns

- **Don't bind Slate widgets to FCk_Handle_CrowdAgent directly.** Bind to ViewModel + DataCollector snapshots. The handle's underlying entity may be destroyed any frame.
- **Don't write to ECS state from the debugger.** It's read-only by design. The single exception is the "selected" PMG render toggle.
- **Don't add a new panel without updating the mockup first.** The mockup is the contract.
- **Don't sample inside Slate paint callbacks.** Sample once per ViewModel tick, propagate through delegates, repaint via Slate's built-in invalidation.

---

## Limitations / known issues

- **No scrub mode** in v1. The ViewModel has the `_ViewMode` field reserved (Live / Scrub) but only Live is implemented. Adding scrub means snapshotting the ECS each frame into a ring buffer — expensive at 100+ agents. Do post-ship if needed.
- **Editor-only-ish.** The window opens in PIE; opening in a packaged build works but the WorkspaceMenuStructure dep flags this as editor-friendly. Ship behavior in a packaged build will be log-only.

## Future work

- Scrub mode.
- Per-agent timeline (replan history visualization).
- Heatmap of neighbor-density across the navmesh.
- Saved snapshot diffs (compare current state to a saved baseline).

## See also

- [CkNavigation/Claude.md](../Claude.md) — path query layer
- [CkCrowd/Claude.md](../../CkCrowd/Claude.md) — agent + steering
- [CkGoapDebugger/Claude.md](../../../../CkGameplayDebugger/Source/CkGoapDebugger/Claude.md) — sibling debugger; same MVVM patterns
- [Debugger_Mockup/](Debugger_Mockup/index.html) (delete post-ship)
