# PROGRESS — voxelnav-port

> **The living tracker. Trust this file over any session memory or summary.**
> Update at every ruled decision, accepted unit, and session close — never batched.

## Status board

| Phase | State | Evidence |
|---|---|---|
| 0 — Scaffold | **OPEN** — docs authored; baseline PENDING; units not yet dispatched | this file |
| 1 — Octree + voxelize | not started (PHASE_1.md authored at 0→1 boundary) | — |
| 2 — Pathfinding | not started | — |
| 3 — Chunks & dynamics | not started | — |
| 4 — Consumers | not started | — |
| 5 — Perf (merging) | not started | — |
| 6 — Deferred pool | closed by default; items open by decision entry only | — |

## Baseline (campaign open)

- Branch: `feature/ckvoxelnav-port` @ `c11766760` (== origin/dev tip 2026-08-03), UNMODIFIED
  Source/. Invocation: `--build --test --parallel 1 --no-nullrhi` (single-shot, no --config).
- **Total: 981 | Passed: 974 | Failed: 7 | Skipped: 0 | Contaminated: 0 | Duration: 15m24s (tests)**
- Pre-existing failing names (delta-zero means: exactly these 7, no more, no fewer):
  1. `Ck_AutoTest_PathNetworkFollower_RoutePrefersNetwork` (goal-exactness assert)
  2. `Ck_AutoTest_PathNetworkFollower_ComponentTransferUsesDisconnectedIslands`
  3. `Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward` (fixture: no north navmesh boundary)
  4. `Ck_AutoTest_PathNetworkFollower_LocalShortcutUsesSameComponentGap`
  5. `Ck_AutoTest_PathNetworkFollower_ProjectsRibbonWaypointWithinNavQueryExtent` (same fixture issue)
  6. `Ck_AutoTest_PathNetworkFollower_TuningReplansSameGoal`
  7. `Ck_AutoTest_CkJolt_ChaosParity_KinematicPlatformCarry` — editor DIED mid-test (exit 0x3),
     toolbox-marked Failed, run resumed in a fresh lane. Instability, not an assertion red; watch
     whether it recurs at phase gates.
  Also noted: the resumed lane's editor exited 0xFF AFTER completing all tests (results kept).
- All 7 are foreign workstreams (PathNetwork, Jolt parity) — none touched by this campaign.

## Done

- 2026-08-03: Campaign opened. Branch created; sibling detached work preserved
  (`backup/detached-asset-exporter-20260803` = `fcf3da7bd`, unpushed local merge, not ours).
  Doc set authored (PROMPT, PHASE_0, VALIDATION, this file). Research record: 7 dossiers under
  `research/` (5 from the port sweep, 2 from the spatial-structure follow-up).
- 2026-08-03: `research/phase1-port-map.md` (1,381 lines, Opus, orchestrator-reviewed) — full type
  translation, function-level port map (PORT/PORT+FIX/SEAM/DROP per function), 14-stage resumable
  voxelizer design, AStar adapter + volume entity designs. Found SIX verified upstream Nav3D bugs
  (free-space Morton stuffed into 6-bit SubNodeIndex; backwards NavNodeRef unpack ctor; cube-vs-edge
  bounds check in FindNeighbourInDirection; GetRandomPoint one-past-last layer index; 8x-duplicated
  blocked-parent propagation; full-cube RasterizeLayer scan) — all fixed in the map. Behavior change
  to expect at Phase 1 exit: occupancy decided by collision shapes (Jolt), not LOD0 render tris —
  baked results differ from Nav3D by design.

## In-flight

- Baseline toolbox build+test (must complete before 0A–0D Source/ edits land). First launch failed:
  this worktree's `CkAuto/UnrealToolbox.exe` (+LogViewer/crashpad) were unsmudged LFS pointers —
  fixed via `git -C CkAuto lfs pull` (worktree-specific hazard, now healed); run relaunched.
- Opus prep agent drafting `research/phase1-port-map.md` (read-only vs Source/; docs-only write —
  safe during the build).

## Decisions

- **[C-D1]** Module name `CkVoxelNav` (ns `ck::voxelnav`). Zero token collision in the 122-dir
  census; median length; names the representation (CkAStar/CkGrid precedent). Runner-up CkVolumeNav.
- **[C-D2]** Campaign branch bases on origin/dev tip `c11766760`, not the superproject's recorded
  gitlink `7f4cc524d` (an ancestor of it) and not the detached `fcf3da7bd` (unpushed sibling merge,
  preserved as backup branch). Superproject pointer bump happens at ship time.
- **[C-D3]** Search runs on CkAStar via an `astar::AStarGraph` SVO adapter + post-search refinement
  (visibility pruning + 3D funnel). Nav3D's Theta*/LazyTheta* are NOT ported: they don't fit
  CkAStar's closed relaxation loop, and McGill-2024 evidence shows refinement recovers the quality
  while CkAStar's budgeted/warm-start machinery is what horde scale actually needs. CkAStar itself
  untouched.
- **[C-D4]** Graph layer addresses abstract cells from day 1; node merging (the ~4× leaf-count /
  sub-1ms lever) lands in Phase 5 behind that abstraction with an A/B benchmark gate.
- **[C-D5]** The box-occupancy primitive + static-domain filters + first `BroadPhaseQuery` wrapper
  live in **CkJolt**, not CkVoxelNav. Keeps the JPH allowlist closed and the capability reusable.
- **[C-D6]** Commits stay local on `feature/ckvoxelnav-port` (CkFoundation; CkTests gets a same-named
  branch when tests land). No pushes / pointer bumps / dev delivery without an explicit maintainer
  go. (Maintainer's standing goal authorizes the campaign work itself, not publication.)
- **[C-D7]** Scale premise (maintainer, 2026-08-03): CkFoundation must serve future horde-scale
  games (hundreds-to-thousands of navigating enemies), not just Rewind99 (~150 agents). Immutable
  post-bake octree (lock-free parallel reads) is a design requirement, not a nicety.
- **[C-D8]** Recorded follow-ups OUTSIDE this campaign's scope: (a) batch probe AABB updates
  (`CkProbe_Processor.cpp:711` → `NotifyBodiesAABBChanged(ids,N)`); (b) adopt `Get_OverlapEntities`
  / broadphase tier at Compass/Minimap/EQS iterate-all sites; (c) `CkSpatialHash` per-frame-rebuilt
  proximity grid module (kNN + body-free entities) when the first massed consumer lands;
  (d) CkRaySense/CkOverlapBody Chaos→Jolt consolidation.
- **[C-D9]** Phase 3 must audit Nav3D's dynamic-occlusion machinery before porting it — upstream
  issue #39 reports it broken in v2.0. Port the design, verify the behavior, don't trust the code.
- **[C-D10]** (OQ-1, blocker) 0B's consumer surface must be JPH-free: CkJolt ships opaque
  TPimplPtr-backed `FCk_Jolt_QuerySession` + `FCk_Jolt_BoxProbe` value types; the JPH-signature
  functions stay CkJolt-internal. Rejected alternative: allowlisting CkVoxelNav for direct JPH
  includes — would hollow out the Jolt-agnostic backend the maintainer explicitly asked for.
  PHASE_0.md 0B amended.
- **[C-D11]** (OQ-2) Voxel build processors: `FGroup_Transform`, `RunAfter
  FProcessor_JoltWorld_WaitForAsync` + `RunBefore FProcessor_JoltWorld_Step` — the only window
  provably outside the async step. Adjacent finding recorded as follow-up (e) under [C-D8]:
  CkEqs queries Jolt from `FGroup_PostTransform`, i.e. after Step dispatches the async batch — a
  latent async-mode exposure, NOT ours to fix in this campaign.
- **[C-D12]** The port map's recommendations (research/phase1-port-map.md §6 OQ-3..OQ-10) are
  adopted as written — incl. OQ-9 rename `_VoxelExtent`→`_FinestCellSizeUu` (PHASE_0 amended),
  OQ-4 landscape fence (packaged builds have no Jolt landscape outside WITH_EDITOR → volumes over
  landscape voxelize as free; ensure via `Get_NumStaticBodies` sanity signal + doc fence), drop of
  the L1 overlap cache (hierarchy provides the pruning; Jolt any-hit replaces the triangle
  narrowphase). Any implementation-time deviation needs a new decision entry.

## Blockers

- (none)

## Next step

Baseline gate completes → record counts above → dispatch Phase 0 units 0A/0B/0C (parallel, opus)
then 0D → orchestrator re-runs gate → phase boundary ritual → author PHASE_1.md.

## Session log

- 2026-08-03 — orchestrator: Fable 5 (interactive). Research (7 Opus dossiers, ~1.3M subagent
  tokens) → plan → campaign opened: branch, doc set, baseline started. Routing: all execution
  units → opus; judgment/audit inline at orchestrator.
