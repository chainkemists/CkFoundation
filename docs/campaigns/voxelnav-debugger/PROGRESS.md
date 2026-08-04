# PROGRESS - voxelnav-debugger

> Living tracker. Update at every gate and session boundary. Stable mission: [PROMPT.md](PROMPT.md).

## Current state

**As of 2026-08-04:** implementation through the outside-PIE Level Editor overlay is complete.
CkVoxelNav exports immutable bounded snapshots; `CkVoxelNavEditor` builds exact cooked-Jolt previews
from placed volume actors; and the Crowd Debugger now has grouped controls, a true 3D viewport,
camera presets, Live/Retained/Editor sources, and a direct Level Editor overlay toggle. The latest
focused editor build and all seven `Ck.VoxelNav.DebugSnapshot.*` tests are green.

**Authoritative heads at entry:**

- CkPlugins root `baffa65a1e8ac1b7b3b87a29a3a5a2126f7ee0a1`, branch `feature/3d-navigation`.
- CkFoundation `eef267b553243814f4f30481a0e41bdaf08ade9e`, branch `feature/ckvoxelnav-port`.
- CkGameplayDebugger `e05812b129195a05a66b33262fd83e845e4a8827`, detached at entry; root already carried the
  pre-existing `e5c2dc9..e05812b` pointer change.
- CkTests `4a98ddcce2115c626deda82b065d4aa98c50e881`, branch `feature/ckvoxelnav-port`.

**Baseline being diffed against:** [BASELINE_20260804-152229.md](BASELINE_20260804-152229.md).
The editor build succeeded; 982 tests ran, with 974 passed, 8 failed, 0 skipped, and 0
contaminated. The exact failing set is recorded in the linked snapshot.

**Next action:** run the final full-suite delta gate, complete the user-driven rendered editor/PIE
walkthrough, then commit each plugin and the root pointer updates without staging foreign dirt.

**Blocked on:** nothing.

## Known foreign dirt - preserve, never stage by default

- CkFoundation: 76 modified `Content/CkUsf/GeneratedLooks/*.uasset` files, owned by the prior
  generated-look/test lane.
- Root: pre-existing CkGameplayDebugger pointer update `e5c2dc9..e05812b`.
- Root untracked: `.agents/`, `.codex/`, `AGENTS.md`, and `CONTINUATION_RelayClientRegistration/`.

## Decision log

| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-08-04 | Open a new campaign rather than append to the closed port campaign | Cooked/editor preview and debugger UI were explicitly deferred from that campaign and have independent gates. | Never; preserve the old close evidence. |
| 2026-08-04 | Use one immutable snapshot for embedded and Level Editor renderers | Avoids two visibility truths and prevents either renderer from reaching into runtime state. | If a renderer proves it needs data not appropriate for the shared contract. |
| 2026-08-04 | Require exact cooked Jolt input for Current editor previews | Runtime occupancy is Jolt-defined; approximate editor geometry would misdiagnose navigation. | Only if runtime geometry ownership changes. |
| 2026-08-04 | Keep runtime-only scripted volumes supported but label them RuntimeOnly outside PIE | They have no editor-visible params before script execution. | If all consumers migrate to shared authored definitions. |
| 2026-08-04 | Replace, not extend, the custom 2D center canvas for 3D | The existing widget has no 3D projection/depth surface and its RMB gesture conflicts with viewport orbit. | Never; a separate 2D overview may remain as an optional mode. |
| 2026-08-04 | Use a placed `ACk_VoxelNavVolume_UE` as shared authoring | A map actor supplies transform/bounds outside PIE and composes the identical params into ECS at BeginPlay; a DataAsset has no placement and a host component adds indirection without benefit. | If runtime volume ownership stops being ECS-first. |

## Research record

- Files read: CkVoxelNav build/merge/volume APIs and module doc; CkJolt subsystem, cooked world
  data/restore, and editor cooker; CkCrowdDebugger window, data collector, view model, settings,
  custom viewport; CkDebuggerCommon viewport/lifetime docs; CkGridEditor EdMode drawing; existing
  VoxelNav/CrowdDebugger test exemplars.
- Patterns extracted: ECS-free budgeted VoxelNav build; immutable octree publication; per-map Jolt
  cooked index/cells with versioned shape blobs; value-copy debugger collectors; per-user debugger
  settings; Level Editor PDI overlay; `SEditorViewport`/`FEditorViewportClient` for a docked scene.
- Neighboring features to mimic: CkVoxelNav `FBuildState`, CkJolt cooked restore, CkGridEditor
  authored overlay, and CkDebuggerCommon teardown/refresh contracts.
- Verified: current Crowd viewport is XY-only; Jolt gameplay subsystem excludes editor worlds;
  VoxelNav can build outside ECS through a backend; merged cells are the actual search graph;
  current reference volume has 91,752 raw cells and 359 merged cells.
- Inferred pending implementation proof: a strict cooked-shape query can be factored without
  changing runtime VoxelNav results; an authored volume facade can share params with runtime
  composition without weakening ECS ownership.

## Dated entries

### 2026-08-04 - campaign opened

- Ran: read-only repository and skill archaeology; three Terra sidecars covered editor preview
  architecture, 3D viewport patterns, and test planning.
- Confirmed: exact outside-PIE preview requires a separate editor data path; retained PIE alone is
  insufficient. The existing Crowd Debugger is the correct product home but not a reusable 3D
  renderer.
- Inferred: CkVoxelNavEditor plus a JPH-free cooked query value in the Jolt ownership boundary is
  the smallest architecture that preserves exactness. Phase 1 must prove this before UI integration.
- Follow-ups recorded, not chased: runtime octree serialization remains out of scope; World
  Partition strictness and landscape parity are explicit preview validity cases.

### 2026-08-04 - pre-change baseline recorded

- Ran: detached `UnrealToolbox --build --target=Editor --test` against the absolute project path.
- Confirmed: editor build succeeded; 982 total, 974 passed, 8 failed, 0 skipped, 0 contaminated.
- Boundary: the eight named failures in [BASELINE_20260804-152229.md](BASELINE_20260804-152229.md)
  predate campaign source edits. Final acceptance is no new failures plus all focused campaign tests
  green.

### 2026-08-04 - Phase 0 snapshot contract complete

- Added plain snapshot values for merged free, raw free, occupied, chunks, portals, dirty/repair
  bounds, build/repair progress, source identity, epoch, fingerprint, and explicit source status.
- Added deterministic layer/depth/clip/cap filtering and a cache-key lookup that occurs before
  octree enumeration; publication replaces the whole value snapshot and increments generation.
- Added the runtime volume adapter without exposing an ECS handle, UObject/UWorld pointer, JPH
  type, or live octree to consumers. Invalid handles reject quietly; incomplete valid volumes
  fail explicitly without publishing partial state.
- Ran: detached Toolbox `--build --target=Editor --test --test-pattern DebugSnapshot`.
- Confirmed: editor build succeeded; 4 total, 4 passed, 0 failed, 0 skipped, 0 contaminated.
  Evidence: `Saved/Logs/BuildTest-VoxelNav-DebugSnapshot-Phase0-Rerun.log`.
- Comment audit: corrected full-occupied-leaf expansion to all 64 layer-0 subcells, made leaf-store
  mismatch reject atomically, and removed the one compile-time type/local shadowing defect found by
  the first gate.

### 2026-08-04 - outside-PIE editor preview and debugger integration complete

- Added a placed `ACk_VoxelNavVolume_UE` whose authored box/build values are the same params composed
  into the runtime ECS volume at BeginPlay.
- Added a JPH-free strict cooked-world query contract in CkJolt and a paced editor subsystem that
  validates cooked Jolt data before publishing exact VoxelNav cells. Missing/stale/failed sources do
  not fabricate Current geometry.
- Replaced the Crowd Debugger's noisy flat toggle strip with Navigation, Crowd, and Diagnostics
  groups. Added a real 3D viewport with perspective orbit and top/bottom/left/right/front/back/frame
  controls, plus merged/raw/occupied/chunk/portal/repair layers.
- Added Auto, Live PIE, Retained, and Editor Preview sources. Editor snapshots publish as immutable
  shared generations, so the docked viewport does only a pointer comparison on its 4 Hz idle refresh;
  the Level Editor overlay never copies cell arrays per render.
- Added the outside-PIE `Level Editor Overlay` checkbox. Current, Building, Stale, Missing, and Failed
  are visibly distinct; Missing/Failed render no retained cells; the overlay suppresses itself in PIE.
- Ran `Saved/Logs/BuildTest-VoxelNav-Debugger-Integrated-8.log`: editor build succeeded; 7 total,
  7 passed, 0 failed, 0 skipped, 0 contaminated, 57 seconds. The new lifecycle test directly toggles
  the Level Editor overlay and restores prior mode state.
- Independent review found and then cleared per-frame snapshot copying, ambiguous stale rendering,
  missing async redraw, PIE bleed-through, and repeated 4 Hz editor recombination.
- After fetching and rebasing every affected plugin onto current `origin/dev`, tightened the editor
  query to require current runtime hashes for every selected cooked actor and to use the identical
  persistent-level map key as runtime. Post-rebase evidence:
  `Saved/Logs/BuildTest-VoxelNav-Debugger-PostRebase.log`, build succeeded, 7/7 passed,
  0 contaminated.

### 2026-08-04 - full-suite delta gate

- Ran a fresh four-lane full suite in `Saved/Logs/Test-VoxelNav-Debugger-Full.log`: 982 total,
  974 passed, 8 failed, 0 skipped, 0 contaminated, 3 minutes 13 seconds.
- Six PathNetworkFollower failures matched the baseline. Baseline failures
  `Ck_AutoTest_StateMachine_DivergenceFirstBranch` and `SurfaceOffset` improved to green.
- Two failures outside the baseline appeared only in that parallel run:
  `AngelscriptCodeCoverage.IntegrationTest` and `GitLink.Subprocess.RunToFile.RoundTripBinary`.
  Neither module was changed. Each passed 1/1 with zero contamination in a separate fresh-process
  rerun (`Test-VoxelNav-Debugger-NewFailure-Integration.log` and
  `Test-VoxelNav-Debugger-NewFailure-RoundTripBinary.log`). They are recorded as parallel-only flakes,
  not attributed to this change and not hidden from delivery evidence.

## Open items

| Item | Status | Next step |
|---|---|---|
| Phase 0 snapshot and cache | Complete | Preserve the value-only boundary through later phases. |
| Phase 1 exact cooked editor query + authored volume | Implemented | Rendered Current-cook walkthrough remains an editor verification item. |
| Phase 2 editor preview subsystem | Implemented | Focused automated gate green. |
| Phase 3 grouped Crowd Debugger + 3D viewport | Implemented | Rendered camera-preset walkthrough remains manual. |
| Phase 4 live/retained/editor sources + Level Editor overlay | Implemented | Automated toggle/lifetime gate green; PIE/retained visual walkthrough remains manual. |
| Phase 5 stress/manual/final gate and delivery | In progress | Run full delta gate, commit plugin changes and root pointers, hand off exact walkthrough. |

**Rule:** no completion claim while any open item is unresolved.
