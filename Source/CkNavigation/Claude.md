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

// Register or unregister an actor's actor-level navigation contribution. Separate component
// registration remains owned by the component APIs.
static void Request_SetActorNavigationRegistered(AActor* InActor, bool InRegistered);

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

## Implementation notes

Rationale relocated out of the source during the 2026-07-25 comment sweep. These are the
"why it looks like that" answers for `FCk_Nav_Algorithm` / `UCk_Utils_Nav_UE`:

- **`FindPathSync` passes the query filter explicitly.** The `FPathFindingQuery` ctor derives the
  filter from NavData when `SourceFilter` is null; passing it explicitly avoids relying on that
  fallback (and is what makes the `_QueryFilter` tag mapping work).
- **It calls `ARecastNavMesh::FindPath` directly**, not through the nav system — that skips the
  NavSys agent-dispatch step, and the query ctor has already populated `Query.NavAgentProperties`
  from `InNavData.GetConfig()`.
- **No typesafe handle by design.** Pathfinding is a service exposed to any entity with a Transform
  feature (CkEcsExt) plus the path-result fragment slot, which the Utils add lazily on first
  request — so there is nothing for an `FCk_Handle_Nav` to mean.
- **`Try_ProjectOntoNavmesh` is a single-shot helper.** CkEqs' `_ProjectOntoNav` post-pass inlines
  the same `UNavigationSystemV1::ProjectPointToNavigation` call directly rather than going through
  this UFUNCTION, to avoid dispatch overhead per candidate. Keep the two in sync.

---

## Anti-patterns

- **Don't enqueue path requests from a client.** `Request_FindPath` checks authority and fails with `NotAuthority`. Pathing is server-only.
- **Don't poll `Get_PathStatus()` in a per-frame loop.** Bind `OnPathReady` / `OnPathFailed` instead.
- **Don't reach into `FFragment_Nav_PathResult` directly to mutate waypoints.** It's `friend`-protected to the processor + `UCk_Utils_Nav_UE`. CkCrowd has friend access for path-corridor manipulation during replan; no other module should.
- **Don't bypass the budget by calling `FCk_Nav_Algorithm::FindPathSync` directly.** The static API exists for the debugger's Health Check probe and similar diagnostics. Game code goes through `Request_FindPath`.

---

## Limitations / known issues

- ~~`_QueryFilter` field is reserved but unused~~ — **live since the tag→class table**
  (`UCk_Nav_ProjectSettings_UE::_QueryFilters`): the tag resolves to a `UNavigationQueryFilter`
  subclass at query time; empty/unmapped falls back to NavData's default. A request may also carry
  `_QueryFilterClassOverride` (a `TSubclassOf` that outranks the tag for THAT query) — added for
  CkCrowd's strict/permissive planning phases, where a per-dispatch filter swap is not a project
  policy the table could express. Note the start/end projection is UNFILTERED, so a query whose
  filter excludes the area under its own start still projects onto it — callers standing inside an
  excluded band must move their start out first (CkCrowd's `Get_EscapedQueryStart`).
- No async path queries. `FindPathSync` is fast enough at 8/frame for any realistic scenario; if it ever isn't, a dedicated async processor lives at the next layer.
- No off-mesh links / jumps. Recast supports them but we don't surface them.
- **The deferred-FindPath queue is process-wide, not keyed on world** (`ck_nav_processor::GDeferredNavRequests`
  in `CkNav_Processor.cpp`) — it is not multi-PIE-instance safe. Entries are dropped when their
  handle goes invalid and force-failed with `NoNavData` past `ck.Nav.MaxDeferralSeconds` (5s).

## Acquire / release — a path episode must be ENDED, not just abandoned

`Request_FindPath` acquires an episode; **`Request_AbandonPath` releases it**, and every terminal
that ends a movement must call one. This is not optional bookkeeping: the result slot is shared, so
a caller that walks away without releasing leaves it reading whatever the acquire parked there.

Until 2026-08-19 there was no release at all. `FCk_Request_CrowdAgent_Stop` dropped the crowd's own
tags and nothing wrote the slot again, so a stopped agent reported `Pending` **for the rest of its
life** — every `Get_PathStatus` consumer was told a query was in flight forever, and the in-world
path-trouble overlay drew a permanent marker over a stationary NPC. It shipped for three weeks
because the two consumer-side guards that dropped the late result did so with a bare `return`.

`Request_AbandonPath` clears the status, the waypoints AND the destination (a `None` slot still
holding the old corridor is a half-cleared mirror), fires `Failed_Cancelled` for anything still
queued, purges this entity's entries from the deferral queue, and stamps the caller's post-abandon
revision so a query that drains afterwards is recognised as superseded rather than applied.

**`Pending` is now bounded.** `FProcessor_CrowdAgent_PathPendingWatchdog` (CkCrowd) reconciles the
slot against the agent's real movement state and fails an episode no provider answered within
`_PathPendingTimeoutSeconds`. Note the bound lives in CkCrowd, not here: CkNavigation's own 5s
deferral timeout only covers queries that actually reached its queue, and the PathNetwork and
VoxelNav branches never enqueue one.

## Future work

- Hierarchical path-finding (HPA*) if maps grow large enough that whole-map queries get slow.
- Alternate query filters per agent type (e.g., narrow-shoulder NPCs use a tighter filter).
- Async query worker (background thread).

## See also

- [CkCrowd/Claude.md](../../CkCrowd/Claude.md) — the primary consumer
- [CkCrowdDebugger/Claude.md](../../../../CkGameplayDebugger/Source/CkCrowdDebugger/Claude.md) — diagnostic UI
- [PLAN.md](PLAN.md) (delete this link post-ship)
