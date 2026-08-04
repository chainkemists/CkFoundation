# Campaign: voxelnav-port — CkVoxelNav volumetric 3D navigation

> **Stable mission brief. Do not record moving state here — that lives in [PROGRESS.md](PROGRESS.md).**
> Freshness: authored 2026-08-03 at campaign open. Death condition: superseded when the campaign
> closes (VALIDATION.md fully green) — then this folder is history, and `Source/CkVoxelNav/Claude.md`
> is the living doc.

## Mission

Build **CkVoxelNav**: a first-party volumetric 3D navigation module (Sparse Voxel Octree
free-space representation + pathfinding for flying/swimming/zero-g agents) inside CkFoundation,
ported and adapted from **Nav3D 2.0** (`F:\Nav3D-2.0`, MIT, Darby Costello — attribution required),
with **all world-geometry queries through CkJolt** (never UE collision), ECS-driven lifecycle, and
headroom for horde-scale games (hundreds-to-thousands of navigating agents).

**Done means:** every item in [VALIDATION.md](VALIDATION.md) is green, evidence recorded in
PROGRESS.md.

## Non-goals (fenced out — do not drift into these)

- Replacing CkNavigation/Recast for walking agents. CkNavigation's "Keep Recast" decision stands.
- UE `UNavigationSystemV1`/`ANavigationData` integration (dead code upstream; we expose our own API).
- Tactical reasoning port, cooked/serialized bake, World Partition streaming, off-thread
  voxelization, GPU voxelization, CkGameplayDebugger inspector — all **deferred pool** (Phase 6),
  opened only by explicit decision entry.
- The CkSpatialHash proximity-grid module and the CkJolt probe-batching fix are **separate efforts**
  (recorded as follow-ups, not this campaign's scope).

## Locked decisions (context; the numbered log of record is PROGRESS.md § Decisions)

1. **Name:** `CkVoxelNav`, namespace `ck::voxelnav`, API macro `CKVOXELNAV_API`, T4 module.
2. **Geometry backend:** narrow interface (QueryBodies / IsBoxOccupied / GetBodyBounds /
   IsSegmentBlocked); the only shipped impl calls CkJolt's public surface. CkVoxelNav never
   includes JPH headers directly; the occupancy primitive lives **in CkJolt**.
3. **Search:** SVO satisfies `ck::astar::AStarGraph` and runs on CkAStar (time-sliced budgeted
   searches, warm-start/`ValidateExistingPath`). **No Theta*/LazyTheta* port.** Path quality comes
   from a post-search refinement pass (visibility pruning via the ported octree ray-marcher + 3D
   funnel). CkAStar itself stays untouched.
4. **Cell abstraction from day 1, node merging later:** the graph layer addresses abstract "cells"
   (so McGill-style node merging drops in at the perf phase without reworking search/neighbors);
   plain octree cells first for parity.
5. **Voxelization v1:** budgeted, resumable, game-thread processor(s); every Jolt-touching
   processor declares `RunAfter FProcessor_JoltWorld_WaitForAsync` (hard CkJolt rule).
6. **Stable ids everywhere:** volume/chunk identity is integer ids, never actor pointers.
   Serialization writes packed `uint32` node refs (never raw bitfield memory); Morton pinned `uint64`.
7. **Consumer seam:** paths install via `FCk_Nav_Algorithm::MarkPathPending` /
   `InstallExternalPath` (the CkPathNetwork precedent); CkCrowd gains a third provider branch.
8. **Port hygiene:** no back-compat shims; house style throughout; fix the known upstream bugs on
   the way (XY-only ContainsPoint, NewObject-in-hot-loop raycaster, float-keyed TMaps, O(n)
   ProjectPoint scans, dead mutexes).

## Architecture & module layout

```
Source/CkVoxelNav/
  CkVoxelNav.Build.cs            (CkModuleRules; deps: Core CoreUObject Engine GameplayTags +
                                  CkAStar CkCore CkEcs CkEcsExt CkJolt CkLabel CkLog CkNavigation
                                  CkProfile CkRecord CkSettings CkThirdParty)
  CkVoxelNav_Log.{h,cpp}         (namespace ck::voxelnav)
  CkVoxelNav_Module.{h,cpp}
  Claude.md                      (MUST open with a "vs CkNavigation / vs CkSpatialQuery / vs CkAStar"
                                  boundary paragraph)
  Public/CkVoxelNav/             (NO Private/ folder — house layout, .cpp lives here too)
    Octree/                      (core SVO: types, layers, leaves, Morton, rasterizer, raycaster —
                                  engine-light, backend-agnostic)
    Backend/                     (geometry backend interface + CkJolt-backed impl)
    Volume/                      (volume/chunk entity quartet: Fragment_Data, Fragment, Processor, Utils)
    Path/                        (AStarGraph adapter, refinement, path request quartet)
    Settings/
```

Vendored: `Source/CkThirdParty/Public/CkThirdParty/libmorton/` (verbatim from Nav3D's copy, MIT
LICENSE kept; CkThirdParty.build.cs include path + Claude.md table row + allowlist rule naming
CkVoxelNav sole direct consumer).

## Phase map (each phase gets its PHASE_N.md authored at its boundary, per ck-methodology)

| Phase | Scope | Exit observation (headline) |
|---|---|---|
| 0 | Scaffold: libmorton vendor, CkJolt occupancy API, CkVoxelNav skeleton, first autotest | build+test green, delta-zero vs baseline, scaffold test green |
| 1 | Octree core + Jolt voxelization: port SVO types/rasterizer behind the backend; volume entity; budgeted build processor | a gym scene bakes; occupancy spot-checks match Jolt geometry |
| 2 | Pathfinding: AStarGraph adapter, octree ray-marcher, refinement pass, path request API (C++/BP/AS) | autotest paths traverse authored scenes; refinement shortens paths measurably |
| 3 | Chunks & dynamics: partitioning, stable-id adjacency, cross-chunk paths, dynamic occluder rebuild (audit upstream issue #39 first) | multi-chunk + occluder autotests green |
| 4 | Consumers: InstallExternalPath wiring, CkCrowd third branch, flying-agent exclusion tags, pitch-capable facing | flying agent traverses a gym in PIE `[EDITOR-VERIFY]` |
| 5 | Perf: node merging behind the cell abstraction, benchmark gate, bake parallelism | merged vs plain A/B numbers recorded; no regression in path tests |
| 6 | Deferred pool (each item opens by decision entry only) | — |

## Executor rules (sub-agents read this section verbatim)

- **Skills to load by task type:** building/testing → `build-test` (toolbox only — NEVER raw
  Build.bat/UnrealEditor-Cmd); writing fragments/processors/requests → `ck-macros-and-codegen`;
  authoring tests → `ck-tests-authoring-and-running`; AS exposure questions →
  `ck-angelscript-interop`; change gating → `ck-change-control`.
- **Exemplars (mimicry is mandatory, invention is not):** module quartet shape = `CkTimer`
  (14 files, no Private/); provider/volume/follower shape = `CkPathNetwork`; Jolt usage =
  `Source/CkJolt/Claude.md` (read it in full before touching Jolt) + `CkJoltQuery_Utils.cpp`;
  AStar adapter = `CkPathNetwork_RouteGraph.h` (`static_assert(astar::AStarGraph<...>)`).
- **Research record:** `research/*.md` in this folder — coupling map (`research-coupling.md` has
  the file-by-file port dispositions and the backend interface), Jolt surface (`research-jolt.md`),
  nav-stack seams (`research-navStack.md`), conventions/naming (`research-conventions.md`),
  external evidence (`research-external-survey.md`).
- **Fences:**
  - Do NOT mine `Source/CkNavigation/Plan/` gate docs for mechanism — two of them teach the retired
    ProcessorInjector. `CK_REGISTER_PROCESSOR` at the top of the processor .cpp is current law.
  - Do NOT include JPH headers outside CkJolt. Do NOT create a second `JPH::PhysicsSystem`.
  - Do NOT touch `Plugins/CkGameplayDebugger`, `Plugins/CkTests` pointer state, or any dirty file
    you didn't author — other sessions own them. CkTests gets its own campaign branch when tests land.
  - Do NOT blanket-delete `Script/Generated/`. AS bindings are auto-generated from
    `UCk_Utils_*_UE` classes (`_UE` suffix + `ScriptMixin` meta are load-bearing).
  - Deferred `Request_*` APIs end with `const FCk_Delegate_Request_OnCompleted& InDelegate`
    (AutoCreateRefTerm, no C++ default, ALWAYS last).
  - No source edits while a build/test gate is running; no Script/ edits during test runs.
  - New automation tests need touch+relink; new AS autotests need `--discover-fresh`.
- **STOP conditions (all units):** any design fork, any unenumerated observation, two failed
  attempts on one step → stop, report verbatim evidence. Executors never make design decisions.

## Session-start ritual (any resuming session)

1. Read PROGRESS.md top-to-bottom; trust it over memory/summaries.
2. Read the current PHASE_N.md; re-verify its entry criteria (branch, baseline, HEAD hash).
3. `git -C Plugins/CkFoundation status` — expect `feature/ckvoxelnav-port`, clean apart from your
   own in-flight work. Superproject dirty state on other submodules belongs to other sessions.
4. Check the staleness sweep: if code moved since PROGRESS.md's last entry, reconcile the doc first.
