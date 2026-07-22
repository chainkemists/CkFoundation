# CkPoi v2 refactor — plan index

> **Last updated:** 2026-07-21. Update this table's Status column in the SAME commit as each gate's
> landing — see Gate_NN exit criteria. If this table and a Gate file ever disagree, trust neither
> until you check `git log` for which one actually moved last.

Mission brief: [PROMPT.md](PROMPT.md). Living log: [PROGRESS.md](PROGRESS.md). Design source:
[REFACTOR_MultiProjectorPoi.md](REFACTOR_MultiProjectorPoi.md).

## Gates

| Gate | Name | Status | Depends on |
|---|---|---|---|
| 1 | [Cadence primitive + CkVisibleRange](Plan/Gate_01_VisibleRange.md) | ✅ Done (2026-07-21) | — |
| 2 | [CkPoiDisplayDefinition](Plan/Gate_02_PoiDisplayDefinition.md) | ✅ Done (2026-07-21) | Gate 1 |
| 3 | [CkPoi meta-feature rewrite](Plan/Gate_03_PoiRewrite.md) | ⏳ Pending | Gate 2 |
| 4 | [CkCompass + CkMinimap rewire](Plan/Gate_04_CompassMinimap.md) | ⏳ Pending | Gate 3 |
| 5 | [Gyms + cleanup + full gate](Plan/Gate_05_GymsAndCleanup.md) | ⏳ Pending | Gate 4 |

## Post-ship cleanup

Once Gate 5 lands and the full test gate is green: this PLAN.md, the Gate_*.md files, and
PROGRESS.md are disposable — delete them. Their permanent contribution survives in
`CkPoi/Claude.md`, `CkPoiDisplayDefinition/Claude.md`, and `CkVisibleRange/Claude.md`.
`REFACTOR_MultiProjectorPoi.md` may be kept as historical record or deleted at that point — decide
at ship time, not now.
