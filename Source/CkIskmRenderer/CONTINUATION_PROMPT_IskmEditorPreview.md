# CONTINUATION PROMPT — ISKM editor-world preview (entity spawner)

**Picking up:** a completed read-only investigation into "[CkFoundation] Why are certain features
not previewing out of PIE in the editor world when drag-and-dropping them in the level via the
entity spawner (e.g. ISKM)?" — root cause is confirmed; your job is Phase 0 onward of the fix
campaign in `docs/campaigns/iskm-editor-preview/PROMPT.md`. Read that file and its PROGRESS.md
before doing anything else; this prompt adds the session context those don't carry.

## 1. Repo state

- **You are in** the CkPlugins2 host project (`D:/Repos/CkPlugins2`); the work lives in the
  `Plugins/CkFoundation` submodule.
- **This branch:** `bugfix/iskm-editor-preview` in CkFoundation, based on `origin/dev` @
  `ac101e415`. It contains DOCUMENTATION ONLY (campaign docs + this prompt + a digest HTML).
  No code changes exist anywhere for this campaign yet.
- **The superproject was NOT bumped** to this branch — its gitlink still points wherever dev is.
  Do not bump the CkFoundation pointer to a docs-only branch.
- **Sibling in-flight work (critical):** at investigation time another session owned uncommitted
  editor-selection work in CkFoundation: 10 modified files
  (CkEntitySpawner_Actor.h/.cpp, CkIsmSubsystem.h/.cpp, CkIsmProxy_Fragment.h,
  CkIsmProxy_OutlineProcessor.cpp, CkIsmProxy_Processor.cpp, CkUnrealComponent_Processor.cpp,
  CkComponentHost_Subsystem.h/.cpp) plus untracked
  `Source/CkEcs/Public/CkEcs/EditorSelectionOwner/` (the `ck::editor_selection_owner` API).
  It flipped between working-tree-dirty and stashed
  ("On dev: selection-redirect A/B stash for crowd-test attribution", `03a32641f` +
  untracked companion `618ab0f13`) DURING the investigation — treat the tree as volatile.
  **First thing: check whether that work has landed on dev** (look for
  `Source/CkEcs/Public/CkEcs/EditorSelectionOwner/CkEditorSelectionOwner.h` in committed
  history). If yes → rebase this branch onto dev and re-verify line cites. If no → Phase 2 is
  blocked; Phases 0/3/4 are not.

## 2. The bug, precisely

User-visible: drag-drop an EntityScript onto the level via `ACk_EntitySpawner_UE` → some
features show a live preview in the editor world (ISM meshes, hosted components, transforms
track the spawner drag) but ISKM shows nothing until PIE.

Technical: the preview entity is spawned into a dedicated editor ECS world
(`UCk_EditorEcsWorld_Subsystem_UE`, own registry, Editor worlds only) whose processor graph
ghosts every `WorldTypeRequirement = RuntimeOnly` processor. EntityScript
BeginPlay/Replicate/PendingReplicationRetry are RuntimeOnly → previews run
**Construct + OnConstructed only**. Independently, CkIskmRenderer never received the
editor-hardening pass CkIsmRenderer got (`fb09f4ed7`).

## 3. The ONE open question (answer it first — Phase 0)

We could not open the editor (read-only session, repo was busy). A set-up SKMC in an Editor
world renders a **frozen pose, not nothing** (engine one-shot InitAnim tick,
`SkeletalMeshComponent.cpp:1070-1093` in the UnrealEngine-Angelscript fork). Therefore:

| If you observe | Cause | Act on |
|---|---|---|
| Nothing at all + silent log | ISKM composition never ran: script composes in BeginPlay / OnReplicationComplete (both never fire in previews) | Move preview-relevant composition to `DoConstruct`; also consider Phase 3 warning |
| Nothing + on-screen "missing content" print | Gym scripts' own early-out — `RendererData` failed to resolve (`CkIskmRenderer_GymStation.as:80-84`) | Asset path/loading issue, not lifecycle |
| `CK_ENSURE` fires from IskmProxy/IskmRenderer Setup | Setup ran and failed at a specific guard | The named guard (`CkIskmProxy_Processor.cpp:90-118`, `CkIskmRenderer_Processor.cpp:22-39`) |
| Frozen mesh, wrong/ref pose, unselectable | Layer 2 only — the expected state after composition succeeds | Phase 2 (ISM-parity pass) |

Ask the user (or check yourself in the editor): **which EntityScript was drag-dropped, and which
row above matches?** Everything downstream branches on this.

## 4. What was already established — do NOT re-investigate

| Ruled out | Evidence |
|---|---|
| "ISKM subsystem doesn't exist in editor worlds" | `CkIskmSubsystem.cpp:92-104` accepts Editor/EditorPreview (`927acdf00`) |
| "ISKM processors are world-gated out of the editor graph" | 0 `WorldTypeRequirement` hits module-wide → default `All` |
| "Manager actor needs BeginPlay" | `DoInitialize` called explicitly post-spawn (`CkIskmSubsystem.cpp:130`); BeginPlay is Super-only |
| "Utils Add has world checks" | `CkIskmRenderer_Utils.cpp:8-27`, `CkIskmProxy_Utils.cpp:20-43` — none |
| "The editor ECS world doesn't tick" | `IsTickableInEditor()=true`; ticks all schedulers each editor frame, pump-to-quiescence; skipped only while PIE is active |
| "A set-up SKMC renders nothing in editor worlds" | Engine renders frozen pose at registration (see §3) |
| "Reference ISKM scripts compose in BeginPlay" | Gym stations compose entirely in `DoConstruct` (`CkIskmRenderer_GymStation.as:74-128`) |

## 5. Critical files

- `Source/CkEntitySpawner/Public/CkEntitySpawner/CkEntitySpawner_Actor.cpp` — spawner; editor
  path is `EditorOnly_RebuildEntity`/`EditorOnly_DoRebuildEntity` (:221-313), runtime path is
  `BeginPlay→DoSpawnEntity` (:411-460). PIE-transition + in-flight-destroy re-arm guards here.
- `Source/CkEcs/Public/CkEcs/Subsystem/CkEcsEditor_Subsystem.cpp` — the editor ECS world:
  ShouldCreateSubsystem (:43), transient entity setup incl. `FTag_EditorOnlyEntity` cascade (:87)
  and ClientAndHost/Authority net identity (:79-84), graph build with Editor context (:301-307),
  `Request_SpawnEditorEntity` (:193-222), `Get_IsEditorEcsMutationSafe`.
- `Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript_Processor.h` — the three RuntimeOnly
  lifecycle processors (:85, :139, :166). `.cpp:224` is the Construct call site.
- `Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorDescriptor.h` — `WorldTypeRequirement` enum +
  ghost-node contract (:37-58).
- `Source/CkIskmRenderer/Public/CkIskmRenderer/CkIskmSubsystem.cpp` — ISKM manager subsystem +
  actor; `GetOrCreate_RendererActor` keyed per RendererData (:108-134) — Phase 2 splits this
  per-(RendererData, SelectionOwner) in editor worlds.
- `Source/CkIskmRenderer/Public/CkIskmRenderer/Proxy/CkIskmProxy_Processor.cpp` — Setup
  (:76-220s): acquires SKMC, sets mesh/anim class; Phase 2 adds editor-selection caching +
  `SetUpdateAnimationInEditor` here.
- `Source/CkIsmRenderer/Public/CkIsmRenderer/CkIsmSubsystem.cpp` — THE TEMPLATE. Editor
  world-type override (:150), transient renderer actors + "Editor worlds never fire BeginPlay"
  explicit init (:188-195), leaked-actor cleanup (:133-141).
- `Source/CkIsmRenderer/Public/CkIsmRenderer/Proxy/CkIsmProxy_Processor.cpp` — editor
  selection-owner caching pattern to mirror (:161-171); note one ISM proxy processor is
  RuntimeOnly (`CkIsmProxy_Processor.h:145`) — check which before assuming symmetry.
- `Plugins/CkTests/Script/CkIskmRenderer/CkIskmRenderer_GymStation.as` — reference ISKM
  composition (all `DoConstruct`); good Phase 0 vehicle.
- `docs/campaigns/iskm-editor-preview/PROMPT.md` + `PROGRESS.md` — the plan and living state.

## 6. Architecture gotchas learned this session

- **Ghost nodes**: RuntimeOnly processors stay in the editor graph for ordering but never
  execute — no log, no warning. Silent-absence bugs look exactly like this one.
- **`FTag_EditorOnlyEntity`** cascades from the editor transient to every descendant
  (`CkEntityLifetime_Utils.cpp:539-540`); `TIgnoreInEditor<...>` in a processor's template list
  switches its editor-view query (drops that requirement, requires the tag) vs runtime view
  (excludes the tag) — see `CkProcessor.h:324-342`.
- **The editor transient is net-identity ClientAndHost/Authority** so both AuthorityOnly and
  CosmeticOnly net-gated processors pass in previews (`CkEcsEditor_Subsystem.cpp:79-84`).
- **Editor viewports don't repaint continuously** — the subsystem has `Request_Redraw`; visual
  changes made outside spawn/destroy may need it.
- **PIE start/stop tears down the editor ECS registry**; all mutations must go through the
  `Get_IsEditorEcsMutationSafe` re-arm pattern (see `EditorOnly_DoRebuildEntity`).
- **Editor worlds never fire actor BeginPlay** — any actor a feature spawns for previews needs
  explicit idempotent init (ISM's `DoInitialize` pattern) and `RF_Transient` (or it bakes into
  the level package — ISM shipped a leaked-actor cleanup for exactly that, `CkIsmSubsystem.cpp:92-141`).
- **SKMC pose ticking in editor worlds** is gated on the editor-only `bUpdateAnimationInEditor`
  flag per component; `VisibilityBasedAnimTickOption` alone does nothing there.
- **SKMC pool is per-renderer-actor** (`_Pool_FreeSKMCs` on `ACk_IskmRenderer_Actor_UE`); after
  the Phase-2 per-owner split, Acquire/Release must stay owner-local or pooled components
  migrate across selection owners.

## 7. Recommended flow for your first session

1. Check PROGRESS.md — has anything moved since 2026-07-16?
2. Check whether the sibling `editor_selection_owner` work landed on dev (§1). Rebase if so.
3. Run Phase 0 (the §3 table). This needs the editor — if you can't launch it, ask the user to
   do the drag-drop and report the row + paste the Output Log.
4. Branch on the result per §3. Any code change: capture the toolbox build/test baseline FIRST
   (none was captured — the investigation session ran no builds). Known pre-existing red on
   2026-07-16 tips: `Ck.Crowd...OccupiedGoal RESUME` (782/783, A/B-proven) — don't re-attribute it.
5. Build + test via the Unreal Toolbox only (`/build-test` skill). Never raw Build.bat or
   `UnrealEditor-Cmd -ExecCmds="Automation ..."`.
6. Phase 3's warning heuristic needs maintainer sign-off on shape before implementing.
7. Update PROGRESS.md before ending your session.

## 8. Suggested first message (for the human to paste)

> I'm continuing the ISKM editor-preview campaign. Read
> `Plugins/CkFoundation/Source/CkIskmRenderer/CONTINUATION_PROMPT_IskmEditorPreview.md` fully,
> then `docs/campaigns/iskm-editor-preview/PROMPT.md` and `PROGRESS.md` (same submodule). Start
> with the Phase-0 discriminator: I'll drag-drop an ISKM EntityScript in the editor and tell you
> what I see + paste the log; match it against the §3 table and take it from there.
