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
// Add the crowd-agent feature to any entity.
static FCk_Handle_CrowdAgent Add(
    UPARAM(ref) FCk_Handle& InOwner,
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
  FProcessor_CrowdAgent_ApplyOffset    ← writes SceneNode
  FProcessor_CrowdAgent_PushApart      ← post-integration de-overlap (dtCrowd port: 0.7 factor,
                                         4 iterations — deliberately UNDER-RELAXED, so it does NOT
                                         guarantee zero interpenetration in a single frame)
       │
       ▼
[Replication]
  Server transform → client smoothing  ← FProcessor_Transform_InterpolateToGoal_Location
```

**NOT BUILT** (documented here for years, never implemented — do not look for them):
`SleepEvaluator`, `Piercing`, `ProgressEval`, `TriggerReplan`. There is **no stuck/no-progress
detection anywhere in this module**. Note that stock UE doesn't solve deadlock in its solver either —
dtCrowd has no impatience/stagnation/min-speed mechanism at all; UE detects a blocked agent one tier
UP (`UPathFollowingComponent` block detection: 10 feet-samples at 0.5s intervals, all within 10cm of
their centroid ⇒ `OnPathFinished(Blocked)`) and hands it to the behaviour tree. **We have no
equivalent tier.** If an agent cannot make progress, nothing notices. That is a known gap.

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

`FFragment_CrowdAgent_PathFollow` also carries `_CurrentSegmentStart` — the world-space start of the
current path segment. Steering's plane-crossing waypoint retirement needs the *incoming* segment
direction, and CkNavigation's `ExtractWaypoints` strips the path's start point, so `Waypoints[-1]`
does not exist for the first segment. Captured at both path-install sites.

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
FTag_CrowdAgent_Asleep            # DEFINED AND EXCLUDED, BUT NOTHING EVER STAMPS IT (see below)
```

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
| `_SeparationInertia` | 0.5 | Lerp toward last frame's separation force; kills frame-to-frame flicker. |
| `_MaxNeighborsForSteering` | 6 | Top-N cap (sorted by distance). |
| `_CollisionFlags` / `_IgnoreFlags` | -1 / 0 | Present on the struct; **no processor currently reads them.** |

Avoidance-sampler tuning (velBias, penalty weights, sample density, trigger/stride) lives in
`UCk_Crowd_ProjectSettings_UE` (`Settings/CkCrowd_ProjectSettings.h`), not on the agent.

---

## Patterns

### Spawn an agent

```cpp
auto Params = FCk_Fragment_CrowdAgent_ParamsData{42.f, 192.f};
Params.Set_MaxSpeed(240.f);
Params.Get_Tags().AddTag(FGameplayTag::RequestGameplayTag(TEXT("Crowd.Agent")));
auto Agent = UCk_Utils_CrowdAgent_UE::Add(OwnerEntity, Params);
```

### Move it somewhere

```cpp
auto Request = FCk_Request_CrowdAgent_MoveTo{TargetWorldLoc};
UCk_Utils_CrowdAgent_UE::Request_MoveTo(Agent, Request, OnReachedDelegate, OnFailedDelegate);
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

**If you are tempted to add an impatience timer, a stagnation term, a minimum-speed floor, or
randomised jitter to break a deadlock: don't.** dtCrowd has none of these, and reaching for one means
the cost function is broken somewhere above. UE's answer to a genuinely blocked agent is a *higher
tier* (block detection → abort the move → let the behaviour tree decide), which this module does not
have.

---

## Anti-patterns

- **Never write SceneNode position from a steering processor.** The pipeline is `Steering → DesiredVelocity → VelocityBridge → FFragment_Velocity_Current → EulerIntegrator → SceneNode`. Skipping any step is a bug.
- **Never enqueue MoveTo from a client.** Server-authoritative. `Request_MoveTo` checks authority.
- **Never bypass `_MaxNeighborsForSteering`.** It's the perf cliff — a careless "let me just look at all 30 neighbors" inside a custom processor will tank stress runs.
- **Don't read `FFragment_Velocity_Current` to drive steering decisions.** Read `FFragment_CrowdAgent_DesiredVelocity` (the steering output) or compute fresh. The current velocity is a frame behind and includes the velocity clamp.
- **Don't add a new flag bit beyond bit 31.** It's a `uint32`; the bitfield is documented; pick a reserved slot.

---

## Limitations / known issues

- **No queueing logic.** The contract is "move agent to a target." Queue managers (slot reservation, "you're 3rd in line", line-up at counter) are gameplay-side — typically implemented in GOAP or per-gym fragments that pick the target the agent steers toward. Counter clumping in this module's purview is the *expected* behavior of separation-force-only avoidance; lining up is a different concern.
- **NPCs are non-blocking to the player.** They have no `UPrimitiveComponent`, so the player's `UCharacterMovementComponent` has nothing to collide with. The player can walk through any NPC that soft-push didn't displace in time. This is the design — the only mitigation in scope is the soft-push amplification when the player is a separation-neighbor. Hard pushback (an NPC-side collider that physically blocks the player) is a future-work item, not a bug.
- **No stuck / block detection — the biggest gap.** Nothing in this module notices an agent that stops making progress. There is no `ProgressEval`, no `TriggerReplan`, no `OnGoalBlocked` signal (older versions of this doc claimed all three). If an agent cannot reach its goal it will keep trying forever, silently. UE's equivalent tier is `UPathFollowingComponent` block detection (10 samples × 0.5s within 10cm ⇒ abort the move as `Blocked` and hand it to the behaviour tree). Worth building.
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

## See also

- [CkNavigation/Claude.md](../CkNavigation/Claude.md) — path query layer
- [CkCrowdDebugger/Claude.md](../../../../CkGameplayDebugger/Source/CkCrowdDebugger/Claude.md) — diagnostic UI
- [CkPhysics/EulerIntegrator/](../CkPhysics/Public/CkPhysics/EulerIntegrator/) — position integrator
- [CkSpatialQuery/](../CkSpatialQuery/) — neighbor query backend
