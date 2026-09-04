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

---

## Anti-patterns

- **Don't enqueue path requests from a client.** `Request_FindPath` checks authority and fails with `NotAuthority`. Pathing is server-only.
- **Don't poll `Get_PathStatus()` in a per-frame loop.** Bind `OnPathReady` / `OnPathFailed` instead.
- **Don't reach into `FFragment_Nav_PathResult` directly to mutate waypoints.** It's `friend`-protected to the processor + `UCk_Utils_Nav_UE`. CkCrowd has friend access for path-corridor manipulation during replan; no other module should.
- **Don't bypass the budget by calling `FCk_Nav_Algorithm::FindPathSync` directly.** The static API exists for the debugger's Health Check probe and similar diagnostics. Game code goes through `Request_FindPath`.

---

## Limitations / known issues

- ~~`_QueryFilter` field is reserved but unused~~ — **live, provider-neutral**: the tag resolves
  through `UCk_Nav_ProjectSettings_UE::_QueryFilters` (tag → `UCk_NavFilterDefinition_DataAsset`)
  or the native filter-definition registry, and the Recast adapter
  (`NavSurface/Recast/CkNavSurface_RecastAdapter`) compiles the definition into an engine filter at
  query time; empty/unmapped falls back to NavData's default. A request may also carry
  `_QueryFilterOverride` (a `FGameplayTag` that outranks the request's `_QueryFilter` for THAT
  query) — used by CkCrowd's strict/permissive planning phases. Note the start/end projection is
  UNFILTERED, so a query whose filter excludes the area under its own start still projects onto
  it — callers standing inside an excluded band must move their start out first (CkCrowd's
  `Get_EscapedQueryStart`).
- No async path queries. `FindPathSync` is fast enough at 8/frame for any realistic scenario; if it ever isn't, a dedicated async processor lives at the next layer.
- No off-mesh links / jumps. Recast supports them but we don't surface them.
- **The deferred-FindPath queue is per-world** (`ck::FFragment_Nav_DeferredRequests` on the
  world's transient entity; ring math and latest-wins coalescing live in `ck::nav::IsNewerRevision`
  / `AddDeferredLatest`, `CkNav_Fragment.h`) — multi-PIE-instance safe. Entries are dropped when
  their handle goes invalid and force-failed with `NoNavData` past `ck.Nav.MaxDeferralSeconds` (5s).

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

## NavSurface provider dispatch

`UCk_Utils_NavSurface_UE` is provider-neutral: every query capability resolves the world's provider
and calls that provider's capability table (`NavSurface/CkNavSurface_ProviderTable.h`). A provider
registers a complete `FCk_NavSurface_ProviderTable` at its module startup — CkNavigation registers
Recast's own, CkGroundNav registers its adapter — and an incomplete table is refused. A world's
choice lives on its transient entity (`FFragment_NavSurface_Provider`), seeded from
`UCk_Nav_ProjectSettings_UE::_DefaultNavSurfaceProvider` (Recast) the first time the revision watch
runs, and changed per world with `Request_SetProvider`. The choice is mirrored per world so that
`Get_BoundarySegments`, which is callable off the game thread, never reads the registry to find its
provider. Area markup dispatches the same way: `Request_AreaMarkup` drains into the world
provider's `_ApplyAreaMarkup`, `Get_IsMarkupLive` asks that provider whether the paint landed, and
the markup entity's teardown drains into `_ReleaseAreaMarkup`. An unregistered provider answers
`NoProvider` (or `Unknown_ProviderNotReady`, `NoData`, an empty box) everywhere.

## Settled is the condition to wait on, not a hop count

The table's `_IsSurfaceSettled` capability is the provider's own answer to "have you
finished": nothing in flight and nothing pending for this world -- no build, no repair, no deferred
re-derivation -- so the surface it has published is the one every query will answer from.
`UCk_Utils_NavSurface_UE::Get_IsSurfaceSettled` answers false while the NEUTRAL markup pipeline still
holds work of its own -- a paint queued on a markup entity, or a markup entity between
`Request_DestroyEntity` and the `FGroup_EndPlay` release that hands it to the provider, both asked
through `ck::nav_surface::Get_HasPendingMarkupWork` -- and only then dispatches to the provider, so a
world whose provider registered no table is not settled. This is what a caller waits on after a paint, a
release, or a `Request_SurfaceRebuild_ForTesting` kick -- the kick still completes `Succeeded` the
moment it is issued, because issuing is all it does. A fixed number of ticks encodes how long one
provider happens to take, and stops being enough the moment that provider's internal staging
changes or the world switches to the other one. Recast reports settled
when its health is `Ready` (nav data present, no build in progress) and `UNavigationSystemV1`'s
dirty-areas queue has drained; it deliberately does not consult `IsNavigationDirty()`, which reports
a navmesh with zero tiles as permanently needing a rebuild and would never settle.

## The rebuilt signal is pushed with bounds, polled without

`OnSurfaceRebuilt` carries the world entity and an `FBox` of the region that changed. A provider
that knows where it rebuilt calls `ck::nav_surface::Request_NotifySurfaceRebuilt(World, Bounds)`
once per publish (C++ only; providers are C++), and `FProcessor_NavSurface_RevisionWatch` drains
that queue one broadcast per region, in publish order — two publishes in a frame are two signals,
not one. A provider that only advances a counter — Recast — is still observed: on a tick that
drained nothing, the watch polls `_SurfaceRevision` and broadcasts once with an INVALID box, which
is the contract's way of saying the bounds are unknown and the whole surface must be treated as
changed. A listener must handle that box. The drain stamps the revision it caught up to on every
path, so a push is never re-reported by the poll behind it.

The watch is pinned to `FGroup_Gameplay_TimeDelta`. Publishes land in `FGroup_Transform`
(CkGroundNav's volume build and republish) and that group opens the next frame ahead of
`FGroup_Gameplay`, where CkCrowd's path refresh consumes the signal — one deterministic frame,
every time.

## Area tags mean something without a UNavArea

An area tag has two registrations, contributed side by side by whichever module owns the area.
`ck::nav_surface_recast::Register_AreaTag` gives Recast the `UNavArea` subclass that carries the
tag; `ck::nav_surface::Register_AreaPolicy` (`NavSurface/CkNavSurface_AreaPolicy.h`) gives every
other provider the tag's MEANING as an `FCk_NavSurface_AreaPolicy` — `Walkability` (the area is
removed outright, which today only `Nav.Area.Impassable` is) or `Cost` plus the multiplier crossing
it costs. Both registries park their seeding in an `FRegistrar` static and run it on first read,
because the gameplay-tag manager does not exist when a translation unit's statics do; the two flush
independently, so a provider that never touches a `UNavArea` still gets its policies. Registering a
tag twice with disagreeing policies ensures and the first one stands. The crowd areas read their
multiplier off the `UNavArea` CDO's `DefaultCost` rather than restating it, so the Recast cost and
the neutral cost cannot drift.
