# Gate 3 Addendum — Hybrid Force + Sampling Avoidance

**Date:** 2026-05-02
**Status:** ⏳ Design accepted, awaiting plan-doc + implementation
**Supersedes:** the post-Gate-2 portion of [Gate_03_Separation.md](Gate_03_Separation.md)
**Does not change:** Gate 0, Gate 1, Gate 2 — all complete and shipped

---

## 1. Why an addendum

Gate 3 sub-tasks 3A + 3B + 3C landed and unlocked end-to-end head-on tests. PIE smoke testing in the Crowd Separation gym (commits `adb75c6` + `03b28c8`) revealed the symptomatic failure mode of pure force-based reciprocal separation:

- Two agents on a head-on path **vibrate in place**, fighting between path-follow (toward goal) and separation (perpendicular). The MaxSpeed clamp eats both contributions; neither yields.
- After the vibration window, agents **clip through each other** because lateral nudges aren't large or early enough to clear the cylinder geometry.

The core defect of the current solver is that it has neither a **commitment bias** (agents flip directions easily on marginal forces) nor **predictive avoidance** (agents react only to current overlap, not imminent overlap). Both properties are present in the two industry-tested crowd systems we've now studied:

- **Recast/Detour Crowd** (Mikko Mononen) — production navigation crowd shipped in dozens of titles; battle-tested at ~150 agents.
- **Ant RTS Plugin** (UE Marketplace) — RTS-scale crowd with explicit dual-solver architecture for the perf-vs-quality tradeoff.

Both informed this addendum. **Neither uses pure force-based separation as the primary solver.** They both use commitment + prediction. We need to too.

The other immutable constraint is the product context. Per project memory:

> Rewind99 (formerly BusterBlock): VHS rental store sim, open world, ~110-130 NPCs, 4-player co-op (server-authoritative crowd, replicated state, client smoothing).

The 8-day window of the original PLAN was a planning artefact for a smaller perf budget than this game can afford to spend on crowd. Adding a sampling-avoidance pass at this scale is well within budget *if* it's gated on need.

---

## 2. Strategic shift

**From:** force-based separation everywhere (current `FProcessor_CrowdAgent_Separation`).
**To:** **tuned force solver everywhere + sampling-avoidance override when clumped or in queue zones.** ORCA/RVO2 deferred to post-Gate-7 as a third-tier extension; the sampling-override seam is the slot they plug into.

### Phased rollout

| Phase | Scope | Expected behaviour | Implementation cost |
|---|---|---|---|
| **1** | Inertia weighting + acceleration-delta clamp on existing force solver | Eliminates vibration in sparse zones (open-world streets, lone NPCs passing each other). Head-on encounters in queues still suboptimal. | ~1 day |
| **2** | Sampling-avoidance override processor (dtCrowd-style penalty-scored velocity sampling) gated on neighbor count + zone-tag override | Predictive avoidance in clumped zones (counter queue, doorway, convergence). Smooth, no vibration. | ~2-3 days |
| **3** | (Deferred) ORCA / RVO2 swap-in for the sampling stage | Mathematically optimal avoidance. Behaviour ceiling above (2). Slots into the same Phase-2 processor seam. | Post-Gate-7 |

Phase 1 is independently shippable and verifiable before committing to Phase 2.

---

## 3. Reference codebases — citation tables

These tables are the source of truth for the math, constants, and architecture choices below. File paths are local to my machine; these are the canonical implementations.

### Recast/Detour Crowd

Location: `D:\Repos\UnrealEngineAngelscript\Engine\Source\Runtime\Navmesh\Private\DetourCrowd\`

| Aspect | Citation | Detail |
|---|---|---|
| Per-frame update order | `DetourCrowd.cpp:1219-1233` | paths → proximityData → nextMovePoint → steering → avoidance → move (4-iter push-apart) → corridor |
| Penalty function | `DetourObstacleAvoidance.cpp:processSample() 323-426` | Sum of 4 penalties: desired-velocity deviation, current-velocity deviation, side preference, time-to-collision |
| Penalty formula | `DetourObstacleAvoidance.cpp:414-419` | `vpen + vcpen + spen + tpen` where `tpen = wToi * (1.0 / (0.1 + tmin*invHorizTime))` |
| Default weights | `DetourObstacleAvoidance.cpp:471-475` | `wDesVel=2.0`, `wCurVel=0.75`, `wSide=0.75`, `wToi=2.5`, `horizTime=2.5s` |
| Adaptive sampling | `DetourObstacleAvoidance.cpp:sampleVelocityAdaptive() 520-608` | 7 divs × 2 rings × 5 depth iterations → 71 samples |
| Custom-pattern cap | `DetourObstacleAvoidance.h:84` | `DT_MAX_CUSTOM_SAMPLES = 16` |
| Neighbor cap | `DetourCrowd.h:44` | `DT_CROWDAGENT_MAX_NEIGHBOURS = 6` |
| Acceleration clamp | `DetourCrowd.cpp:integrate() 53-69` | `dv = nvel - vel; if \|dv\| > maxAccel*dt: scale dv to maxAccel*dt` — clamps **velocity change**, not raw speed |
| Post-hoc push-apart | `DetourCrowd.cpp:updateStepMove() 1601-1662` | 4 iterations of `disp += dir * (penetration * 0.5 * 0.7 / dist)` — `COLLISION_RESOLVE_FACTOR = 0.7` |
| Slowdown at goal | `DetourCrowd.cpp:1461-1466` | Linear taper from full speed at `2*radius` distance to 0 at goal |
| Path-follow direction | `DetourCrowd.cpp:calcStraightSteerDirection() 130-140` | `dir = normalize(corner - pos)` — **no remaining-distance braking** in the corner direction itself |
| Active flag | `DetourCrowd.h:190`, `.cpp:813` | Binary 0/1; no sleep state — inactive = removed from pool |
| Proximity grid | `DetourProximityGrid.cpp` + `DetourCrowd.cpp:400` | Cell size = `3 * maxAgentRadius`. Cleared and rebuilt every frame in `updateStepProximityData`. |

### Ant RTS Plugin

Location: `E:\UE_5.6\Engine\Plugins\Marketplace\AntRTSCr15a91b355b0dV6\Source\Ant\`

| Aspect | Citation | Detail |
|---|---|---|
| Solver dispatch | `AntSubsystem.cpp:1290` | `agent.AvoidanceType == EAntAvoidanceTypes::AntDefault ? DefaultSolver(agent) : ORCASolver(agent)` — **per-agent enum**, not numeric threshold |
| AntDefault solver | `AntSubsystem.cpp:DefaultSolver() 774-973` | Force-style: merge overlap normals into CW/CCW constraint axes, raycast to collision point. **No inertia, no velocity prediction.** Faster than RVO. |
| ORCA solver | `AntSubsystem.cpp:ORCASolver() 975-1112` | Builds half-plane lines per neighbor (cut-off circle or legs depending on collision case), solves 2D LP via `RVOProgram2`, falls back to 3D LP `RVOProgram3` on infeasibility |
| ORCA time horizon | `AntSubsystem.h:540` | `RVOTimeHorizon = 5.0s` (default) — predicts collisions over this window |
| MaxOverlapForce cap | `AntSubsystem.h:182` | Default `0.4`. Applied as `OverlapForce.Normalize() * MaxOverlapForce` blended into preferred velocity. |
| Sleep flag | `AntSubsystem.h:267`, `.cpp:1283 + 1393` | `bSleep` skips solver. **Wakes only on `bCollided` or `bIsOnNavLink`** — not speed/timer based. |
| Spatial grid | `AntGrid.h:41-123`, `AntSubsystem.h:502-503` | Cell size `250` units. **Lock-free** via index-based linked lists. |
| Parallel solve | `AntSubsystem.cpp:1269` | `ParallelFor` over agents. Each agent reads neighbors' velocities (safe: writes deferred to post-loop). |
| Collision groups | `AntSubsystem.h:20-56` | 32-bit `IgnoreFlag` mask. 32 channels. Bit-AND tested in `QueryCylinder`. |
| Movement timeout | `AntSubsystem.h:394`, `.cpp:1484-1500` | `MissingVelocityTimeout` accumulates Manhattan delta between preferred and actual velocity; trips event when sum > threshold. **Caller decides what to do** (replan, cancel, sleep). |

### Headline takeaways for this design

1. **dtCrowd doesn't use ORCA.** It uses Mononen's penalty-scored velocity sampling. ORCA is a different (heavier, mathematically tighter) algorithm that Ant ships *additionally* to its force-style default.
2. **Both reference systems have a commitment bias.** dtCrowd via `weightCurVel` in the penalty (penalizes deviating from current velocity). Ant's solver dispatch isn't commitment per se but the underlying integrate-with-MaxAccel step provides the same effect.
3. **Both reference systems clamp velocity *changes*, not raw speed.** Our current Steering processor clamps *speed* via the `[Prev-delta, Prev+delta]` clamp on `NewSpeed` but writes a freshly-directioned velocity each frame — **direction can flip arbitrarily**. That's the source of vibration.
4. **dtCrowd has both an avoidance pass *and* a 4-iteration post-hoc push-apart.** The push-apart handles "you're already overlapping right now"; the sampler handles "you're about to overlap." Two separate systems, different time horizons.
5. **Neither reference system runs ORCA on every agent.** dtCrowd doesn't ship ORCA at all. Ant exposes it as a per-agent opt-in.

---

## 4. Phase 1 — Tuned Force Solver

**Goal:** eliminate vibration in sparse zones using the cheapest possible mods to the existing solver. Ship-shape behaviour for ≥80% of the agent-pair encounters in Rewind99 (open-world strolling, single-line passes).

### 1.1 Inertia weighting on the separation force

Add a weighted blend of last frame's separation force into the new computation, so the agent's lateral push direction has rotational inertia. Modeled directly on dtCrowd's `weightCurVel` penalty (`DetourObstacleAvoidance.cpp:415`) but applied as a force-blending coefficient instead of a sample score (since we're not sampling yet).

```cpp
// In FProcessor_CrowdAgent_Separation
const auto NewForce  = ComputeFromCache(...);   // existing logic
const auto LastForce = SeparationForce.Get_Force();
const auto Inertia   = InParams.Get_SeparationInertia();    // new tunable, default 0.5
SeparationForce._Force = FMath::Lerp(NewForce, LastForce, Inertia);
```

`Inertia = 0.0` means today's behavior (instant force changes, vibrates).
`Inertia = 1.0` means force never changes.
**Default = 0.5** — half-life of ~1 frame for lateral force shifts. dtCrowd's `weightCurVel/weightDesVel = 0.75/2.0 = 0.375` ratio — same order of magnitude.

### 1.2 Velocity-change clamp — new dedicated processor

The existing `FProcessor_CrowdAgent_Steering` clamps `NewSpeed` magnitude per frame but lets the *direction* flip arbitrarily because the path-follow `Direction` is recomputed every frame.

We introduce a new `FProcessor_CrowdAgent_AccelClamp` (rather than inlining the clamp into Steering) so Phase 2's sampling override has the same clamping behaviour without duplicating the logic. **One processor, multiple writers** — Steering writes its raw output, then later Sampling may overwrite it; the clamp normalises whichever wrote last.

**View:** `TReadWrite<FFragment_CrowdAgent_DesiredVelocity>`, `TReadOnly<FFragment_CrowdAgent_Params>`.
**Group:** `FGroup_Physics`. **RunAfter:** `FProcessor_CrowdAgent_Steering` (Phase 1) + `FProcessor_CrowdAgent_AvoidanceSample` (Phase 2). **RunBefore:** `FProcessor_CrowdAgent_VelocityBridge`.

```cpp
// FFragment_CrowdAgent_DesiredVelocity gains a _LastVelocity field, set at end-of-tick from
// the clamp's output. This is the baseline for next frame's delta clamp — independent of the
// physics-side _CurrentVelocity (which has been through Velocity_Clamp's min/max trimming).
const auto MaxDelta = InParams.Get_MaxAcceleration() * InDeltaT.Get_Seconds();
const auto LastVel  = InDesired.Get_LastVelocity();
const auto NewVel   = InDesired.Get_Velocity().GetClampedToMaxSize(InParams.Get_MaxSpeed());
const auto Dv       = NewVel - LastVel;
const auto DvLen    = Dv.Size();
if (DvLen > MaxDelta)
{
    InDesired._Velocity = LastVel + Dv * (MaxDelta / DvLen);
}
else
{
    InDesired._Velocity = NewVel;
}
InDesired._LastVelocity = InDesired._Velocity;
```

Mirrors `DetourCrowd.cpp:integrate() 53-69`. This is the single biggest behavioral fix Phase 1 delivers — direction can no longer snap-flip, acceleration is enforced on the velocity vector itself, not just its magnitude.

**Schema change:** `FFragment_CrowdAgent_DesiredVelocity` adds a private `_LastVelocity` field (FVector, default zero). Friend access for the new clamp processor.

### 1.3 Tunables added (Phase 1)

In `FCk_Fragment_CrowdAgent_ParamsData`:

```cpp
// Inertia coefficient on separation force. 0=no inertia (instant changes, vibrate-prone),
// 1=fully sticky (force never changes). Defaults to dtCrowd-equivalent ratio.
UPROPERTY(...) float _SeparationInertia = 0.5f;
```

`_MaxAcceleration` already exists; the new clamp uses it directly without adding a new tunable.

### 1.4 Phase 1 acceptance

Re-running `Ck_GymCrowd_Sep_HeadOnNS` and `Ck_GymCrowd_Sep_HeadOnEW`:
- Two agents pass without vibrating at any point
- They may still clip on the closest pass (Phase 2 handles that)
- Single agent with no neighbors: identical behavior to today
- 5-agent cluster: doesn't oscillate; some pile-up at goal expected (Phase 2 fixes)

If those criteria pass, Phase 2 is gated on whether observed clipping is actually unacceptable for Rewind99 gameplay. If we can stomach occasional clipping in the open world but need zero-clipping in queues, Phase 2 is mandatory for the queues alone.

---

## 5. Phase 2 — Sampling Avoidance Override

**Goal:** zero-clipping in clumped scenarios using a stripped-down dtCrowd-style sampler that runs only when needed.

### 2.1 New processor: `FProcessor_CrowdAgent_AvoidanceSample`

**View:**
```cpp
ck::TReadOnly<FFragment_CrowdAgent_Params>,
ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
FTag_CrowdAgent_HasProbe,
FTag_CrowdAgent_Walking,
TExclude<FTag_CrowdAgent_Asleep>,
```

**Group:** `FGroup_Physics`. **RunAfter:** `FProcessor_CrowdAgent_Steering` (so we override its output). **RunBefore:** `FProcessor_CrowdAgent_VelocityBridge` (so the bridge sees our override).

### 2.2 Trigger logic — when does sampling run?

Per the user's directive (this addendum's Q2 thread): **tunable trigger with project-default + zone-tag override.**

Two sources, evaluated in order:
1. **Per-agent zone-tag override.** If the agent has a gameplay tag in `CrowdAvoidance.AlwaysSample` or its lifetime owner is in such a zone, sampling is forced on. Designer-controlled — Rewind99 designers tag `Crowd.QueueZone.RentalCounter` as always-sample.
2. **Numeric threshold (project default).** Otherwise sampling triggers when `NeighborCache.Num() >= _AvoidanceSampleNeighborThreshold` (project setting, default 3).

Single explicit opt-out: `CrowdAvoidance.NeverSample` tag forces force-only (use case: tutorial NPCs, scripted set-pieces).

### 2.3 Sample pattern — adaptive but stripped down

Modeled on `DetourObstacleAvoidance.cpp:sampleVelocityAdaptive() 520-608` but simplified for our scale.

dtCrowd defaults: 7 divs × 2 rings × 5 depth = 71 samples.
**Our defaults: 8 divs × 2 rings × 1 depth = 16 samples.** Why fewer:
- We're calling sampling on the ~20-30 clumped agents per frame, not all 130. Lower per-agent sample budget is fine.
- Single-depth iteration eliminates the 5× nested-refinement multiplier dtCrowd uses to converge on the optimum. Our acceptance criteria are "doesn't clip" not "provably optimal velocity."
- 8 angular divisions = one sample per 45° + 8 mirrored = covers all relevant dodge directions.

Pattern (mirrors `DetourObstacleAvoidance.cpp:548-567` geometry):
- Center sample (no movement) — 1 candidate
- Ring 1 at `0.5 * MaxSpeed` magnitude, 8 angular samples
- Ring 2 at `1.0 * MaxSpeed` magnitude, 8 angular samples (offset 22.5° from ring 1)
- Total = 17 candidates. Round to 16 by dropping the center sample (we always have a "stay current velocity" baseline via inertia).

### 2.4 Penalty function — three of four dtCrowd weights

Per `DetourObstacleAvoidance.cpp:414-419`:
```
penalty = wDesVel*deviation_from_desired
       + wCurVel*deviation_from_current
       + wSide*side_preference          // SKIPPED — see below
       + wToi*(1 / (0.1 + ttc/horizon))
```

We carry forward `wDesVel`, `wCurVel`, `wToi`. **We skip `wSide`** — the side preference (always pass on left or always on right) needs road-rule context Rewind99 doesn't have (mixed indoor/outdoor, no lane convention). The cost saving is one cross product + dot product per (sample × neighbor) pair.

**Defaults carried from dtCrowd, adjusted for our 16-sample (vs 71-sample) budget:**

| Weight | dtCrowd default | Our default | Notes |
|---|---|---|---|
| `_AvoidanceWeightDesVel` | 2.0 | 2.0 | Path-follow attraction. Same. |
| `_AvoidanceWeightCurVel` | 0.75 | 1.0 | Bumped — fewer samples means less natural commitment, so we lean harder on this. |
| `_AvoidanceWeightToi` | 2.5 | 2.5 | Time-to-collision. Same. |
| `_AvoidanceHorizonTime` | 2.5s | 2.5s | Same. |

Time-to-collision computation per sample × neighbor: standard sphere-sweep. With 16 samples × 6 neighbors = 96 TTC tests per sampling agent per frame. At 30 sampling agents per frame: 2,880 TTC tests/frame on a single thread. Trivial.

### 2.5 Round-robin scheduling — 1-in-3 with force fallback

Per the user's directive: **1-in-3 (20Hz per-agent), force-solver output as the off-frame fallback.**

```cpp
const auto FrameIndex = static_cast<int32>(GFrameCounter);
const auto AgentIndex = static_cast<int32>(GetTypeHash(InHandle));
const auto Stride     = ProjectSettings::Get_AvoidanceSampleStride();   // default 3
if (((FrameIndex + AgentIndex) % Stride) != 0)
{ return; }   // off-frame: leave InDesired._Velocity as the force solver wrote it
```

Properties:
- Each agent samples every 3rd frame. Peak reaction lag = 50ms (human threshold ~200ms).
- Agents are deterministically distributed across frames via entity-index modulo, so cost per frame is uniform regardless of which agents are clumped.
- On the 2 of 3 frames an agent doesn't sample, the force solver's velocity stands. With Phase 1's inertia + accel-delta clamp, that's a smooth fallback (not a stale snapshot).

### 2.6 Output behavior

When sampling fires for an agent, it overwrites `_DesiredVelocity` directly. The Steering processor wrote a force-tuned velocity earlier in the frame; we replace it.

Importantly: **we don't combine** with the force solver's output (no further blending). Sampling has already considered the desired velocity (via `wDesVel`) and the current velocity (via `wCurVel`) — those *are* the force solver's effective inputs. Combining again would double-apply path-follow.

The Phase 1 `FProcessor_CrowdAgent_AccelClamp` runs after both Steering and AvoidanceSample (`RunAfter` on both), so the sampler's overwrite still lands inside the velocity-delta budget. Sampling doesn't apply its own clamp — the dedicated processor handles it for whoever wrote last.

### 2.7 Tunables added (Phase 2)

In `UCk_Crowd_ProjectSettings_UE`:

```cpp
UPROPERTY(...) int32 _AvoidanceSampleStride = 3;            // 1-in-3 round-robin
UPROPERTY(...) int32 _AvoidanceSampleNeighborThreshold = 3; // numeric trigger
UPROPERTY(...) int32 _AvoidanceSampleAngularDivs = 8;
UPROPERTY(...) int32 _AvoidanceSampleRings = 2;
UPROPERTY(...) float _AvoidanceWeightDesVel = 2.0f;
UPROPERTY(...) float _AvoidanceWeightCurVel = 1.0f;
UPROPERTY(...) float _AvoidanceWeightToi = 2.5f;
UPROPERTY(...) float _AvoidanceHorizonTime = 2.5f;
```

Per-agent override (rare): `FFragment_CrowdAgent_AvoidanceOverride` carrying any of the above can be added to specific agents. Most agents inherit project defaults.

Zone-tag opt-in: `TAG_CrowdAvoidance_AlwaysSample` and `TAG_CrowdAvoidance_NeverSample`. Checked on the agent itself or via lifetime-owner walk.

---

## 6. Phase 3 — ORCA / RVO2 (deferred)

ORCA slots into the same processor seam Phase 2 establishes. Where Phase 2's body has the sampling math, Phase 3's variant has the half-plane / 2D-LP math from `AntSubsystem.cpp:ORCASolver() 975-1112`.

### What stays unchanged when Phase 3 ships:
- `FProcessor_CrowdAgent_Separation` (Phase 1 force solver)
- `FProcessor_CrowdAgent_NeighborSync` (Gate 3A)
- The trigger logic + zone-tag override mechanism
- The round-robin scheduler
- The output contract (override `_DesiredVelocity`)

### What changes:
- New `FProcessor_CrowdAgent_AvoidanceORCA` replaces sampling for opted-in agents (third tier above zone-always-sample). Numeric threshold or zone tag could promote *to ORCA* the same way Phase 2 promotes *to sampling*.
- ORCA tunables (`_ORCATimeHorizon` mirroring `AntSubsystem.h:540`'s 5.0s default).

The reason ORCA gets its own processor rather than a flag inside the sampling one: cleaner separation of concerns, easier to A/B test, and matches Ant's per-agent solver dispatch (`AntSubsystem.cpp:1290`).

---

## 7. Impact on the existing Gate 3 sub-tasks

| Sub-task | Status | Disposition |
|---|---|---|
| **3A** — Probe + NeighborCache | ✅ Done (`05ba5f147`) | Unchanged. Cache is the input to all three solvers (force / sampling / ORCA). |
| **3B** — Separation force solver | ✅ Done (`5e20a5693`) + scaling fix (`a26a0904b`) | Phase 1 adds inertia weighting (§4.1). Otherwise unchanged. |
| **3C** — Steering integration | ✅ Done (`5933ffb77`) + damping fix (`299af74e3`) | Phase 1 changes the final clamp from speed-magnitude to velocity-delta (§4.2). |
| **Separation gym** | ✅ Done (`adb75c6` + `03b28c8`) | Stays as the manual venue. Phase 1 + 2 verified there before AutoStations land. |
| **Debug-draw processor** | ✅ Done (`d6f565749`) | Unchanged — visualises whatever's in `_SeparationForce`, regardless of solver. |
| **NEW: 3D — Sampling Avoidance Override** | Phase 2 | Per §5. New processor + tunables + tags. |

---

## 8. AutoStation tests

Tests must pass at both Phase 1 and Phase 2 levels (with and without sampling). The sampling-active tests can use the `Crowd.QueueZone.AutoTest` zone tag to force Phase 2 mode.

### `UCk_AutoTest_Crowd_Separation_HeadOnPass`
- Spawn 2 agents 1500cm apart, head-on
- Phase 1 expectation: agents pass; min-separation ≥ `_Radius * 1.0` (touching is acceptable, clipping is not)
- Phase 2 expectation: agents pass; min-separation ≥ `_Radius * 1.5` (clean lateral dodge)

### `UCk_AutoTest_Crowd_Separation_Convergence`
- 5 agents converging on overlapping points within 100cm
- Phase 1 expectation: agents arrive, possibly piled at goal
- Phase 2 expectation: agents arrive within 200cm of target, no two final positions within `_Radius * 2`

### `UCk_AutoTest_Crowd_Separation_Vibration` (NEW — directly catches the bug we hit)
- 2 agents head-on, sample `_DesiredVelocity` direction every 50ms for 3s
- Assert: max angular delta between consecutive samples < 30° (Phase 1 gate)
- Asserts both Phase 1 inertia + accel-delta clamp are wired correctly

---

## 9. Perf budget — explicit numbers

At 130 agents, ~30 of which are clumped enough to trigger sampling:

| Solver | Per-frame ops | Notes |
|---|---|---|
| Phase 1 force solver | 130 × ~12 = 1.6K | Slight bump over today (inertia lerp adds 6 ops) |
| Phase 2 sampling override | (30 / 3) × 16 × 6 = 960 | 1-in-3 round-robin × 16 samples × 6 neighbors |
| **Total** | ~2.6K ops/frame | <0.1ms on a single thread |

For comparison, a hypothetical pure-sampling-everywhere build:
- 130 × 71 × 6 = 55K ops/frame (~30× more)

For comparison, a hypothetical pure-ORCA-everywhere build:
- 130 × 6² + 2D LP per agent ≈ much higher, dominated by LP cost

The hybrid wins by **only paying sampling cost on the ~20% of agents who need it, on 1 in 3 frames.** This is the Ant model (`AntSubsystem.cpp:1290`'s per-agent solver dispatch) but with the dispatch decision driven by ECS view membership rather than an enum field — more idiomatic for our architecture, and zero-cost when agents stay in the cheap path.

---

## 10. Out of scope

Documented here so we don't relitigate:

- **Side preference (`wSide`)** — costs a cross product per (sample, neighbor) pair, gives ~no benefit in mixed indoor/outdoor with no lane rules.
- **dtCrowd's 4-iteration push-apart** (`DetourCrowd.cpp:1601-1662`) — useful for "you're already overlapping" but our current design tolerates instant overlap until the sampler converges. If post-Phase-2 testing shows excessive clipping during the 50ms sample latency window, this lands as a 1-day patch.
- **Sleep optimization** — Ant's `bSleep` (`AntSubsystem.cpp:1393`) wakes only on `bCollided || bIsOnNavLink`. Worth ~30-50% perf at idle. Gate 4 of the original PLAN already has this; not duplicating here.
- **Lock-free ParallelFor** (Ant's `AntSubsystem.cpp:1269`) — our processors are already TProcessor which the scheduler can run in parallel where the view's read/write declarations allow. Existing infrastructure, no addendum work.
- **Movement timeout** (`AntSubsystem.h:394`) — original PLAN's Gate 4 covers this as `_ReplanThresholdSeconds`. Same behavior, different name.
- **Side-preference jitter** — the entity-index sin/cos jitter in our current 3B solver (`CkCrowdAgent_Separation_Processor.cpp` post-`Force.IsNearlyZero()` check) is no longer needed after Phase 2 — sampling's `wCurVel` does the same job better. Keep through Phase 1, remove with Phase 2.

---

## 11. Commit boundaries

Mirroring Gate 2's per-sub-task structure:

1. `docs(Navigation): Gate 3 addendum — hybrid avoidance design`
2. `feat(Crowd): Phase 1.1 — separation force inertia weighting`
3. `feat(Crowd): Phase 1.2 — AccelClamp processor (velocity-delta clamp)`
4. `feat(Crowd): Phase 2 — sampling avoidance override processor`
5. `feat(Crowd): Phase 2 — zone-tag opt-in/out for sampling`
6. `feat(CrowdGym Separation): rebind controls to exercise Phase 2 trigger paths`
7. `feat(CrowdAutoTest): HeadOnPass + Convergence + Vibration AutoStations`
8. `chore: bump submodules — Gate 3 hybrid done`

---

## 12. References

- **Recast/Detour Crowd** (Mikko Mononen et al.): `D:\Repos\UnrealEngineAngelscript\Engine\Source\Runtime\Navmesh\Private\DetourCrowd\` — primary reference for the Phase 2 sampler + Phase 1 accel-delta clamp.
- **Ant RTS Plugin**: `E:\UE_5.6\Engine\Plugins\Marketplace\AntRTSCr15a91b355b0dV6\Source\Ant\` — primary reference for the dual-solver dispatch architecture.
- **Original Gate 3 plan**: [Gate_03_Separation.md](Gate_03_Separation.md) — superseded by this addendum for the sub-task spec; the goal/acceptance criteria in §"Goal" and §"Acceptance criteria" remain authoritative.
- **PLAN.md**: top-level rewrite plan. Gate 3 row updates to ✅ Done after Phase 2 ships and AutoStations pass.
