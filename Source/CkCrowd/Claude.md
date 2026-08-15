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
  FProcessor_CrowdAgent_FaceAngle       ← lerps yaw to face desired velocity (yaw only)
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
| `FFragment_CrowdAgent_SeparationForce` | Computed force vector | Separation |
| `FFragment_CrowdAgent_ProbeRef` | Handle to the probe child entity | Setup |
| `FFragment_CrowdAgent_FaceAngle` | Current/target yaw | Add() |
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

Avoidance-sampler tuning (velBias, penalty weights, sample density, trigger/stride) and
stationary-markup tuning live in `UCk_Crowd_ProjectSettings_UE`
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

**`FProcessor_CrowdAgent_BlockDetect`** runs two detectors, because neither alone is sufficient:

| detector | how | catches | misses |
|---|---|---|---|
| **Geometric** (primary) | a *stationary* neighbour sits on the final waypoint such that `SelfRadius + NbrRadius` exceeds the arrival radius — so the closest the agent can physically get is further out than "arrived" | agent-occupied goals, **exactly and immediately**, naming the blocker | walls, props, multi-agent plugs |
| **No-progress** (safety net) | REMAINING PATH DISTANCE (agent → current waypoint + the polyline tail) sampled every `_BlockDetectionInterval`; a stall is `_BlockDetectionNoProgressWindowSeconds` without the windowed minimum improving by `_BlockDetectionProgressEpsilonCm` | everything else — walls, fixtures, multi-agent plugs, **and orbiting** | nothing the geometric detector is for (it names the blocker, which this cannot) |

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

**Off-path drift is healed on the same cadence.** If the agent is more than
`_BlockDetectionOffPathRepathThresholdCm` (XY) from the segment it is currently following, it is
re-pathed outright. A teleport, a save restore or an external shove otherwise leaves Steering
chasing waypoints on a corridor the agent no longer stands near, and nothing else in the pipeline
notices. Being displaced is not a stall, so this does not consume the stall budget.

The geometric detector's **engagement range is derived, not a knob**:
`(2 * SelfRadius) + ArrivalRadius + BrakingDistance + 20cm` slack, where
`BrakingDistance = MaxSpeed² / (2 * MaxAcceleration)`. Engaging at the agent's own braking distance
is the last moment it could still stop cleanly; a neighbour parked on a goal 30m away is not
blocking anything yet.

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
| `NoProgress` | bounded by `_BlockedMaxRetries`, then `OnGoalFailed` + `GoalBlocked` cleared + Idle | the obstruction is static — a wall, a fixture, a plug the planner cannot see. It never clears, so the old behaviour (`BlockedRecheck` finds no agent blocker, re-paths, re-enters the same wedge, forever) was a silent hang no caller could observe |

`BlockedRecheck` still tests for an agent blocker FIRST for both causes, so a `NoProgress` agent that
happens to have someone standing on its goal holds without spending a retry.

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

Three independent gates must all pass: triggered (project trigger mode + per-agent `AvoidancePolicy`
override + zone tags), on this agent's 1-in-N frame (round-robin stride), and at least one cached
neighbour. Scheduling: `FGroup_Physics`, RunAfter Steering (whose output it overwrites) + NeighborSync
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

- **Sample density / single depth iteration is deliberate.** `BuildSamplePattern` runs one depth
  iteration (no `adaptiveDepth` refinement) — a shipped engine configuration:
  UE's `ECrowdAvoidanceQuality::Low` is 5 divs × 2 rings + centre at velBias 0.5
  (`CrowdManager.cpp:182-187`); our default 8 × 2 + centre is denser than that. The bias centre is
  Detour's "sample at zero pattern offset" (`DetourObstacleAvoidance.cpp:551-554`) — VelBias of the
  desired velocity ("slow down"), never a true stop; there are no off-grid desired/current/zero
  anchors (see invariant 2).
- **Side preference is the agent's own choice, not a function of neighbour position.**
  `SideRightness` scores a candidate against the agent's own left-perpendicular, so two head-on
  agents that both "pass left" diverge (their lefts point in opposite world directions).
  Neighbour-relative side logic degenerates exactly in the head-on case (cross ≈ 0).
- **Penalty-weight defaults** (`_AvoidanceWeightDesVel`/`CurVel`/`Side`/`Toi`) mirror dtCrowd's —
  `DetourObstacleAvoidance.cpp:471-475`.

**If you are tempted to add an impatience timer, a stagnation term, a minimum-speed floor, or
randomised jitter to break a deadlock: don't.** dtCrowd has none of these, and reaching for one means
the cost function is broken somewhere above. UE's answer to a genuinely blocked agent is a *higher
tier* — block detection → re-path → abort the move → let the caller decide — and this module HAS
that tier (`BlockDetect` / `BlockedRecheck`, see "Blocked goals"). Stagnation belongs there, where it
sees the whole path, and never in the per-frame velocity scoring.

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

- **No queueing logic.** The contract is "move agent to a target." Queue managers (slot reservation, "you're 3rd in line", line-up at counter) are gameplay-side — typically implemented in GOAP or per-gym fragments that pick the target the agent steers toward. Counter clumping in this module's purview is the *expected* behavior of separation-force-only avoidance; lining up is a different concern.
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
- Anchor / queue gameplay primitives (currently gameplay-side; could move into the module if the queue pattern stabilizes and is reused across games).
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

### Navmesh constraint, escape, and stationary markup

- **ConstrainToNavmesh is the stage the original dtCrowd port dropped.** Detour passes every
  integrated position through `dtPathCorridor::movePosition` (`DetourCrowd.cpp:1345`,
  `updateStepMove`) — a `moveAlongSurface` walk that means a dtCrowd agent **cannot leave the
  navmesh**: walls stop it and it slides. Without the stage, any lateral force (separation redirect,
  avoidance velocity, push-apart shove) moved the transform straight through a navmesh boundary,
  wall-eroded band included. `RECOVERY_EXTENT_RADIUS_MULTIPLIER = 4.0` self-healing exists so a
  one-frame corner leak cannot disable the clamp forever.
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

## See also

- [CkNavigation/Claude.md](../CkNavigation/Claude.md) — path query layer
- [CkCrowdDebugger/Claude.md](../../../../CkGameplayDebugger/Source/CkCrowdDebugger/Claude.md) — diagnostic UI
- [CkPhysics/EulerIntegrator/](../CkPhysics/Public/CkPhysics/EulerIntegrator/) — position integrator
- [CkSpatialQuery/](../CkSpatialQuery/) — neighbor query backend
