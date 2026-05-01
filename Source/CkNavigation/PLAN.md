---
title: Navigation + Crowd Rewrite Plan
status: in_progress
window: 8 days
last_updated: 2026-04-29
---

# Navigation + Crowd Rewrite — Master Plan

## Why this exists

The dtCrowd-based navigation stack is being replaced wholesale. Recast and UE's
NavigationSystem are kept (path planning); dtCrowd is replaced with a custom
ECS-native steering / avoidance system tuned for the rental-store gameplay
shape (90–110 NPCs, mixed indoor density, server-authoritative, hybrid
Player-as-Actor / NPC-as-Entity).

This file is the **executive index** for the rewrite. It does not contain
gate-level detail — each gate has its own file under [Plan/](Plan/). Update
the status table here as gates land; do not bloat this file.

For the debugger UX contract see [Plan/Debugger_Mockup/index.html](Plan/Debugger_Mockup/index.html).

## Scope decisions (locked)

| Decision | Choice | Why |
|---|---|---|
| **Recast / UE Nav** | Keep | Voxelization + tile regen + editor tooling is months of work to replicate; works fine. |
| **dtCrowd** | Wipe | Frozen agents, MoveTargetDirty races, regen-rebuild dance; corridor-optimize fights local avoidance. Endless edge cases. |
| **Avoidance algorithm** | Separation-force + piercing (no ORCA in this window) | 80% of rental-store cases handled. ORCA can drop into Gate-3-shaped slot post-ship. |
| **Neighbor query backend** | Reuse `CkSpatialQuery` (Jolt probe) | 100 probes/frame is trivial for Jolt; no need for a second broadphase. |
| **Steering output target** | `FFragment_Velocity_Current` via friend access from CkCrowd processors | The chain is `Steering → FFragment_Velocity_Current → FProcessor_EulerIntegrator_Update (computes _DistanceOffset) → sibling consumer enqueues Request_AddLocationOffset → SceneNode`. The integrator does **not** itself write the SceneNode; a per-feature consumer reads `_DistanceOffset` (proven pattern: [CkProjectile_Processor.cpp:23-31](../CkProjectile/Public/CkProjectile/CkProjectile_Processor.cpp)). Gate 2's `FProcessor_CrowdAgent_ApplyOffset` is a copy-paste-with-renames of that pattern. |
| **Queueing logic** | **Out of scope.** Counter queueing is a gameplay/GOAP concern (slot reservation, "you're 3rd in line"), not a steering concern. CkCrowd's contract ends at "moves agent to a target"; the queue manager that picks targets is gameplay-side. | Avoidance alone produces a clump at counters, not a line. ORCA wouldn't fix this either. Gate 7 verifies the rental-store scenario doesn't need queueing for ship; if it does, it's a separate effort. |
| **Replication** | Server-auth only; positions replicate to clients with smoothing | 100 agents × client-side prediction = waste. Smoothing is `FProcessor_Transform_InterpolateToGoal_Location`-style on the consumer side. |
| **Player** | Stays an `ACharacter`; gets a Player Proxy Entity that mirrors transform into the steering layer | Gate 5. Hybrid is supported by design, not retrofit. |
| **Animation / actor-less NPCs** | Out of scope for this window | Separate ~5–7 week effort once the steering layer ships. |
| **Off-mesh links / flowfield / ORCA** | Out of scope | Rental store has no jumps; 100 agents in waypoint mode is fine; ORCA is a Gate-3-replacement once we know separation-only feels wrong. |

## Module layout

Two new sibling modules in **CkFoundation** plugin + one new module in
**CkGameplayDebugger** plugin:

```
Plugins/CkFoundation/Source/
├── CkNavigation/         # SLIM — Recast/FindPath wrapper. No crowd, no agents, no dtCrowd.
│   ├── CkNavigation.Build.cs
│   ├── CkNavigation_Module.{h,cpp}
│   ├── CkNavigation_Log.{h,cpp}
│   └── Public/CkNavigation/
│       ├── Nav/                     # FindPath request type, result fragment, processor, algorithm
│       ├── Settings/                # MaxPathQueriesPerFrame, NavQuerySearchHalfExtent
│       └── Utils/                   # UCk_Utils_Nav_UE BP API
│
└── CkCrowd/              # NEW — agents, steering, avoidance, player proxy
    ├── CkCrowd.Build.cs
    ├── CkCrowd_Module.{h,cpp}
    ├── CkCrowd_Log.{h,cpp}
    └── Public/CkCrowd/
        ├── Agent/                   # FCk_Handle_CrowdAgent + fragments + processors + utils
        ├── PlayerProxy/             # FCk_Handle_PlayerProxy + transform mirror
        └── Settings/                # Tuning defaults: separation weight, sleep threshold, replan threshold

Plugins/CkGameplayDebugger/Source/
└── CkCrowdDebugger/      # NEW — MVVM debugger. Replaces CkNavDebugger.
    ├── CkCrowdDebugger.Build.cs
    ├── CkCrowdDebugger_Module.{h,cpp}
    ├── CkCrowdDebuggerStyle.{h,cpp}
    └── Public/CkCrowdDebugger/
        ├── Data/                    # DataCollector, Types, HealthCheck
        ├── ViewModel/               # FCkCrowdDebugger_ViewModel + 5 multicast delegates
        └── Window/                  # SCkCrowdDebuggerWindow + 5 panels (NavmeshStatus, AgentList, AgentDetail, Stats, EventLog)

Plugins/CkTests/Script/CkCrowd/  # NEW — manual + AutoStation gyms, one set per gate
```

### Build.cs dependency contracts

| Module | Public deps | Notes |
|---|---|---|
| `CkNavigation` | `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`, UE `NavigationSystem`/`AIModule` | No dependency on CkCrowd. Self-contained. |
| `CkCrowd` | `CkNavigation`, `CkPhysics`, `CkSpatialQuery`, `CkShapes`, plus the standard ECS chain | Steering writes into `FFragment_Velocity_Current` via friend access. |
| `CkCrowdDebugger` | `CkNavigation`, `CkCrowd`, `CkPmg`, `CkDebuggerCommon`, plus Slate / WorkspaceMenuStructure / EditorStyle / AppFramework / ToolMenus | Mirrors `CkGoapDebugger` deps. |

## Gate list + status

Each gate has a self-contained file under [Plan/](Plan/) with: goal, acceptance
criteria, file inventory, gym spec (manual + AutoStation), debugger additions,
known risks. **Update the Status column here as gates land.** Do not embed
gate detail in this file — keep the index lean so parallel agents only load
their gate's slice.

| # | Title | File | Day | Status | Notes |
|---|---|---|---|---|---|
| 0 | [Foundation](Plan/Gate_00_Foundation.md) | Module skeletons, agent fragment, debugger window opens, Gym 0 spawns/despawns N agents | D1 | ✅ Done | Compile clean. PIE verified: `ck.CrowdDebugger 1` opens window; spawn/remove agents reflect in Agent List within 1 frame; click → Agent Detail Identity populates and survives across spawn/remove. |
| 1 | [Pathfinding](Plan/Gate_01_Pathfinding.md) | CkNavigation slim rewrite — FindPathSync + project settings + path-result fragment + path visualization in debugger | D2 | ✅ Done | Path query API live (`utils_nav::Request_FindPath`), processor drains via CopyAndRemove, OnPathReady/Failed signals fire to AS. Pathfinding gym + procedural floor + path overlay verified. Navmesh Status panel + Health Check button in debugger. AutoStations: Failure (off-mesh target) + Success (reachable target on a fixture map). |
| 2 | [Locomotion](Plan/Gate_02_Locomotion.md) | Single agent walks A→B. CrowdAgent equivalent of `FProcessor_Projectile_Update` — reads `_DistanceOffset`, enqueues transform request. | D3 | ⏳ Pending | Pattern replication from CkProjectile (proven precedent). ~0.75 day. |
| 3 | [Separation](Plan/Gate_03_Separation.md) | Multi-agent neighbor queries + boids-style separation force. CkSpatialQuery probe per agent. | D4 | ⏳ Pending | Parallelizable — separation + agent flag bitfield can be split. |
| 4 | [Doorways + Replan](Plan/Gate_04_Doorways_Replan.md) | Piercing, sleep/idle, replan-on-blocked. | D5 | ⏳ Pending | Three independent processors → 2–3 parallel agents possible. |
| 5 | [Player Proxy](Plan/Gate_05_PlayerProxy.md) | Mirror Player ACharacter into a CrowdAgent-flavored entity. Soft-push behavior when player is a neighbor. | D5 | ⏳ Pending | Half day. Enabler for "feels right" in rental store. |
| 6 | [Stress + Tuning](Plan/Gate_06_StressTuning.md) | 100+ agents, real-world feel. Stats panel in debugger. AutoStation regression assertions. | D6–D7 | ⏳ Pending | **Higher risk than Gate 2.** Bundles perf tuning AND gameplay tuning in 2 days. If perf eats time, gameplay tuning is what gets cut and shifted into Gate 7. |
| 7 | [Rental-Store Scenario](Plan/Gate_07_RentalStore.md) | Second-pass tuning under the actual rental-store scenario: customers wandering shelves + employees moving + Player walking. Final tuning + docs. | D8 | ⏳ Pending | **Not buffer — second-pass tuning.** Replan/sleep thresholds get retuned here under realistic feel; Gate 6 sets initial values, Gate 7 finishes them. |

Status legend: ⏳ Pending  🟡 In progress  ✅ Done  ⚠️ Blocked  🔁 Iterating

## Calendar (best case → realistic)

| Day | Plan | Realistic |
|---|---|---|
| D1 | Gate 0 | Gate 0 |
| D2 | Gate 1 + start Gate 2 | Gate 1 |
| D3 | Finish Gate 2 + Gate 3 | Gate 2 |
| D4 | Gate 4 (parallel) + Gate 5 | Gate 3 |
| D5 | Gate 5 + start Gate 6 | Gate 4 + 5 |
| D6 | Gate 6 stress | Gate 6 stress |
| D7 | Gate 6 tuning | Gate 6 tuning |
| D8 | Gate 7 | Gate 7 (or buffer for slipped gates) |

If we slip past D5 cumulatively, drop Gate 7's "rental store scenario" gym
to a smaller showcase. The core steering / avoidance / replan / player-proxy
stack is all done by Gate 5 — Gates 6+7 are validation + tuning, not new
functionality. **There is no schedule slack** beyond Gate 7; build that
into expectations rather than treating Gate 7 as buffer.

## Workflow contract for parallel agents

When dispatching a parallel agent for a gate:

1. Their entry point is the gate's file under `Plan/`. They load only that file plus this index.
2. **They update only their own status row in this PLAN.md** when their gate lands. They do not edit other gates' status rows or other gates' plan files. Cross-row edits create merge fights.
3. **They append their work to the relevant module's `Claude.md`** as the gate's permanent contribution. The Claude.md file grows as gates land.
4. Their gym(s) go in `Plugins/CkTests/Script/CkCrowd/` named per the gate doc.
5. Their assertions — when applicable — go in an AutoStation that ships in the gym alongside the manual walkable variant.
6. **Mockup is frozen after Gate 5 lands.** Gates 0–5 may amend `Plan/Debugger_Mockup/` if a panel's shape changes. Gate 6+ tuning-driven tweaks (numeric formatting, color polish, sparkline range) go straight to Slate — do not edit the HTML for these. The mockup retires post-ship anyway.

When a gate has parallel sub-tasks (Gate 4 has piercing + sleep + replan as
independent processors), the gate file lists the sub-tasks; spawn one agent
per sub-task with the file as their context.

## Cross-cutting concerns (read once before any gate)

- **NPC == Entity.** No `AActor` for NPCs. Anything that wants to attach an actor (debug visualization, animation rig once it's added) goes through `CkActor` bridge.
- **Server authority.** All steering / pathing / replan runs server-only. Clients receive replicated transforms with smoothing. Gates that build replication-aware behavior must call `UCk_Utils_Net_UE::Get_HasAuthority` before enqueueing requests.
- **Velocity is the ECS contract.** CkCrowd writes `FFragment_Velocity_Current`. EulerIntegrator advances. SceneNode updates. Replication handles the rest. **Do not bypass this** — no direct SceneNode writes from steering processors.
- **Format specifiers** for `CK_ENSURE_IF_NOT` / `ck::Format` use `{}`, never `%s`/`%d`.
- **AS gotchas** for gym authoring: no lambdas (use UFUNCTIONs + `n"FunctionName"` delegate names), no `static_cast` (use `Cast<>`), `f"{Var}"` interpolation. See [Plan/Gym_Authoring_Cheatsheet.md](Plan/Gym_Authoring_Cheatsheet.md).
- **Debugger contract.** Each gate adds the panels / fields that mockup §N specifies. Do not invent new panels mid-gate; if the mockup is wrong, update the mockup first then the gate file then the implementation.

## Post-ship cleanup (do not start until all gates land)

1. Delete `Plan/` folder (gate plans are disposable).
2. Delete the mockup (it's served its purpose).
3. Keep `Claude.md` in each module — that's the permanent guide.
4. Keep this `PLAN.md` only if the status table has historical value as a record of when each gate landed; otherwise also delete.

The persistent debugger mockup-equivalent post-ship is the actual debugger
window; no need for an HTML version once the Slate one ships.
