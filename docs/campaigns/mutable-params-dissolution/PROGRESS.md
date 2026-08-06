# PROGRESS — mutable-params-dissolution

Living doc. Newest entries on top within each section. See PROMPT.md for scope, rules and method.

## State

- **Phase:** NOT STARTED. Scope defined 2026-08-06 from the audit at the end of the
  `spec-fragment-granularity` campaign; nothing executed yet.
- **Branch:** `refactor/spec-fragment-params-residues` (CkFoundation), tip `6f5d12b12`, based on
  `dev` at `4afcd039f`. Local only — nothing pushed, no submodule pointer bumps.
- **Baseline to diff against:** 1004 total / 1002 passed / 2 failed. The two reds are the
  pre-existing `PathNetworkFollower_*` pair (sibling navmesh work), NOT ours. Known contention
  flake: `CkJolt_ChaosParity_CcdProjectileStopsAtThinWall` — re-run the full suite before blaming a
  change for it. Full detail in PROMPT.md § Baseline.

## Feature ledger

Order is the recommended one; CkCompass first because it is CkMinimap's twin and the least
defensible thing to leave behind.

| # | Feature | Status | Commit | Gate |
|---|---|---|---|---|
| 1 | CkCompass | not started | — | — |
| 2 | CkUI WorldSpaceWidget | not started | — | — |
| 3 | CkPathNetwork + Follower | not started | — | — |
| 4 | CkGoap Action → CkAStar Params | not started | — | — |
| 5 | CkCameraShake | not started (confirm the mutation exists first) | — | — |
| 6 | CkCrowdAgent | not started (confirm the mutation exists first) | — | — |
| 7 | CkPmg Text | not started | — | — |
| 8 | CkVoxelNavPath | not started | — | — |
| 9 | CkAStar test utils | not started | — | — |

## Log

- 2026-08-06: campaign scoped. Scan A (`TReadWrite<FFragment_*_Params>` in a processor view) is the
  reliable structural tell and returned the six processor-side entries above; scan B
  (`Set_*` onto a retained Params) added three Utils-side ones and ~170 legitimate
  build-a-local-Spec-then-Add hits that are NOT defects. Predecessor campaign fixed CkMinimap
  (`9347e2062`), which is the template.
