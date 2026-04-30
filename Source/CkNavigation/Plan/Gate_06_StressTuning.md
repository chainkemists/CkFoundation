# Gate 6 — Stress + Tuning

> **Status:** ⏳ Pending
> **Day target:** D6–D7 (2-day budget)
> **Parallelizable:** Limited — tuning passes need a single decisive eye
> **Depends on:** Gates 0–5 (all complete)

## Goal

Validate the system at **150 agents** (target: rental store at peak with margin), tune
the dozens of constants introduced across Gates 2–5 against actual gameplay feel, and
land a stats panel + frame-timing dashboard so future regressions are catchable.

This is the unknowable middle 20% — the work that's hard to schedule precisely. Two days
budgeted; if it slips beyond that, Gate 7's "rental store scenario" gets reduced in scope.

## Acceptance criteria

1. ✅ 150 agents pathing simultaneously: total crowd-related ms < 4.0 ms / frame on the dev machine.
2. ✅ No agent stalls for > 1 second across a 60-second observation window in the stress gym.
3. ✅ No replan-storm cascades (≤ 8 simultaneous replans across 60s window).
4. ✅ Memory: < 128 KB per agent (fragments only; not counting probe entity overhead).
5. ✅ Debugger Stats panel matches [mockup §1 right column](Debugger_Mockup/01_main.html): all stats fields populated, sparkline rendering tick total over last 60s.
6. ✅ Debugger Stats panel breakdown: separate ms for Steering / Neighbor query / Path requests / Tick total.
7. ✅ Tunable constants finalized + documented in CkCrowd/Claude.md "Tuning" section. No magic numbers in gym files.
8. ✅ AutoStation `UCk_AutoTest_Crowd_Stress_150` runs in headless PIE: spawn 150 agents on a 60-second scripted scenario, assert no agent stalls > 1s, assert tick total stays < 4ms median.
9. ✅ Profiling pass: top 3 hotspots identified + addressed (or explicitly accepted as "fine"). Document in CkCrowd/Claude.md.

## Sub-tasks

### Sub-task 6A — Stress gym authoring

**File:** `Plugins/CkTests/Script/CkCrowd/CkCrowdGym_Stress_*.as`

Map: a large open area (10000 × 10000 cm) with structured obstacles:
- 4 cluster-spawn alcoves (`Gym.Crowd.Stress.Spawn.NW/NE/SW/SE`)
- 1 central counter (`Gym.Crowd.Stress.Counter`) — narrow approach lane
- 4 cross-lanes connecting opposite spawns (forces head-on flows)

Press **1** → spawn 150 agents at random clusters with goals at the counter (counter-rush)
Press **2** → spawn 150 agents 4-way (each cluster targets opposite cluster — counter-flow)
Press **3** → spawn 150 agents random goals (background noise)
Press **0** → despawn all + reset

Live display:
- Active agents
- Asleep
- Replanning
- Frame ms breakdown
- Worst stall (max time any agent has been stuck)

### Sub-task 6B — Profiler pass

Use `stat unit` + Unreal Insights to identify hotspots. Expected suspects:
- `FProcessor_CrowdAgent_NeighborSync` (set ops on overlap diff)
- `FProcessor_CrowdAgent_Separation` (per-neighbor force calc)
- `FProcessor_Nav_HandleRequests` (FindPathSync calls)

Mitigations to have in pocket:
- **Neighbor cap (`_MaxNeighborsForSteering = 6`)** already in Gate 3. If 6 is too high, drop to 4 and verify behavioral acceptance still passes.
- **Throttled neighbor sync**: instead of every frame, sync every 2 frames using deterministic round-robin (agent's handle id mod 2 == frame parity). Cuts neighbor-sync cost by 50%; behavioral cost is one extra frame of staleness.
- **Path query budget**: `_MaxPathQueriesPerFrame` from CkNavigation. Default 8; can grow to 16 if budget is the bottleneck.
- **Separation force lookup**: pre-square `_SeparationRadius` so the inner loop uses squared distances throughout.

Each mitigation has a tunable. Don't ship them all enabled — pick the ones the profiler points at.

### Sub-task 6C — Tunable lockdown

Lock final defaults for every tunable introduced in Gates 0–5, document in CkCrowd/Claude.md as a single "Tunables Reference" table:

| Tunable | Default | Range | Why |
|---|---|---|---|
| `_Radius` | 42 | 20–80 | Standard humanoid waist radius. |
| `_Height` | 192 | 100–300 | Standard humanoid height. |
| `_MaxSpeed` | 240 | 100–600 | Walking speed (cm/s). 600 = run. |
| `_MaxAcceleration` | 480 | 200–1200 | 2× MaxSpeed = 0.5s ramp. |
| `_MaxTurnRate` | 4.0 | 1.0–8.0 | rad/s. 4.0 = full turn in 1.6s. |
| `_ArrivalRadius` | 30 | 10–80 | Stop tolerance at goal. |
| `_WaypointArrivalRadius` | 25 | 10–50 | Advance tolerance per waypoint. |
| `_SeparationRadius` | 100 | 60–200 | Push activation distance. |
| `_SeparationLookahead` | 100 | 60–200 | Probe radius extension. |
| `_SeparationWeight` | 2.0 | 0.5–5.0 | Force multiplier. |
| `_MaxNeighborsForSteering` | 6 | 3–10 | Per-frame cap. |
| `_PiercingAngle` | 0.5 | 0.2–1.0 | rad. |
| `_PiercingActivateRadius` | 80 | 40–150 | cm. |
| `_SleepIdleSeconds` | 1.5 | 0.5–5.0 | s. |
| `_ReplanThresholdSeconds` | 2.0 | 1.0–5.0 | s. |
| `_ReplanProgressThreshold` | 5.0 | 2.0–20.0 | cm/s. |
| `_MaxReplansPerPath` | 3 | 1–10 | Before Failed. |
| `_PlayerProxySoftPushRadius` | 120 | 80–200 | cm. |
| `_PlayerYieldMultiplier` | 2.0 | 1.0–4.0 | Boost factor. |

These end up as defaults in `FCk_Fragment_CrowdAgent_ParamsData` constructors. Project settings in `CkCrowd_ProjectSettings` allow per-game overrides for the global defaults.

### Sub-task 6D — Stats panel polish

Per [mockup §1 right column](Debugger_Mockup/01_main.html), the Stats panel shows:

- Total / Awake / Asleep / Replanning / Failed
- Avg neighbors per awake agent
- Steering / Neighbor query / Path requests / Tick total (ms)
- Sparkline: tick total over last 60s

DataCollector samples timing via `SCOPE_CYCLE_COUNTER` per processor. The stats fields update per ViewModel-tick (no rebuild storms).

The sparkline is a fixed-width SVG path; data is a 60-element rolling buffer. Update once per second; clear data on debugger close.

## Gym spec — AutoStation

`UCk_AutoTest_Crowd_Stress_150`:
- 60-second timeline:
  - t=0: Spawn 150 agents at 4 clusters
  - t=0.5: All issue MoveTo a randomized point within a 5000cm-wide central zone
  - t=20: Re-target to opposite cluster (counter-flow stress)
  - t=40: Re-target to scattered points
  - t=60: End — gather stats + assert
- During the run, sample every 250ms:
  - Per-agent stall duration (max across all agents)
  - Tick total ms
  - Path-query budget hits
- Assert max-stall < 1.0s
- Assert 95th percentile tick total < 4.0ms
- Assert 0 agent transitions to Failed
- `FinishSuccess()` if all assertions pass

## Debugger additions

This gate has no new panel features — only polishing the Stats panel + sparkline. The
existing structure from Gates 0–5 is the contract; no shape changes.

## Risks / unknowns

| Risk | Likelihood | Mitigation |
|---|---|---|
| **Bundled perf + gameplay tuning in 2 days — gameplay tuning gets squeezed if perf eats time** | **High** | This is the highest-risk item in the calendar (per CTO review — higher than Gate 2). Mitigation: do perf tuning first (sub-task 6B), since it's mechanical with the prepared mitigations. If perf is clean (likely — neighbor cap + CkSpatialQuery reuse), full window goes to gameplay tuning. **If perf is not clean, gameplay tuning slides into Gate 7** (which is therefore second-pass tuning, not buffer). |
| 150 agents tank fps below target on dev machine | Medium | Profiler-guided mitigation (sub-task 6B). 4ms is the *target*; if the real number is 6ms with no visible regression, ship it and note for follow-up. |
| Behavioral feel is bad in spite of perf being fine | Medium-High | Tuning pass IS this gate. If tuning isn't enough, the next-step option is ORCA (Gate 3 replacement, 2–3 day post-ship effort). |
| **Replan / sleep thresholds tuned in Gate 6 don't survive the rental-store scenario in Gate 7** | High (expected) | This is normal. Initial threshold tuning happens against synthetic stress (cluster-rush, counter-flow). Realistic feel is only knowable under the rental-store scenario. Gate 7 retunes — that's why Gate 7 is "second-pass tuning," not buffer. |
| Memory grows unboundedly because of replan history / event log | Medium | Cap event log at 200 entries (rolling). Replan history per-agent capped at last 5 entries. |
| Path-query budget at 8/frame → bottleneck for replan storms | Medium | Bump to 16/frame if profiler shows it. Document as a tunable. |

## Done criteria checklist

- [ ] Stress gym walkable; 150-agent counter-rush feels coherent.
- [ ] AutoTest_Stress_150 passes consistently (run 3 times in a row).
- [ ] Stats panel sparkline updates live; matches mockup visually.
- [ ] All tunables documented in CkCrowd/Claude.md "Tunables Reference" with rationale.
- [ ] Profiler pass: top 3 hotspots noted with action taken or accepted.
- [ ] PLAN.md status row updated to ✅ Done.
- [ ] If any new mitigations were added (throttled neighbor sync, etc.), they're behind a project setting flag with default = current behavior, so we can A/B them in Gate 7.
