# CkNavigation

> **Module status:** ⏳ Skeleton only — full module landed in [Gate 1](PLAN.md). Until Gate 1 lands, this file describes the *target* shape.

**Purpose:** Thin Recast / `UNavigationSystemV1` wrapper. Asynchronous-from-the-caller's-perspective path queries that drop a result fragment + signal on the requester entity. Nothing else — no agents, no steering, no avoidance. Those live in `CkCrowd`.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`, plus UE `NavigationSystem` / `AIModule` / `GameplayTags`.
**Used by:** `CkCrowd` (the only first-party consumer). Other modules can use it for pathing without depending on CkCrowd.

---

## Public API (BP-callable, AS-bindable)

```cpp
// Request a path. Result is delivered to a fragment on InHandle and via the bound delegate.
static FCk_Handle Request_FindPath(
    UPARAM(ref) FCk_Handle& InHandle,
    const FCk_Request_Nav_FindPath& InRequest,
    const FCk_Delegate_Nav_OnPathReady& InOnReady,
    const FCk_Delegate_Nav_OnPathFailed& InOnFailed);

// Read the current path result (if any).
static const FCk_Nav_PathResult& Get_PathResult(const FCk_Handle& InHandle);
static ECk_Nav_PathStatus       Get_PathStatus(const FCk_Handle& InHandle);
static bool                     Has_Path(const FCk_Handle& InHandle);

// Re-bindable signal API (for code that wants to react to every path request, not just the one it issued).
static void BindTo_OnPathReady (UPARAM(ref) FCk_Handle& InHandle, const FCk_Delegate_Nav_OnPathReady&  InDelegate, ...);
static void BindTo_OnPathFailed(UPARAM(ref) FCk_Handle& InHandle, const FCk_Delegate_Nav_OnPathFailed& InDelegate, ...);
```

`FCk_Request_Nav_FindPath` carries:

```cpp
FVector       _TargetLocation;          // World space
bool          _AllowPartialPath = true;
FGameplayTag  _QueryFilter;             // (reserved; v1 uses NavData's default filter)
```

---

## Result + diagnostics

`FCk_Nav_PathResult` (stored as `FFragment_Nav_PathResult` on the requester entity):

```cpp
TArray<FVector>           _Waypoints;             // World-space, post-funnel
FVector                   _DestinationLocation;
ECk_Nav_PathStatus        _Status;                // None / Pending / Ready / Failed / Partial
FCk_Nav_PathDiagnostics   _Diagnostics;           // Why it failed + last query timing
```

`ECk_Nav_PathFailReason` (cleaned, no dtCrowd-era entries):

```
None, NoNavSystem, NoNavData, NoDefaultFilter,
StartProjectFailed, EndProjectFailed,
FindPathError, FindPathNoPath, FindPathInvalid,
EmptyPath, NotAuthority, BudgetExceeded
```

The diagnostics struct carries: last fail reason, last target / agent location, projected start / end, raw path point count, extracted waypoint count, last query duration in ms. The debugger reads this directly for its Agent Detail panel.

---

## Architecture

```
                          ┌────────────────────────────┐
   FCk_Request_Nav_FindPath│ FProcessor_Nav_HandleRequests│
   (enqueued by caller)──→│   • drains request fragment │
                          │   • runs FindPathSync       │
                          │   • populates PathResult    │
                          │   • fires OnPathReady/Failed│
                          └────────────────────────────┘
                                      │
                                      ▼
                          FFragment_Nav_PathResult + signals
```

`FCk_Nav_Algorithm` is a static helper: `ToRecastFloat3` / `FromRecastFloat3` / `FindPathSync` / `ExtractWaypoints` (funnel string-pull). Callable from outside the processor — useful for the debugger's Health Check synthetic probe.

The module has **no subsystem** (the dtCrowd-era one was for crowd init, which is gone).

---

## Project settings

`UCk_Nav_ProjectSettings_UE`:

```cpp
int32 _MaxPathQueriesPerFrame   = 8;        // Throttle. 0 disables (returns BudgetExceeded).
float _NavQuerySearchHalfExtent = 500.0f;   // cm — projection extent for start/end snap-to-navmesh
```

---

## Patterns

### Issuing a path request

```cpp
auto Request = FCk_Request_Nav_FindPath{TargetWorldLoc};
Request.Set_AllowPartialPath(true);
UCk_Utils_Nav_UE::Request_FindPath(MyEntity, Request, OnReadyDelegate, OnFailedDelegate);
```

### Reading the current path

```cpp
if (UCk_Utils_Nav_UE::Has_Path(MyEntity))
{
    const auto& Result = UCk_Utils_Nav_UE::Get_PathResult(MyEntity);
    for (const auto& Waypoint : Result.Get_Waypoints()) { ... }
}
```

### Health check (debugger / startup smoke)

```cpp
const auto Result = FCk_Nav_Algorithm::FindPathSync(World, FromLoc, ToLoc, /*partial*/ true);
if (Result.Get_Status() != ECk_Nav_PathStatus::Ready)
    /* surface Result.Get_Diagnostics().Get_LastFailReason() */;
```

---

## Anti-patterns

- **Don't enqueue path requests from a client.** `Request_FindPath` checks authority and fails with `NotAuthority`. Pathing is server-only.
- **Don't poll `Get_PathStatus()` in a per-frame loop.** Bind `OnPathReady` / `OnPathFailed` instead.
- **Don't reach into `FFragment_Nav_PathResult` directly to mutate waypoints.** It's `friend`-protected to the processor + `UCk_Utils_Nav_UE`. CkCrowd has friend access for path-corridor manipulation during replan; no other module should.
- **Don't bypass the budget by calling `FCk_Nav_Algorithm::FindPathSync` directly.** The static API exists for the debugger's Health Check probe and similar diagnostics. Game code goes through `Request_FindPath`.

---

## Limitations / known issues

- `_QueryFilter` field is reserved but unused in v1. Plug into `UNavigationQueryFilter` subclasses post-ship if needed.
- No async path queries. `FindPathSync` is fast enough at 8/frame for any realistic scenario; if it ever isn't, a dedicated async processor lives at the next layer.
- No off-mesh links / jumps. Recast supports them but we don't surface them.

## Future work

- Hierarchical path-finding (HPA*) if maps grow large enough that whole-map queries get slow.
- Alternate query filters per agent type (e.g., narrow-shoulder NPCs use a tighter filter).
- Async query worker (background thread).

## See also

- [CkCrowd/Claude.md](../../CkCrowd/Claude.md) — the primary consumer
- [CkCrowdDebugger/Claude.md](../../../../CkGameplayDebugger/Source/CkCrowdDebugger/Claude.md) — diagnostic UI
- [PLAN.md](PLAN.md) (delete this link post-ship)
