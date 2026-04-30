# Gate 2 — Locomotion

> **Status:** ⏳ Pending
> **Day target:** D2 (afternoon) → D3
> **Parallelizable:** Limited — Sub-task 2A blocks 2B blocks 2C
> **Depends on:** Gate 0 ✅, Gate 1 ✅
> **🔴 HIGH RISK** — see "Risks / unknowns" below; this is the gate where the velocity → integrator → SceneNode chain gets validated and possibly hardened.

## Goal

A single agent walks A → B following the path produced in Gate 1. This means:
- A steering processor turns the current waypoint into a desired velocity
- That velocity gets written into `FFragment_Velocity_Current` via friend access
- The existing `FProcessor_EulerIntegrator_Update` advances position
- The SceneNode reflects the new position (this is the **part with the unknown**)
- The path-follower advances waypoints as the agent crosses them
- Arrival behavior: agent stops cleanly within `_ArrivalRadius`

After this gate: one agent can be told `Request_MoveTo(target)` and it walks there at its
configured speed, accel/decel-clamped, turning smoothly. **No avoidance yet** — that's Gate 3.

## Acceptance criteria

1. ✅ `UCk_Utils_CrowdAgent_UE::Request_MoveTo(InAgent, target)` issues a path request via CkNavigation, then on path-ready transitions the agent to a `Walking` state.
2. ✅ Agent transform updates frame-over-frame to match `Position += Velocity * dt`. Verified visually + by transform delta logs.
3. ✅ Speed ramps up from 0 to `_MaxSpeed` according to `_MaxAcceleration` (no instant jump).
4. ✅ Speed ramps down to 0 within `_ArrivalRadius` of the final waypoint.
5. ✅ Turn rate is clamped — agent does not snap-rotate when waypoint changes direction.
6. ✅ When agent crosses the threshold of one waypoint, advances to next; when final waypoint reached, transitions to `Idle`.
7. ✅ Path viz from Gate 1 stays in sync — line gets shorter as waypoints are consumed.
8. ✅ Debugger Agent Detail "Steering Forces" section populated; "Transform" section shows live velocity & desired velocity matching mockup §1.
9. ✅ AutoStation `UCk_AutoTest_Crowd_Locomotion` runs: spawn 1 agent, request move to a known target 500cm away, assert `OnGoalReached` fires within `T = 500/maxSpeed + 1.0s` margin, assert final position is within `_ArrivalRadius` of target.
10. ✅ Replicated test: AutoStation runs in a 2-client PIE, asserts client transforms reach within 50cm of server transform within 0.5s of server arrival (smoothing convergence).

## Sub-tasks (must run sequentially)

### Sub-task 2A — Validate the velocity → SceneNode chain (highest priority)

**Before writing the steering processor**, verify the existing pipeline works at small scale.

1. Add one CrowdAgent to an entity. Add a Velocity feature (`UCk_Utils_Velocity_UE::Add`).
2. Add an EulerIntegrator (`UCk_Utils_EulerIntegrator_UE::Request_Start`).
3. Manually `Request_OverrideVelocity(handle, FVector(100, 0, 0))`.
4. **Verify** the entity's SceneNode position increases by ~100 cm/sec.

If the chain works → proceed to 2B. If it doesn't, the existing pipeline has a gap and the
gate budget grows by 0.5–1 day to fix it. Possible gaps to investigate:

- The integrator computes `_DistanceOffset` but no processor enqueues a `FCk_Request_Transform_AddLocationOffset`. **Likely needed**: a new `FProcessor_EulerIntegrator_ApplyOffset` that reads `_DistanceOffset` post-integration and writes the SceneNode.
- The integrator runs in `FGroup_Physics`, transform requests run in `FGroup_Transform`; ordering is correct, but the bridge processor has to actually enqueue the request.

**If a fix is required**, it lives in `Plugins/CkFoundation/Source/CkPhysics/Public/CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.{h,cpp}` (or a new sibling file). Document the gap in CkPhysics/Claude.md.

### Sub-task 2B — Steering processor + path follower

**File:** `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Steering_Processor.{h,cpp}`

```cpp
class CKCROWD_API FProcessor_CrowdAgent_Steering : public ck_exp::TProcessor<
    FProcessor_CrowdAgent_Steering,
    FCk_Handle_CrowdAgent,
    ck::TReadOnly<FFragment_CrowdAgent_Params>,
    ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
    ck::TReadOnly<FFragment_Nav_PathResult>,           // From CkNavigation
    ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
    CK_IGNORE_PENDING_KILL>
{
public:
    using Group = FGroup_Physics;                       // Runs before EulerIntegrator
    using RunBefore = TDepList<FProcessor_EulerIntegrator_Update>;
    // ...
};
```

The steering processor:
1. Reads current waypoint from `_PathFollow._WaypointIndex`
2. Computes `Direction = normalize(NextWaypoint - Position)`
3. Computes `DesiredSpeed` based on:
   - Distance to next waypoint (decelerate near it)
   - Distance to final goal (decelerate near it)
   - Per-frame `_MaxAcceleration` clamp from current speed
4. Writes `_DesiredVelocity = Direction * DesiredSpeed`
5. Advances `_WaypointIndex` if within `_WaypointArrivalRadius`
6. Fires `OnGoalReached` signal if past final waypoint

### Sub-task 2C — Velocity bridge

**File:** `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_VelocityBridge_Processor.{h,cpp}`

```cpp
class CKCROWD_API FProcessor_CrowdAgent_VelocityBridge : public ck_exp::TProcessor<
    FProcessor_CrowdAgent_VelocityBridge,
    FCk_Handle_CrowdAgent,
    ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>,
    ck::TReadWrite<ck::FFragment_Velocity_Current>,
    CK_IGNORE_PENDING_KILL>
{
public:
    using Group = FGroup_Physics;
    using RunAfter = TDepList<FProcessor_CrowdAgent_Steering>;
    using RunBefore = TDepList<FProcessor_Velocity_Clamp, FProcessor_EulerIntegrator_Update>;
    friend class ck::UCk_Utils_Velocity_UE;             // For friend access; or use Request_OverrideVelocity
    // ...
};
```

The bridge writes `_DesiredVelocity` into `FFragment_Velocity_Current`. Velocity_Clamp then enforces min/max, EulerIntegrator advances position, SceneNode picks up the offset (per Sub-task 2A).

Optional refinement: add a per-tick velocity smoothing (lerp from current to desired by `accel * dt`) here, so steering is allowed to set "ideal" desired velocity and the bridge smooths it. This avoids steering having to know about acceleration at all. Defer that decision until tuning.

### Sub-task 2D — Turn rate / face angle

**File:** `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_FaceAngle_Processor.{h,cpp}`

Reads `_DesiredVelocity`, computes target yaw, lerps current yaw at `_MaxTurnRate` rad/sec, writes to SceneNode rotation. Independent of the velocity chain — agent orientation is decoupled from movement direction (the agent can briefly walk sideways while turning).

### Sub-task 2E — Move request handling

**Files:** `CkCrowdAgent_Utils.cpp` + new `FProcessor_CrowdAgent_HandleMoveRequests`.

Public API:

```cpp
// In UCk_Utils_CrowdAgent_UE
static FCk_Handle_CrowdAgent
Request_MoveTo(
    UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
    const FCk_Request_CrowdAgent_MoveTo& InRequest);
// _Target FVector + optional _ArrivalRadiusOverride
// + optional FCk_Delegate_CrowdAgent_OnGoalReached / _OnGoalFailed completion delegates

static FCk_Handle_CrowdAgent
Request_Stop(UPARAM(ref) FCk_Handle_CrowdAgent& InAgent);
// Cancels current move, ramps velocity to 0 over deceleration window
```

The handle-requests processor drains `FFragment_CrowdAgent_MoveRequests`, fires
`UCk_Utils_Nav_UE::Request_FindPath` on each. On path-ready signal, transitions to Walking
(state machine transition stamps `FTag_CrowdAgent_Walking`).

## File inventory (additions)

```
Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/
    CkCrowdAgent_Fragment.{h,cpp}                    # New fragments:
                                                     #   FFragment_CrowdAgent_DesiredVelocity
                                                     #   FFragment_CrowdAgent_PathFollow (waypoint index, arrival radius)
                                                     #   FFragment_CrowdAgent_MoveRequests (variant of MoveTo / Stop)
                                                     #   FFragment_CrowdAgent_FaceAngle (current yaw, target yaw, max turn rate)
                                                     # New tags:
                                                     #   FTag_CrowdAgent_Walking
                                                     #   FTag_CrowdAgent_Idle
                                                     #   FTag_CrowdAgent_PathPending
    CkCrowdAgent_Steering_Processor.{h,cpp}
    CkCrowdAgent_VelocityBridge_Processor.{h,cpp}
    CkCrowdAgent_FaceAngle_Processor.{h,cpp}
    CkCrowdAgent_HandleRequests_Processor.{h,cpp}
    CkCrowdAgent_Signals.{h,cpp}                     # OnGoalReached, OnGoalFailed signals
```

`FCk_Fragment_CrowdAgent_ParamsData` grows in this gate:

```cpp
// Additions in Gate 2:
float _MaxSpeed = 240.0f;             // cm/s — rental store default
float _MaxAcceleration = 480.0f;      // cm/s² — accel = 2x max speed = ~0.5s ramp
float _MaxTurnRate = 4.0f;            // rad/s — full 360° turn in 1.6s
float _ArrivalRadius = 30.0f;         // cm — final stop tolerance
float _WaypointArrivalRadius = 25.0f; // cm — waypoint advance tolerance
```

## Gym spec — manual

`Crowd Locomotion` gym:

- Source alcove (`Gym.Crowd.Locomotion.Source`) at world origin
- Three target alcoves (`Gym.Crowd.Locomotion.Target_Near` 300cm, `_Mid` 800cm, `_Far` 1500cm)
- Press **Space** → spawn agent at Source, request move to next target in sequence (cycles A→B→C→A)
- Press **R** → reset (despawn all agents)
- Live display on Source shows: agent count, last move duration, last `OnGoalReached` time

## Gym spec — AutoStation

Three AutoStations:

`UCk_AutoTest_Crowd_Locomotion_StraightLine`:
- Spawn 1 agent at (0, 0, 50)
- Request move to (500, 0, 50)
- Assert `OnGoalReached` fires within 3.0s timeout
- Assert final position within 30cm of (500, 0, 50)

`UCk_AutoTest_Crowd_Locomotion_Acceleration`:
- Spawn 1 agent
- Request move to a far target (2000cm)
- Sample velocity at t=0.0, t=0.25, t=0.5, t=1.0
- Assert speed at t=0.0 is 0
- Assert speed at t=0.5 is at least 50% of max (proves accel ramp)
- Assert speed at t=1.0 is at max

`UCk_AutoTest_Crowd_Locomotion_Replication`:
- Run in 2-client PIE
- Spawn 1 agent on server
- Request move
- After 0.5s of arrival, sample client transforms and assert all within 50cm of server transform

## Debugger additions (per [mockup §1](Debugger_Mockup/01_main.html))

| Panel | Gate 2 contribution |
|---|---|
| Agent List | Status badge: `Walking` (green) or `Idle` (grey). |
| Agent Detail Transform section | Live: Position, Rotation Yaw, Velocity, Desired Velocity (highlighted in orange when active), Max Speed, Radius/Height. |
| Agent Detail Steering Forces section | Path-follow vector (always shown when walking), Separation/Pierce/Player-yield rows present but show `— inactive` placeholder. |
| Stats | New: `Steering` (ms), `Tick total` (ms). Sparkline becomes meaningful. |

## Risks / unknowns

| Risk | Likelihood | Mitigation |
|---|---|---|
| **Velocity → SceneNode chain has the gap flagged in study (Sub-task 2A)** | **High** | Sub-task 2A is the gating step. Budget 0.5–1.5 days. If the gap exists, the fix is a small bridge processor (`FProcessor_EulerIntegrator_ApplyOffset`) that enqueues `Request_AddLocationOffset` with the integrator's `_DistanceOffset`. |
| Replication smoothing breaks at 240 cm/s server velocity | Medium | Verify with the 2-client AutoStation. Existing `FProcessor_Transform_InterpolateToGoal_Location` should handle this — if it doesn't, tune its convergence factor. |
| Acceleration ramp interacts badly with arrival deceleration (overshoot) | Medium | The steering processor must compute "stopping distance" = `v² / (2*decel)` and start decelerating at that distance from the final waypoint. Tested by `Locomotion_StraightLine` AutoStation final-position assertion. |
| Turn rate clamp creates orbits around tight waypoints | Low | If observed, allow the steering to use a "stride-skip" — peek 2 waypoints ahead and choose the closer-to-current-heading one. Defer until Gate 4. |

## Done criteria checklist

- [ ] Sub-task 2A validated; if gap existed, bridge processor written + CkPhysics/Claude.md updated.
- [ ] Single agent walks A→B in PIE, smooth ramp, stops cleanly.
- [ ] All 3 AutoTests pass.
- [ ] Replicated AutoTest passes (2-client PIE).
- [ ] Debugger shows live velocity / desired velocity matching mockup §1.
- [ ] Path viz updates as waypoints consumed.
- [ ] PLAN.md status row updated to ✅ Done.
- [ ] CkCrowd/Claude.md updated with locomotion API + the velocity-bridge pattern.
- [ ] If Sub-task 2A required a fix, that fix is documented in CkPhysics/Claude.md too.
