# Streamer diagnostic visibility — PROGRESS

## Current state

**As of 2026-08-24:** Gate 0 is done; Gate 1 source work is complete and awaits manual capture verification.
**Baseline being diffed against:** no current full-suite baseline was captured because the shared checkout had unrelated, moving submodule pointers. The focused final-binary policy test passed 1/1.
**Next action:** run the exact manual capture check with `-CkStreamerMode` in the target marketing game.
**Blocked on:** nothing.

## Decision log

| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-08-24 | Use a standalone CkCore diagnostic-visibility contract | The policy must govern raw draw, retained geometry, and viewport UI. | Never; only evolve its documented public modes. |
| 2026-08-24 | Keep published launch/CVar spellings | Marketing needs a stable capture command. | A versioned replacement is explicitly requested. |

## Dated entries

### 2026-08-24 — core policy and final seams compiled

- Ran: final incremental CkPlugins Editor build plus `Ck.CkCore.Diagnostics.Visibility.CVar` → build succeeded; total 1, passed 1, failed 0, contaminated 0. Evidence: `Saved/Logs/StreamerDiagnosticVisibility-CallbackFix-20260824.log` lines 77 and 361-366.
- Confirmed: CkCore now owns `ECk_DiagnosticVisibility_Mode`, `-CkStreamerMode`, and `ck.Debug.StreamerMode`; existing diagnostic callers consume the policy directly.
- Confirmed: retained PMG/DebugScene and viewport watermark/profile-HUD paths retain local state while the global policy hides their final presentation seam.
- Confirmed: the legacy GameplayDebugger canvas callback now exits before menu/submenu canvas drawing, closing the final post-change audit escape.
- Inferred (requires manual capture): all affected surfaces are visually absent in the target game's actual marketing launch configuration.

### 2026-08-24 — audit and campaign start

- Confirmed: `UNREAL NAV: Pending` is CkCrowd runtime diagnostic text; its current producer has an early suppression guard.
- Confirmed: watermark, GameplayDebugger profile HUD, PMG DebugShape, and CkDebugScene have final visibility seams outside the draw-wrapper contract.
- Inferred: a diagnostic policy that owns CVar transitions can hide retained surfaces without overwriting their local settings; focused lifecycle tests will confirm this.

## Open items

| Item | Status | Next step |
|---|---|---|
| Core policy | In progress | Implement typed contract and focused test. |
| Retained surfaces | Pending | Migrate PMG and DebugScene final visibility. |
| Viewport UI | Pending | Migrate watermark and profile HUD dynamic visibility. |
| Capture proof | Pending | Run fresh build/tests and provide manual launch checklist. |
