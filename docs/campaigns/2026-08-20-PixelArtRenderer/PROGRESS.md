# PROGRESS — CkPixelArt (t3ssel8r-style 3D pixel-art renderer)

> Living doc — the ONLY place current state lives. PROMPT.md is the locked charter.
> Executor: update this file at every phase boundary, every gate verdict, and every blocker.
> Never improvise past a failed gate — record it under Blockers and stop the phase.

## Status: READY FOR EXECUTION — Phase 0 not started.

Decisions D1–D8 LOCKED 2026-08-20 (maintainer directive to execute; F1 scoped-SVE-re-open
approved; F2 module split approved; F3 render-side snap; F4 content exemplars → Phase 7 backlog;
F5 routing = Opus executes, Fable audits gates).

## Baseline (fill at Phase 0 entry, BEFORE any edit)

- CkFoundation start commit: _(fill)_ · branch `feature/pixel-art-renderer`
- CkTests start commit: _(fill)_ · branch `feature/pixel-art-renderer`
- Full suite (`--test --no-live`): total ____ · pass ____ · fail ____
- Failing test NAMES: _(fill — the delta-zero reference for every later gate)_

## Phase checklist

| Phase | Deliverable | Status | Exit evidence |
|---|---|---|---|
| 0 | Spike: module skeleton, hardcoded upscale proven, D8 gate, empirical checks 7a–7d, baseline | NOT STARTED | |
| 1 | CkPixelArtRender productionized: registry, CVars, lifecycle, fraction/config tests | NOT STARTED | |
| 2 | Snap + margin + remainder; creep/stutter A/B verified | NOT STARTED | |
| 3 | CkCamera ortho (attribute + requests + ViewInfo) | NOT STARTED | |
| 4 | CkPixelArt module: subsystem/preset/settings/CVars, D7 preconditions | NOT STARTED | |
| 5 | PixelArt look (outline + banding + palette + band-shift edges) | NOT STARTED | |
| 6 | Gym, gate of record, perf table, docs, VALIDATION executed | NOT STARTED | |
| 7 | BACKLOG (separate sign-off): god rays, cloud shadows, per-object snap, stencil point-light, BB adoption | BACKLOG | |

## Decision gates — verdicts

- D8 (screen-percentage mechanism): _(fill at Phase 0 step 5.G — (a) cvar / (b) Unchecked driver, + the three-viewport evidence table)_
- Phase 0 checks: 7a PP-at-internal-res ____ · 7b auto-planes resolution-dependence ____ ·
  7c PIE-vs-standalone DPI ____ · 7d ortho light functions ____ (UNTESTED allowed)
- Phase 2 sign gate 6.G: _(fill — final CompSign + evidence captures)_

## Blockers

_(none — executor: paste exact commands + full error text here, then END the phase. Do not
work around a failed gate.)_

## Human verification queue (maintainer)

_(Phase 6 populates from VALIDATION.md §C with exact steps. Items may be queued earlier —
e.g. Phase 0's D1 evidence screenshot, Phase 2's creep captures.)_

## Session log

- 2026-08-20 (Fable, session 1): research fan-out (5 sweeps) → RESEARCH_*.md; PROMPT.md
  authored; decisions locked per maintainer directive; full executor package written
  (PHASE_0–6, VALIDATION.md, this file). No source-code changes anywhere; the campaign folder
  is the only writing. Repo ground at authoring time: CkFoundation dev @ `96b5cad0e`, clean.
