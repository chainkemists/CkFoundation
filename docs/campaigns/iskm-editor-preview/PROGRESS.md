# PROGRESS — iskm-editor-preview

Living state doc. Update at every session end; trust this over memory of prior sessions.

## Current state (2026-07-17)

| What | State |
|---|---|
| Investigation | **COMPLETE** — root cause confirmed (2 layers), see [PROMPT.md](PROMPT.md) |
| Phase 0 (discriminator) | **COMPLETE** — mesh appears (composition ran) but FROZEN pose; viewport click selects the spawner. Layer 2 is the whole story for the render half. |
| Phase 1 (sequencing gate) | **CLEARED** — EditorSelectionOwner landed on dev; branch rebased onto dev @ `d1c2450e9` |
| Phase 2 (ISKM editor pass) | **DONE** — selection half landed by sibling session (`fd345a68c` + `4313b1820`); frozen-pose half (`SetUpdateAnimationInEditor`) + outliner-hide + per-owner leak reclaim committed this session (`7243f89cb`). |
| Phase 3 (loud truncation warning) | NOT STARTED — heuristic shape needs maintainer sign-off first |
| Phase 4 (docs) | NOT STARTED — module Claude.md contract update still pending |
| NEW defect A (drag-no-move) | **FIXED** (`7a067bcbb`) — `fd189d061` had removed the PostEditMove(finished) rebuild; child-entity compositions (all gym stations) can't follow a root-only push. Restored rebuild-on-release + extended transform-property resolver to match `InitialTransform`. |
| NEW defect B (outliner) | **FIXED** (`7243f89cb`) — `bListedInSceneOutliner=false` on `ACk_IskmRenderer_Actor_UE` (ISM parity). |
| NEW defect C (per-owner leak) | **FIXED for ISKM** (`7243f89cb`) — self-reclaim in `Release_BaseSKMC` + `OnLevelActorDeleted` backstop + `Deinitialize` per-owner destroy. **Ecosystem-wide follow-up:** ISM per-owner renderers + `ACk_EditorSelectionProxyHost_Actor_UE` leak identically (invisible → unnoticed); NOT fixed here. |
| Code changes on this branch | 2 code commits this session (`7a067bcbb`, `7243f89cb`) — **local only, NOT pushed** |
| Build/test baseline | **STILL NOT CAPTURED** — toolbox has no engine bound for CkPlugins on this machine (needs interactive `e` engine-pick once). Automation gate has NOT run against these commits. In-editor behavior confirmed by maintainer ("Works!"). |

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

### 2026-07-17 — Session 2 (Phase 0 + fix pass)

- **Rebase confirmed:** EditorSelectionOwner ecosystem landed on dev (`67220ae96` ContextOwner
  reshape, `972b10e1e` creation-time propagation, `CkEcs/EditorSelectionOwner/` present). Branch
  rebased onto dev @ `d1c2450e9` (local; origin branch tip is pre-rebase). The sibling session
  ALSO landed the ISKM selection half: `fd345a68c` (per-owner renderer actors) + `4313b1820`
  (weak-key map, full world validity). Phase 2 shrank to the anim/leak/outliner residue.
- **Phase 0 result (maintainer-run):** dropped an ISKM EntityScript → mesh renders (composition
  ran → Layer 1 not implicated for that script), pose FROZEN, viewport click selects the spawner
  (sibling selection work verified working). Matches the "frozen mesh → Phase 2 only" row.
- **Three defects surfaced during Phase 0 and fixed:**
  - **A (drag-no-move):** dragging the spawner moved nothing. `PostEditMove` pushed only the ROOT
    entity transform; gym scripts compose proxies on child entities with independent world-space
    transforms (no scene-node link), so the push never reached them. `fd189d061` (2026-05-07) had
    removed the on-release rebuild that used to re-anchor them. Fix (maintainer chose "restore
    rebuild-on-release"): `PostEditMove(InIsFinished=true)` → `EditorOnly_RebuildEntity`; drag keeps
    the in-place push. Also extended the transform-property resolver to match `InitialTransform`
    (gyms name it that; the default only matched `SpawnTransform`, so previews spawned at origin).
  - **B (outliner):** ISKM renderer actors showed in the outliner; ISM hides them via
    `bListedInSceneOutliner=false`. Added the same in the ctor.
  - **C (per-owner leak):** deleting a spawner leaked its per-owner ISKM renderer forever. Added
    self-reclaim at end of `Release_BaseSKMC` (owner gone + pool empty), an `OnLevelActorDeleted`
    backstop for the already-quiescent case, and `Deinitialize` per-owner destroy. Reclaim is
    deferred until pool quiescence so it can't race the ~4-tick entity-destroy cascade (would trip
    `FProcessor_IskmProxy_EndPlay`'s SKMC-missing ensure).
  - **D (frozen pose — the Phase 2 core):** `SetUpdateAnimationInEditor(true)` via new
    `EditorOnly_EnableAnimationTicking`, applied to base SKMCs + both submesh-creation sites.
- **Commits (local, unpushed):** `7a067bcbb` fix(CkEntitySpawner); `7243f89cb` fix(CkIskmRenderer).
- **Verification gap (honest):** compiles + in-editor behavior confirmed by maintainer ("Works!");
  automation suite NOT run — toolbox has no engine bound for CkPlugins on this machine (one-time
  interactive `e` pick still needed). Least-certain claim: leak-reclaim timing under a multi-entity
  scene (that the `Release_BaseSKMC` self-destroy never fires while a sibling proxy is still live on
  the same per-owner actor — guarded by `_LiveSKMCs.IsEmpty()`, but unexercised by tests).

**Open items handed to next session:**
- Run the automation gate once the toolbox engine is bound; A/B-stash if any red appears (no
  pre-change baseline was captured — known pre-existing red on 2026-07-16 tips was
  `Ck.Crowd...OccupiedGoal RESUME`, 782/783, do not re-attribute).
- Phase 3 (loud truncation warning) — still needs maintainer sign-off on heuristic shape.
- Phase 4 docs — update `CkIskmRenderer/Claude.md` editor-preview contract + consider a new
  `CkEntitySpawner/Claude.md`.
- Ecosystem-wide leak follow-up (defect C): ISM per-owner renderers + `ACk_EditorSelectionProxyHost_Actor_UE`.
- Publish: origin branch needs a force-push (local rebase); do NOT bump the superproject
  CkFoundation pointer to this branch until Phases green.

## Next action

Bind the toolbox engine for CkPlugins (interactive `e` pick), then run the automation gate against
`7243f89cb` and record the delta here. Then Phase 3 (pending sign-off) / Phase 4 docs.
