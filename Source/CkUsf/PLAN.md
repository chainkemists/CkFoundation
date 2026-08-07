# Stylize campaign — executive index (PLAN.md)

> **Written:** 2026-08-06. Status table updated in the SAME commit as each gate landing —
> a stale row here is a doc bug (ck-methodology §7, Exhibit A).
> **This doc dies when:** the campaign ships; delete with Plan/ per post-ship cleanup.

Mission: [PROMPT.md](PROMPT.md). Living state: [PROGRESS.md](PROGRESS.md).
Research: [Plan/Research_yShade_HandDrawn.md](Plan/Research_yShade_HandDrawn.md),
[Plan/Research_yShade_CelDither.md](Plan/Research_yShade_CelDither.md).

## Gates

| Gate | Name | Contract | Status |
|---|---|---|---|
| 1 | Foundation — generator extension + pattern library + param-count proof | [Plan/Gate_01_Foundation.md](Plan/Gate_01_Foundation.md) | ✅ Done (2026-08-06) |
| 2 | ScreenDither — look + subsystem + presets + tests | [Plan/Gate_02_ScreenDither.md](Plan/Gate_02_ScreenDither.md) | ⏳ Pending |
| 3 | CelShade — look + subsystem + stencil per-object + entity API | [Plan/Gate_03_CelShade.md](Plan/Gate_03_CelShade.md) | ⏳ Pending |
| 4 | HandDrawn — look + subsystem + presets | [Plan/Gate_04_HandDrawn.md](Plan/Gate_04_HandDrawn.md) | ⏳ Pending |
| 5 | Ship — project defaults, CVars, gyms, docs, audit | [Plan/Gate_05_Ship.md](Plan/Gate_05_Ship.md) | ⏳ Pending |

Gate order rationale: 2 before 3/4 because ScreenDither has the fewest unknowns (no GBuffer
reads, no stencil, post-tonemap) — it proves the look+subsystem+preset+test recipe end-to-end
cheaply; 3 before 4 because CelShade carries the most new infrastructure (stencil path, entity
API) and its lessons feed HandDrawn, which is then mostly mechanical.

## Post-ship cleanup

Delete PROMPT.md, PLAN.md, PROGRESS.md, and Plan/ once CkUsf/Claude.md carries the permanent
contract (feature docs, stencil contract, subsystem API, limitations). The Claude.md is the
survivor; these are scaffolding.
