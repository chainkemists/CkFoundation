# CkCrowd

> **Module status:** ⏳ Not yet created — first lands in [Gate 0](../CkNavigation/PLAN.md), grows through [Gate 7](../CkNavigation/Plan/Gate_07_RentalStore.md). This file describes the *target* shape.

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
[CkCrowd: per-frame steering loop]
  FProcessor_CrowdAgent_NeighborSync   ← reads probe overlaps
  FProcessor_CrowdAgent_SleepEvaluator ← decides asleep
  FProcessor_CrowdAgent_Piercing       ← decides piercing pairs
  FProcessor_CrowdAgent_Separation     ← computes separation force
  FProcessor_CrowdAgent_Steering       ← combines path-follow + separation → desired velocity
  FProcessor_CrowdAgent_VelocityBridge ← writes FFragment_Velocity_Current
  FProcessor_CrowdAgent_FaceAngle      ← lerps yaw to face desired velocity
  FProcessor_CrowdAgent_ProgressEval   ← did the agent progress?
  FProcessor_CrowdAgent_TriggerReplan  ← stuck → re-fire FindPath
       │
       ▼
[CkPhysics]
  FProcessor_Velocity_Clamp            ← min/max enforcement
  FProcessor_EulerIntegrator_Update    ← position += velocity * dt
  → FFragment_EulerIntegrator_Current
       │
       ▼
[CkPhysics or bridge]
  ApplyOffset processor                ← writes SceneNode (validated in Gate 2)
       │
       ▼
[Replication]
  Server transform → client smoothing  ← FProcessor_Transform_InterpolateToGoal_Location
```

---

## Fragments owned by an agent

| Fragment | Purpose | Added by |
|---|---|---|
| `FFragment_CrowdAgent_Params` | Reflected params (radius, height, max speed, separation weight, flags, etc.) | `Add()` |
| `FFragment_CrowdAgent_PathFollow` | Current waypoint index, arrival radii | `Add()` |
| `FFragment_CrowdAgent_DesiredVelocity` | Steering output | `Add()` |
| `FFragment_CrowdAgent_NeighborCache` | Per-frame trimmed list of nearby agents | NeighborSync |
| `FFragment_CrowdAgent_SeparationForce` | Computed force vector (+ active flag) | Separation |
| `FFragment_CrowdAgent_PiercingPairs` | Neighbors currently in piercing state with this agent | Piercing |
| `FFragment_CrowdAgent_SleepState` | Idle accumulator | SleepEvaluator |
| `FFragment_CrowdAgent_ProgressTracker` | Last progress timestamp + distance | ProgressEval |
| `FFragment_CrowdAgent_FaceAngle` | Current/target yaw | Add() |
| `FFragment_CrowdAgent_MoveRequests` | Variant of pending request types | Per-tick (drained) |

A child entity hosting the Cylinder + Probe is spawned on agent setup. It carries the `Crowd.Agent` gameplay tag for filtering.

---

## Tags

```
FTag_CrowdAgent_NeedsSetup        # Setup pending
FTag_CrowdAgent_HasProbe          # Probe child spawned
FTag_CrowdAgent_Walking           # Has goal + path; steering updates desired vel
FTag_CrowdAgent_Idle              # No goal
FTag_CrowdAgent_PathPending       # FindPath in flight
FTag_CrowdAgent_Asleep            # Excluded from steering / separation / piercing views
FTag_CrowdAgent_Failed            # Permanent failure (path failed N times)
FTag_CrowdAgent_IsObstacleOnly    # Marker for player proxy (steering does not apply)
```

The Asleep tag is the central perf optimization: any processor that can ignore asleep agents
adds `TExclude<FTag_CrowdAgent_Asleep>` to its view.

---

## Player Proxy (sub-feature)

The player remains an `ACharacter`. A **Player Proxy Entity** mirrors the player's transform
+ velocity into the steering layer each frame. NPCs see the proxy as a regular neighbor.

```cpp
// Once per local player, server-side, on PlayerController BeginPlay:
UCk_Utils_PlayerProxy_UE::Add(MyPlayerController);
```

The proxy entity has `FTag_CrowdAgent_IsObstacleOnly` — steering doesn't compute desired
velocity for it, but other agents include it in their neighbor scan. Soft-push amplification
lives in the separation processor: when a neighbor's `_Flags` includes `PLAYER_PROXY` AND
the agent has `_PlayerYieldEnabled`, the contribution is multiplied by `_PlayerYieldMultiplier`.

---

## Flags bitfield

```
AGENT          = 1 << 0   // standard NPC
EMPLOYEE       = 1 << 1   // employee-tagged NPC
PLAYER_PROXY   = 1 << 2   // the proxy entity
RESERVED_3..31           // future use
```

Used in `_Flags` (this agent's identity) and `_IgnoreFlags` (which neighbor flags to skip
during steering — *not* during the probe scan; the probe still finds them but steering
ignores their force contribution).

---

## Tunables Reference

Defaults below are post-Gate-6 tuned values. Each is a `UPROPERTY` on `FCk_Fragment_CrowdAgent_ParamsData`.

| Tunable | Default | Range | Purpose |
|---|---|---|---|
| `_Radius` | 42 cm | 20–80 | Cylinder radius for neighbor + path. |
| `_Height` | 192 cm | 100–300 | Cylinder height. |
| `_MaxSpeed` | 240 cm/s | 100–600 | Walking speed. |
| `_MaxAcceleration` | 480 cm/s² | 200–1200 | Ramp rate (≈ 2× MaxSpeed). |
| `_MaxTurnRate` | 4.0 rad/s | 1.0–8.0 | ≈ 1.6 s for 360° turn. |
| `_ArrivalRadius` | 30 cm | 10–80 | Final-stop tolerance. |
| `_WaypointArrivalRadius` | 25 cm | 10–50 | Per-waypoint advance tolerance. |
| `_SeparationRadius` | 100 cm | 60–200 | Distance below which separation activates. |
| `_SeparationLookahead` | 100 cm | 60–200 | Probe radius extension past separation. |
| `_SeparationWeight` | 2.0 | 0.5–5.0 | Force multiplier. |
| `_MaxNeighborsForSteering` | 6 | 3–10 | Top-N cap (sorted by distance). |
| `_PiercingAngle` | 0.5 rad | 0.2–1.0 | Min head-on angle (radians) for piercing. |
| `_PiercingActivateRadius` | 80 cm | 40–150 | Distance below which piercing engages. |
| `_SleepIdleSeconds` | 1.5 s | 0.5–5.0 | Idle time before stamping Asleep. |
| `_ReplanThresholdSeconds` | 2.0 s | 1.0–5.0 | No-progress time before replan. |
| `_ReplanProgressThreshold` | 5.0 cm/s | 2.0–20.0 | Min forward progress to count as "moving". |
| `_MaxReplansPerPath` | 3 | 1–10 | After N replans, transition to Failed. |
| `_PlayerProxySoftPushRadius` | 120 cm | 80–200 | Distance below which player-yield amp engages. |
| `_PlayerYieldMultiplier` | 2.0 | 1.0–4.0 | Multiplier on player-proxy separation contribution. |

Project-level overrides live in `UCk_Crowd_ProjectSettings_UE` for the global defaults; per-agent overrides live on the params struct.

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

## Anti-patterns

- **Never write SceneNode position from a steering processor.** The pipeline is `Steering → DesiredVelocity → VelocityBridge → FFragment_Velocity_Current → EulerIntegrator → SceneNode`. Skipping any step is a bug.
- **Never enqueue MoveTo from a client.** Server-authoritative. `Request_MoveTo` checks authority.
- **Never bypass `_MaxNeighborsForSteering`.** It's the perf cliff — a careless "let me just look at all 30 neighbors" inside a custom processor will tank stress runs.
- **Don't read `FFragment_Velocity_Current` to drive steering decisions.** Read `FFragment_CrowdAgent_DesiredVelocity` (the steering output) or compute fresh. The current velocity is a frame behind and includes the velocity clamp.
- **Don't add a new flag bit beyond bit 31.** It's a `uint32`; the bitfield is documented; pick a reserved slot.

---

## Limitations / known issues

- **No ORCA.** Separation-force avoidance is "good enough" for rental store; ORCA is a planned post-ship upgrade if behavior breaks.
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
- Anchor/queue gameplay primitives baked into the module (currently lives in gym files).

## See also

- [CkNavigation/Claude.md](../CkNavigation/Claude.md) — path query layer
- [CkCrowdDebugger/Claude.md](../../../../CkGameplayDebugger/Source/CkCrowdDebugger/Claude.md) — diagnostic UI
- [CkPhysics/EulerIntegrator/](../CkPhysics/Public/CkPhysics/EulerIntegrator/) — position integrator
- [CkSpatialQuery/](../CkSpatialQuery/) — neighbor query backend
