# Continuation — CkCrowd Diagnostic Gym + AutoTests

**One-line summary:** tighten the Phase 2 avoidance iteration loop. Build the AutoTest suite + a new auto-cycling diagnostic gym that dumps RDP-compressed path metrics to log, so the next round of behavior fixes happens against unambiguous data instead of screenshots.

---

## Repo state at session start

| Repo | Branch | Status |
|---|---|---|
| `D:\Repos\CkPlugins` (parent) | `dev` | Working tree clean. Submodule pointers from prior session — will be updated post this session's commits. |
| `Plugins/CkFoundation` (submodule) | `feature/navigation` | HEAD: `748f234c6 fix(Crowd): wSide formula independent of neighbor position`. All Phase 1 + Phase 2 + push-apart processors in place. |
| `Plugins/CkGameplayDebugger` (submodule) | `dev` | HEAD: `d4b1bf6 feat(CrowdDebugger): Gate 3 — Neighbors + Steering Forces sections`. No work this session. |
| `Plugins/CkTests` (submodule) | `feature/navigation` | Rebased onto `origin/dev` this session (24 commits replayed; 1 dropped as upstream-equivalent). |

**Working tree:** clean across all submodules + parent.

---

## Current behavior (as of session end)

✅ **2-agent head-on** works well after the wSide rewrite:
- No vibration
- No clipping
- Smooth lateral arcs, agents return to goal

❌ **2-agent spawn rotation glitch** — visible:
- Agents face each other on spawn (correct — initial yaw toward target)
- Then briefly rotate to face world +X
- Then rotate back to target

❌ **5-agent cluster** is broken:
- Agents converge OK at first
- Then "push toward target" again with one capsule visibly lifting up
- Eventually intersect and rotate rapidly

These regressions need to be debugged against METRICS, not screenshots — that's why we're building the diagnostic gym before continuing the behavior work.

---

## Known bugs to fix immediately

### 1. Spawn rotation glitch (1-line fix)

**Root cause:** `FProcessor_CrowdAgent_AccelClamp::ForEachEntity()` slerps direction from `LastDir` to `NewDir`. When `LastSpeed ≈ 0` (newly spawned agent, never had velocity yet), `LastDir` falls back to `FVector::ForwardVector` (= world +X). On first frame post-path-resolve, slerp goes from world-+X → path-direction. The arc passes through visible intermediate angles. FaceAngle visualizes this as the agent rotating from world-+X to its goal direction.

**Fix:** when `LastSpeed < KINDA_SMALL_NUMBER`, use `NewDir` directly (no slerp from a default; the agent has no prior direction to slerp from):

```cpp
// In FProcessor_CrowdAgent_AccelClamp::ForEachEntity, the LastDir/NewDir block:
const auto LastDir = (LastSpeed > KINDA_SMALL_NUMBER)
    ? LastVel / LastSpeed
    : NewDir;  // no prior direction → adopt new direction immediately, skip the slerp arc
```

File: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.cpp`.

---

## Work plan for this session

### Task A — AutoTests (highest priority)

Implement the three tests already specified in `Plan/Gate_03_Separation_Hybrid_Plan.md` Task 8. Reference pattern: `Plugins/CkTests/Script/CkCrowd/CkAutoTest_Crowd_Pathfinding_Success.as`.

**Three test cases, each is a `UCk_AutoTest_X : UCk_AutoTest_Base` + `ACk_AutoTest_X_Actor : ACk_AutoTestRunner` pair:**

1. **HeadOnPass** — 2 agents 1500cm apart head-on. Polls min-separation every 50ms via timer. Asserts min-sep ≥ 63cm (= `_Radius * 1.5`).
2. **Convergence** — 5 agents around 600cm circle targeting centre. After 6s asserts all reached + no pair within 84cm (= `_Radius * 2`).
3. **Vibration** — 2 agents head-on. Samples `_DesiredVelocity` direction every 50ms. Asserts max angular delta < 30° over 3s.

**Run via Window → Test Automation** filtered to `CkTests.CkAutoTest_Crowd_Separation_*`. Pass/fail with quantitative messages.

**Why first:** zero-friction binary signal. We can run them after every code change without firing up PIE.

### Task B — Spawn rotation fix

The 1-line edit in `CkCrowdAgent_AccelClamp_Processor.cpp` documented above.

### Task C — Diagnostic Gym (new)

**Map:** new level (or dup of existing nav-bake level) with a 6000×3000cm floor and a NavMeshBoundsVolume covering it. GameMode override to a new `ACk_CrowdGym_Diag_GameMode`.

**Layout (simultaneous stations):**
- Origin at world (0, 0, 0). Floor extends ±3000cm X, ±1500cm Y.
- Left station "HeadOn" centred at world (-1500, 0, 0). Spawns 2 agents at (-2250, 0, 100) and (-750, 0, 100). Targets reversed.
- Right station "Cluster" centred at world (+1500, 0, 0). Spawns 5 agents on a 600cm circle around centre. Targets all = centre.
- **Stations 3000cm apart, well past 284cm probe-mutual-overlap threshold.** No cross-station influence.

**Auto-cycle (default ON, controllable via `ck.Crowd.AutoCycle`):**
- t=0.0s: spawn + issue MoveTo for both stations simultaneously
- t=0.0s → t=9.0s: agents execute. Recorder samples per-agent (x, y, speed, dirDeg) at 10Hz (100ms interval) into per-agent ring buffer.
- t=9.0s: cycle "ending" — compute metrics, RDP-simplify path, log digest
- t=10.0s: destroy all agents, increment cycle counter, repeat

**CVar interface:**
- `ck.Crowd.AutoCycle` (default 1) — 0 pauses cycling
- `ck.Crowd.DiagSampleHz` (default 10) — recorder rate
- `ck.Crowd.DiagRDPEpsilon` (default 8.0) — path simplification tolerance in cm

**Console commands (gym-side, not CVar):**
- `Ck_GymCrowd_Diag_Pause` — pauses cycling without modifying CVar
- `Ck_GymCrowd_Diag_Resume`
- `Ck_GymCrowd_Diag_DumpNow` — emit current cycle's digest immediately (for inspecting in-progress state)

### Task D — Path recording infrastructure

**Per-agent path recorder fragment:**
```cpp
struct FFragment_CrowdDiag_Recorder
{
    TArray<FCk_CrowdDiag_PathSample> _Samples;   // (t, x, y, speed, dirRad)
    float _NextSampleAt = 0.0f;
    FVector _StartPos = FVector::ZeroVector;
    FVector _GoalPos  = FVector::ZeroVector;
    float _MinSepAcrossCycle = TNumericLimits<float>::Max();
    float _MinSepTime = 0.0f;
    int32 _DirReversalCount = 0;
    float _MaxAngularDelta = 0.0f;
    bool  _Reached = false;
    float _TimeToGoal = 0.0f;
};
```

A `FProcessor_CrowdDiag_PathRecorder` ticks each frame, samples agents in the diagnostic gym only, populates the buffer, computes per-step metrics (min-sep against neighbors, angular delta from prior sample). Record only on agents tagged `FTag_CrowdDiag_Tracked` (added by gym at spawn).

### Task E — RDP simplification + digest emission

**Algorithm:** Ramer-Douglas-Peucker — standard polyline simplification.
**Implementation:** lift from a known reference (FRDPSimplifier in CkCore if exists, else a 30-line cpp). Recursive; uses point-to-line perpendicular distance against epsilon.

**Digest format** (grep-friendly; one line per field):

```
[CrowdDiag][C{cycle}][{station}][A{idx}] start=(x, y, z) goal=(x, y, z)
[CrowdDiag][C{cycle}][{station}][A{idx}] reached=true t_to_goal=4.20
[CrowdDiag][C{cycle}][{station}][A{idx}] path_len=1632.4 straight=1500.0 efficiency=0.919
[CrowdDiag][C{cycle}][{station}][A{idx}] min_sep_to_neighbors=78.4 at t=2.40
[CrowdDiag][C{cycle}][{station}][A{idx}] dir_reversals=2 max_angular_delta=18.4
[CrowdDiag][C{cycle}][{station}][A{idx}] simplified_path: t=0.00 x=-750.0 y=0.0 v=0
[CrowdDiag][C{cycle}][{station}][A{idx}] simplified_path: t=0.42 x=-650.5 y=2.1 v=240
[CrowdDiag][C{cycle}][{station}][A{idx}] simplified_path: t=1.20 x=-280.3 y=42.5 v=240
... up to ~20 simplified_path lines per agent
[CrowdDiag][C{cycle}][{station}][A{idx}] simplified_path: t=4.20 x=750.1 y=0.5 v=0
```

**Log channel:** `LogCk_Crowd: Display:` so it's visible at default verbosity.
**Naming:** prefix every line with `[CrowdDiag][C{cycle}][{station}][A{idx}]` so I can grep `Saved/Logs/CkTests.log` cleanly.

### Task F — Iterate on cluster behavior using digests

Once the infrastructure is in place:
1. User runs the diagnostic gym in PIE, lets it cycle 5-10 times
2. Saves `Saved/Logs/CkTests.log`
3. I read it via Bash + grep `\[CrowdDiag\]`
4. Compare cluster metrics across cycles, identify the bug pattern (efficiency drop, dir-reversals spike, min-sep violation, etc.)
5. Propose fix; user applies via Live Coding; re-run

This loop replaces the screenshot+verbal-description loop with copy-paste-the-log-file loop.

---

## Confirmed answers from prior session

| Q | Answer |
|---|---|
| RDP epsilon default | 8 cm (current 2-agent test showed no wiggle at this level) |
| Sample rate | 10 Hz (100 ms interval) |
| Cycle duration | 10 s (start there; bump to 12 s if `reached=false` shows up frequently) |
| Default auto-cycle | ON |
| Both stations simultaneous | YES, but spaced 3000+ cm apart so probes can't cross |

---

## Project settings to verify on session start

In **Project Settings → Plugins → CkFoundation → Crowd**:

- `_AccelClampMode` = Enabled
- `_AvoidanceSampleTrigger` = NeighborCountAndZoneTag
- `_AvoidanceSampleNeighborThreshold` = 1 *(production default — sample whenever there's a neighbor)*
- `_AvoidanceSampleStride` = 1
- `_AvoidanceSidePreference` = PassLeft
- `_AvoidanceSampleAngularDivs` = 8
- `_AvoidanceSampleRings` = 2
- Penalty weights: WDes=2.0, WCur=1.0, WSide=0.75, WToi=2.5, HorizonTime=2.5
- `_PushApartMode` = Standard

If saved config has different values from prior session, force these defaults before testing.

---

## Architecture gotchas accumulated to date (read before any code changes)

| Gotcha | Fix / convention |
|---|---|
| AS f-string can't format typesafe handles. Cast to `FCk_Handle` generic before `f"...{handle}..."`. | `FCk_Handle Generic = Typesafe; ck::crowd::Log(f"{Generic}");` |
| Every `TProcessor` needs `CK_REGISTER_PROCESSOR` in its `.cpp` plus `#include "CkEcs/Scheduler/CkProcessorRegistration.h"`. Without it, body never runs (silent no-op). Live Coding may not pick up new registrations cleanly — full IDE rebuild after adding. | Always add the registration line at the top of new processor `.cpp` files. |
| AS Math constants are uppercase: `Math::PI` not `Math::Pi`. One typo cascades into "not declared" errors on every later local in the same function. | Use uppercase. When debugging cascading errors, fix the topmost first. |
| Use `ck::<module>::Log/Warning/Error` not `ck::Trace/Warning/Error` in gyms and module code. `ck::Trace` is `System::PrintString` (on-screen, not filterable). Module-scoped versions hit the real LogCategory and can be filtered via `Log <Category> <Verbosity>`. | `ck::crowd::Log(...)`, `ck::crowd::Warning(...)`, etc. |
| Saved config in `DefaultGame.ini` etc. can override new defaults set in code. After changing a default, verify the in-editor Project Settings UI shows the expected value; reset to default if not. | Force-set new defaults in Project Settings UI on session start. |
| `_LastVelocity` field on `FFragment_CrowdAgent_DesiredVelocityData` is the per-frame baseline for AccelClamp. AccelClamp writes both `_Velocity` and `_LastVelocity` at end of body so subsequent reads are consistent. | Don't touch `_LastVelocity` from outside AccelClamp. |
| AccelClamp's slerp falls back to `FVector::ForwardVector` when `LastSpeed ≈ 0`. Causes the visible spawn-rotation glitch. | KNOWN-BUG #1 above — fix in this session. |

---

## What's out of scope

- ORCA / RVO2 — still post-Gate-7 deferred
- Sleep optimization — Gate 4 of original PLAN
- Replan / movement-timeout — Gate 4
- Z-lift in cluster — needs metric-based diagnosis first; revisit when digests show the pattern
- Push-apart edge cases beyond what HeadOnPass / Convergence AutoTests catch
- The wider PLAN.md status update — Gate 3 ✅ Done line goes in after AutoTests pass

---

## Suggested commit boundaries this session

1. `fix(Crowd): AccelClamp uses NewDir as baseline when no prior velocity` (Task B, the spawn-rotation fix)
2. `feat(CrowdAutoTest): HeadOnPass + Convergence + Vibration AutoStations` (Task A)
3. `feat(CrowdDiag): path recorder fragment + processor` (Task D)
4. `feat(CkCore): RDP polyline simplification helper` (Task E pre-req, if not present in CkCore yet)
5. `feat(CrowdDiag): diagnostic gym with auto-cycling 2+5 agent stations` (Task C)
6. `feat(CrowdDiag): end-of-cycle digest emission to LogCkCrowd` (rest of Task E)
7. *(behavior fixes that come out of the iteration loop, individually committed)*
8. `chore: bump submodules — Gate 3 testing infrastructure done`

---

## Suggested first message for the new session

> Continuing from CONTINUATION_PROMPT_DiagnosticGym.md. Read it fully before doing anything. Goal: ship the AutoTest trio + the diagnostic gym so we have unambiguous metrics for the cluster-behavior debugging loop. Order: Task A (AutoTests) → Task B (spawn-rotation 1-line fix) → Task C-E (diagnostic gym + recorder + digest emission) → Task F (iterate on cluster). Pause for review after each Task; cluster fixes are deferred until the infrastructure can show me the data.
