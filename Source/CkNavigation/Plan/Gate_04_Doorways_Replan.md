# Gate 4 — Doorways + Replan

> **Status:** ⏳ Pending
> **Day target:** D5
> **Parallelizable:** Yes — three independent sub-tasks (4A piercing, 4B sleep, 4C replan-on-blocked). Spawn 3 parallel agents.
> **Depends on:** Gate 0 ✅, Gate 1 ✅, Gate 2 ✅, Gate 3 ✅

## Goal

Three independent behavioral correctness features, all of which become apparent in
"agents in tight spaces":

- **Piercing**: when two agents approach each other at a narrow doorway / aisle entrance, one yields and lets the other through instead of both stalling at the threshold. Implemented as an angle-based collision skip — agents whose approach angles relative to each other exceed `_PiercingAngle` ignore separation force on the pair until they're past each other.
- **Sleep**: agents with no movement target and no nearby neighbors skip the steering / neighbor-sync processors entirely. Cuts CPU cost for idle NPCs (queue, browsing) by 90%+.
- **Replan-on-blocked**: an agent that hasn't made progress toward its current waypoint for `_ReplanThresholdSeconds` re-issues a path query. Catches navmesh changes mid-path and recovers from local-avoidance deadlocks.

After this gate: agents flow through doorways without piling up, idle NPCs cost ~nothing,
and stuck agents recover instead of vibrating in place.

## Acceptance criteria

1. ✅ When 2 agents approach a 100cm-wide doorway from opposite sides, one yields (pierces) and the other passes; both arrive within `T = pathDist/maxSpeed + 1.5s`.
2. ✅ Agents with no goal and no neighbors stamp `FTag_CrowdAgent_Asleep` within `_SleepIdleSeconds` (default 1.5s) and stop appearing in steering's view.
3. ✅ Asleep agents wake (untag) within 1 frame of receiving a goal or having a neighbor enter their probe radius.
4. ✅ When an agent fails to progress toward its current waypoint for `_ReplanThresholdSeconds` (default 2.0s), it re-issues `Request_FindPath` and (if successful) replaces its path corridor; if path query fails, transitions to `Failed` state.
5. ✅ Replan event appears in debugger Event Log per [mockup §1 row "REPLAN"](Debugger_Mockup/01_main.html); pierce events appear with `PIERCE` category; sleep transitions appear with `AWAKE→SLEEP` and `SLEEP→AWAKE` categories.
6. ✅ AutoStation `UCk_AutoTest_Crowd_Doorway` passes: 2 agents through narrow doorway, both arrive, neither stalled longer than 0.5s.
7. ✅ AutoStation `UCk_AutoTest_Crowd_Sleep` passes: spawn idle agent, assert `Asleep` tag within 2.0s, send goal, assert untagged within 1 frame.
8. ✅ AutoStation `UCk_AutoTest_Crowd_Replan` passes: agent paths through corridor, mid-path block the corridor with a static obstacle (or move agent to dead-end), assert replan fires within 3s and either reaches alternate goal or transitions to Failed.

## Sub-tasks (3 parallel agents)

### Sub-task 4A — Piercing

**Files:**
```
Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/
    CkCrowdAgent_Piercing_Fragment.{h,cpp}
        FFragment_CrowdAgent_PiercingPairs    # TArray<FCk_Handle> currently being pierced (per-frame)
    CkCrowdAgent_Piercing_Processor.{h,cpp}
        FProcessor_CrowdAgent_Piercing        # Computes pairs in piercing state; runs after NeighborSync, before Separation.
```

Algorithm (per frame, per agent):
1. Read neighbor cache.
2. For each neighbor, compute approach angle: `dot(self.velocity.normalized(), -nbr.relativeOffset.normalized())`. High dot = head-on.
3. If `acos(dot) >= _PiercingAngle` AND distance < `_PiercingActivateRadius`, pair is "in piercing state."
4. Tie-breaking: only the agent with lower handle id pierces (deterministic; the other still applies separation, but the piercer ignores this neighbor's contribution this frame).
5. Stamp `FFragment_CrowdAgent_PiercingPairs` with the list of neighbors being pierced.

Update `FProcessor_CrowdAgent_Separation` to skip neighbors in the agent's piercing-pairs list.

Per-tunable: `_PiercingAngle = 0.5 rad (~28°)`, `_PiercingActivateRadius = 80 cm`. Default values are conservative; visible-but-not-aggressive piercing.

### Sub-task 4B — Sleep / idle

**Files:**
```
Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/
    CkCrowdAgent_Sleep_Fragment.{h,cpp}
        FFragment_CrowdAgent_SleepState        # _IdleSeconds accumulator
        FTag_CrowdAgent_Asleep                 # Steering/Separation/NeighborSync exclude this in their views
    CkCrowdAgent_Sleep_Processor.{h,cpp}
        FProcessor_CrowdAgent_SleepEvaluator   # Decides asleep-or-not based on goal + neighbor presence
```

The Asleep tag is added to the existing processors' `TExclude<FTag_CrowdAgent_Asleep>` clauses. Once asleep:
- Steering doesn't run for them (no desired velocity update — they freeze in place).
- Separation force is zero (no neighbor contribution).
- NeighborSync still runs for them (cheap probe read) so they wake correctly.

Wake conditions (any of):
- New movement request lands → SleepEvaluator sees pending request, removes Asleep tag.
- Neighbor enters probe radius → NeighborSync writes a non-empty cache → SleepEvaluator sees it next frame → wakes.

The asleep tag is *removed* by SleepEvaluator, not by steering. This keeps the wake logic in one place.

### Sub-task 4C — Replan-on-blocked

**Files:**
```
Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/
    CkCrowdAgent_Replan_Fragment.{h,cpp}
        FFragment_CrowdAgent_ProgressTracker   # Last-progress timestamp + last-distance-to-waypoint
    CkCrowdAgent_Replan_Processor.{h,cpp}
        FProcessor_CrowdAgent_ProgressEval     # Per frame: did agent progress toward current waypoint?
        FProcessor_CrowdAgent_TriggerReplan    # If no progress for _ReplanThresholdSeconds → re-issue path request
```

Progress = `prevDistanceToWaypoint - currentDistanceToWaypoint` accumulated. If this stays below `_ReplanProgressThreshold` (default 5cm) over `_ReplanThresholdSeconds` (default 2.0s), trigger replan.

Replan = call `UCk_Utils_Nav_UE::Request_FindPath(handle, current_goal)`. On path-ready signal, replace the path-follow fragment's waypoint list and reset `_WaypointIndex` to 0. On path-failed, transition to `Failed` (no further auto-replan; manual intervention needed).

Replan event log: emit on every replan trigger (regardless of success). Carries the agent handle, replan reason ("NoProgress"), the new path's waypoint count.

## Tunables added in Gate 4

```cpp
// Piercing
float _PiercingAngle = 0.5f;             // rad — minimum head-on angle for piercing
float _PiercingActivateRadius = 80.0f;   // cm

// Sleep
float _SleepIdleSeconds = 1.5f;          // time without goal/neighbors before sleeping

// Replan
float _ReplanThresholdSeconds = 2.0f;    // time of no-progress before replanning
float _ReplanProgressThreshold = 5.0f;   // cm — minimum forward progress per second
int32 _MaxReplansPerPath = 3;            // before giving up → Failed state
```

## Gym spec — manual

`Crowd Doorways` gym:

- A **U-shaped corridor**: two open ends, one narrow doorway (100cm wide) connecting them
- Two spawn alcoves at the ends, tagged `Gym.Crowd.Doorway.LeftSpawn`, `Gym.Crowd.Doorway.RightSpawn`
- Press **1** → spawn agent at left, target = right
- Press **2** → spawn agent at right, target = left
- Press **3** → spawn 4 agents, 2 each side, simultaneous (stress)
- Press **B** → toggle a movable obstacle in the corridor (test replan)
- Press **S** → spawn 3 agents at left with goals at left (no-target sleep test; they receive no goal)
- Live display: pierce count last 3s, sleep count, replan count

## Gym spec — AutoStation

`UCk_AutoTest_Crowd_Doorway`:
- Spawn 2 agents at opposite ends of a 100cm doorway corridor
- Request crossing moves
- Sample min-distance + per-agent stalled-duration every 100ms
- Assert each agent's max-stalled-duration < 0.5s
- Assert both arrive within `T = corridorLen/maxSpeed + 2.0s`
- Assert pierce event was emitted at least once (read debug log)

`UCk_AutoTest_Crowd_Sleep`:
- Spawn 1 agent at (0,0,50). No goal.
- After `_SleepIdleSeconds + 0.2s` (default 1.7s), assert agent has `FTag_CrowdAgent_Asleep`.
- Send `Request_MoveTo((300, 0, 50))`.
- Assert tag removed within 2 frames.
- Assert agent reaches goal within 3s.

`UCk_AutoTest_Crowd_Replan`:
- Spawn 1 agent. Request move through a corridor.
- Mid-path: spawn a static obstacle blocking the corridor + bake nav.
- Assert replan event fires within `_ReplanThresholdSeconds + 0.5s`.
- Assert agent either re-routes around obstacle and arrives, or transitions to `Failed` (acceptable: navmesh has no alternate route).

## Debugger additions (per [mockup §1](Debugger_Mockup/01_main.html))

| Panel | Gate 4 contribution |
|---|---|
| Agent List | Status badge: new value `Replan` (yellow). `Asleep` rendered in dim grey. |
| Agent Detail Steering Forces | `Pierce` row populated with active/inactive + count of pierced neighbors. |
| Agent Detail Sleep & Replan | New section: Sleep state, Idle for, Replan count, Last replan timestamp + reason. |
| Stats | New: `Asleep`, `Replanning` counts. |
| Event Log | New categories: `PIERCE` (info-cyan), `AWAKE→SLEEP` (dim), `SLEEP→AWAKE` (dim), `REPLAN` (warn-yellow). |

## Risks / unknowns

| Risk | Likelihood | Mitigation |
|---|---|---|
| Piercing ping-pong: agent A pierces nbr B, A moves slightly past, exits piercing, separation pushes them apart, next frame they're back in piercing range | Medium | Add minimum-piercing-duration: once a pair enters piercing state, hold it for at least 0.2s regardless of subsequent angle changes. |
| Sleep evaluator races with movement requests | Low | Sleep evaluator runs in `FGroup_AI` (early in frame); movement-request handler runs in `FGroup_Physics` (mid-frame). One-frame lag at worst. Acceptable. |
| Replan storms: every agent in a stuck cluster fires replan at the same time → path query budget overflow | Medium | Replan inherits the per-frame budget from `_MaxPathQueriesPerFrame`. Round-robin queue ensures progress. Worst case: replan takes 8 frames for 8 agents — still well under perceptible. |
| Replan replaces path mid-corridor → agent rubber-bands | Medium | When replacing a path mid-walk, choose start point as the agent's *projected* position on navmesh, not its raw transform. Snap any visible jump to ≤ `_Radius`. |
| Asleep agents accumulate a stale neighbor cache → wake decision based on out-of-date data | Low | NeighborSync still runs for asleep agents (cheap). Cache is fresh. |

## Done criteria checklist

- [ ] All 3 sub-tasks land. (Spawn 3 parallel agents per sub-task; merge sequentially after acceptance.)
- [ ] All 3 AutoStations pass.
- [ ] Manual gym shows pierce / sleep / replan visibly happening; debugger event log records each.
- [ ] No perf regression at 20 agents (steering + neighbor + separation + new processors total < 1.5 ms).
- [ ] PLAN.md status row updated to ✅ Done.
- [ ] CkCrowd/Claude.md updated with all three behavioral systems.
