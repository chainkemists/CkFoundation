# Gate 1 — Pathfinding

> **Status:** ⏳ Pending
> **Day target:** D2
> **Parallelizable:** No (CkNavigation API surfaces feed Gate 2)
> **Depends on:** Gate 0 ✅

## Goal

Stand up the slim `CkNavigation` module: Recast/UE NavigationSystem wrapper, `FindPathSync`,
project settings, request/result fragments, and the **debugger's Navmesh Status panel + Health
Check button** ([mockup §1 top-left](Debugger_Mockup/01_main.html), [mockup §4 fail state](Debugger_Mockup/04_health_fail.html)).

After this gate: any entity (CrowdAgent or otherwise) can call `Request_FindPath(target)` and
get a `FCk_Nav_PathResult` populated with waypoints + status. The debugger shows navmesh
health and any path failures. **No movement yet** — paths are computed, not followed.

## Acceptance criteria

1. ✅ `UCk_Utils_Nav_UE::Request_FindPath(InHandle, FCk_Request_Nav_FindPath{TargetLocation})` compiles, runs, returns success on a known-good navmesh.
2. ✅ Path result is stored as a fragment on the requester entity. `Get_PathResult(InHandle)` returns a const ref to `FCk_Nav_PathResult`.
3. ✅ Per-request completion delegate fires (`BindTo_OnPathReady` / `BindTo_OnPathFailed`).
4. ✅ Debugger Navmesh Status panel matches [mockup §1](Debugger_Mockup/01_main.html): NavSystem / NavData / Filter / Bounds / SupportedAgents / Last Regen rows, all populated.
5. ✅ Debugger Health Check button runs a synthetic `FindPathSync` from `(0,0,50)` to `(200,0,50)`, displays Pass / Fail with hint text matching [mockup §4](Debugger_Mockup/04_health_fail.html).
6. ✅ Path visualization rides via PMG: each agent with a current path has a debug shape entity that draws line segments through the waypoints. Color: cyan for healthy, yellow for replanning (Gate 4), red for failed.
7. ✅ AutoStation `UCk_AutoTest_Crowd_Pathfinding` runs: spawn 3 agents, kick off 3 path queries to known reachable points, assert all 3 return `Status == Ready` within 1 second, assert their waypoint counts are >= 2.
8. ✅ AutoStation `UCk_AutoTest_Crowd_PathfindingFailure` runs: spawn 1 agent, request path to a point outside navmesh bounds, assert `Status == Failed` and `FailReason == EndProjectFailed`.

## File inventory

### `Plugins/CkFoundation/Source/CkNavigation/Public/CkNavigation/`

```
Nav/
    CkNav_Algorithm.{h,cpp}        # FCk_Nav_Algorithm — ToRecastFloat3, FromRecastFloat3, FindPathSync, ExtractWaypoints
    CkNav_Fragment.{h,cpp}         # FFragment_Nav_PathResult alias, request fragment, FTag_Nav_QueryDirty
    CkNav_Fragment_Data.{h,cpp}    # USTRUCTs:
                                   #   FCk_Nav_PathResult
                                   #   FCk_Nav_PathDiagnostics
                                   #   FCk_Nav_PathStatus enum
                                   #   FCk_Nav_PathFailReason enum (CLEANED — no dtCrowd-era entries)
                                   #   FCk_Request_Nav_FindPath
    CkNav_Processor.{h,cpp}        # FProcessor_Nav_HandleRequests — drains path requests, calls FindPathSync, populates fragment, fires signal
    ProcessorInjector/
        CkNav_ProcessorInjector.cpp
Settings/
    CkNav_ProjectSettings.{h,cpp}  # _MaxPathQueriesPerFrame (default 8), _NavQuerySearchHalfExtent (default 500cm)
Utils/
    CkNav_Utils.{h,cpp}            # UCk_Utils_Nav_UE — Request_FindPath, BindTo_OnPathReady, BindTo_OnPathFailed,
                                   #                   Get_PathResult, Get_PathStatus, Has_Path
```

### Cleaned `ECk_Nav_PathFailReason` enum

```cpp
UENUM(BlueprintType)
enum class ECk_Nav_PathFailReason : uint8
{
    None,
    NoNavSystem,
    NoNavData,
    NoDefaultFilter,
    StartProjectFailed,
    EndProjectFailed,
    FindPathError,
    FindPathNoPath,
    FindPathInvalid,
    EmptyPath,
    NotAuthority,         // server-only requests dropped on client
    BudgetExceeded,       // _MaxPathQueriesPerFrame hit; request will retry next frame
};
```

Removed from the dtCrowd era: `BudgetDisabled`. (Now we keep the budget enabled by default at 8/frame; setting to 0 disables but no agent currently expects 0, so the failure mode is just "rejected, retry.")

### Path visualization (lives in `CkCrowd`, not `CkNavigation`)

Per-agent path visualization is a CkCrowd concern (since path drawing is *for* an agent, not for any nav-querying entity). Adds:

```
Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/
    CkCrowdAgent_PathViz.{h,cpp}   # FProcessor_CrowdAgent_PathViz_Sync — syncs PMG line shape to the path waypoints
                                   # Owned PMG entity referenced as a child via Record
```

The viz spawns a child PMG line-strip entity per agent the first time the agent gets a path; updates on path-changed; destroys with the agent.

### `Plugins/CkGameplayDebugger/Source/CkCrowdDebugger/`

```
Public/CkCrowdDebugger/Data/
    CkCrowdDebugger_HealthCheck.{h,cpp}      # Synthetic FindPath probe — runs on demand
    (DataCollector.cpp gains the navmesh-status sample)
Public/CkCrowdDebugger/Window/
    (NavmeshStatusPanel.cpp populated)
    (Toolbar Health Check button wired)
```

DataCollector sampling for navmesh status:

```cpp
struct FCkCrowdDebugger_NavmeshStatus
{
    bool _NavSystemPresent = false;
    FString _NavDataClassName;
    bool _DefaultFilterValid = false;
    FBox2D _Bounds;
    int32 _SupportedAgents = 0;
    double _LastRegenTimeStamp = -1.0;
};
```

## Gym spec — manual

`Crowd Pathfinding` gym:

- One alcove tagged `Gym.Crowd.Pathfinding.Source` (left side of map)
- Three target alcoves tagged `Gym.Crowd.Pathfinding.Target_A/B/C` placed across the map
- Press **Space** → spawn agent at Source, queue paths to A then B then C in sequence
- Live display on each station shows path status

Visual:

- Cyan path lines from agent to current target
- Each waypoint shown as a small PMG sphere
- Agents don't move yet (no Gate 2) — paths are visible but agents stay put

## Gym spec — AutoStation

Two AutoStations live in the same gym:

`UCk_AutoTest_Crowd_Pathfinding_Success`:
- Spawn 3 agents at known good positions
- Each requests path to a different reachable target
- After 1.0s timer, assert all 3 paths are `Ready`, each has >= 2 waypoints
- `FinishSuccess()`

`UCk_AutoTest_Crowd_Pathfinding_Failure`:
- Spawn 1 agent
- Request path to `(99999, 99999, 99999)` (off-mesh)
- After 0.5s timer, assert path is `Failed` with `EndProjectFailed`
- `FinishSuccess()`

## Debugger additions (per [mockup §1](Debugger_Mockup/01_main.html) and [§4](Debugger_Mockup/04_health_fail.html))

| Panel | Gate 1 contribution |
|---|---|
| Toolbar | Health Check button now functional. Last health check status badge appears in the Navmesh Status panel. |
| Navmesh Status | Fully populated. NavSystem / NavData / Filter / Bounds / SupportedAgents / Last Regen rows. Last health check inline summary. |
| Agent List | New columns/badges: Status (`Walking` is misleading without locomotion — for G1 the badge reads `PathReady` or `Pending` or `Failed`). |
| Agent Detail | Goal & Path section now populated: Goal coords, Distance, Path Status, Waypoints n/m, Last Query (ms + raw point count + extracted count), Fail Reason. |
| Stats | New field: `Path requests` (count this frame). |
| Event Log | New categories: `PATH READY` (green), `PATH FAILED` (red). One row per query result. |

## Risks / unknowns

| Risk | Likelihood | Mitigation |
|---|---|---|
| `FindPathSync` returns success but with degenerate (1-point) path | Medium | `ExtractWaypoints` rejects paths with < 2 points; surfaces as `EmptyPath` failure. |
| Navmesh not yet built at gym start (debounce window) | Medium | Auto-retry `Request_FindPath` once on `NoNavData` or `NoDefaultFilter`. Surface as `Pending` not `Failed` until the second attempt. |
| Per-frame budget (`_MaxPathQueriesPerFrame`) starves an agent | Low | Round-robin queue inside `FProcessor_Nav_HandleRequests`. Tested in Gate 6 stress. |
| Health Check synthetic probe collides with a real agent's request budget | Low | Health Check uses a dedicated path query that bypasses the per-frame budget; flagged separately in stats. |

## Done criteria checklist

- [ ] All public API in `UCk_Utils_Nav_UE` is BP-callable, AS-bindable.
- [ ] Path failure enum has no dtCrowd-era entries.
- [ ] PMG path viz works in PIE: cyan lines, sphere waypoints, destroyed with agent.
- [ ] Health Check button works in idle and broken-navmesh setups (mockup §1 vs §4 visual states).
- [ ] AutoTest_Pathfinding_Success + AutoTest_Pathfinding_Failure both pass.
- [ ] Debugger Navmesh Status panel matches mockup §1 fields exactly.
- [ ] PLAN.md status row updated to ✅ Done.
- [ ] CkNavigation/Claude.md written (final-state guide for the slim module).
- [ ] CkCrowd/Claude.md updated to mention the path-viz child entity pattern.
