# Campaign: ISKM editor-world preview via the Entity Spawner

**Task (verbatim):** "[CkFoundation] Why are certain features not previewing out of PIE in the
editor world when drag-and-dropping them in the level via the entity spawner (e.g. ISKM)?"

**Status:** Investigation complete (2026-07-16, read-only). Implementation NOT started.
**Branch:** `bugfix/iskm-editor-preview`, based on `origin/dev` @ `ac101e415`.
**Companion docs:** [PROGRESS.md](PROGRESS.md) (living state) ·
[digest-2026-07-16.html](digest-2026-07-16.html) (visual summary) ·
`Source/CkIskmRenderer/CONTINUATION_PROMPT_IskmEditorPreview.md` (session handoff).

---

## Root cause (confirmed, two independent layers)

### Layer 1 — the editor preview runs a truncated EntityScript lifecycle

Drag-drop never goes through the runtime spawn path. The actor factory stores an instanced
EntityScript on the spawner; `EditorOnly_RebuildEntity` (end-of-frame, `EWorldType::Editor` only)
calls `UCk_EditorEcsWorld_Subsystem_UE::Request_SpawnEditorEntity`
(`Source/CkEntitySpawner/Public/CkEntitySpawner/CkEntitySpawner_Actor.cpp:305`). That subsystem
exists only for Editor worlds (`Source/CkEcs/Public/CkEcs/Subsystem/CkEcsEditor_Subsystem.cpp:43`),
owns its own EnTT registry, and builds the full processor graph with
`ECk_ProcessorWorldTypeContext::Editor` (`CkEcsEditor_Subsystem.cpp:301-307`).

Processors declaring `WorldTypeRequirement = RuntimeOnly` become **ghost nodes** — RunAfter/
RunBefore edges kept, `ForEachEntity` never fires (`Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorDescriptor.h:37-50`).
The following are RuntimeOnly (`Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript_Processor.h:85,139,166`):

- `FProcessor_EntityScript_Replicate`
- `FProcessor_EntityScript_PendingReplicationRetry`
- `FProcessor_EntityScript_BeginPlay`

**Consequence:** editor previews get `Construct` + `OnConstructed` only. `BeginPlay` and
`Promise_OnReplicationComplete` NEVER fire. Any feature composed — or any initial
`Request_PlayAnimation`-style kickoff issued — from BeginPlay or a replication-gated callback is
silently absent from previews.

### Layer 2 — editor preview is opt-in per feature; ISKM never got its hardening pass

ISM previews because it was deliberately hardened (`fb09f4ed7` "render ISM meshes at editor
time"): Editor world-type support on the subsystem, renderer-actor init moved out of BeginPlay
("Editor worlds never fire BeginPlay", `Source/CkIsmRenderer/Public/CkIsmRenderer/CkIsmSubsystem.cpp:193`),
transient renderer actors, and (in-flight, see Constraints) per-owner selection-proxy actors.

ISKM's module is world-type-clean — subsystem accepts Editor/EditorPreview
(`Source/CkIskmRenderer/Public/CkIskmRenderer/CkIskmSubsystem.cpp:92-104`, from `927acdf00`),
explicit `DoInitialize` (no BeginPlay reliance), zero world gates in Utils/processors, and
`96aeaf1dd` even added a `TIgnoreInEditor<FTag_IskmProxy_Movable>` accommodation
(`CkIskmProxy_Processor.h:115-126`) — editor preview is clearly *intended*. What ISKM lacks:

1. **No `ck::editor_selection_owner` integration** (0 hits module-wide) — no per-owner renderer
   actor, so preview meshes can't redirect selection to the spawner (ISM/component-host get this
   from the in-flight sibling work).
2. **No `SetUpdateAnimationInEditor`** (0 hits module-wide) — the engine one-shot-ticks a SKMC at
   registration in Editor worlds then freezes it behind `bUpdateAnimationInEditor`
   (engine `SkeletalMeshComponent.cpp:1070-1093`, gates at `:1682-1706, :1718-1737`). Even a
   working ISKM preview renders a frozen pose.

### The unresolved discriminator (needs the editor — [EDITOR-VERIFY])

A fully set-up SKMC in an editor world renders a **frozen pose, not nothing**. So:

- Symptom is *nothing at all* → composition never ran: BeginPlay-composed script (Layer 1), a
  loud `CK_ENSURE` in the ISKM Setup chain, or an asset-load early-out (the gym scripts skip with
  an on-screen print when `RendererData` fails to resolve, `CkIskmRenderer_GymStation.as:80-84`).
- Symptom is *frozen/unselectable mesh* → Layer 2 only.

The reference gym scripts compose entirely in `DoConstruct`
(`Plugins/CkTests/Script/CkIskmRenderer/CkIskmRenderer_GymStation.as:74-128`), so if those are the
repro, Layer 2 is the whole story.

---

## Feature preview coverage (verified against code @ ac101e415)

| Feature | Previews? | Why |
|---|---|---|
| EntityScript Construct/OnConstructed | yes | Construction chain has no WorldTypeRequirement |
| EntityScript BeginPlay / OnReplicationComplete | **no** | RuntimeOnly ghosts (`CkEntityScript_Processor.h:85,139,166`) |
| Transform (CkEcsExt) | yes | Runtime processor RuntimeOnly but EditorOnly twin exists (`CkTransform_EditorProcessor.h:25`) |
| ISM rendering | yes | Deliberate pass `fb09f4ed7` + in-flight selection-proxy work |
| UnrealComponent host | yes | Setup gates on `WorldType==Editor` → per-owner RF_Transient host actor |
| Probe shapes (CkSpatialQuery) | partial | 13 runtime processors RuntimeOnly; EditorOnly draw-only processor (`df279b7d9`) |
| CkEqs | no | Both processors RuntimeOnly, no editor twin |
| **ISKM Plan-1 (SKMC)** | **no** | Module world-clean but zero editor affordances (this campaign) |
| ISKM Plan-2 (batched crowds) | unknown | No gates found; GPU path never observed in an Editor world |

---

## Fix plan

### Phase 0 — Discriminator [EDITOR-VERIFY]
Drop an ISKM EntityScript (a CkIskmRenderer gym station is a good vehicle) via the entity spawner
with the Output Log open.
- **Silence** → composition never ran. Read the failing script: if it composes in
  BeginPlay/OnReplicationComplete, move preview-relevant composition to `DoConstruct` (house
  doctrine: "Construct: compose features") and re-test.
- **Ensure fires** → Setup ran and failed; fix that specific break.
- **Frozen mesh appears** → proceed straight to Phase 2.

### Phase 1 — Sequencing gate (do not skip)
The `ck::editor_selection_owner` layer is **in-flight sibling work** (see Constraints). Phase 2
builds on it. Wait until it lands on `dev`, then rebase this branch.

### Phase 2 — ISM-parity editor pass for CkIskmRenderer (the durable fix)
Port the proven ISM recipe:
- Per-`(RendererData, SelectionOwner)` editor renderer actor on
  `UCk_IskmRenderer_Subsystem_UE` (mirror ISM's per-owner split) + `RegisterProxyActor` /
  `IsSelectionChild` wiring so spawner selection reaches ISKM previews.
- In `FProcessor_IskmProxy_Setup`, when `World->WorldType == EWorldType::Editor`, resolve
  `ck::editor_selection_owner::TryGet(InHandle)` once and cache an `FObjectKey` on
  `FFragment_IskmProxy_Current` (mirror `CkIsmProxy_Processor.cpp:161-171` incl. the
  teardown-stability rationale); route `Acquire_BaseSKMC` through the per-owner renderer actor.
- `SetUpdateAnimationInEditor(true)` on acquired base SKMCs and submesh children in editor
  worlds. Ship this even if Phase 0 resolves the visible symptom — without it a "fixed" preview
  is a frozen pose.

**Files:** `CkIskmSubsystem.h/.cpp`, `Proxy/CkIskmProxy_Processor.cpp`, `Proxy/CkIskmProxy_Fragment.h`.
**Risks:** SKMC pool must stay owner-local after the per-owner split (Acquire/Release pairing);
N ticking preview SKMCs cost editor-frame CPU (a 5x5 gym army = 25 ticking meshes — consider a
project setting or cap); Plan-2 batched preview is out of scope until Plan-1 is proven.

### Phase 3 — Make the silent truncation loud (framework guard)
Once-per-class `ck::ecs` warning when an editor-preview EntityScript finishes construction while
its class overrides BeginPlay and composed nothing renderable ("BeginPlay does not run in editor
previews — compose preview-relevant features in Construct"). **The heuristic shape is an
unwritten-norm fork — get maintainer sign-off before implementing (non-negotiable #6).**

### Phase 4 — Document the contract
- `Source/CkIskmRenderer/Claude.md`: editor-preview lifecycle contract + what Phase 2 added.
- `Source/CkEntitySpawner/Claude.md` (NEW — Source/CLAUDE.md flags "no doc yet"): the preview
  architecture (editor ECS world, Construct-only lifecycle, per-feature opt-in recipe).

### Gates (every phase)
- Build + automation tests via the Unreal Toolbox only (`/build-test` skill) — never raw
  Build.bat / UnrealEditor-Cmd.
- Capture the test baseline BEFORE first change and diff after each phase. Known baseline note:
  `Ck.Crowd...OccupiedGoal RESUME` was a pre-existing red on 2026-07-16 tips (A/B-proven,
  782/783) — do not attribute it to this campaign without re-proving.
- Phase 2 exit: [EDITOR-VERIFY] drag-drop / move / Ctrl+Z / Ctrl+Y / delete / PIE-start-stop
  passes with an ISKM script, mesh animates in preview, clicking the preview selects the spawner.

---

## Constraints

1. **Sibling in-flight work (coordination hazard).** A concurrent session owns uncommitted
   editor-selection work in this repo: 10 modified files across `CkEntitySpawner`,
   `CkIsmRenderer`, `CkUnrealComponent` + untracked `Source/CkEcs/Public/CkEcs/EditorSelectionOwner/`.
   It has appeared both as working-tree modifications and as a stash
   ("On dev: selection-redirect A/B stash for crowd-test attribution", `03a32641f` +
   untracked companion `618ab0f13`). It touches **zero** CkIskmRenderer files. Do not modify
   those files; do not start Phase 2 until it lands; rebase this branch when it does.
2. **This branch must not be merged into dev before Phases 0–2 complete** — it currently carries
   documentation only.
3. All cited line numbers refer to `ac101e415`; re-verify after rebasing.

## Provenance

Read-only investigation, 2026-07-16, in the CkPlugins2 host project. Findings produced by a
12-agent workflow (5 parallel readers → synthesis → 5 adversarial verifiers): 4 load-bearing
claims CONFIRMED, 1 UNCLEAR (the Phase-0 discriminator — inherently needs the editor). Engine
citations refer to the UnrealEngine-Angelscript 5.7 fork (resolve your local engine checkout via
`CkAuto/Get-ProjectEnginePath.ps1` from the host project root).
