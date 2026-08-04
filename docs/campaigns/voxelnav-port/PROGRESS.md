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

- Branch: `feature/ckvoxelnav-port` @ `c11766760` (== origin/dev tip 2026-08-03).
- Full toolbox build+test counts: **PENDING — run in flight; record totals + failing names here
  before any Source/ edit lands.**

## Done

- 2026-08-03: Campaign opened. Branch created; sibling detached work preserved
  (`backup/detached-asset-exporter-20260803` = `fcf3da7bd`, unpushed local merge, not ours).
  Doc set authored (PROMPT, PHASE_0, VALIDATION, this file). Research record: 7 dossiers under
  `research/` (5 from the port sweep, 2 from the spatial-structure follow-up).

## In-flight

- Baseline toolbox build+test (must complete before 0A–0D Source/ edits land).

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

## Blockers

- (none)

## Next step

Baseline gate completes → record counts above → dispatch Phase 0 units 0A/0B/0C (parallel, opus)
then 0D → orchestrator re-runs gate → phase boundary ritual → author PHASE_1.md.

## Session log

- 2026-08-03 — orchestrator: Fable 5 (interactive). Research (7 Opus dossiers, ~1.3M subagent
  tokens) → plan → campaign opened: branch, doc set, baseline started. Routing: all execution
  units → opus; judgment/audit inline at orchestrator.
