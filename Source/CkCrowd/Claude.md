# CkCrowd

> **Module status:** ✅ Built. This file was originally written as an *aspirational target* doc before
> the module existed, and was never reconciled with what actually shipped. It has been corrected
> against the code (2026-07-14).
>
> **Do not trust any older copy of this file.** Large parts of it described machinery that was never
> built — a Piercing processor with `_PiercingAngle`/`_PiercingActivateRadius` tunables, a
> SleepEvaluator, a ProgressEval/TriggerReplan stuck-detection tier with `_ReplanThresholdSeconds`/
> `_MaxReplansPerPath`, a PlayerProxy feature (`UCk_Utils_PlayerProxy_UE`, `_PlayerYieldMultiplier`),
> and the `FTag_CrowdAgent_Failed` / `FTag_CrowdAgent_IsObstacleOnly` tags. **None of those exist**
> (verified: zero symbols in `Source/CkCrowd/`). That fiction cost real debugging time — head-on
> avoidance was believed covered by "Piercing" when the actual mechanism is the velocity-obstacle
> sampler, and a two-agent standoff was misdiagnosed because the docs implied a stuck-detector
> existed. Sections below now state explicitly what is NOT built.

**Purpose:** ECS-native crowd steering / avoidance / agent management. Replaces dtCrowd. Sized for ~150 server-authoritative humanoid NPCs in a mixed indoor / open-world game (rental store).

**Depends on:** `CkNavigation` (path queries), `CkPhysics` (Velocity, EulerIntegrator), `CkSpatialQuery` (Jolt-backed neighbor queries), `CkShapes` (cylinder bodies), plus the standard ECS chain.
**Used by:** Project-side game code that needs NPC movement.

---

## Public API at a glance

```cpp
// Compose the crowd-agent feature DIRECTLY onto a transform-bearing entity (no child entity).
static FCk_Handle_CrowdAgent Add(
    UPARAM(ref) FCk_Handle_Transform& InOwner,
    const FCk_Fragment_CrowdAgent_ParamsData& InParams);

// Move it.
static FCk_Handle_CrowdAgent Request_MoveTo(
    UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
    const FCk_Request_CrowdAgent_MoveTo& InRequest);

// Or stop it.
static FCk_Handle_CrowdAgent Request_Stop(
    UPARAM(ref) FCk_Handle_CrowdAgent& InAgent);

// Bind to lifecycle signals.
static void BindTo_OnGoalReached(UPARAM(ref) FCk_Handle_CrowdAgent& InAgent, const FCk_Delegate_CrowdAgent_OnGoalReached& InDelegate, ...);
static void BindTo_OnGoalFailed (UPARAM(ref) FCk_Handle_CrowdAgent& InAgent, const FCk_Delegate_CrowdAgent_OnGoalFailed&  InDelegate, ...);
```

The handle `FCk_Handle_CrowdAgent` is a typesafe handle (`FCk_Handle_TypeSafe` derived). One per agent. Holds no state itself; state lives in the entity's fragments.

---

## Architecture — data flow per frame

```
[Caller]
  Request_MoveTo(target)
       │
       ▼
[CkCrowd]
  FProcessor_CrowdAgent_HandleRequests
       │
       ▼
[CkNavigation]                           ▲
  FProcessor_Nav_HandleRequests          │
  → FFragment_Nav_PathResult ────────────┘
       │
       ▼
[CkCrowd: per-frame steering loop]   (the REAL, registered processors)
  FProcessor_CrowdAgent_NeighborSync    ← reads probe overlaps → NeighborCache
  FProcessor_CrowdAgent_BlockDetect     ← is the goal unreachable? re-path first, then
                                          → Idle + GoalBlocked + OnGoalBlocked
  FProcessor_CrowdAgent_Separation      ← reactive repulsion force (only within _SeparationRadius)
  FProcessor_CrowdAgent_Steering        ← path-follow + lateral-clamped separation → desired velocity
  FProcessor_CrowdAgent_AvoidanceSample ← THE avoidance layer: velocity-obstacle sampler,
                                          OVERWRITES the desired velocity (see "Avoidance" below)
  FProcessor_CrowdAgent_AccelClamp      ← bounds per-frame magnitude + direction change
  FProcessor_CrowdAgent_VelocityBridge  ← writes FFragment_Velocity_Current (RunAfter AccelClamp)
  FProcessor_CrowdAgent_FaceAngle       ← slews yaw toward the desired-velocity heading (yaw only),
                                          but only while genuinely moving and only once a large
                                          heading change persists (see "Facing")
       │
       ▼
[CkPhysics]
  FProcessor_Velocity_Clamp            ← min/max enforcement
  FProcessor_EulerIntegrator_Update    ← position += velocity * dt
  → FFragment_EulerIntegrator_Current
       │
       ▼
  FProcessor_CrowdAgent_ApplyOffset    ← stages the integrator delta into PendingDisplacement
  FProcessor_CrowdAgent_PushApart      ← post-integration de-overlap (dtCrowd port: 0.7 factor,
                                         4 iterations — deliberately UNDER-RELAXED, so it does NOT
                                         guarantee zero interpenetration in a single frame);
                                         stages its shove into PendingDisplacement
  FProcessor_CrowdAgent_ConstrainToNavmesh ← THE SINGLE TRANSFORM WRITER (grounded agents; a flying
                                         one is excluded and drained by
                                         FProcessor_CrowdAgent_ApplyDisplacement3D instead, which
                                         enqueues the staged offset whole): walks the accumulated
                                         displacement along the navmesh surface
                                         (ANavigationData::FindMoveAlongSurface — dtCrowd's
                                         corridor movePosition, which the original port dropped)
                                         and enqueues ONE Request_AddLocationOffset. The agent
                                         Transform is its feet anchor: XY follows the surface walk
                                         and Z lands on the reached navmesh surface. Worlds with no
                                         nav data pass through untouched. Master switch:
                                         _NavmeshConstraintMode (project settings), default Enabled.
       │
       ▼
[Replication]
  Server transform → client smoothing  ← FProcessor_Transform_InterpolateToGoal_Location
```

Plus `FProcessor_CrowdAgent_BlockedRecheck` (FGroup_Gameplay): resumes a held agent when its goal clears.

Plus `FProcessor_CrowdAgent_PathPendingWatchdog` (FGroup_Gameplay): the reconciler for the shared
nav-path slot. **Read this before touching episode teardown.**

Every provider — CkNavigation, CkPathNetwork, CkVoxelNav — parks the SAME
`FFragment_Nav_PathResult` at `Pending` via `MarkPathPending`, but on the PathNetwork and VoxelNav
branches no CkNavigation request is enqueued, so `MarkPathPending` is that slot's only writer and
`OnRouteResolved` / `OnVoxelPathResolved` are the only things that can advance it — both gated on
the agent still holding `PathPending` or `Walking`. `Request_Stop` removes both. Until 2026-08-19
that combination meant a stopped agent's slot read `Pending` **forever**, and every consumer of
`Get_PathStatus` was told a query was in flight for the life of the entity.

The fix is an acquire/release pair, not a special case at the Stop site:
`DoAbandonActiveProviderQuery` is the SINGLE episode-end seam — `Request_Stop` and every fresh
dispatch route through it, so a terminal added later cannot forget the release. It advances the
revision (superseding any late result) and abandons whichever provider owned the query, read from
`FFragment_CrowdAgent_PathFollow::_ActiveProvider`. That field is RECORDED at dispatch rather than
re-derived, because `RequestPathForActiveGoal`'s provider choice reads mutable state the teardown
has already changed.

The watchdog is keyed on the **slot**, not on `FTag_CrowdAgent_PathPending`, and that is
deliberate: a tag-keyed watchdog is structurally blind to the exact state this defect produced
(slot Pending, tag gone), so it could never have caught it. Two rows — a live episode past
`_PathPendingTimeoutSeconds` is failed with `PendingTimeout` (status only; `OnPathResolved` owns
the tag transition and the single `OnGoalFailed`, so doing both here would double-broadcast), and
a Pending slot with NO live episode is the orphan: cleared, with an ensure naming the entity, and
deliberately no broadcast because nothing is bound to hear one.

**Do not "fix" a future recurrence by filtering the draw or the watchdog view on `Idle`.** That
hides the state instead of releasing it, which is what let this ship.

Plus `FProcessor_CrowdAgent_StationaryMarkup` (FGroup_Gameplay) + `_NavMarkup_EndPlay`: an agent
at or below `_StationaryMarkupSpeedThreshold` past `_StationaryMarkupDelaySeconds` (windowed
physical displacement, NOT the Idle tag — a blocked/pressing walker plugs a corridor exactly
like an idle agent) paints a
`UCk_NavArea_CrowdAgent` COST disc (`UCk_NavAreaMarkup_UE`, actor-free) so fresh paths — a
joiner's first FindPath, BlockedRecheck's re-path — route AROUND standing crowds; unpainted the
moment it genuinely moves. Cost (64x — any finite toll has a break-even line length where
detouring costs more than crossing; 64x with the 2x extent multiplier puts that at ~85 agents,
beyond any plausible queue; 8x broke at ~2 agents, 16x at ~20), never a hole:
a plugged corridor still paths through, and the mesh under the agent stays walkable for its own
clamp/path starts. The path planner is otherwise agent-blind and the avoidance sampler is a
short-horizon local optimizer — without this tier, an agent headed past a standing line presses
into it forever. Master switch `_StationaryMarkupMode` (default Enabled). Server-only.

Plus `FProcessor_CrowdAgent_PathRefresh` (FGroup_Gameplay, RunAfter StationaryMarkup): the discs
only bend paths computed AFTER they paint — a path computed before a crowd formed is a frozen
polyline the agent follows into the crowd (UE's own `UPathFollowingComponent` re-paths when the
navmesh under its path rebuilds; the dtCrowd port dropped that half of the mechanism). Each tick,
a Walking agent whose REMAINING path crosses a disc with a paint serial NEWER than its path's is
re-pathed at its own goal (BlockedRecheck's resume dance). One-shot per (path, disc-set): the
serial compare early-outs the common frame, a clean scan fast-forwards the path serial, and a
path that legitimately paid a disc's cost is never re-planned for the same disc twice. Discs
become trigger-eligible only once the rebuilt mesh actually REPORTS the cost area at their
location (`_ConfirmedOnMesh`; the settle seconds are just a pre-filter for that poly query), and
a disc adjacent to the agent's own goal is exempt (joining a queue legitimately ends beside
standing agents). Because the toll is paid per distance crossed, an agent ALREADY INSIDE the
band would find finishing the crossing cheaper than backing out plus detouring — so every
re-path site (MoveTo, BlockedRecheck, PathRefresh) plans from just OUTSIDE the band via
`Get_EscapedQueryStart` + the FindPath request's start override when the agent is inside and its
goal is not. Master switch `_PathRefreshMode` (default Enabled).

**Two-phase planning** (`_PlanAroundStandingCrowds`, default Enabled, gated on markup being
Enabled) sits on top of the toll: every agent FindPath plans FIRST with a STRICT filter that
treats the markup area as IMPASSABLE. The toll alone has a break-even — for a destination close
behind a standing crowd, crossing beats any detour, so single-phase planning legitimately walked
agents into bodies they could never pass (the queue-cross field symptom). A strict answer of
Failed on a genuine PLANNING VERDICT (no-path / find-path error / invalid / empty), or Partial
ending short of the goal, re-dispatches the episode ONCE with the permissive toll filter
(`OnPathResolved`, before the path-trouble stamp, revision advanced) — which is how queue-joiners
still reach a slot beside standing bodies. Infrastructure failures never trigger the fallback:
projection is unfiltered (a strict projection miss fails permissive identically), NoNavData is
filter-independent, and the pending watchdog's `PendingTimeout` MUST terminate exactly once — a
fallback there resurrected the timed-out episode into a second Pending wait
(`CkAutoTest_Crowd_Watchdog_PendingTimeoutFailsEpisodeOnce` pins this). Such an episode ends in
strict phase with `_StrictPlanFailed` false, so its OnGoalFailed payload reads structural, not
crowd-blocked. Strict is retried only on NEW evidence
(fresh MoveTo, BlockedRecheck resume, PathRefresh trigger, caller ForceReplan); the stall ladder's
re-paths carry none, and retrying strict there doubles every rung's Pending stop-start cycle into
a measurable facing whip. The strict filter composes with a host's own filter through the params'
`_NavQueryFilterStrict` tag (register a strict VARIANT of the permissive filter — classes cannot
compose at query time); unset, the framework's `UCk_NavQueryFilter_AvoidStandingCrowds` rides the
FindPath request's `_QueryFilterClassOverride`. `_StrictPlanFailed` on PathFollow records "no
crowd-free route existed this episode" and surfaces in the OnGoalFailed payload. Coverage:
`CkAutoTest_Crowd_NarrowGap_BlockedDetours`, `CkAutoTest_Crowd_QueueCross_RoutesAround`; gyms
`Crowd NarrowGap`, `Crowd QueueCross`.

**NOT BUILT** (documented here for years, never implemented — do not look for them):
`SleepEvaluator`, `Piercing`. `ProgressEval`/`TriggerReplan` never existed either, but the capability
they claimed — noticing an agent that cannot reach its goal — now DOES exist, as `BlockDetect` /
`BlockedRecheck` (see "Blocked goals").

Note that stock UE doesn't solve deadlock in its *solver* either — dtCrowd has no impatience,
stagnation, min-speed or randomisation mechanism at all. UE detects a blocked agent one tier UP
(`UPathFollowingComponent` block detection: 10 feet-samples at 0.5s intervals, all within 10cm of their
centroid ⇒ `OnPathFinished(Blocked)`) and hands it to the behaviour tree. **We now have an equivalent
tier**, with one addition UE lacks: a geometric occupied-goal test that answers immediately and names
the blocker, rather than waiting out a stagnation window. **If you are tempted to fix a deadlock by
adding an impatience timer or a minimum-speed floor to the cost function — don't.** That is the mistake
this tier exists to make unnecessary.

---

## Fragments owned by an agent

| Fragment | Purpose | Added by |
|---|---|---|
| `FFragment_CrowdAgent_Params` | Reflected params (radius, height, max speed, separation weight, flags, etc.) | `Add()` |
| `FFragment_CrowdAgent_PathFollow` | Current waypoint index, arrival radii | `Add()` |
| `FFragment_CrowdAgent_DesiredVelocity` | Steering output | `Add()` |
| `FFragment_CrowdAgent_NeighborCache` | Per-frame trimmed list of nearby agents | NeighborSync |
| `FFragment_CrowdAgent_LocalBoundary` | Cached navmesh boundary walls (dtCrowd's `dtLocalBoundary`) | `Add()`; refreshed by AvoidanceSample |
| `FFragment_CrowdAgent_SeparationForce` | Computed force vector | Separation |
| `FFragment_CrowdAgent_ProbeRef` | Handle to the probe child entity | Setup |
| `FFragment_CrowdAgent_FaceAngle` | Committed target yaw/pitch + the facing filter's engage flag and pending-candidate bucket | Add() |
| `FFragment_CrowdAgent_MoveRequests` | Variant of pending request types | Per-tick (drained) |
| `FFragment_CrowdAgent_InstalledRoute` | Goal + network epoch of the installed corridor (PathNetwork) | OnRouteResolved |
| `FFragment_CrowdAgent_PendingDisplacement` | Per-frame displacement staging (ApplyOffset + PushApart write; ConstrainToNavmesh consumes) | `Add()` |
| `FFragment_CrowdAgent_NavMarkup` | Stationary timer + strong ref to the painted cost-area object + paint serial/age (PathRefresh trigger data) | `Add()` |

`FFragment_CrowdAgent_PathFollow` also carries `_CurrentSegmentStart` — the world-space start of the
current path segment. Steering's plane-crossing waypoint retirement needs the *incoming* segment
direction, and CkNavigation's `ExtractWaypoints` strips the path's start point, so `Waypoints[-1]`
does not exist for the first segment. Captured at both path-install sites. Mirrors
`UPathFollowingComponent`'s `MoveSegmentDirection` (`PathFollowingComponent.cpp:954`).

`FFragment_CrowdAgent_InstalledRoute` exists because the PathNetwork corridor fragment **persists
across MoveTos and network rebuilds** — without the goal+epoch identity,
`FProcessor_CrowdAgent_OnRouteResolved` cannot tell a fresh plan from the corridor it already
installed. A rebuild replans the *same* goal at a *new* epoch, so the epoch compare is what forces a
re-install. `HandleRequests` clears it on every network-routed MoveTo.

**NOT BUILT:** `FFragment_CrowdAgent_PiercingPairs`, `FFragment_CrowdAgent_SleepState`,
`FFragment_CrowdAgent_ProgressTracker`.

A child entity hosting the Cylinder + Probe is spawned on agent setup. It carries the `Crowd.Agent` gameplay tag for filtering.

---

## Tags

```
FTag_CrowdAgent_NeedsSetup        # Setup pending
FTag_CrowdAgent_HasProbe          # Probe child spawned
FTag_CrowdAgent_Walking           # Has goal + path; Steering + AvoidanceSample require this
FTag_CrowdAgent_Idle              # No goal
FTag_CrowdAgent_PathPending       # FindPath / FindRoute in flight
FTag_CrowdAgent_DebugOverride     # Debugger "took control" — gameplay must not issue its own MoveTo
FTag_CrowdAgent_GoalBlocked       # Goal is unreachable; agent is Idle but still WANTS it (resumable)
FTag_CrowdAgent_Asleep            # DEFINED AND EXCLUDED, BUT NOTHING EVER STAMPS IT (see below)
FTag_CrowdAgent_Flying            # Free-space agent; opts out of every surface-bound / planar stage
```

**`FTag_CrowdAgent_Flying`** is stamped by `Add` from `_AgentMode` and never changes afterwards. It
partitions the agent population rather than disabling anything: `ConstrainToNavmesh`,
`StationaryMarkup`, `PathRefresh`, `AvoidanceSample`, `Separation`, `PushApart` and `FaceAngle` all
exclude it, and two Flying-gated processors take over the two jobs that still have to happen —
`FProcessor_CrowdAgent_ApplyDisplacement3D` (the pending-displacement drain, enqueuing the full 3D
offset) and `FProcessor_CrowdAgent_FaceAngle3D` (yaw + pitch). Steering, AccelClamp, VelocityBridge,
the integrator and ApplyOffset are already dimension-agnostic and run unchanged. A flying agent gets
**no avoidance and no de-overlap at all** — the sampler and both forces are planar by construction and
a volumetric replacement is not built — and its path must come from a volumetric provider
(`CkVoxelNav`); a Recast polyline is a floor-hugging route no flyer wants.

**`FTag_CrowdAgent_Asleep` is dead weight.** Every steering-side processor carries
`TExclude<FTag_CrowdAgent_Asleep>`, but no code path anywhere adds the tag — the SleepEvaluator that
was supposed to stamp it was never built. The exclusion is therefore a no-op today. (It is load-bearing
for tests in one respect: an *idle* agent is never sampled or steered because those views require
`FTag_CrowdAgent_Walking`, which is what actually lets agents stand still.)

**NOT BUILT:** `FTag_CrowdAgent_Failed`, `FTag_CrowdAgent_IsObstacleOnly`.

**Standing still** is a STATE, not a velocity choice: an idle agent has no `Walking` tag, so Steering
and AvoidanceSample never run on it and nothing writes it a velocity. A *walking* agent cannot elect
to stop dead for a neighbour (matching dtCrowd, where zero velocity is not in the candidate set) —
stopping happens via goal braking, the final-stop latch, or `Request_Stop`.

---

## Player Proxy — **NOT BUILT**

`UCk_Utils_PlayerProxy_UE`, `_PlayerYieldMultiplier`, `_PlayerProxySoftPushRadius`, the flags bitfield
(`AGENT`/`EMPLOYEE`/`PLAYER_PROXY`) and `_Flags` **do not exist**. `_CollisionFlags` / `_IgnoreFlags`
exist on the params struct but no processor reads them for player-yield behaviour. If the player needs
to displace NPCs today, that is not implemented.

---

## Tunables Reference

Every row below was verified against `FCk_Fragment_CrowdAgent_ParamsData` on 2026-07-14. Rows that used
to be here for `_Piercing*`, `_Sleep*`, `_Replan*`, `_MaxReplansPerPath` and `_PlayerProxy*`/
`_PlayerYield*` have been **deleted: those params never existed.**

| Tunable | Default | Purpose |
|---|---|---|
| `_Radius` | 42 cm | Agent radius. Two agents "touch" at 84cm (the radius sum) — the number most crowd assertions key off. |
| `_Height` | 192 cm | Agent height. Probe half-height is `_Height / 2`. |
| `_MaxSpeed` | 240 cm/s | Walking speed. |
| `_MaxAcceleration` | 480 cm/s² | Ramp rate (≈ 2× MaxSpeed). |
| `_MaxTurnRate` | 4.0 rad/s | **Minimum turn radius is `_MaxSpeed / _MaxTurnRate` = 60cm** — larger than `_WaypointArrivalRadius`. This is why waypoint retirement must not be proximity-only. |
| `_ArrivalRadius` | 30 cm | Final-stop tolerance (goal). |
| `_WaypointArrivalRadius` | 25 cm | Per-waypoint proximity tolerance. Only the *secondary* retirement test — the primary is the plane crossing (see Steering). |
| `_SeparationRadius` | **100 cm** | Distance below which the reactive separation force acts. NOTE: barely above the 84cm radius sum, so the usable repulsion band is only ~16cm and the force is tiny (~4cm/s) at first contact. Real avoidance is the sampler, not this. |
| `_SeparationLookahead` | 100 cm | **Probe radius = `_Radius + _SeparationLookahead`** (= 142cm). Two probes overlap each other, so agents detect each other at up to 284cm. |
| `_SeparationWeight` | **0.5** | Force multiplier. (This doc previously claimed 2.0 — wrong.) |
| `_SeparationInertia` | 0.5 | Lerp toward last frame's separation force; kills frame-to-frame flicker. Mirrors dtCrowd's `weightCurVel` concept (`DetourObstacleAvoidance.cpp:472`), applied as a force-blend factor because this solver does not sample-and-score separation; 0.5 approximates Detour's `wCurVel`/`wDesVel` = 0.375 ratio. |
| `_MaxNeighborsForSteering` | 6 | Top-N cap (sorted by distance). |
| `_CollisionFlags` / `_IgnoreFlags` | -1 / 0 | Present on the struct; **no processor currently reads them.** |
| `_AgentMode` | `Grounded` | `Flying` makes `Add` stamp `FTag_CrowdAgent_Flying`. Read once, at Add — changing it later changes nothing. See the Tags section for what the tag partitions. |

Avoidance-sampler tuning (velBias, penalty weights, sample density, trigger/stride),
stationary-markup tuning and the facing speed floor (`_FacingSpeedFloorCm`, see "Facing")
live in `UCk_Crowd_ProjectSettings_UE`
(`Settings/CkCrowd_ProjectSettings.h`), not on the agent. `_StationaryMarkupSpeedThreshold`
defaults to 84 cm/s, the former half-radius-per-0.25-second rule for the standard 42 cm agent;
the classifier uses XY displacement over the actual sample window rather than instantaneous
velocity.

---

## Patterns

### Spawn an agent

```cpp
auto AgentEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(ParentEntity);
auto AgentTransform = UCk_Utils_Transform_UE::Add(AgentEntity, SpawnTransform, ECk_Replication::DoesNotReplicate);

auto Params = FCk_Fragment_CrowdAgent_ParamsData{42.f, 192.f};
Params.Set_MaxSpeed(240.f);
Params.Get_Tags().AddTag(FGameplayTag::RequestGameplayTag(TEXT("Crowd.Agent")));
auto Agent = UCk_Utils_CrowdAgent_UE::Add(AgentTransform, Params);
```

### Move it somewhere

```cpp
auto Request = FCk_Request_CrowdAgent_MoveTo{TargetWorldLoc};
UCk_Utils_CrowdAgent_UE::Request_MoveTo(Agent, Request, OnReachedDelegate, OnFailedDelegate);
```

A MoveTo to (within 20cm of) the goal a Walking agent is ALREADY heading to is silently dropped — the
guard exists so a noisy re-issuer cannot reset the waypoint cursor every frame and stop the
final-stop ever latching. A caller who knows the world changed under the frozen polyline says so:

```cpp
Request.Set_ForceRepath(true);   // skips the same-goal guard, keeps the same goal/episode
```

### Stop it cleanly

```cpp
UCk_Utils_CrowdAgent_UE::Request_Stop(Agent);
```

### Read an agent's neighbors (for gameplay, e.g., "is anyone near me?")

```cpp
const auto& Cache = UCk_Utils_CrowdAgent_UE::Get_NeighborCache(Agent);
for (const auto& Nbr : Cache.Get_Neighbors())
    if (Nbr.Get_Distance() < 80.f) /* react to close neighbor */;
```

---

## Blocked goals — when an agent CANNOT reach its destination

Two agents sent to the same point cannot both stand on it. Before this tier existed, the second agent
pressed against the first forever, vibrating, and eventually **shoved it off its own goal** until the
point came within arrival radius. Nothing in the system was aware.

**`FProcessor_CrowdAgent_BlockDetect`** runs three detectors, because none alone is sufficient:

| detector | how | catches | misses |
|---|---|---|---|
| **Geometric** (primary) | a *stationary* neighbour sits on the final waypoint such that `SelfRadius + NbrRadius` exceeds the arrival radius — so the closest the agent can physically get is further out than "arrived" — AND stands strictly nearer the goal than the agent itself (2D). The nearer test is the same acyclicity rule the cluster detector enforces on anchors: without it, the markup-bootstrapped innermost and its own settled dependent hold each other — each inside the other's foreclosure ring (< `SelfR+NbrR−Arrival` of the goal) — and the goal is never taken (observed: 0/15 reached, whole pack terminal, ~50% of runs once upstream timing shifted the settle race) | agent-occupied goals, **exactly and immediately**, naming the blocker | walls, props, multi-agent plugs; a foreclosing body the agent has already gotten NEARER the goal than (it presses on until arrival or the no-progress ladder — the ladder's crowd-regime starvation is known deferred work) |
| **Cluster** (propagation) | a cached neighbour has SETTLED (reached-and-Idle, `GoalBlocked`, `GoalFailedHold`, or stationary-markup PAINTED), is parked inside our depth-chained GOAL REGION (`SelfRadius + NbrRadius + ArrivalRadius + _CrowdedGoalContactPadCm + AnchorDepth * (SelfRadius + NbrRadius)` of our `_ActiveGoal`), is in 2D contact (`SelfRadius + NbrRadius + _CrowdedGoalContactPadCm`), stands strictly nearer the goal, and lies in a ±60° cone around our direction of travel | the agents the geometric detector structurally cannot answer for — everyone behind the first ring of a crowd converging on one destination, and anyone whose destination a stranger simply stopped on | anything not made of settled agents standing on the destination |
| **No-progress** (safety net) | REMAINING PATH DISTANCE (agent → current waypoint + the polyline tail) sampled every `_BlockDetectionInterval`; a stall is `_BlockDetectionNoProgressWindowSeconds` without the windowed minimum improving by `_BlockDetectionProgressEpsilonCm` | everything else — walls, fixtures, multi-agent plugs, **and orbiting** | nothing the other two are for (it names no blocker) |

**The no-progress detector measures progress along the path, not displacement.** It used to be UE's
feet-sample centroid ring (`PathFollowingComponent.cpp:1556-1608`) — N samples all within a small
radius of their centroid — and that ring had a hole big enough to hide the module's worst failure
mode in: `ConstrainToNavmesh`'s `FindMoveAlongSurface` walk turns a wall press into a lateral
*slide*, so a wall-blocked agent moves several metres along the wall with nonzero velocity while
getting no closer to its goal. Every sample lands far from the centroid, the ring never trips, and
the agent stays `Walking` forever with nothing in the system aware. Path progress cannot be faked by
sliding, and it subsumes the orbit case the ring also missed, so the ring is gone rather than kept
alongside.

**A stall is answered before it is escalated.** The overwhelmingly common cause is that the Recast
polyline is FROZEN at plan time (`CkNav_Algorithm`) while the world it was planned against moved. So
a stall first spends up to `_BlockDetectionMaxStallRepaths` internal re-paths at the same goal —
issued through `FProcessor_CrowdAgent_HandleRequests::RequestPathForActiveGoal`, which is the only
sanctioned way for framework-internal code to reach a re-path, because the caller-facing same-goal
guard would swallow it. Real progress (the windowed minimum improving by the epsilon) refunds the
budget; an exhausted budget is what promotes the stall to a block.

⚠️ **Known gap: the no-progress ladder does not engage against slow pack-rim creep.** The stall
window trips only after `_BlockDetectionNoProgressWindowSeconds` (3.0s) without the windowed minimum
improving by `_BlockDetectionProgressEpsilonCm` (30cm) — an implied floor of ~10 cm/s. An agent
creeping inward at or above that against the outside of a settled pile refunds the window forever
while never arriving, and every re-path site (including `PathRefresh`, which spends no rung of the
ladder) calls `DoResetProgressWindow` and re-seeds the baseline. Measured against a 15-agent
shared-goal pile, the ladder never reached even its first re-path. **Do not read a bounded stall
termination into this tier for a crowd-pressure stall** — it is reliable against walls and static
plugs, which is what it was measured on. Hardening it is deferred work.

**A re-path brakes the agent.** `DoRepathAtActiveGoal` swaps `Walking` for `PathPending`, which
drops the agent from Steering's view but not from AccelClamp's or VelocityBridge's — it used to
keep shipping the velocity it stalled with and walk-cycle into the obstruction for the whole
re-path window. It now zeroes **both** `_Velocity` and `_LastVelocity` (AccelClamp's target
self-feeds from `_Velocity`, so zeroing that alone buys one `MaxAccel` step and then plateaus at
what it just wrote back), and resets `_ProtectedLeadingWaypointCount` like every other re-path
site. A caller-driven `DoForceReplan` keeps its momentum on purpose and is untouched.

**Off-path drift is healed on the same cadence, and the heal is bounded.** If the agent is more
than `_BlockDetectionOffPathRepathThresholdCm` (XY) from the segment it is currently following, it
is re-pathed outright. A teleport, a save restore or an external shove otherwise leaves Steering
chasing waypoints on a corridor the agent no longer stands near, and nothing else in the pipeline
notices. The heal spends a rung of the SAME `_BlockDetectionMaxStallRepaths` ladder a stall does
and falls through to a `NoProgress` block once that budget is gone — an agent that keeps drifting
or being clipped must terminate boundedly instead of re-pathing every cadence forever. A one-off
displacement still heals for free: walking the re-planned corridor makes progress, and progress
refunds the whole budget.

The geometric detector's **engagement range is derived, not a knob**:
`(2 * SelfRadius) + ArrivalRadius + BrakingDistance + 20cm` slack, where
`BrakingDistance = MaxSpeed² / (2 * MaxAcceleration)`. Engaging at the agent's own braking distance
is the last moment it could still stop cleanly; a neighbour parked on a goal 30m away is not
blocking anything yet.

**The cluster detector exists because the geometric one only ever answers for the agent touching the
GOAL.** Send fifteen agents to one point and the innermost ring is the only part of the crowd in
contact with anything that detector can see; everyone behind it learns nothing and presses inward,
each shoving the ring ahead of it, until the no-progress ladder stops them one at a time — ~9s+ each,
and a pack that keeps creeping refunds the ladder's budget and never terminates at all. The cluster
rule closes that gap by making a settled agent an ANCHOR: contact with an agent that has already
stopped on the ground you are walking to, ahead of you, is itself proof the destination is
unreachable — so settling propagates outward (occupant → ring 1 → ring 2) and the crowd converges to
a stable packed formation within about a second of contact.

**It never reads the neighbour's goal, and that is the point.** Goal identity looked like the natural
test and is the wrong one: agents sent to *nearby but unequal* points — a slot offset, a projected
destination, two customers at the same shelf — form exactly the pile this tier exists for and would
never match. And why a body is parked in your way is not a property you can act on: a stranger who
simply stopped there obstructs precisely as much as a rival. What replaces goal identity is geometry,
in three parts, each load-bearing:

- **Goal region, chained by depth** — the neighbour must be parked within `SelfRadius + NbrRadius +
  ArrivalRadius + _CrowdedGoalContactPadCm + AnchorDepth * (SelfRadius + NbrRadius)` of *your* goal.
  The base term is "close enough that you could not stand where it does and still be short of your
  own arrival tolerance"; the depth term is what lets the rule reach past the first ring (see below).
  Since markup makes every stationary stranger a candidate anchor (see below), **this test plus
  contact/nearer/cone is the entire protection against false blocks** — a picket line is "settled" now,
  and only its distance from your goal keeps it inert. A stranger that is not itself crowd-blocked
  (`GoalCrowded` or `GoalOccupied`)
  contributes depth 0, so it never anchors beyond the base radius: `StationaryLine_PathsRouteAround`,
  both `PathRefresh` tests and `PathNetworkStationaryDetour` park their pickets 450-539cm from the
  walker's goal against a 124cm base region, a 3.6-4.3× margin.
- **Strictly nearer** — makes the anchor relation acyclic. A settled agent behind you can never stop
  you short of ground that is still free, and two agents can never hold each other.
- **±60° frontal cone** (`FrontalConeCosine`, a file-local constant, not a knob) — the neighbour must
  actually be in the way. Strictly-nearer alone admits a body up to ~75° off the line to the goal, so
  without the cone an agent brushing laterally past someone parked near the goal in a wide corridor
  stops for no reason. A degenerate direction — standing on the goal, or interpenetrating the
  neighbour — passes the cone rather than failing it; the other tests already answer those states.

**The region grows with the pile, via a depth chain.** A fixed region does not work, and the failure
is not theoretical: with the flat 124cm region a 15-agent crowd settled at 96-180cm from the goal —
real packing is sparser than hex arithmetic predicts — so ring-2 bodies fell outside the region,
could anchor nobody, and the 15th agent walked forever with no qualifying contact. Capacity landed
at exactly the crowd size that had to fit.

So each crowd block — `GoalCrowded` *and* `GoalOccupied` — stamps a **depth** on the agent
(`_CrowdedGoalDepth`): 1 for stopping behind a body standing on the goal itself, +1 per ring
outward. An anchor's depth extends the region the *next* agent back is allowed to find it in, by
one body diameter per ring — the exact rate at which a physical pile grows. Every other blocked
reason, and every path that clears blocked state, resets the depth to 0, so a chain can only
extend through agents that are themselves crowd-blocked and therefore **always bottoms out at a
body genuinely standing on the destination**. That is what keeps the rule from drifting outward on
its own: depth is earned, never assumed. When several neighbours qualify, the **shallowest** wins,
so a chain never gets longer than the pile requires.

`GoalOccupied` chaining was added 2026-08-20 (it originally reset to 0): in a crowd that was still
spread out at block time, nearly everyone SEES the occupant and blocks `GoalOccupied` — so no
chain ever formed, the zone never grew, and a held rim agent whose six-nearest cache no longer
contained the occupant found no evidence at re-check and resumed into a pack that never moved,
once a second. An occupied hold stopped for the same physical fact a crowded one did — its blocker
IS the body on the goal (a chain foot, depth 0) — so it earns ring-1 depth by the same rule rather
than by exception.

Conditions 3-5 are unchanged and still all required — the depth only ever widens *where* an anchor
may stand, never what makes it one.

**Painted stationary markup is the fourth way to count as settled, and it is what ROOTS the chain.**
A pure reached/blocked predicate has a bootstrap hole that only shows up under load: the avoidance
sampler's crowd-pressure standoff can exceed the arrival radius, so the innermost agent hovers just
outside its goal — measured at 39-40cm against a 30cm radius — never arrives, and therefore no
anchor of any kind is ever created. The whole cascade has nothing to start from and the entire crowd
walks forever. Markup closes it precisely because it paints on **windowed physical stillness alone**
(`_StationaryMarkupDelaySeconds`, 1.5s), with no reference to goal state: a Walking-but-jammed agent
paints exactly like a standing one. The jammed innermost becomes an anchor, its ring blocks off it,
the inward pressure releases, and it then genuinely arrives. That a Walking agent can anchor is not a
contradiction — the strictly-nearer test means it never blocks on its own dependents, so it keeps
pressing with less opposition rather than deadlocking against the ring it just stopped.

Two consequences worth stating. A markup anchor can **unpaint** by moving again, leaving its
dependents held on an obstruction that has gone; `BlockedRecheck` resumes them within its 1s cadence,
so the churn is bounded and accepted. And a crowd agent that was **never commanded anywhere** — no
goal, no reached or blocked state, simply parked — now anchors too, which is correct: a body standing
on your destination obstructs it whether or not anything ever told it to go there. With
`_StationaryMarkupMode` Disabled the disjunct simply drops out and the tier degrades to
reached/blocked anchors only.

The rule is deliberately NOT gated on the geometric detector's engagement window: a real crowd stacks
far deeper than one braking distance from its goal, and contact with a settled obstruction ON the
destination IS the engagement. Master switch `_CrowdedGoalBlockMode` (default Enabled). Coverage:
`CkAutoTest_Crowd_BunchUp_SettlesAtSharedGoal`.

**`BlockedRecheck` evaluates the same cluster rule, and that symmetry is load-bearing.** The recheck
decides resume-vs-hold by re-running blocker detection, and an agent anchored on a NEIGHBOUR rather
than on the goal has nothing in the occupied-goal test that can see the pack — it would resume every
cadence, walk back in, re-block, and oscillate between held and pressing forever (and on a
`NoProgress` cause, burn `_BlockedMaxRetries` doing it and fail the move outright). A live cluster
blocker therefore holds without spending a retry, exactly as an occupied goal does; resume still
happens through the ordinary full-repath path once the pack genuinely drains.

**The recheck re-validates the REMEMBERED blocker first, by direct read — the cache scans alone
cannot be trusted to declare the goal clear.** Both scans read the neighbour cache, which is the
top-`_MaxNeighborsForSteering` nearest bodies; a rim holder in a settled pack no longer carries the
goal's occupant among its N nearest, so the scans go blind exactly when everything is at rest, and
the agent resumes into a crowd that never moved (observed under full-suite load: a lone resume 0.5s
into the BunchUp quiet window, and a mass resume one cadence after a mass block). So the recheck
first re-validates `_BlockedBy` from the handle itself — transform, settled/stationary state, and
the occupancy geometry its cause blocked under (`Get_AnchorQualifyingDepth` is shared between the
cluster scan and this re-validation precisely so the two cannot drift). Only when the body that
caused the hold no longer qualifies do the scans get to decide; a departed or destroyed blocker
therefore resumes exactly as before.

**Re-validation deliberately does NOT re-litigate strictly-nearer or the frontal cone.** Both are
hold-CONSTRUCTION rules — nearer keeps the anchor relation acyclic, the cone proves the anchor is
in the way of travel — and neither concern survives into a hold that already exists: the edges were
acyclic when built, and a held agent is not travelling. Post-settle PushApart drift of a few
centimetres routinely flips both (observed: an agent blocked `GoalOccupied` at 42.0cm relaxed to
38.4cm, ended up nearer the goal than its own still-standing blocker, and resumed into the pack
half a second into the quiet window). A hold re-validation asks one question only: does that body
still occupy the ground.

**Known and accepted:** an agent that reached its goal and was later shoved off it still counts as
settled, so a newcomer holds at contact with it even while the goal point is momentarily free. The
hold is resumable and the formation is stable, which is worth more than chasing the transient.
Likewise, an agent whose goal sits just past a stranger parked in a doorway now HOLDS instead of
pressing — that is the intended reading of "cannot get there", and the hold resumes the moment the
doorway clears.

**On block:** `Walking` → `Idle` + `FTag_CrowdAgent_GoalBlocked`, and `OnGoalBlocked` fires **once per
episode** (not once per re-check) with a payload naming the blocker — exactly what a gameplay-side
queue manager needs to send the NPC somewhere else instead of having it wait.

**Policy is per-agent** (`_BlockedPolicy`):
- **`HoldAndRetry`** (default) — stop, wait, re-check every `_BlockedRecheckInterval`, and **fully
  re-path and resume the moment the goal clears**. Right default for a shop: an NPC that waits a metre
  from a taken shelf and slides in when it frees needs no gameplay changes at all.
- **`FailMove`** — `OnGoalBlocked`, then `OnGoalFailed`, then Idle. UE's semantics; the caller owns recovery.

**Under `HoldAndRetry` the two block CAUSES retry differently, and the split is load-bearing:**

| cause | retry | why |
|---|---|---|
| `GoalOccupied` | **unbounded**, exactly as before | the blocker is another AGENT and it will move. Queue NPCs sit here for as long as the line takes; making a long wait emit `OnGoalFailed` would break every gameplay-side queue that deliberately defers to `HoldAndRetry` |
| `GoalCrowded` | **unbounded**, same as `GoalOccupied` | the obstruction in front is another agent too, and `_BlockedBy` names the settled neighbour this agent came to rest behind rather than whoever holds the goal. It differs from `GoalOccupied` only in what the payload points at, which is what a queue manager needs to tell "the shelf is taken" from "the way to it is full" |
| `NoProgress` | bounded by `_BlockedMaxRetries`, then `OnGoalFailed` + `GoalBlocked` cleared + Idle | the obstruction is static — a wall, a fixture, a plug the planner cannot see. It never clears, so the old behaviour (`BlockedRecheck` finds no agent blocker, re-paths, re-enters the same wedge, forever) was a silent hang no caller could observe |

`BlockedRecheck` still tests for an agent blocker FIRST for every cause — occupied goal, then
cluster — so a `NoProgress` agent that happens to have someone standing on its goal, or a settled
pack between it and that goal, holds without spending a retry.

**`OnGoalFailed` says WHY** (`FCk_CrowdAgent_GoalFailedInfo`): the high-level reason
(`PathFailed` / `PathEndsShortOfGoal` / `NoProgressRetriesExhausted` / `BlockedFailMovePolicy`),
the nav layer's detail (`ECk_Nav_PathFailReason`), the goal, and the load-bearing bit —
`_NoCrowdFreeRouteExisted`. True means the strict planning phase found no route avoiding STANDING
BODIES this episode: they move, so a caller retries later or waits (a bounded
`MoveTo + Set_ForceRepath(true)` is the sanctioned retry — it is the one override that clears the
failure hold). False means the failure is structural — walls, fixtures, no navmesh — and retrying
against unchanged nav is futile: rotate the target or give up. This is the queue-manager /
planner contract; a consumer that ignores the flag either blacklists targets a crowd merely stood
near, or retries forever into a wall. Coverage:
`CkAutoTest_Crowd_NarrowGap_NoRouteFailsClean` (also the no-progress ladder's dedicated
instrument: a point-plug cannot refund the stall window the way pack-rim creep does).

**What is deliberately NOT offered: silently widening the arrival radius to declare "arrived".** It
lies to a caller who asked for "within `_ArrivalRadius` of X" and may range-check against it — and it
is **terminal**: an agent that has "arrived" 100cm out is Idle and will never walk the last metre when
the blocker leaves. `HoldAndRetry` exists precisely so blocked is a *hold*, not a dead end.

An arrived (Idle) agent also now only absorbs `_PushApartIdleYield` (0.25) of a de-penetration shove,
so a newcomer pressing it takes most of the correction. Not zero — idle-vs-idle overlap must still
resolve, and the at-rest non-penetration assertion in `Crowd_Separation_Convergence` depends on it;
clamped at 0.05 minimum because zero would leave two idle agents interpenetrated forever.

Why 0.25 and not 1.0 (the product case): at 1.0 an NPC standing at a shelf is body-checked clean off
its own goal by the next customer, and an *arrived* agent has no restoring drive — the eviction is a
one-way random walk outward. At 0.25 a walker pressing a stander absorbs ~4× more of the correction
than the stander; idle-vs-idle still separates, just over a few more frames.

**`_PushApartIdleYield` is a bound on the damage, not the cure.** The real fix for a newcomer that
presses forever is BlockDetect noticing the goal is unreachable and stopping the press; the yield only
limits how far a stander is shoved in the meantime (and when a player barges through a crowd).

---

## Avoidance — how agents actually avoid each other

**`FProcessor_CrowdAgent_AvoidanceSample` is the avoidance layer.** It is a port of dtCrowd's
velocity-obstacle sampler (`DetourObstacleAvoidance.cpp`): each frame it builds a cloud of candidate
velocities, scores each by `DesPen + CurPen + ToiPen + SidePen`, and **overwrites** the desired
velocity Steering wrote. It runs after Steering, before AccelClamp.

The **separation force is not the avoidance system** — it is a short-range reactive nudge that only
acts inside `_SeparationRadius` (100cm, barely past the 84cm radius sum). Agents that avoid each other
properly never let it engage.

Gates (project settings): `_AvoidanceSampleTrigger` = `NeighborCountAndZoneTag`,
`_AvoidanceSampleNeighborThreshold` = 1 (any neighbour at all triggers it), `_AvoidanceSampleStride` = 1
(every frame). The view requires `FTag_CrowdAgent_Walking` + `FTag_CrowdAgent_HasProbe`.

Four independent gates must all pass: triggered (project trigger mode + per-agent `AvoidancePolicy`
override + zone tags), on this agent's 1-in-N frame (round-robin stride), NOT inside the
final-approach envelope (below), and at least one cached neighbour.

### The final-approach envelope — the sampler stands down for the last stretch

**An agent on its LAST path leg and within
`ActiveArrivalRadius + _AvoidanceFinalApproachSuppressionCm + PackExtent` of the final waypoint does
not sample at all** — Steering's path-follow velocity survives untouched. This is the standard
Detour-caller idiom, and it is here to break a hard lock-out rather than to tune one: once a ring of
bodies has settled at contact distance around the destination, every inward candidate closes on a
near-overlapping neighbour, the `C < 0` overlap branch of `TimeToCollision` floods `ToiPen` across the
*whole* cloud, and the cheapest candidate becomes a standoff **wider than the arrival radius**. The
agent hovers just outside its goal forever — measured at 39-40cm against a 30cm radius, with the
entire crowd Walking and nothing terminal. No penalty weight fixes that; the cloud has no cheap
inward candidate left to pick.

**`PackExtent` is what sizes the envelope to the pile actually standing there**, and without it the
envelope is sized for an EMPTY destination. It is the largest planar distance from the final waypoint
to any cached neighbour that counts as SETTLED, capped at a file-local `PackExtentCeilingCm` = 400cm;
no settled neighbours means 0, and the envelope is then exactly the fixed `30 + 90 = 120cm` it always
was. The failure it closes is a *second*, separate standoff from the overlap one above: against a
settled pile, the sampler's ORDINARY (non-overlap) time-to-impact term out-votes the desired-velocity
term by several penalty points at 84-120cm, which is a stable equilibrium **outside** a fixed 120cm
envelope and therefore never suppressed. That cost function is dtCrowd-faithful and correct — it
simply has no notion that the bodies ahead will never move — so the fix is not to touch it but to
stand the sampler down at the pack's boundary. **Doing so is what lets contact happen at all, and
contact is the evidence `BlockDetect`'s cluster tier blocks on**; a sampler-held agent starves that
tier of its only input and the pile never propagates outward to it. The 400cm ceiling exists because
beyond roughly four rings a settled body near the path is indistinguishable from an unrelated parked
stranger, and predictive avoidance must stay ON when merely passing one.

"Settled" is the SAME predicate the cluster detector anchors on — reached, `GoalBlocked`,
`GoalFailedHold`, or stationary-markup painted — shared as
`ck::ck_crowd_agent_settled_algorithm::Is_NeighbourSettled` rather than restated, precisely so the
two tiers cannot drift apart. A divergence would let the sampler hold an agent off a pack the
detector considers anchorable, which is the standoff both exist to end.

The envelope mirrors Steering's arrival test exactly (same `_WaypointIndex == Num()-1` predicate, same
3D metric for the agent's own distance), so it is a strict superset of the condition it exists to let
the agent satisfy. What is given up is predictive avoidance over that last stretch; what still runs is
**separation** (lateral-clamped) and **PushApart**, so contact inside the envelope is resolved by
de-penetration — the correct authority that close to a destination, where there is nowhere left to
route around to. An explicit per-agent override (`SamplingAlways` policy, or the `AlwaysSample` tag)
outranks the envelope: those are deliberate caller instructions, not defaults to second-guess. `0`
disables the feature outright — the gate short-circuits before `PackExtent` is ever measured, so the
pack term cannot resurrect a disabled suppression.

### The corridor stand-down — the sampler also stands down between tight walls

A second stand-down, same doctrine (gate whether the sampler runs, never edit its scores): between
OPPOSING navmesh walls whose corridor width is under `2 × radius + _CorridorStandDownSlackCm`
(default slack 40 → 124cm for the standard agent), the sampler IS the jitter. The navmesh bakes the
gap walkable — the body fits — but the sampler penalises inward candidates from BOTH sides at once,
and with any neighbour pressure the winner oscillates between wall-hugging candidates: the agent
fidgets through a doorway it could simply walk. `Is_InTightCorridor`
(`CkCrowdAgent_AvoidanceSample_Algorithm.h`, unit-pinned) detects the bracket from the cached local
boundary (pairs of agent-facing walls whose closest-point directions oppose within ~60°); the
stand-down hands the corridor to path-follow + the navmesh clamp, which cannot leave the mesh
anyway. OPEN corridors only: a STATIONARY body within the pinch's reach vetoes the stand-down —
the sampler's standoff against an immovable blocker is the calm behaviour the facing contract pins
(measured: 128° of yaw travel against the 90° ceiling the one time this gate ran without the
veto). The same explicit per-agent overrides outrank it. Master switch `_CorridorStandDown`
(default Enabled). Coverage: `CkAutoTest_Crowd_NarrowGap_TraverseCalm`; gym `Crowd NarrowGap`.

The stride gate is a pure function of `GFrameCounter` and the agent hash, so skipping carries no
round-robin bookkeeping to half-update; a suppressed agent is also not counted in `Crowd Agents
Sampled`. Scheduling: `FGroup_Physics`, RunAfter Steering (whose output it overwrites) + NeighborSync
(whose cache it reads); RunBefore AccelClamp/VelocityBridge is enforced *indirectly* by AccelClamp's
RunAfter on it. It is a `TParallelProcessor` because the Samples × Neighbors scoring loop is the
module's only nested-loop hot spot.

### Invariants this port must preserve — each was violated once, and each cost days

Three bugs (2026-07-13/14), all in the cost function, all invisible to every behavioural test:

1. **The ToI term must discriminate between candidates when agents already overlap.**
   `TimeToCollision` returned a candidate-INDEPENDENT `0.0f` for `C < 0`, making the collision penalty
   constant across the whole sample set. The sampler then degenerated to "pick the path-follow anchor"
   — i.e. drive straight into the neighbour it was avoiding. Restored dtCrowd's overlap branch
   (`htmin = -htmin * 0.5`, `DetourObstacleAvoidance.cpp:351-356`).

2. **Stopping must never be free.** The port added an explicit `FVector::ZeroVector` candidate and
   returned `ToiPen = 0` when no collision was predicted. A stationary pair predicts no collision, so
   "stop" paid nothing while every forward candidate paid the full penalty — and once stopped, inertia
   made staying stopped cheaper still. **Two agents converging on a shared waypoint both stopped dead
   and never moved again.** Fixed by (a) floor `TMin` at the horizon so the ToI term is never zero
   (Detour inits `tmin = horizTime`), and (b) delete the zero anchor and centre the cloud on
   `DesiredVelocity * velBias` (0.5) — Detour's most conservative candidate is *half speed forward*,
   never a stop.

3. **The desired velocity must never yield.** Steering used to damp path-follow by separation
   intensity (`PathFollowDamp = max(0, 1 - SepMag/MaxSpeed)`). Head-on separation is antiparallel to
   the path, so the two cancelled and the desired velocity collapsed to zero exactly when neighbours
   pressed — handing the sampler a ~zero anchor and re-creating (2). dtCrowd rebuilds `dvel` at full
   speed every frame and clamps an opposing separation contribution to **pure lateral**
   (`DetourCrowd.cpp:1499-1510`): separation may REDIRECT forward motion, never CANCEL it. Steering now
   does the same.

Also restored: **RVO reciprocity** (`vab = 2*vcand - vel - nei.vel`) — the port had plain VO, so each
agent solved as if the other would not move, and both over-corrected.

Port details owned here (the processor is deliberately comment-light — this doc is where the
rationale lives):

- **Sample density and adaptive refinement.** `BuildCandidatePattern` builds one cloud —
  `_AvoidanceSampleAngularDivs` × `_AvoidanceSampleRings` + centre, default **8 × 2**, denser than
  UE's `ECrowdAvoidanceQuality::Low` (5 divs × 2 rings + centre at velBias 0.5,
  `CrowdManager.cpp:182-187`). `SelectWinner` then runs Detour's **full adaptive refinement**:
  `_AvoidanceSampleDepth` iterations (default **5**), each halving the cloud radius and re-centring
  on the previous depth's winner (on the winner's *scored* velocity when reachability projection is
  enabled, so distinct clouds cannot collapse onto one post-projection plateau). There is no
  `BuildSamplePattern` symbol and refinement is not disabled — earlier revisions of this file
  claimed both. The depth-1 bias centre is Detour's "sample at zero pattern offset"
  (`DetourObstacleAvoidance.cpp:551-554`) — VelBias of the desired velocity ("slow down"), never a
  true stop; there are no off-grid desired/current/zero anchors (see invariant 2).
- **Side preference is scored neighbour-relative, on the candidate velocity.**
  `CalculateNeighborSidePenalty` takes the normalised direction to the neighbour and the candidate
  velocity `vcand`, and penalises the cross product `dir × vcand` signed by the configured
  preference — so the penalty is per-neighbour and averaged over the cache. There is **no
  `SideRightness` symbol** and no agent-frame left-perpendicular; earlier revisions of this file
  described both, and also claimed the neighbour-relative form degenerates head-on. It does not:
  A sees B at +X, so a +Y candidate scores zero penalty under PassLeft and A goes +Y; B sees A at
  −X, penalises +Y, and goes −Y. The two diverge. What degenerates head-on is a *cross-product of
  the two velocities*, which is not what this computes. Note this is a Ck term with no stock Detour
  counterpart — `DetourObstacleAvoidance.cpp` has no side preference at all — so it is **not**
  claimed as port parity (the setting's own tooltip says so). `_AvoidanceSidePreference` defaults to
  `PassLeft`; `Disabled` zeroes the term and is the configuration that matches stock Detour.
- **Penalty-weight defaults** (`_AvoidanceWeightDesVel`/`CurVel`/`Side`/`Toi`) mirror dtCrowd's —
  `DetourObstacleAvoidance.cpp:471-475`.

### Navmesh walls are obstacles too — the local boundary

The sampler used to score candidates against neighbouring **agents only**, which is half of what
dtCrowd scores: dtCrowd also feeds the obstacle query the navmesh boundary walls it keeps in a
per-agent `dtLocalBoundary` (`DetourCrowd.cpp:1557-1564`). Without them a candidate aimed straight
at a wall or a `UNavArea_Null` fixture hole costs *nothing*, so under neighbour pressure it is
routinely the cheapest one — `ConstrainToNavmesh` then eats the whole displacement and the agent
walks on the spot. Measured on an identical 14-NPC soak: walkers with a near-zero desired velocity
329 → 1 and into-wall pinned samples 33 → 2 once the sampler was taken out of the picture, which is
what identified the cost function rather than the clamp as the cause.

`FFragment_CrowdAgent_LocalBoundary` is the port of `dtLocalBoundary`: the nearest
`MAX_LOCAL_SEGS = 8` wall segments within the collision query range, re-queried once the agent has
travelled a quarter of that range from the cached centre (`DetourCrowd.cpp:1284-1286`). The query
runs **inside `AvoidanceSample`'s `ForEachEntity`**, so an agent that never samples never pays for
it, and it goes through `ARecastNavMesh::FindEdges` — UE's `findWallsInNeighbourhood` is
`findLocalNeighbourhood` fused with per-poly `getPolyWallSegments`, already converted to Unreal
space, so the module needs no `Navmesh` module dependency and no hand-rolled coordinate conversion
(`PathRefresh` already reaches `ARecastNavMesh` the same way). Doing a navmesh query inside a
`TParallelProcessor` is safe because `INITIALIZE_NAVQUERY` uses a **stack-local** `dtNavMeshQuery`
off the game thread and the shared one only on it (`RecastNavMesh.cpp:49-52`); the `dtNavMesh` is
only read.

Scoring is `processSample`'s segment loop verbatim (`DetourObstacleAvoidance.cpp:369-405`): per
segment either the `touch` special case or `isectRaySeg`, then `htmin *= 2` ("avoid less when facing
walls") into the SAME `tmin` the neighbour sweeps lower, so a wall and a neighbour compete for the
one time-to-impact term. Two per-frame filters read the agent's current position while the segments
themselves stay cached — dtCrowd's back-face filter (`DetourCrowd.cpp:1561`, only walls whose
walkable side faces the agent) and `prepare()`'s `touch` precompute, which is per agent and never
per candidate.

**The query range is not the influence radius, and the gap is deliberate.** `isectRaySeg` bounds its
ray parameter to `[0, 1]`, and the ray direction is a candidate VELOCITY, so a wall further away than
`_MaxSpeed × 1s` (240uu at defaults) is never intersected by any candidate and contributes nothing.
The 504uu query range exists so the CACHE still holds the walls the agent will care about after it
has moved, not because walls that far away are scored. Practical consequence when reading the
existing crowd suite: only agents within ~240uu of a navmesh boundary can behave differently at all,
which is why fixtures parked 250uu+ inside a ±1000uu volume are unaffected by this change.

**The side semantics are derived, not assumed** (`MakeWallOutwardNormal`, and the derivation lives
in that comment because getting it backwards would penalise exactly the candidates that escape a
wall): a segment is emitted in its source polygon's vertex winding, Recast winds a polygon so the
interior is the `area2 < 0` side, and `Unreal2RecastPoint` negates *both* horizontal axes — so
`(-(End - Start).Y, (End - Start).X)` is Detour's `snorm` exactly, and the 2D dot and cross are
sign-identical across the two spaces. Not ported: UE's own `TooCloseToSegmentDistPct` sample
rejection (returns a `-1` penalty that invalidates the candidate outright) — stock Detour has no
such branch and it would need a candidate-invalidation channel through `FCandidateScore`. Ported
from UE rather than stock Detour: `dtLocalBoundary`'s 50uu height filter, so a wall at the far end
of a ramp cannot act as a wall right here.

Settings: `_AvoidanceWallSegments` (default `Enabled`; `Disabled` restores the old agent-only
scoring in one config line, for A/B only) and `_AvoidanceWallQueryRangeMultiplier` (default 12.0 ×
agent radius, dtCrowd's `collisionQueryRange`, `CrowdFollowingComponent.cpp:43`). The sampler's
existing zero-neighbour early-out is unchanged, so a lone agent walking a corridor still does no
wall query — walls only matter here because neighbour pressure is what makes an into-wall candidate
look cheap. `DiagAvoidanceScoreTap` scores against the same walls so its trace still matches the
sampler's choice, but it runs `RunBefore AvoidanceSample` and therefore reads the boundary as the
PREVIOUS sampled frame left it; on a refresh frame its penalties can differ slightly from the ones
the sampler actually used.

**If you are tempted to add an impatience timer, a stagnation term, a minimum-speed floor, or
randomised jitter to break a deadlock: don't.** dtCrowd has none of these, and reaching for one means
the cost function is broken somewhere above. UE's answer to a genuinely blocked agent is a *higher
tier* — block detection → re-path → abort the move → let the caller decide — and this module HAS
that tier (`BlockDetect` / `BlockedRecheck`, see "Blocked goals"). Stagnation belongs there, where it
sees the whole path, and never in the per-frame velocity scoring.

---

## Facing — the body does NOT transcribe the desired-velocity heading

`FProcessor_CrowdAgent_FaceAngle` (grounded, yaw) and `FProcessor_CrowdAgent_FaceAngle3D` (flying,
yaw + pitch) are the only rotation writers, and both read the same input the avoidance sampler
writes: `FFragment_CrowdAgent_DesiredVelocity`. That input is **not** a facing signal. It is the
sampler's per-frame winner among candidate velocities, it flips between candidates a frame or two at
a time under neighbour pressure, and its heading is scale-free — it slews at the full turn rate
whether the agent is travelling at 240 cm/s or pressing at 2 cm/s into a body it cannot pass. A
processor that chased it unconditionally made a jammed agent **whip on the spot at `_MaxTurnRate`
while its feet went nowhere** (measured: 11 sign reversals and 276° of total yaw travel in 6s, with
the input heading peaking at 152.9° flips). Two filters stand between that input and the body.

**A speed floor with hysteresis decides whether facing tracks at all.** Below
`_FacingSpeedFloorCm` (project settings, default **10 cm/s**) facing DISENGAGES: no target update, no
rotation request, the body simply holds. It RE-ENGAGES only above **2× the floor**
(`FacingEngageHysteresisMultiplier`, a file-local constant) — a bare threshold is a cliff an
oscillating speed toggles across every frame, which would reintroduce exactly the churn the floor
removes. The 10 cm/s default is `_BlockedStationarySpeedThreshold`, so facing and block detection
agree on which agents are standing still; `0` disables the floor. The grounded twin gates on planar
speed (yaw is independent of vertical motion); the flying twin gates on 3D speed and holds **pitch**
under the same gate, so a hovering flyer chases neither axis.

**A persistence filter decides whether a large heading change is believed.** While engaged, a new
heading within `TargetTrackToleranceRad` (~15°) of the committed target is accepted immediately —
that is a genuine gradual turn, and the tolerance sits below the sampler's 22.5° minimum winner-flip
quantum precisely so ordinary turning is never delayed. A larger change is held as a **candidate**
and committed only after it repeats within tolerance for `TargetPersistFrames` (3) consecutive
frames; a frame matching neither the committed target nor the standing candidate restarts the
bucket. A sampler flip lasts 1-2 frames and therefore never commits; a real 90° path corner pays
~50ms of commit latency and then slews normally. The slew itself is untouched: `_MaxTurnRate`
toward the committed target, exactly as before.

**A dead-band decides whether a small heading change moves the target at all.** The tolerance
accept above fires EVERY frame, so sub-tolerance heading noise — separation/blend wobble under
neighbour pressure, well below the sampler's flip quantum — used to be transcribed to the body at
the full turn rate: the residual micro-jitter a moving crowd still showed once the big flips were
persistence-filtered (PIE-observed 2026-08-20 at 100+ agents; raising `_FacingSpeedFloorCm` cannot
fix it and only freezes facing on slow movers). Changes within `_FacingDeadBandDeg` (project
settings, default 6°, 0 disables) of the committed target now leave it untouched, so the body
converges on its facing and stays; a genuine slow arc accumulates past the band within a few
frames and tracks normally, trailing the true heading by at most the band. Keep the band below the
15° tolerance — at or above it every turn pays the persistence filter's commit latency instead. An
in-band frame also resets the pending candidate bucket, exactly as any other non-matching frame
does. Coverage: `Test_Crowd_FaceAngle_DeadBand.cpp` (algorithm), the Facing AutoTest (contract).

**On re-engagement the committed target is seeded from the body's CURRENT orientation.** Without
that seed the target the filter starts from is stale — 0 at first engage, or the heading of a
journey that ended pointing somewhere else — and the persistence window becomes a window in which
the body slews the *wrong way* before the fresh heading commits: a visible twitch at every move
start. Seeding is not a hole in the flip filter, because the seed is the body's own stable rotation
and never the noisy heading: facing simply resumes from where the agent already points, and the
filter still decides when it may begin turning away from there. Only yaw needs it — pitch has no
persistence filter and commits fresh on every engaged frame, so it has no stale window.

Both filters live in `CkCrowdAgent_FaceAngle_Algorithm.h` and their state (`_FacingEngaged`, the
pending yaw and its frame count) on `FFragment_CrowdAgent_FaceAngle`. **There is no path that writes
`_TargetYaw` without passing both**, and `Get_TargetYawDegrees` / `Get_TargetPitchDegrees` therefore
now report the COMMITTED facing target rather than the raw heading — which is what a caller, a
debugger row or an animation graph actually wants to read.

**The upstream oscillation is deliberately still there.** This tier fixes the body's TRACKING of a
noisy heading, not the noise; the desired velocity swinging under neighbour pressure is the
sampler's business and is a separate piece of work. Do not "fix" a facing complaint by damping
Steering or the sampler from here. Coverage:
`CkAutoTest_Crowd_Facing_CalmWhilePressingBlockedGap`.

---

## Anti-patterns

- **Never write SceneNode position from a steering processor.** The pipeline is `Steering → DesiredVelocity → VelocityBridge → FFragment_Velocity_Current → EulerIntegrator → PendingDisplacement → ConstrainToNavmesh → SceneNode`. Skipping any step is a bug.
- **Never write a crowd agent's Transform position from anywhere but the agent's one displacement drain** — `ConstrainToNavmesh` for a grounded agent, `ApplyDisplacement3D` for a flying one (the two views are disjoint on `FTag_CrowdAgent_Flying`, so it is still exactly one writer per agent). A second writer bypasses the navmesh constraint and re-opens the through-the-wall bug. New displacement sources accumulate into `FFragment_CrowdAgent_PendingDisplacement` instead. Rotation is a separate concern with its own single writer per agent (`FaceAngle` / `FaceAngle3D`) and does not compete with either.
- **Never enqueue MoveTo from a client.** Server-authoritative. `Request_MoveTo` checks authority.
- **Never bypass `_MaxNeighborsForSteering`.** It's the perf cliff — a careless "let me just look at all 30 neighbors" inside a custom processor will tank stress runs.
- **Don't read `FFragment_Velocity_Current` to drive steering decisions.** Read `FFragment_CrowdAgent_DesiredVelocity` (the steering output) or compute fresh. The current velocity is a frame behind and includes the velocity clamp.
- **Don't add a new flag bit beyond bit 31.** It's a `uint32`; the bitfield is documented; pick a reserved slot.

---

## Limitations / known issues

- **No queue ownership logic.** `CkQueue` owns admission, tickets, origins, slots, limits, slot claim/settle/reacquire radii, and planner-facing events. `CkCrowd/Public/CkCrowd/Queue` is only the optional framework adapter that translates a revisioned Queue assignment into a correlated `MoveTo` and reports its outcome. It reports `Reached` at the Queue claim radius, continues the owned move to the tighter settle radius, and reacquires a claimed slot only after displacement crosses the Queue reacquire radius; the assignment revision is still reported at most once. Ordinary queue retargets issue a replacement `MoveTo` without `Stop`, preserving Crowd velocity across a still-owned route change. A displaced claimed member is the deliberate hard takeover: `Stop` clears external/outward momentum before station keeping returns it. While an agent is queued, the adapter reacquires the assignment if another movement consumer replaces or stops its episode; use Queue suppression or leave before deliberately handing that agent to another locomotion owner. Counter clumping without `CkQueue` remains the expected behavior of separation-force-only avoidance.
- **NPCs are non-blocking to the player.** They have no `UPrimitiveComponent`, so the player's `UCharacterMovementComponent` has nothing to collide with. The player can walk through any NPC that soft-push didn't displace in time. This is the design — the only mitigation in scope is the soft-push amplification when the player is a separation-neighbor. Hard pushback (an NPC-side collider that physically blocks the player) is a future-work item, not a bug.
- ~~No stuck / block detection~~ — **BUILT (2026-07-14).** See "Blocked goals" below. Older copies of this doc claimed a `ProgressEval`/`TriggerReplan` tier existed when nothing did; a real one now does, under different names.
- **PushApart does NOT guarantee zero interpenetration.** It is a faithful dtCrowd port: resolve factor 0.7, 4 iterations — deliberately under-relaxed, so it does not fully resolve overlap in a single frame. Under sustained inward pressure (e.g. N agents driving at one shared point) a small residual overlap is expected and is not a bug. Stock UE ships dtCrowd's version *disabled* entirely and relies on physics capsules instead.
- **~~No ORCA~~ — the avoidance IS a velocity-obstacle sampler** (dtCrowd's, RVO-reciprocal). This line used to say "separation-force avoidance is good enough; ORCA is a planned upgrade", which misled readers into believing the crude repulsion force was the whole avoidance system. See the Avoidance section above.
- **No flow-field follower.** Waypoint-only at 150 agents is fine; flow-field is overkill for our scale.
- **No off-mesh links / jumps.** Rental store has no jumps.
- **No agent-following mode.** "Customer follows employee" would be a nice-to-have; planned post-ship.
- **Single local player only.** Multi-local-player would need one proxy per controller; works architecturally, never tested.
- **Player Proxy is server-only.** Client proxies exist but are not consulted by client-side steering (which only smooths replicated transforms).

## Future work

- ORCA / RVO2 avoidance as a swappable separation-replacement.
- Flow-field follower for wide formations (e.g., parade scenarios).
- Agent-following / leashing.
- Off-mesh link traversal (jumps, ladders).
- Async path queries.
- Additional queue layout algorithms and movement adapters belong behind `CkQueue`'s existing extension seams; CkCrowd must not absorb queue ownership.
- Hard pushback collider on NPCs (`CkOverlapBody`-based) so the player physically can't pass through. ~0.5 day; intentionally deferred.

## Implementation notes

Relocated from code comments — the processors are deliberately comment-light and this doc owns the
rationale.

### AccelClamp — the ramp is vector-space, on purpose

Steering writes the **raw** target velocity; `FProcessor_CrowdAgent_AccelClamp` brings it into the
per-frame budget. It replaced a per-frame scalar `PreviousSpeed` clamp inside Steering, which bounded
magnitude only and left direction free to snap.

- **Direction and magnitude are decoupled deliberately.** A linear vector lerp *dips* in magnitude
  through a direction change — lerping (240,0) toward (0,240) passes through (120,120), magnitude
  170, so agents visibly slow ~30% during a 90° turn. Pedestrian NPCs must rotate at `_MaxTurnRate`
  while holding pace; speed changes at `_MaxAcceleration` independently. Only the recombined
  magnitude+direction is written back.
- Mirrors `DetourCrowd.cpp` `integrate()` :53-69 — capping `|dv| <= MaxAccel * dt` is what kills the
  head-on vibration mode, since Steering writes a fresh `Direction * TargetSpeed` every frame and the
  direction can flip arbitrarily. (dtCrowd's matching separation clamp: dvel rebuilt at full maxSpeed
  every frame `DetourCrowd.cpp:1468-1469`, opposing separation clamped to pure lateral :1499-1510.)
- `UCk_Crowd_ProjectSettings_UE::_AccelClampMode = Disabled` exists **for A/B comparison only** —
  production leaves it Enabled; disabling it restores the snap-flips that drive vibration.
- **Order of the two fallbacks matters.** Newly spawned agents have `LastSpeed ≈ 0`, and falling back
  to `FVector::ForwardVector` for `LastDir` produced a visible rotate-from-world-+X glitch on the
  first frame after path resolve. So `NewDir` is computed *before* `LastDir`, and `LastDir` falls back
  to `NewDir`.

### Processor scheduling details

- **`FProcessor_CrowdAgent_ApplyOffset`** is a pattern replication of `FProcessor_Projectile_Update`.
  It shares `MarkedDirtyBy` (`FTag_EulerIntegrator_NeedsUpdate`) with it; the two act on disjoint
  entity sets (agents vs projectiles), so the explicit RunAfter (EulerIntegrator_Update first) exists
  only to silence the dirty-marker-conflict advisory. `PumpPolicy` is `SkipPump` because the
  integrator's NeedsUpdate tag is sticky and a `DeltaT = 0` re-run would re-enqueue the same
  `_DistanceOffset`, doubling per-frame movement. Its `TExclude<FTag_CrowdAgent_Asleep>` is
  forward-compatible only (nothing stamps that tag today).
- **`FProcessor_CrowdAgent_OnPathResolved`** polls `FFragment_Nav_PathResult` rather than binding a
  delegate per move-request: it is view-iteration driven, only touches agents actually waiting for a
  path, and the view filter excludes the entity again as soon as `PathPending` clears.
- **`FProcessor_CrowdAgent_VelocityBridge`** calls `UCk_Utils_Velocity_UE::Request_OverrideVelocity`
  (public API) rather than writing `FFragment_Velocity_Current::_CurrentVelocity` directly, so CkCrowd
  needs no cross-module friend declaration into CkPhysics.

### Path install & waypoint retirement

- **Path install skips leading corners the agent is already past** (`OnPathResolved`). A stale first
  corner arises two ways: the path was computed async while the agent kept its momentum (MoveTo
  deliberately preserves velocity through `PathPending` — worst under FollowTarget's frequent
  repaths), or the navmesh start-projection landed behind the agent. Without the skip, Steering aims
  at the behind-corner (its plane test anchors on the agent's own install location, so "crossed"
  never fires) and the agent visibly walks **backward** before turning around — the "360 at path
  start" bug.
- **Waypoint retirement** mirrors `UPathFollowingComponent::HasReachedCurrentTarget`
  (`PathFollowingComponent.cpp:1293-1302`): `dot(target - feet, segmentDir) < 0` means passed. The
  **final** waypoint is deliberately excluded (`Num()-1` bound) — an older loop could silently consume
  a final waypoint inside the arrival radius, leaving the cursor past the end with `OnGoalReached`
  never fired (agent stuck Walking at zero velocity, goal never reported).
- **A corner is only given up once the chord onward from the agent is navigable.** Both retirement
  tests are laterally BLIND — the plane through the corner is unbounded, and proximity says nothing
  about which side of the corridor the agent stands on — so an agent a few uu inside a corner
  retires it and re-aims at `Waypoints[k+1]` along a chord that cuts the `UNavArea_Null` hole the
  planner routed around. `ConstrainToNavmesh` then eats the whole displacement (measured: `clipFrac`
  1.00 against a 240uu/s desired velocity, 2.8s walking on the spot) until BlockDetect notices.
  Detour never has this problem because `dtPathCorridor` re-string-pulls from the agent's CURRENT
  position each frame; a FROZEN polyline must instead prove the next chord walkable, so both legs
  now gate on `ANavigationData::Raycast(agentPos → Waypoints[k+1])` and hold the corner (on-mesh by
  construction) when it is blocked. The raycast runs only on frames a retirement condition already
  fired, i.e. at corners. Within 3uu of the waypoint retirement is unconditional — there the chord
  IS the path segment and a ray along a boundary edge can flicker. The same gate is applied at
  install time in `SkipAlreadyPassedLeadingWaypoints` (`OnPathResolved`, `OnRouteResolved`), whose
  projection-only test has the identical blindness. **A `FTag_CrowdAgent_Flying` agent is excluded
  outright**, as is `OnVoxelPathResolved` — a volumetric corridor is not planned against Recast, so
  a Recast ray through free space would strand a flyer on a waypoint it can reach perfectly well.
  Master switch `_WaypointRetirementLineOfSight` (default `Enabled`; `Disabled` restores the
  laterally-blind behaviour, for A/B only). Worlds with no nav data are unaffected either way.
  Coverage: `CkAutoTest_Crowd_Steering_CornerRetirementKeepsAgentOnMesh`.

### Navmesh constraint, escape, and stationary markup

- **ConstrainToNavmesh is the stage the original dtCrowd port dropped.** Detour passes every
  integrated position through `dtPathCorridor::movePosition` (`DetourCrowd.cpp:1345`,
  `updateStepMove`) — a `moveAlongSurface` walk that means a dtCrowd agent **cannot leave the
  navmesh**: walls stop it and it slides. Without the stage, any lateral force (separation redirect,
  avoidance velocity, push-apart shove) moved the transform straight through a navmesh boundary,
  wall-eroded band included. `RECOVERY_EXTENT_RADIUS_MULTIPLIER = 4.0` self-healing exists so a
  one-frame corner leak cannot disable the clamp forever.
- **Grounding runs on a LEASE (`FFragment_CrowdAgent_Grounding`), never only on displacement.**
  The constraint used to early-out whenever the frame staged zero displacement, which coupled a
  stationary agent's grounding to the avoidance solver happening to emit something. For years that
  coupling held by ACCIDENT: PushApart's resolver never terminated (geometric decay toward exact
  contact), so every settled agent with a touching neighbour got sub-millimetre displacement every
  frame and was re-grounded every frame. `_PushApartSlopCm` ended the non-termination — correctly —
  and stationary agents' Z froze wherever it was: any elevation error (spawn-frame fall, long-frame
  unclamped vertical integration, ramp edges) became permanent, the agent floated, and every path
  it asked for returned NoRouteFound for the rest of the session (the BusterBlock floating-NPC
  regression, 2026-08). The lease is the deliberate replacement: `_SecondsSinceVerified` ticks
  every frame and is reset by ANY constraint pass, so a displacing frame IS a verify and costs
  nothing extra, while a resting agent is reconciled once per `_GroundingVerifyIntervalSeconds`
  (default 1s, phase-spread by entity hash so a same-frame-composed crowd doesn't verify in
  lockstep). The idle verify corrects **Z only**, past `_GroundingVerifyMinCorrectionCm` — the
  projection's nearest-poly answer carries a lateral nudge near navmesh edges, and folding that XY
  into a resting agent would creep settled formations, re-creating what the slop fixed. Cost:
  ~`StationaryPop / (Interval × FPS)` projections per frame (~5/frame at 300 stationary agents),
  vs. the 300/frame the old accident silently paid. A verify that finds no mesh within ±body
  height records `_IsOffNavmesh`/`_SecondsOffNavmesh` (`Get_IsOffNavmesh`,
  `Get_SecondsOffNavmesh`) instead of silently leaving the agent stranded — deliberately elevated
  agents are REPORTED, never pulled down (the recovery extent stays ±`Height`). Why not the
  alternatives: a settle-edge one-shot cannot retry (mid-bake spawn stays stranded forever) and
  the dominant Z-error sources have no settle edge; a "grounding dirty" tag has an unbounded
  writer set and NO writer at all when a nav tile rebuilds under a motionless agent. **Do not
  reintroduce a plain zero-displacement early-out as an optimisation.**
- **Off-mesh displacement is HELD, and recovery never lifts beyond a step.** Two follow-on
  defects the lease exposed (BusterBlock, 2026-08-25, both measured live): (1) agents have no
  gravity, so the old both-projections-fail pass-through let a walker step off a cliff edge and
  keep walking IN THE AIR at constant Z — 800+ uu of open-air travel at Z=1 over a beach at
  Z=-382, ending as a permanent hoverer; `_OffMeshDisplacementMode` (default Hold) now keeps the
  agent in place instead, reported, and block detection terminates the episode boundedly.
  (2) the 4×radius recovery searching ±Height vertically would snap an agent UP onto elevated
  navmesh islands (foliage tops, berms) it could never have walked onto — each snap raising the
  feet into reach of the next island, a ratchet measured climbing Z 3→319 in 16s;
  `_GroundingRecoveryMaxStepUpCm` (default 50) refuses lifts beyond a step (`[RECOVERY-REJECT]`),
  while downward recovery stays unlimited. A stranded agent also re-reports every 30s
  (`still OFF the navmesh after ...`), so a long session's log names its floaters without having
  caught the transition edge.
- **`Get_EscapedQueryStart` gates on PAINTED, not `_ConfirmedOnMesh`** — unlike the re-path trigger.
  The escape is pure geometry: planning from a pushed-out start is valid the moment the disc exists
  and merely arrives early when the rebake hasn't landed. Gating it on `_ConfirmedOnMesh` opened a
  paint-to-confirm window where a fresh MoveTo from inside the band planned *through*, and a repaint
  (push-apart drift) reopened that window by resetting the flag.
- **The escape is a RAY-MARCH along the lean direction** (away from the nearest disc centre), not a
  fixed-point pairwise push. For a painted *line* the discs' push-out zones overlap (spacing < 2×
  required radius), so each pairwise push lands inside the neighbouring disc's zone and the iteration
  ping-pongs between neighbours until the cap — giving up on exactly the scenario the tier exists for.
  Along a fixed ray each convex disc is exited at most once, so the march terminates within one step
  per disc.
- **`FFragment_CrowdAgent_NavMarkup` stillness sampling is windowed, not instantaneous**, specifically
  so a PushApart shove spike cannot unpaint a standing queue. `_ConfirmedOnMesh` exists because tile
  rebuild latency is unbounded under churn — a fixed settle timer alone let the one-shot re-path fire
  against the *pre*-rebuild mesh, return the same straight path, and burn the paint serial.
- **PushApart port provenance:** a direct port of `DetourCrowd.cpp` `updateStepMove` :1601-1662, with
  `COLLISION_RESOLVE_FACTOR` from :1599.

### Query API notes

- **`Get_SeparationForce` is the only assertion surface for an agent's neighbour-detection VOLUME.**
  The probe is Jolt-side geometry and a defect in it (the Y-axis cylinder incident) is invisible to
  every behavioural crowd test we own. It reads on an *idle* agent because the separation processor
  excludes only `FTag_CrowdAgent_Asleep`, not Idle.
- **`Get_CurrentWaypointIndex`** is meant to be paired with the path's waypoint list — e.g.
  `UCk_Utils_PathNetworkFollower_UE::Get_RouteResult`'s compiled waypoints, which
  `InstallExternalPath` copies verbatim into the nav path result — to say exactly *which* point the
  agent is steering at.
- **`Get_IsStationaryMarkupConfirmed`** is false while the agent is moving, before
  `_StationaryMarkupDelaySeconds` elapses, and while the async navmesh tile rebake is still in flight.

### Debug & diagnostics

- **Settings, not CVars, for the persistent toggles.** `ck.Crowd.Debug` and `ck.Crowd.DrawBreadcrumbs`
  are now UPROPERTYs on `UCk_Crowd_DebugSettings_UE` (`Settings/CkCrowd_DebugSettings.h`), read via the
  settings BPFL, so they persist across editor sessions via `EditorPerProjectUserSettings.ini`. The
  CVar *names* keep their historical spellings (`ck.Crowd.Debug`, `ck.Crowd.DrawBreadcrumbs`,
  `ck.Crowd.DrawPlannedPaths`, plus the newer `ck.Crowd.Debug.AgentBody`) so console muscle memory
  keeps working. File-scope CVar defaults mirror the UPROPERTY defaults and are overridden by
  `PostInitProperties` hydration once the CDO exists; each CVar change callback writes the new value
  back into the CDO UPROPERTY and `SaveConfig`s, so a console-driven change persists. The
  CrowdDebugger's toolbar checkboxes resolve the same CVars via
  `IConsoleManager::FindConsoleVariable`, so a checkbox flip flows through the CVar callback into the
  settings and checkbox state stays aligned with persisted state.
- **`ck.Crowd.SelectedEntityId`** (-1 = none) survives as a real CVar: the CrowdDebugger writes the
  selected agent's `GetTypeHash` there, and every draw processor renders that agent *regardless* of the
  global toggles.
- **Identity colour.** `FFragment_CrowdAgent_DebugColor` is opt-in — only present on agents that
  called `Set_DebugColor`, so production agents carry zero overhead. `Get_DebugColor` falls back to
  `UCk_Utils_LinearColor::Get_StableColorFromHash`, so every visualisation (body capsule, breadcrumb,
  planned-path overlay, debugger Agent List swatch) reaches the same colour for the same entity and
  untracked agents still render distinguishably. `FFragment_CrowdAgent_DebugBody`'s
  `_LastAppliedColor`/`_LastAppliedVisible` exist so `DrawBody_Update` only pushes `Set_Color`/
  visibility requests to the PMG handles when the state-tinted value actually changes.
- **Agent debug body** (`FProcessor_CrowdAgent_DrawBody_Setup`): the cone spawns with
  `ECk_Pmg_ConeOrientation::Forward`, which bakes the apex along +X — this replaced an earlier
  `Pitch = -90` SceneNode workaround; the orientation now lives in the mesh. Capsule/cone proportions
  are the ones the old inline gym viz used, derived from agent radius/height so any agent sizing flows
  through. Ring/body colours mirror the CkCrowd Debugger mockup legend.
- **DebugDraw arrow scaling is aesthetic.** Separation force magnitudes reach several hundred with
  overlapping neighbours at weight 2.0 and would dwarf the agent at 1:1, so the arrow uses
  `VisualScale` 0.5 capped at `MaxArrowLength` 300. The circle uses 24 segments (smooth without
  spamming line calls) anchored at the feet to show the floor-projected force zone. Neighbour lines
  are drawn before the arrow so the arrow paints on top. Immediate-mode `DrawDebug` is chosen over PMG
  entity overlays because it is always-current, never-persistent, and allocates nothing per tick.
- **Diag recorder sizing.** `ck.Crowd.SampleHz` defaults to 10Hz — over a 9s diag-gym cycle that is
  ~90 samples per agent, cheap in memory and trimmed when the agent destructs at cycle end. Consumers
  are the diagnostic gym's `EmitDigest` and the debugger's world-draw breadcrumb overlay.
  `_StartPos`/`_GoalPos` are captured at `Track()` time so efficiency (`path_len / straight`) is
  computable at digest time without re-querying the agent. Breadcrumbs are lifted
  `BreadcrumbLiftZ = 96` (the diag gym's capsule half-height) purely for display; recorder data stores
  the unaltered transform position.
- **Diag digest** (`UCk_Utils_CrowdAgent_Diag_UE::EmitDigest_ForAgent`) — one Display line each,
  prefix `[CrowdDiag][C{cycle}][{station}][A{idx}]`:
  `start=(x,y,z) goal=(x,y,z)`; `reached={true|false} t_to_goal={s}`;
  `path_len={cm} straight={cm} efficiency={0..1}` (efficiency = straight/path_len, 1.0 = perfect
  line); `min_sep_to_neighbors={cm} at t={s}` (-1 sentinel = no neighbours observed in the window);
  `dir_reversals={n} max_angular_delta={deg}`; then one `simplified_path: t= x= y= z= v=` per
  RDP-kept sample. Z is emitted so floor-clip bugs (root dropping below the floor) are grep-visible,
  not just visual. Display verbosity is deliberate: the lines land in `Saved/Logs/CkTests.log` without
  bumping `LogCk_Crowd` to Verbose.
- **`ck.Crowd.RDPEpsilon` default 8cm** — chosen so a straight head-on test yields ~2-3 keypoints and a
  curving cluster path ~10-20: enough to read the path shape, light enough to grep without paging.
  Lower = more keypoints retained; higher = more aggressive collapse.

### `ck.Crowd.DiagNavClip` — why an agent's movement is not landing

`FProcessor_CrowdAgent_DiagNavClip` (default 0, `ECVF_Cheat`) answers "is the navmesh clamp eating
this agent, and which wedge is it in" without touching the clamp. It sits in `FGroup_Physics`
between PushApart and ConstrainToNavmesh — it MUST read `FFragment_CrowdAgent_PendingDisplacement`
before the clamp zeroes it — and replicates the clamp's math read-only (same NavData, same
projection extents including the 4x recovery widening, same `FindMoveAlongSurface` walk). Walking
AND PathPending agents are sampled; Idle, Asleep and Flying are not. Its state fragment
(`FFragment_CrowdAgent_DiagNavClip`) is added lazily on the first enabled frame, so an off CVar
costs one CVar read and nothing else.

A frame counts as clipped when `staged >= 0.5uu` and either the surface walk ate it
(`clipFrac >= 0.75`, or the walk/projection failed) or the walk was demonstrably fine yet almost
nothing landed (`applied < 0.15 * staged` with `clipFrac < 0.5`) — the latter is `ExternalHold`, and
it accuses a second Transform writer rather than the clamp. `applied` is measured against the
position held at the PREVIOUS sample, so it is one frame stale; irrelevant at the half-second
granularity an episode needs. 0.5s of consecutive clipped frames opens an episode (`START`),
which re-reports every 2s (`HOLD`) and closes on the first unclipped frame (`END`, carrying the
duration and the per-agent episode count). One `Log`-verbosity line each, prefixed `[CrowdNavClip]`:

```
[CrowdNavClip] START agent=<handle> state=<Walking|PathPending> pos=X= Y= Z= staged=X= Y= (uu)
  desired=X= Y= Z= (uu/s) projectOk=<0|1> recoveryOk=<0|1> moveOk=<0|1> clipUu= clipFrac= applied=
  wp=<idx>/<num> curWp=X= Y= Z= segStart=X= Y= Z= goal=X= Y= Z= pathStatus=<Ready|Partial|Pending|Failed|None>
  rayAgentToWp=<blocked|clear|na> raySegStartToWp=<...> rayAgentToGoal=<...>
  class=<OffMesh|SurfaceWalkFailed|ExternalHold|PathCrossesBoundary|SteeringOffPath|Other>
[CrowdNavClip] HOLD  <same fields> dur=<s>
[CrowdNavClip] END agent= state= pos=X= Y= Z= staged= applied= dur=<s> episodes=<n>
```

The three rays are `ANavigationData::Raycast` on the same NavData (true == BLOCKED, `na` when the
waypoint index is invalid or the agent has no path result); they are what separates "the planner's
corridor crosses a navmesh boundary" (both agent→wp and segStart→wp blocked) from "steering has
wandered off the corridor" (agent→wp blocked, segStart→wp clear).

## See also

- [CkNavigation/Claude.md](../CkNavigation/Claude.md) — path query layer
- [CkCrowdDebugger/Claude.md](../../../../CkGameplayDebugger/Source/CkCrowdDebugger/Claude.md) — diagnostic UI
- [CkPhysics/EulerIntegrator/](../CkPhysics/Public/CkPhysics/EulerIntegrator/) — position integrator
- [CkSpatialQuery/](../CkSpatialQuery/) — neighbor query backend
