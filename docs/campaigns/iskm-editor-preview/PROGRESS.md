# PROGRESS — iskm-editor-preview

Living state doc. Update at every session end; trust this over memory of prior sessions.

## Current state (2026-07-16)

| What | State |
|---|---|
| Investigation | **COMPLETE** — root cause confirmed (2 layers), see [PROMPT.md](PROMPT.md) |
| Phase 0 (discriminator) | NOT STARTED — needs editor open ([EDITOR-VERIFY]) |
| Phase 1 (sequencing gate) | BLOCKED — sibling editor-selection work not yet on dev |
| Phase 2 (ISKM editor pass) | NOT STARTED |
| Phase 3 (loud truncation warning) | NOT STARTED — heuristic shape needs maintainer sign-off first |
| Phase 4 (docs) | NOT STARTED |
| Code changes on this branch | **NONE** — documentation only |
| Build/test baseline | **NOT CAPTURED** — no builds were run this session (read-only). Capture via toolbox before first code change. |

## Session ledger

### 2026-07-16 — Session 1 (investigation, read-only)

- Traced the full drag-drop → editor-preview pipeline; confirmed the editor ECS world runs
  Construct/OnConstructed only (BeginPlay/Replicate/PendingReplicationRetry are RuntimeOnly ghost
  nodes — `CkEntityScript_Processor.h:85,139,166`).
- Confirmed CkIskmRenderer is world-type-clean but has zero editor affordances
  (no `editor_selection_owner`, no `SetUpdateAnimationInEditor` — 0 hits module-wide).
- Confirmed engine behavior: a set-up SKMC in an Editor world renders a frozen pose, not nothing
  (engine `SkeletalMeshComponent.cpp:1070-1093`).
- Built the feature-coverage table and 5-phase fix plan (PROMPT.md).
- Verified cited history: preview system built in the 2026-04-23 burst
  `e319881e5 → 43e189329 → fb09f4ed7 → df279b7d9 → 7ce2f978e`; ISKM intent shown by
  `927acdf00` + `96aeaf1dd`.
- Discovered the sibling session's in-flight editor-selection work (10 files + new
  `CkEcs/Public/CkEcs/EditorSelectionOwner/`; observed both as stash `03a32641f`+`618ab0f13`
  and as live working-tree modifications). Zero ISKM files in it. Phase 2 must wait for it.
- Deliverables committed on `bugfix/iskm-editor-preview`: PROMPT.md, this file, the visual
  digest, and `Source/CkIskmRenderer/CONTINUATION_PROMPT_IskmEditorPreview.md`.

**Open items handed to next session:** Phase 0 discriminator (the ONE unresolved question:
"nothing at all" vs "frozen mesh"); which EntityScript the reporter actually drag-dropped.

## Next action

Run Phase 0 from PROMPT.md: drag-drop an ISKM EntityScript via the entity spawner in the editor
with the Output Log open, and classify the symptom (silence / ensure / frozen mesh).
