# CkGameSettings campaign — PROGRESS (living document)

**Rule for executors:** update this file at every phase boundary, at every deviation from a PHASE
doc, and before ending any session. A fresh session trusts THIS file over its memory.

## Status board

| Phase | Status | Session date | Gate result (tests green / suite delta) | Commit(s) |
|---|---|---|---|---|
| 0 — Scaffold + registry | NOT STARTED | | | |
| 1 — Persistence + apply | NOT STARTED | | | |
| 2 — Packs | NOT STARTED | | | |
| 3 — Widgets | NOT STARTED | | | |
| 4 — Close-out | NOT STARTED | | | |

## Baseline (fill in Phase 0, before any change)

- Date/commit baseline was captured at:
- Full suite (`--test --no-live`): total = , passed = , failed =
- Failing names (verbatim):
- Known context: 9 pre-existing BB suite failures documented in memory/toolbox notes; `Ck.*.Net`
  only runs via explicit `--test-pattern`.

## Branches

| Repo | Branch | Created | Tip SHA (update each session) |
|---|---|---|---|
| CkFoundation | `feature/game-settings` | | |
| CkTests | `feature/game-settings-tests` (when first needed) | | |
| BusterBlock | `feature/game-settings-adoption` (Phase 4 only) | | |

## Decisions verified at implementation time (executor fills)

- PIE detection mechanism used (Phase 1, PROMPT fence 10):
- `FPlatformUserId` BP-exposability finding (Phase 1.1):
- Toolbox audio device finding (Phase 2.4):
- Registry-vs-spec test split, if the Phase 0.7 fallback was needed:

## Deviations from PHASE docs

_(none yet — every entry: what the doc said, what reality was, what you did, why)_

## Blockers

_(STOP entries land here: phase/step, expected observation, actual observation, verbatim
error/log excerpt, what you ruled out. Do NOT improvise past a STOP.)_

## Follow-ups (out of campaign scope, recorded not fixed)

- CkCVar AS gap (`INTERNAL_*`/`BlueprintInternalUseOnly` → no AS wrapper) — known, pre-existing,
  separate follow-up.
- Full BB `bb.*` settings migration beyond the Phase-4 slice.
- Upscaler pack (DLSS/FSR/XeSS), cloud storage provider, split-screen exercise of Player scope.

## Handoff (Phase 4 fills)

- Ready for audit: NO
- `[EDITOR-VERIFY]` items outstanding: see VALIDATION.md §Human
- Branch/SHA summary for Adam:
