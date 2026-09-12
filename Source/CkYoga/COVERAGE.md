# Current closeout status after Texture shell migration (2026-09-12)

| Required row | Current evidence | Status |
| --- | --- | --- |
| Authored stable outer shells | GOAP and Texture own stable tabs/splitters/pane composition in authored resources; shared window chrome is authored for its consumers | 2/27 complete; 10 partial; 15 without a full authored shell |
| Inspector migration | GOAP specialized inspector panes are authored/hybrid ports; GOAP Inspector Gateway and other debugger inspector surfaces remain to migrate | Open |
| Local controller and two users | Synthetic native controller and virtual-user evidence exists, but no real local-player gamepad navigation/confirm/back gate | Open |
| Representative performance | Structural virtualization evidence exists; no approved budget/reference machine or representative measured-host result | Open, approval needed before pass/fail |
| Every-debugger teardown | GOAP and Texture shell teardown are focused green examples; the production-host matrix remains incomplete | Open |

Texture evidence: `scratch/yoga-texture-shell-build-final-20260912.log` succeeded; fresh real-RHI `scratch/yoga-texture-shell-test-final-20260912.log` passed `Ck.TextureDebugger.AuthoredWindow` 1/1 with compatible/rejected reload, six authored page tabs, Refresh retention, narrow reachability and authored-view teardown. The inspected narrow capture establishes scroll reachability, not deferred browser breakpoint restacking. Packaged proof, keyboard-only accessibility and localization remain deferred, and Resource Inspector `+` remains excluded.

# CTO-dispositioned closeout matrix (2026-09-11)

| Census row | CTO disposition | Campaign consequence |
| --- | --- | --- |
| Narrow browser breakpoint restacking | Deferred; not needed in the medium term | Accept readable/reachable narrow layouts through wrap and scroll. Do not claim browser breakpoint parity. |
| Resource Inspector navigation `+` | Excluded | Remove it from closeout acceptance until a future product contract defines the add-set. |
| Native outer shells | Required migration | Stable debugger chrome, tabs, splitters, panes and inspector placement must be authored HTML/CSS; specialized native controls may mount only through explicit authored ports. |
| GOAP and other inspector surfaces | Required migration | Author inspector layout/presentation for GOAP Inspector Gateway and every remaining debugger inspector while retaining production model/action ownership. |
| GOAP Agent List P9 retirement | Accept recommended safe retirement | Move and test its surviving selection-sync role before deleting the un-slotted panel. |
| Packaged host | Deferred | Preserve staging declarations; prove cook/stage/runtime later in a real packaged consumer, not CkPlugins. |
| Controller/multi-user | Required bounded gate | Prove real local-player gamepad navigation/confirm/back and two-local-Slate-user isolation. Network multiplayer is conditional on actual client exposure. |
| Accessibility | Keyboard traversal only, deferred | No accessibility gate blocks this campaign; do not claim broader semantics or certification. |
| Localization/RTL | Deferred | Preserve localized text identity where already present, but no culture matrix blocks closeout. |
| Performance | Required representative gate | Measure representative large-data debugger hosts; obtain approval for thresholds/hardware before judging pass/fail. |
| Lifetime | Required for every debugger | Every debugger must prove release of views, ports, callbacks, subscriptions, focus/capture and popups across its applicable teardown paths. |

This matrix supersedes the unresolved wording in the publication census below. The campaign remains open because outer-shell/inspector migration, the bounded controller gate, representative performance evidence and complete debugger teardown evidence are unfinished.

# Published closeout census and acceptance boundary (2026-09-11)

Published runtime-checkpoint tips: CkFoundation `8f3daa93e2969553579efa8efd45e048dcbeba08`, CkGameplayDebugger `bb5fd18f5a1b16ce633bc17335a71d78ed190a01`, and CkTests `92587fa30d21fdc419fcd836379d87c215f4ea86`; CkPlugins root `8e60f187428c550eaa29d791039f014dbd387837` is open and unmerged in PR #39. Fresh serial real-RHI `Ck.UiAuthoring` evidence passes 151/151 in `Saved/Logs/Test-UiAuthoring-Closeout-R222-RealRHI.log`; the earlier NullRHI attempt is not acceptance evidence.

Current source does not support a completed P9 retirement claim: GOAP Agent List is un-slotted but still constructed/refreshed for selection synchronization. GOAP outer chrome/splitters and its distinct registered native ECS Inspector gateway remain; Texture retains native outer shell and Checker Apply/Restore; AI remains a roster-only authored slice. Apply the CTO matrix above: the native shells/inspectors and all-debugger teardown are required work, controller and performance retain bounded gates, while packaging, keyboard accessibility, localization and narrow restacking are deferred and Resource Inspector `+` is excluded.

# GOAP authored specialized cohort checkpoint (2026-09-11)

R218 built successfully and final serialized real-RHI R219 passed the selected authored GOAP cohort 10/10 with zero failed/skipped/contaminated tests. Production-path coverage now includes Agent Column, Agent List, Catalog, Decision, Graph, Search Trace, Squad, Timeline, World State, and the Squad-table empty fixture. Graph reset coverage proves subscription detachment across the ViewModel reset broadcast, deferred authored-view/native-port release, explicit resume, remount, and final teardown. The older GOAP module-matrix row is superseded for those surfaces; bounded outer-shell/Inspector integration and the P9 retirement sweep remain subject to a final source census.

# Dialog and Texture authored cohort checkpoint (2026-09-11)

The R72 authored cohort passes 12/12 with clean contamination status. Dialog timed identity across real time advancement is directly covered. Texture real-window six-tab physical routing, active-page state, and Refresh selection retention are accepted with inspected wide/narrow captures; this is not Texture PIE or an outer-shell migration, and checker Apply/Restore plus broader parity gaps remain open.

# StyleLab full-window checkpoint (2026-09-11)

StyleLab is now accepted for the real full-window authored path: shared chrome/scroll/sample, semantic and physical `All Tones` checkbox interaction, state mutation, `NotifyChanged`, and retained sample-root rebuild on production tick. R68/R69 build and 1/1 real-RHI evidence plus inspected wide/narrow captures are recorded in `PROGRESS.md`; browser/package/controller/accessibility/localization/performance parity remains open.

# CkGameplayDebugger declarative-layout coverage

## Current authored migration checkpoint (2026-09-11)

Input and Save authored migrations are accepted within their bounded production paths. Input has R48 build, R49 real-RHI Input-only claim (the combined run also contained a failed Save test), runtime archive, and inspected wide/narrow Controls captures. Save has R65 build, R66 cached real-RHI 1/1, runtime archive, and inspected wide/narrow AuthoredPie captures. Save behavior covers real save mount, physical authored filter/highlight input, problems-only child+ancestor projection, four provenance toggles preserving ancestry, and stable selection/expansion/reload/lifetime. StyleLab is accepted in the newer checkpoint above; packaged/browser/controller/accessibility/localization/performance parity and narrow browser restacking remain gaps.

Status: **inventory retained; shared authoring foundations now cover controls, typed collections and read-only virtualized tables.**  This records the non-graph Slate
surfaces in `Plugins/CkGameplayDebugger/Source` for the HTML/CSS layout campaign.  Graph editor
widgets remain outside this campaign.  A current `<native>` port is a transitional mounting
mechanism, not the intended endpoint for tables, trees, inspectors, search controls, viewports, or
custom-painted controls.

## Resource Inspector loading/error presentation

ResourceStates-Editor.log passes9/9 consumer tests with clean error/ensure/fatal/
AngelScript-warning scan. PresentationStates.NativeScenarios uses installed resources
and routed pointer actions: ready/loading/error visibility and recovery, ready-empty
suppression, retained selection/data/query/note/pin/activity, invalid-key rejection,
and accepted reload. Native Loading.png/Error.png at960x640 were inspected. These
are deterministic presentation previews; they do not prove asynchronous loading,
request cancellation or network error handling. Shared dialog support and the full
capability gallery remain incomplete.
## Resource Inspector Activity

ResourceActivity-R3-Editor.log passes8/8 consumer tests with zero error/ensure/fatal/
AngelScript-warning matches. Activity.History uses installed markup and native mouse
routing; covers model event contents/no-ops/rejections, queued reentrant events,
100-record retention, ordering/identity, and retained tab/repeat on reload. Native
Activity.png was inspected at960x640. This adds a realistic bounded history example;
it does not complete the component gallery or reference visual parity. Loading/error
states, dialog composition and the remaining campaign gates stay open.

## Resource Inspector retained dialog implementation

The shared retained dialog is now implemented with explicit Slate-user ownership,
authored `content` and `body` slots, an open bool binding and dismiss action.
Resource Inspector uses qualified `inspector-dialog/content` getters and the dialog
to confirm clearing pinned snapshots. The native consumer covers routed backdrop
blocking, cancel/confirm behavior, retained selection and note state, rejected
reload preservation, accepted reload identity and a 960x640 capture.

Final focused evidence: BuildTest-UiDialog-R8.log passes 114/114 authoring tests; UiDialog-R5-ResourceCompatibility.log passes 10/10 consumer tests. Archived actual logs have matching success counts and no error/ensure/fatal/AngelScript diagnostics. Shared tests cover owner-only focus, Tab/DPad/Back, reload and hidden/removal/owner-release behavior, plus routed pointer capture release preserving unrelated captures. Full local-player/controller sessions, adversarial callback teardown, game/package, accessibility, performance and the rest of the campaign remain open. The historical context-menu reopen failure remains unexplained despite passing current samples.

## Native region geometry

StyleLabGeometry-R3-Editor.log passes111/111 authoring tests, with zero error,
ensure, fatal or AngelScript-warning matches. ProfileGeometry uses the real native
pane at wide/narrow/150% scale, asserting button containment, wrapping and height
growth. Narrow captures were inspected. Shared SCkUiRegion bridges allotted width
to constrained desired height for native Slate parents. This establishes this
consumer geometry case, not coverage of every debugger or layout transition.
Workbench.reference.html exists; browser preview is policy-blocked and unverified.
Native translation and the full component gallery remain incomplete.

## Dialog debugger authored migration

The Dialog debugger now mounts four authored regions for cooldown controls,
runtime commands, search and main content while retaining its native window
chrome, real collector and command routing. `DialogDebugger-R5-Authored-PIE-
Editor.log` passes1/1 against real registry lines, timed and Forever cooldowns,
filter/highlight, active-only, Save/Load, retained reload and stale owner release.
Fresh960x640 and520x620 captures show readable, non-overlapping native controls
and coherent authored rows/panes after the shared intrinsic-region repair.

This accepts the scoped conventional Dialog migration. Direct retained identity
proof is for the Forever record/repeat item; timed progress is real but timed-row
identity is not separately asserted. Live style-axis, browser, game/package,
controller, accessibility, localization, performance and broad lifetime gates
remain open.

## AI debugger authored roster migration

The AI Overview NPC-health roster now mounts one retained authored selectable table while existing AI window chrome and other panes remain native. R14's incremental build succeeded in 12.02s; `Ck.AiDebugger.AuthoredRoster.PIE` passed 1/1 in 33s, exit 0, zero failed/skipped/contaminated, with two editor processes (inline discovery and focused lane). The production-path two-agent fixture verifies physical Crowd-handle keys, live record/row identity, atomic removal, selection routing, compatible/rejected resource reload and teardown rejection. Runtime and discovery archives are clean of automation errors, ensures, fatal/critical/unhandled diagnostics, script errors/warnings, `FullReload=true`, and timeouts.

Fresh wide and narrow captures at 23:07:44 were inspected: the roster has two readable names/statuses and selection at wide width, while narrow proves horizontal reach/later context without roster overlap. This accepts only the roster migration. Legacy surrounding AI shell content clips/overlaps at constrained widths, so no whole-window responsive claim is made. Stage/evidence/event/topology panes, picker, spatial viewport, browser, game/package, controller, localization, accessibility, performance, and broad lifetime acceptance remain open.
## Current custom slot runtime boundary

StyleLabProfiles-R2-Editor.log passes110/110 shared tests; StyleLabProfiles-
Compatibility-Editor.log passes6/6 Style Lab tests. Curated profiles are now an
installed authored consumer of debug-inspector and horizontal wrapping repeats.
Buttons support per-record label color and tooltip while preserving profile name
actions, settings selection and notifications. Actual visual/scale acceptance and
focused new field/wrap rejection coverage remain next; this does not complete the
remaining Style Lab groups or other debuggers.

### Previous shared inspector evidence

DebugInspector-Editor.log passes109/109 shared tests. FCkDebug_UiRegistry now provides
retained debug-inspector with localized title-bind and required body slot, backed
by SCkDebug_InspectorPanel. Debugger.InspectorContainer verifies live/rebound FText,
collapse and search retention, constrained wide/narrow body measurement and atomic
missing-body rejection. Style Lab migration and actual consumer visual/scale gates
are next; these are not yet proven by the shared test.

### Previous composition evidence

CustomSlotsComposition-Editor.log passes108/108 shared tests. Added valid nested
custom-container search retention, stale/current action scopes, parent removal and
active owner expiry. RetainedCapture.CustomSlots passes native pointer ancestry
transfer, optional-slot and parent removal, exact cancellation and parent reentry
gating. Direct private-child reentry, repeat-in-slot, external alias and production
inspector migration remain open. Retained custom table/tree cells remain rejected
by the current read-only cell contract and require a later capability increment.

### Previous initial runtime evidence

CustomSlotsRuntime-R3-Editor.log passes107/107 shared tests. The view now renders
authored slot child views through the nested transaction and persistent mounts.
NativeRuntimeAndAtomicity covers retained search draft/focus, child actions,
optional-slot retirement/reinsertion, owner expiry, rejected reloads and malformed
nested mount rejection before prepare/publication. Deeper composition, capture,
reentry, external alias and production inspector migration remain open. The new
Plan/TestWorkbench.md defines the browser/native/gallery coverage deliverables;
it does not claim those artifacts or full capability coverage are complete.

## Previous slot prerequisite boundary

CustomSlotSchema-Editor.log passes106/106 shared tests. Retained custom slot schemas
and typed/template parser transport exist. Rendering is pending: FCkUiView still
rejects custom children and does not populate Arguments.Slots. This is not a usable
custom-container capability yet. Gate_02l_CustomContainerSlots.md owns the remaining
transaction/ownership/retirement/inspector migration requirements. See PROGRESS.md
for current gates; chronological sections below retain prior evidence.
## Earlier Surface capability boundary

Current Surface & Lighting checkpoint: shared authoring 102/102
(SurfaceLighting-Shared-R3-Editor.log), Texture Debugger 30/30
(TextureSurfaceLighting-R3-Editor.log), and Resource Inspector 6/6
(ResourceInspector-SurfaceLighting-Editor.log). Runtime error scans are clean;
Texture has two clipboard warnings with passing clipboard assertions.

Surface & Lighting now uses authored repeated cards; its native card builders are
removed. Production collector tests cover independent expansion, live shadow facts,
retained identity, localized text, rejected reload and empty context. Four populated
captures were produced; narrow 1.5x and wide 1x were inspected after nowrap correction.
Shared repeat tests additionally cover collection replacement, parent-removal focus,
real-window wrapping/non-overlap/collapse and 1/1.5/2 constraint measurement.
Resource Inspector/gallery repeat examples, nested capture/reentry and nested
collection content remain open, alongside the full campaign acceptance below.
Prior checkpoint: 92/92 shared authoring tests (UiLocalizedLabels-Editor.log),
28/28 Texture Debugger tests (TextureLocalizedLabels-Editor.log), and 6/6 Resource
Inspector tests (ResourceInspector-LocalizedLabels-Editor.log).

Ordinary buttons support localized/live text and enabled bindings. Tables support
selectable=false with silent selection clearing on reload. Table-column label-bind
and search placeholder-bind preserve live FText identity, reject missing/unset or
conflicting forms before custom factories, support template forwarding, and stop
reading bindings after view release. Texture Health, Scene Audit and Material Inputs
use the original native localized header/hint keys; Material Inputs verifies native
FText identity for all five headers and both hints.

UV and Material Inputs authored behavior and populated/narrow captures pass their
focused gates. Prior capture inspection and current automated captures are recorded
below; this is not full visual or localization acceptance across all debuggers.

Stable-key repeated authored subtrees now power Surface & Lighting's cards.
Full CSS coverage, cross-file
composition, remaining debugger migrations, game/package/controller and broader
localization/performance acceptance remain open.

## Earlier capability inventory

The chronological checkpoints below supersede this earlier inventory.

`CkSlateLayout` currently parses `row`, `column`, `overlay`, `text`, `button`, `search`, `image`, `scroll`, `splitter`,
`table`, `table-column`, `tree`, and `native`, plus registered stateless or retained custom leaf tags (`Public/CkSlateLayout/CkUiDocument.h`,
`Public/CkSlateLayout/CkUiWidgetRegistry.h`). Document-local template/param/use declarations expand typed authored bodies with lexical instance IDs and atomic validation. Focused evidence: Ck.UiAuthoring.Templates.ParseExpansionAndRejection and ViewAtomicity (UiTemplates-Editor.log). Cross-file libraries and slots remain pending. Its typed live data is text/text-changed,
images, visibility/bools and float numbers (`Public/CkSlateLayout/SCkUiSurface.h`).
Custom factories receive validated typed literals/bindings/actions from an immutable
view snapshot. Retained factories prepare updates atomically and preserve control
identity by id/tag and optional state key; omission releases the component. Focus
handling now snapshots actual and virtual Slate users; see the latest gate evidence in PROGRESS.md. Native/stateless/retained ports forward root
measurement metadata; direct/custom constrained text geometry has focused coverage.
Other custom measurement implementations, styling and child composition require adapter coverage.
CSS is limited to flex sizing, min/max/exact size,
gap, padding, font size/weight, foreground/background colour, and alignment
(`Private/CkUiDocument.cpp`). It has a declarative typed collection/table adapter: stable keyed
records, authored columns, read-only text/image/scroll/stateless-custom cells, field/global bindings,
filtering, sorting, selection callbacks, and bounded native row virtualization. Text cells support
dynamic color and tooltip bindings. It now has typed atomic tree data and a single-column native tree with authored read-only rows (UiTree-Editor.log,47 authoring tests). Registered retained text-input and typed text-change/commit events now have52-test authoring evidence (UiTextInput-Editor.log), including native typing, validation, reentrant reload, draft/caret retention, live enabled/read-only/error bindings and owner release. Undo-history/text-selection retention through reload remains open. It does not yet have authored context menus,
combo/check/numeric input, tabs, grids, canvas anchoring, or viewport/painted controls.
Constrained text wrapping and generic vertical authored subtrees are implemented. The43-test authoring gate covers nested wrapped columns, live shrink/growth, retained focus/offsets, bounded1k/10k nested tables and explicit overflow-wrap policies (UiOverflowWrap-Editor.log); broader composition/visual evidence remains tracked in PROGRESS.md.

The reload transaction and stable ID handling in `SCkUiSurface.cpp` are usable foundations: documents
are staged before commit, and native/search/table/scroll IDs retain mounted widget identity where
their compatibility contract matches. New adapters must retain the same identity and focus/selection
contract, rather than recreate their Slate widget on each refresh. Horizontal scroll and overlay have focused production evidence; later adapter gates and remaining acceptance limits are recorded in PROGRESS.md.

## Native test application

CkTests Resource Inspector loads installed resources through the production view. Its first model/view test covers search, numeric sorting, keyed selection,0/1/12/1k/10k datasets, bounded virtualization, valid/rejected file reload and model release. Test-ResourceInspector-Layout.log passed1/1; wide/narrow Slate captures were inspected, including the narrow title/action correction. BuildTest-ResourceInspector-Host-R2.log passed2/2 including the real one-client PIE host: catchall install/removal, inspector search focus/path, viewport/cursor/focus restoration, repeated close, reopen and post-EndPIE detachment. Category navigation additionally passes Test-ResourceInspector-Navigation.log2/2: native tree selection, category/query intersection, stable selected category and expansion across reload, rejected category/scenario reentrancy, and canonical10k scenario restoration. Shared flex-wrap now keeps all scenario actions visible in narrow captures. BuildTest-UiSingleSelection.log50/50 and Test-ResourceInspector-SingleSelection.log2/2 verify row/column/reverse/default wrapping and exact native single-selection for table/tree, including refresh/reload and collapsed-child selection; latest narrow/wide captures inspected. The shared text-input session-note form additionally passes Test-ResourceInspector-Form.log3/3, with native keyboard commit/rejection/reload and inspected narrow/wide captures. This does not prove split-screen/controller/travel/package behavior, the full browser reference or missing control families.

## CkUIDebugger focused production PIE migrations

The real `SCkUIDebuggerWindow` mounts its Event History inspector, empty state, scroll, and repeat from installed authored resources. AngelScript supplies the gameplay-tag asset, layout-config data-asset literal, and concrete CommonUI widget class. `Build-UiDebuggerHistory-Authored-R5.log` succeeds and the original one-start cached `BuildTest-UiDebuggerHistory-Authored-R6-Final.log` passes `Ck.UIDebugger.History.PIE` 1/1. The fixture proves production layout create/push/pop/clear, compatible-refresh stable record/item identity, exactly one clear event, real Clear History pointer input and empty reconciliation, consecutive newest-100 retention, populated wide/narrow captures, and teardown before `EndPIE`. The lifetime closeout build `Build-UiDebuggerHistory-Lifetime-R2.log` succeeds; its one-start combined `BuildTest-UiDebuggerHistory-Lifetime-R3-Final.log` passes both History rows 2/2 in 31 seconds with zero failed/skipped/contaminated tests.

History, Layers, Summary, and the UI-specific command shell are installed authored resources. Layers add retained hierarchy/projection, Active/Inactive pills, and the 16-widget cap; Summary adds the visibility-bound no-layout message and outlined ACTIVE/INPUT/LAYERS cards; the `commands` region supplies layer filter/clear, active-only, Force Refresh, Expand/Collapse All, Clear History, and both name-depth directions. R27 build `Build-UiDebuggerCommands-R27.log` succeeded, SHA256 `2F74C65F8EA1C75144BFD88BE0C47D44F0CCB97B92A757C26BDD92F01FED4628`; cached real-RHI R28 `Test-UiDebuggerCommands-R28-Final.log` passed both CkUIDebugger rows 2/2 in 33 seconds, zero failed/skipped/contaminated, SHA256 `B970725E5076B0D52008A9387F820BA222C360ABB48DE03DBA85BD9A63ADB2DD`. Its runtime archive `UiDebuggerCommands-R28-Editor.log` SHA256 `E2E9F4AD9ACB2693F815FD03D8E6ADD1E48C84EAD89410D17F011514FC937C40` has zero latent timeouts or relevant diagnostics except an unrelated Chromium USB line. Physical Clear History observed enabled `SCkUiStyledButton` capture/tag/path at revision 2 with zero input suspensions; post-transition History/repeat/tree settled to `[Push]`, one primary child, and three visible nodes. Fresh inspected captures are wide 1206x766 SHA256 `CA8DEACDE0F28B357214D032864FA7A5BED6C7A831EB2DCDB17761497B5084A0` and narrow 426x426 SHA256 `AB281184091EBF04EFCDF156A382578EFC16E521EC2D2329E44B41FA2D1771F7`. Native WindowChrome remains only for its shared host controls; browser breakpoint parity, game/package/controller/accessibility/localization/performance, whole Gate 05, and campaign acceptance remain open.

## Module matrix

| Module | Non-graph UI evidence | Declarative target and pending adapter/template work |
| --- | --- | --- |
| CkAggroDebugger | `Public/CkAggroDebugger/Window/SCkAggroDebuggerWindow.cpp`; authored `Resources/UI/AggroDebugger.ui.html/.css`; real authority PIE fixture | Scoped conventional migration accepted by R6: retained engaged-only/search controls, whole-owner filtering, flat stable-key owner/target repeat, native meter semantics, compatible/rejected reload, teardown rejection and inspected wide/narrow captures. Broader environment acceptance remains campaign work. |
| CkAiDebugger | `Public/CkAiDebugger/Window/SCkAiDebuggerWindow.cpp`; authored `Resources/UI/AiDebuggerRoster.ui.html/.css`; real two-agent PIE fixture | NPC-health roster slice accepted by R14 with physical Crowd keys, retained rows, native selection routing, reload and teardown coverage. Remaining AI shell and panes still require authored migration; constrained-width legacy shell responsiveness is open. |
| CkAStarDebugger | `Window/SCkAStarDebuggerWindow.cpp`, `SearchHistory.cpp`, `StatsPanel.cpp`; painted `GridView/SCkAStarDebugger_GridView.cpp` | Author window/history/stats; add a registered grid-canvas control for the interactive painted grid. |
| CkAudioDebugger | `Window/SCkAudioDebuggerWindow.cpp`; painted `FalloffCurve.cpp`, `Radar.cpp` | Author shell and ordinary controls; register retained curve and radar components. |
| CkCrowdDebugger | `Window/SCkCrowdDebuggerWindow.cpp`, `AgentListPanel.cpp`, `AgentDetailPanel.cpp`, `EventLogPanel.cpp`, `StatsPanel.cpp`, `NavmeshStatusPanel.cpp`, `Viewport/SCkCrowdDebugger_3dViewport.cpp` | Add list/detail/event-log templates and a viewport adapter; do not leave any as permanent ports. |
| CkDebuggerCommon | `Widgets/`, `Search/`, `Window/`, `Devices/`, `Behavior/` | Shared template/component library. Promote its common cards, panes, labels, pills, search, toggles and inspector rows first. |
| CkDebuggerLauncher | `Private/Window/SCkDebuggerLauncher.cpp`, `SCkDebuggerSuiteWindow.cpp` | Author launcher/suite composition once command/menu adapter is available. |
| CkDialogDebugger | `Public/CkDialogDebugger/Window/SCkDialogDebuggerWindow.cpp` | Standard window and controls; base template candidate. |
| CkEcsDebugger | `Window/CkDebuggerWindow_Main.cpp`, `Panels/CkDebuggerPanel_EntityList.cpp`, `Panels/CkDebuggerPanel_Inspector.cpp`, `Inspectors/CkInspectorWidgetBuilder.cpp` | Add splitter, entity-list/tree, inspector-row and editable/enum/control adapters; high-value complex representative. |
| CkEntityDebugOverlay | `Private/Slate/SCkDebugOverlay_Root.cpp`, `FocusCard.cpp`, `WorldTag.cpp` | Add overlay root plus anchored/canvas and wrap/card templates. It is a runtime overlay, not a graph exception. |
| CkEqsDebugger | `Window/SCkEqsDebuggerWindow.cpp`, `QueryList.cpp`, `CandidatePanel.cpp`, `TestBreakdownPanel.cpp` | Add retained selectable-list/table adapters with context-menu and row-action support. |
| CkGameplayDebugger | No Slate layout source found. | No campaign surface. |
| CkGoapDebugger | `Window/SCkGoapDebuggerWindow.cpp`, `AgentListPanel.cpp`, `SquadTable.cpp`, `SearchTracePanel.cpp`, `TimelineDock.cpp`, `InspectorGateway.cpp`, `WorldStateRail.cpp` | Recommended next complex pilot: author the nested splitter shell and controls; implement declarative list/table/timeline/inspector adapters. Graph pane stays graph-specific. |
| CkGridEditor | `Private/EdMode/Ck2dGridSystem_EdModeToolkit.cpp` | Editor toolkit surface; include its control layout in the inventory and add an editor-only host adapter over shared authored controls. |
| CkInputDebugger | `Public/CkInputDebugger/Window/SCkInputDebuggerWindow.cpp` | Standard window composition; base template candidate. |
| CkInputHudOverlay | `Widgets/SCkInputHud_Root.cpp`; painted `SCkInputHud_Ribbon.cpp` | Author overlay composition and register the retained ribbon component. |
| CkInsightsDebugger | `Window/SCkInsightsAnalyzerTab.cpp`; painted `Widgets/SCkFrameBarChart.cpp`, `SCkFramePresenceStrip.cpp` | Author analyzer shell; add chart and presence-strip component adapters. |
| CkIntentDebugger | `Window/SCkIntentDebuggerWindow.cpp`, `DevicesPanel.cpp`, `KeyStatePanel.cpp`, `LayerStackPanel.cpp`, `NearMissPanel.cpp`, `ResolutionPanel.cpp`, `TimelineDock.cpp`; painted `OctantDial.cpp` | Add device/list/timeline templates and a dial component adapter. |
| CkJoltBakeInspector | `Window/SCkJoltBakeInspectorWindow.cpp`, `Viewport/SCkJoltBakeInspectorPreview.cpp` | Author shell and add retained preview-viewport adapter. |
| CkJoltDebugger | `Window/SCkJoltDebuggerWindow.cpp`, `OutlinerPanel.cpp`, `DetailPanel.cpp`, `Viewport/SCkJoltDebugger_3dViewport.cpp` | Add selectable outliner/detail-list and viewport adapters; preserve stable item identity across refresh. |
| CkMapDebugger | `Window/SCkMapDebuggerWindow.cpp` (custom painting) | Register map canvas/control; author surrounding controls. |
| CkNavmeshDebugDraw | No Slate layout source found. | Draw-debug module, no campaign surface. |
| CkObjectPoolingDebugger | `Window/SCkObjectPoolingDebuggerWindow.cpp` | Standard window composition; base template candidate. |
| CkOptimizationDebugger | `Window/SCkOptimizationDebuggerWindow.cpp`, `SCkPerfLabPage.cpp`; painted `SCkOptimizationSnapshotViewer.cpp` | Author shell/page; register snapshot viewer control. |
| CkOptimizationDebuggerEditor | No Slate layout source found. | Editor mode support; out of runtime authoring scope. |
| CkPerfLab | No Slate layout source found. | Data/runner module, no campaign surface. |
| CkSaveDebugger | `Window/SCkSaveDebuggerWindow.cpp` | Author window composition; preserve visualizer integration through an adapter if surfaced. |
| CkSaveDebuggerEditor | No Slate layout source found. | Editor visualizer support; out of runtime authoring scope. |
| CkSchedulerDebugger | `Window/SCkSchedulerDebuggerWindow.cpp`, `Widgets/SCkSchedulerDebugger_ProcessorTree.cpp`, `Inspector.cpp`, `Pages/CkSchedulerDebuggerPage_TreeView.cpp` | Add tree, inspector, command/search, frame-navigation and splitter templates. |
| CkSmDebugger | `Window/SCkSmDebuggerWindow.cpp`, `SCkSmDebuggerPackagedWindow.cpp`, `HistoryList.cpp`, `Preview/SCkSmDebugger_PreviewPane.cpp` | Author non-graph shell/history/preview composition; state-machine graph remains bespoke. |
| CkStyleLabDebugger | `Window/SCkStyleLabWindow.cpp`, `Widgets/SCkStyleLab_ControlsPane.cpp`, `SamplePane.cpp`, `InputHudControls.cpp` | Best early component-gallery/template proving ground after common controls. |
| CkTextureDebugger | `Window/SCkTextureDebuggerWindow.cpp`, `TextureHealthTable.cpp`, `SceneAuditTable.cpp`, `DiagnosticPages.cpp` | Texture Health source now authors its inventory columns/cells, horizontal scroll and empty overlay; its panes use the shared authored splitter; bootstrap error chrome and context-menu content builder remain native. Focused behavioral migration gate passed20/20 (TextureAuthoredInventory-R3-Editor.log); single-line ellipsis styling passed36/36 authoring and20/20 Texture tests with inspected TextWrapping captures; remaining visual acceptance is open. Extend adapters to scene-audit/list/diagnostic pages. |
| CkUIDebugger | `Window/SCkUIDebuggerWindow.cpp`; authored `Resources/UI/UiDebuggerHistory.ui.html/.css`, `UiDebuggerLayers.ui.html/.css`, `UiDebuggerSummary.ui.html/.css`, and `UiDebuggerCommands.ui.html/.css`; source-authored production PIE fixture | History ordering/cap/clear and lifetime, Layers hierarchy/projection/actions, Summary no-layout/ACTIVE/INPUT/LAYERS cards, and the authored command shell are accepted by focused gates. Native WindowChrome retains only shared host controls; browser breakpoint parity and broader Gate 05/campaign acceptance remain open. |
| CkVisualLodDebugger | `Window/SCkVisualLodDebuggerWindow.cpp` | Standard window composition; base template candidate. |

## Shared families to turn into declarative templates

- **Window chrome and composition:** `SCkDebug_WindowChrome`, `SCkDebug_PaneHost`,
  `SCkDebug_Card`, `SCkDebug_SectionHeader`, `SCkDebug_RailContainer`, and refresh/world controls
  under `CkDebuggerCommon/Public/CkDebuggerCommon/{Window,Widgets}`.
- **Input and command controls:** `SCkDebug_SearchBar`, `SCkDebug_DualSearchBar`,
  `SCkDebug_CommandBar`, `SCkDebug_NumericEditor`, `SCkDebug_Switch`, `SCkDebug_Stepper`,
  `SCkDebug_IconButton`, `SCkDebug_IconToggle`, `SCkDebug_UnderlineTabs`, and
  `SCkDebug_WorldSelector`.  Current `<search>` covers only a plain `SSearchBox`.
- **Rows, values, and status:** `SCkDebug_KeyValueRow`, `SCkDebug_StatPair`,
  `SCkDebug_StatusPill`, `SCkDebug_ValuePill`, `SCkDebug_CountBadge`, `SCkDebug_Chip`,
  `SCkDebug_AlertRow`, and `SCkDebug_InspectorPanel`.  These should become data-bound templates,
  not copied markup.
- **Retained collections:** texture, EQS, GOAP, Jolt, scheduler, crowd, and ECS all use live
  collection views.  Their adapter contract must cover stable item keys, selection, keyboard/menu
  actions, filtering, sorted columns, and incremental refresh.

## Registered custom-control backlog

These non-graph widgets call `OnPaint`/`FSlateDrawElement` and need typed component adapters (or an
equivalent explicit declarative registry), with their current Slate implementation retained behind
the adapter: `SCkAStarDebugger_GridView`, `SCkAudioDebugger_FalloffCurve`,
`SCkAudioDebugger_Radar`, `SCkDebug_DeviceGamepad`, `SCkDebug_DeviceKeyboard`,
`SCkDebug_DeviceMouse`, `SCkDebug_EventTimeline`, `SCkDebug_FrameStrip`, `SCkDebug_MeterBar`,
`SCkDebug_OrientationCube`, `SCkDebug_ScrubTimeline`, `SCkDebug_Sparkline`,
`SCkEcsDebugger_Sparkline`, `SCkInputHud_Ribbon`, `SCkFrameBarChart`,
`SCkFramePresenceStrip`, `SCkIntentDebugger_OctantDial`, `SCkJoltDebugger_3dViewport`,
`SCkMapDebuggerWindow`, and `SCkOptimizationSnapshotViewer`.

The first registry increment should therefore expose typed retained adapters for split panes,
selectable virtualized collections, editable/select/check/numeric controls, tabs, viewport, canvas
anchors, and the listed painted controls.  It should preserve the existing all-or-nothing document
validation and stable-ID reload guarantees.


## Shared collection and table checkpoint

`CkUiCollection.h/.cpp` supply typed schema/records and stable-key atomic updates.
`SCkUiTable` and `FCkUiView` add authored table columns/cells, filtering, stable-key
selection, sorting and viewport-bounded native rows. D2b26, D2c28 and D2d31 focused
suite logs recorded in the campaign progress evidence cover collection mutation, production table
integration, custom cells, text metadata, selection/held-mouse refresh, and 0/1/12/1000/10000
fixtures. The scale-only collection cases remain distinct from virtualization proof.

Horizontal scroll and overlay passed the32-test authoring gate in BuildTest-UiScrollOverlay-R5.log.
The production fixture verifies narrow/wide table geometry, bounded rows, wheel handling, retained
selection and both offsets, and decorative-layer visibility/desired-size isolation. Texture's20-test
compatibility gate also passed; its authored inventory and splitter migrations passed subsequent focused gates recorded in PROGRESS.md; generic vertical subtrees now have focused authoring evidence; Texture compatibility and inspected long-identifier captures passed the overflow-wrap gate recorded in PROGRESS.md. The34-module
inventory above and the full campaign requirement to replace temporary native islands (except graphs)
remain unchanged.

## Boolean forms checkpoint

FCkUiCheckbox and public BoolChanged events now support retained, model-authoritative two-state input. Test_UiBoolEvents covers parser/template typing, pre-factory rejection, delivery guards and readonly collection restrictions. Test_UiCheckbox covers native down/up keyboard toggle, rejection, live value/label, enabled/readonly guards, reentrant compatible reload, retarget rejection and held-widget owner release. BuildTest-UiCheckbox.log passes54 authoring tests; Test-ResourceInspector-Checkbox-R2.log passes3 app tests with inspected wide/narrow captures. Resource Inspector Lock note drives the shared text-input read-only binding. This does not cover mixed-state checkbox semantics, full control styling/accessibility, numeric/select controls, or the remaining game and debugger acceptance.

## Numeric entry checkpoint

FCkUiNumberInput composes FCkUiTextInput::Create and prepared child updates. Public NumberChanged/NumberCommitted events provide finite float payloads, typed template forwarding, prevalidation and view-lifetime/reload guards. Test_UiNumberEvents and both Test_UiNumberInput fixtures cover these paths, native drafts, integer fractional bounds, exact tiny/large roundtrip, invalid/overflow/underflow rejection, readonly state, compatible/rejected reloads, external value changes and normalization echoes. ResourceInspector.Form.ResourceCount verifies actual collection updates through installed resources. Final BuildTest-UiNumberInput-R5.log passes57 authoring tests; Test-ResourceInspector-NumberInput-R2.log passes4 app tests with inspectedcaptures. Shared text input also now restores model validation after native error clearing and ignores model-equal postcommit notifications outside an edit. Slider/step, double/mixed/optional values, localizednumericinput and retainedundo/selection are not closed by this checkpoint.

## Numeric interaction transport checkpoint

NumberInteraction carries finite Begin/Commit/Cancel values with Pointer/Keyboard/Controller sources. Test_UiNumberInteraction exercises typed template forwarding, guarded delivery, invalid enums/nonfinite values, pre-factory rejection, readonly collections and owner release. BuildTest-UiNumberInteraction.log passes58 authoring tests; UiNumberInteraction-Editor.log has58 success records with no error/ensure/fatal/AngelScript-warning matches. Slider gestures and capture retention remain separate required evidence.

## Retained capture checkpoint

ICkUiRetainedWidget reports weak owned user/pointer/widget descriptors and receives paired local-state transfer hooks. FCkUiView validates mounted ownership, repairs exact capture ancestry for surviving components, and releases owned captures on removal or game-thread view destruction. Test_UiRetainedCapture uses real Slate ProcessReply mouse and virtual-user touch capture, compares full fresh ancestry, rejects invalid/expired/foreign/duplicate reports, and verifies invalid reload, forced capture loss, removal and externally held widgets after view release. BuildTest-UiRetainedCapture-R2.log passes59 authoring tests; Test-ResourceInspector-RetainedCapture.log passes4 app tests with inspected wide/narrow captures. These prove shared capture transfer, not SSlider mouse-up/touch-threshold/controller semantics; those remain in Gate_02h_Slider.md.

## Slider initial native checkpoint

FCkUiSlider is registered through the same retained custom-widget pipeline. Two
Test_UiSlider fixtures cover actual mouse/touch capture replies, retained local
drafts, compatible/rejected reload, release versus theft, readonly transition,
native keyboard/controller navigation and controller focus cancellation, range
validation and rejected model commits. The extended installed ResourceCount fixture
proves preview leaves the collection unchanged and release updates both real rows
and numeric input. BuildTest-UiSlider-R2.log passes61 authoring tests;
Test-ResourceInspector-Slider.log passes4 app tests with inspected wide/narrow
captures. This is initial control evidence. Gate_02h_Slider.md explicitly retains
reentrant callback/capture ordering, controller-to-touch handoff, idle cancellation
propagation and broader focus/ownership/platform acceptance as required follow-up.

## Slider callback-ordering checkpoint

BuildTest-UiSliderOrdering.log passes61 authoring tests; Test-ResourceInspector-SliderOrdering.log passes4 app tests. Reentrant Begin reload uses fresh capture ancestry; Changed removal suppresses deferred events; nested cancellation input is rejected and later input succeeds. Controller-touch handoff, idle cancel propagation, disabled transition and owner release pass. These supersede the initial checkpoint's pending callback/handoff concerns. Routed focus/navigation, vertical orientation, complete authored styling and platform acceptance remain open in Gate_02h_Slider.md.

## Slider orientation checkpoint

Optional horizontal/vertical authoring now passes62 authoring tests and4 app tests (BuildTest-UiSliderOrientation-R2.log and Test-ResourceInspector-Orientation.log). Production fixture verifies atomic invalid token rejection, mounted native identity on idle reload, vertical geometry and mouse endpoints, active/pending-touch axis rejection, and native vertical keyboard/controller steps. These are direct native navigation calls, not routed platform/controller acceptance. Complete authored styling remains open.

## Custom CSS extension checkpoint

Schema.StyleProperties declares typed Number/Length/Color values under lowercase -ck- names. Immutable snapshots reject kind conflicts and partial registration. Test_UiCustomStyles proves parser tokens/cascade, matched-target applicability, unmatched declaration validation, factory/prepared-update delivery and no-callback atomic rejection. BuildTest-UiCustomStyles-R4.log passes64 authoring tests; Test-ResourceInspector-CustomStyles.log passes4 app tests with fresh inspected wide/narrow captures. Native control brush consumption, pseudo-state styling and broader CSS capability remain separate acceptance requirements.

## Native slider CSS checkpoint

Nine slider CSS properties cover normal/hover/disabled bar and thumb colors, shared thumb dimensions and bar thickness. CustomStylesAndLifecycle observes native Slate Paint box/rounded-box output, checks six tints and dimensions, preserves active capture on color reload, rejects active/pending dimension changes and paints a held widget after owner expiry. It exposed and verifies a fix for the missing native IsEnabled binding. BuildTest-UiSliderStyles-R3.log passes65 authoring tests; Test-ResourceInspector-SliderStyles.log passes4 app tests with inspected authored-blue wide/narrow captures. Routed navigation, brush/resource customization and full platform scope remain open.

## Routed slider input checkpoint

RoutedFocusNavigation uses real Slate focus, physical keyboard Right and gamepad DPad input, navigation replies, retained reload and actual blur. It verifies exact draft retention, membership of all current focus ancestors and absence of detached ancestors, one blur cancellation and focused idle Back propagation. SCkUiSlider delegates unclaimed keys to SWidget::OnKeyDown so real events create navigation requests. BuildTest-UiSliderRouting-R3.log passes66 authoring tests; Test-ResourceInspector-SliderRouting.log passes4 app tests with inspected captures. This proves user0 routing; additional-user focus restoration and controller-session ownership remain open.

## Multi-user checkpoint

BuildTest-UiMultiUser-R2.log passes68/68 authoring tests; Test-ResourceInspector-MultiUser.log passes4/4 app tests. Actual editor logs UiMultiUser-R2-Editor.log and ResourceInspector-MultiUser-Editor.log contain68 and4 success records respectively, with no error/ensure/fatal/AngelScript-warning matches. Current and detached focus ancestry is checked for user0 and a virtual user. Slider tests verify foreign accept/back/navigation/pointer/focus loss preserve the owner draft without Changed callbacks, owner commit, and one owner blur cancellation. This is selected-host evidence, not full game/platform acceptance.
## Typed select checkpoint

BuildTest-UiSelect-R4.log passes70 authoring tests and Test-ResourceInspector-Select.log passes5 app tests. Shared custom StringBinding/StringChanged/CollectionBinding supports stable option identity independent of display text. Resource Inspector authors category-select against its existing category projection. Current tests cover native closed keyboard selection, model rejection, external model changes, live labels/removal, popup labels, retained closed reload, and owner release. This checkpoint is superseded for popup behavior by the72+5 checkpoint below; controller acceptance remains required. Fresh wide/narrow captures were inspected; see PROGRESS.md for exact runtime log evidence.
## Open select popup checkpoint

BuildTest-UiSelectPopup-R5.log passes72 authoring tests; Test-ResourceInspector-SelectPopup.log passes5 app tests. Both hosting methods exercise compatible open reload, exact retained anchor/menu/focus, current focus ancestry, live option removal/restoration, silent model changes, routed pointer selection, stale-key rejection and held-anchor removal. Shared optional retained focus transfer preserves hosted popup ancestry via normal Slate callbacks. Fresh wide/narrow app captures inspected. These tests do not establish all controller modes, custom popup-host replacement, reentrant callback reload/removal, full styling or complete debugger migration coverage.

## Select callback checkpoint

BuildTest-UiSelectReentry.log passes74 authoring tests. Two additional production-path tests cover closed native key callbacks performing compatible reload plus authoritative option/model normalization, select removal, and view destruction with held native input rejection. Label reconciliation is asserted before Tick and view expiry through a weak reference. No production/resources changed, so prior5 Resource Inspector tests remain applicable. This does not prove callback reentry from an open popup or controller input. Authored tabs remain planned in Plan/Gate_02i_Tabs.md.

## Initial authored tabs checkpoint

BuildTest-UiTabs-R4.log passes76 authoring tests and Test-ResourceInspector-Tabs.log passes5 app tests. Tabs parser/template transport, view prevalidation with zero factories on bad bindings, native header activation/navigation, authoritative selection, retained header identity and held-owner release are covered. Resource Inspector now authors Overview/Properties; existing form editing/reload passes on Properties. Fresh narrow capture exposes clipped Properties header: responsive tab-strip acceptance is not met. Popup/capture cleanup, explicit accessibility, controller and broader panel state/lifecycle coverage remain required. See PROGRESS.md for gate history and screenshots.

## Responsive tabs checkpoint

BuildTest-UiTabsWrap.log76/76 and Test-ResourceInspector-TabsWrap.log5/5 pass. Existing runtime test now resizes170px then440px and checks full header bounds, wrapped rows, content separation, restored one-row layout and input. Fresh960x640/640x480 captures confirm the earlier clipped Properties header is resolved by shared wrapping. This is width-specific evidence; long localized labels and remaining lifecycle/accessibility/controller contracts are not yet accepted.

## Hidden-tab interaction checkpoint

BuildTest-UiTabsInteractions-R2.log78/78 and Test-ResourceInspector-TabsInteractions.log5/5 pass. Retained transient cleanup hook and subtree capture release are exercised by native selects in both popup hosting modes, sliders with captured drags followed by real late pointer events, and view destruction during capture. Slider cancellation emits Cancel without Commit. Tests do not prove every custom widget, nested menu stack, controller, accessibility, or callback-driven replacement scenario.

## Tab focus checkpoint

The current shared tabs implementation additionally passes Ck.UiAuthoring.Tabs.MultiUserFocus: native virtual-user Right/Home/End navigation without selection proposals, disabled focused/selected/all-header focus repair without disturbing external user zero, and retained header identity/current focus ancestry after compatible reload. Evidence: UiTabsUsers-Editor.log79/79 authoring and ResourceInspector-TabsUsers-Editor.log5/5 app. This is one affected user plus one unrelated user; simultaneous repair of multiple affected users remains unverified. Controller/accessibility and full panel draft/scroll/reorder acceptance remain open. Earlier capability summaries above are historical; PROGRESS.md records subsequent implementation gates.
## Tab panel state and simultaneous focus checkpoint

UiTabsPanelState-R3-Editor.log passes80/80 authoring tests. Tabs.MultiUserFocus now repairs two affected virtual users while leaving user zero external, without selection events. Tabs.PanelStateRetention measures actual overflow, types through native key events, retains draft/editor/scroll across reversed compatible reload, follows the editor's native Default commit for programmatic focus loss, retains accepted state on switching back, and rejects malformed reload atomically. This exposed and verifies preserving the shared scroll's known viewport width while replacing authored content. Combined parent-width/content replacement remains unverified; no broader responsive-reload claim. See PROGRESS.md for subsequent app gate results.
## Authored command-menu checkpoint

UiMenus-R2-Editor.log83/83 authoring and ResourceInspector-Menus-Editor.log6/6 app pass. Top-level menu declarations, strict keys/attributes/typed bindings, cycle/reference/expanded-size limits, template menu-button fields, retained native command presenter and Resource Inspector Actions are implemented. Native tests cover keyboard opening, routed pointer selection, disabled commands, rejected/accepted reload behavior, retained anchor/reopen and stale/owner-release input in both popup hosts. App test proves real model/table population via installed markup command. Fresh wide/narrow captures inspected. Submenu declarations/native construction exist, but nested activation/focus teardown, callback reentry, authored table/tree context-key transport, debugger menu migrations, controller/accessibility and complete style coverage remain pending.
## Nested menu interaction checkpoint

UiMenusNested-R4-Editor.log passes85/85 authoring tests; ResourceInspector-MenusNested-Editor.log passes6/6 app tests. Native submenu hover and pointer activation, reopening, focused command owner teardown and preservation of unrelated virtual-user focus pass in both popup hosting modes. Callback-time owner release passes in the new-window host. Fixture synchronizes/restores the native cursor and chooses the deepest matching entry to avoid ancestor/hidden-button matches. No production edits in the fixture repair. Callback reload, controller/accessibility, authored table/tree context targets and debugger menu migrations remain pending; this does not close the full pipeline campaign.

## Shared menu-session checkpoint

UiMenuSession-R2-Editor.log86/86 and ResourceInspector-MenuSession-Editor.log6/6 pass. FCkUiMenuSession now provides native content, revisioned callbacks, live gates and owned focus independently of SComboButton hosting. StandaloneSessionLifetime mounts session content directly and tests routed native action, gate changes, old snapshot rejection, deactivation focus cleanup and weak lifetime with content retained. Existing button/nested behavior remains covered. Typed row-key context hosting and debugger migration are next; no broader menu/controller/package acceptance claim.

## Authored row context-menu checkpoint

UiContextMenus-R2-Editor.log89/89 and ResourceInspector-ContextMenus-Editor.log6/6 pass. Tables/trees author context-menu references and bind typed ContextActions. Native tests route actual row rightclick and nested command, prove clicked B survives selectionredirectA, reject removed/reinserted identity, preserve rejectedreload and close onacceptedreload/ownerrelease; treeShiftF10 dispatch passes. Installed app Show properties switches the actual detailtab and retains targetkey. Dedicated Context/Menu key is absent from this engine mapping; ShiftF10 is supported. Controller, contextmultiuser, drag/blankspace/filter cases, callbackPushMenu reentry and debugger migrations remain separate acceptance work; no full-suite or fullcampaign completion claim.

## Ordinary button enablement and debugger migration checkpoint

UiButtonEnabled-R2-Editor.log90/90, TextureAuthoredMenus-R4-Editor.log22/22, and ResourceInspector-ButtonEnabled-Editor.log6/6 pass. Ordinary buttons support typed enabled-bind, atomic missing binding rejection, live native disabled state and dispatch-time owner/revision/boolean gates. SceneAudit loads its authored layout and verifies stable rows, filter/highlight, selection and Clear behavior. TextureHealth routes actual rightclick/copy while selection changes and preserves the opening row target. Dedicated SceneAudit visual captures/both copy commands and broader campaign acceptance remain unverified. These checkpoints supersede earlier pending descriptions.


## Scene Audit installed menu and capture checkpoint

SceneAuditAcceptance-Editor.log24/24 TextureDebugger tests verifies both installed native clipboard commands retain opening B after selection changes to A, plus1000-row bounded native realization at960x640/640x480 and1/1.5scale. SceneAuditCountCapture-Editor.log1/1 and inspected narrow PNG verifies no-wrap count badge correction. The stale high-DPI gray-square limitation is cleared by the one-start cached real-RHI `Saved/Logs/Test-SceneAuditIconRecapture-R1.log`: `Ck.TextureDebugger.LayoutCapture.SceneAudit` passed1/1 with0 failed/skipped/contaminated, SHA256 `4706BFEEE43312D81C1C695A17C925F192388C1F73CF4F81FBF4A926C6C9864C`. The refreshed 2026-09-10 12:49:56/57 local captures retained their hashes (960x640 1p0 `A01747DED29270BB5A840AF6CE02AE7B58C806313E1EAD3907407723E89CA7D0`, 1p5 `3B82770DD1E6770D2741E7FBAF86FF24202F5F34B92F3166F5AC9AC42A3C2C9A`; 640x480 1p0 `2E7FECE0EF8FCB0C4FCA4AA1A69D76342CA33FB1D3979AB32440681EDE274C8E`, 1p5 `C90AAFD73153758271F7416F099EF75A4CD8E0C976FA98092DFD71D485272EE2`). Visual inspection of both 1.5 captures confirms two complete magnifiers and no gray squares; this is capture-test plus visual evidence, not pixel-regression automation. Full visual acceptance remains open. No packaged/controller/localization claim.


## Material Inputs and nonselectable tables checkpoint

UiTableSelectable-R2-Editor.log91/91 verifies table selectable=false, default single selection, typed template transport, rejected malformed reload, retained native identity and silent clearing on accepted disable. Native selection must be cleared before SetSelectionMode(None) in this engine.

MaterialInputsAcceptance-Editor.log passes26existing TextureDebugger tests; the2new tests initially rejected unsupported CSS. MaterialInputsAcceptance-R2-Editor.log2/2 passes after resource-only alignment correction. Real collector fixtures cover null and installed checker slots, filter/highlight colors, parameter detail tooltip field, stable record identity on refresh/reload and native selection disabled. Captures exercise200rows from100installed checker slots at two widths/DPI scales with bounded realization; wide1x/narrow1.5x inspected. This is combined focused evidence, not a fresh28/28 aggregate. ResourceInspector-TableSelectable-Editor.log6/6 passes compatibility.

Table column labels and search placeholders remain authored literals; shared localized binding support is the next required capability before full migration/localization acceptance. Remaining debugger, game/package/controller and full style/lifecycle/performance scope is open.

## Pinned snapshots and repeated capture checkpoint

Resource Inspector pinned snapshots use the shared repeat adapter with independent
collapse/removal, duplicate pin updates, accepted/rejected reload and stale-action
checks. ResourceInspector-PinnedSnapshots-R2-Editor.log passes7/7; shared
PinnedSnapshots-R3-Editor.log passes103/103; Texture-PinnedSnapshots-Editor.log
passes30/30. Actual runtime counts match and all error/ensure/fatal/AngelScript-warning
scans are zero. Native wide/narrow captures were inspected, with lower narrow cards
below the scroll fold and requiring additional visual coverage.

Parent repeat removal originally left an externally held child captor active.
Retired child interactions are now captured before detachment and reconciled after
publication. The native regression verifies wrapper transfer, reentry suppression,
collection removal/reinsertion and parent removal. Popup retirement and deeper
nested collections remain open. Authored button labels currently use STextBlock
without width-aware wrapping; add shared support and focused geometry evidence.
## Button wrapping and tabs scroll measurement checkpoint

Ordinary buttons now accept wrap/overflow-wrap/overflow styles and measure native
padding plus constrained SCkFlexText labels. Tabs measure wrapped headers and only
the selected body under the available width, allowing outer vertical scrolls to
reach the full content. No model mutation occurs during measurement.
TabsMeasurement-R2-Editor.log105/105, ResourceInspector-TabsMeasurement-Editor.log7/7,
and Texture-TabsMeasurement-R2-Editor.log30/30 pass with matching actual counts and
zero error/ensure/fatal/AngelScript-warning matches. Inspected Pinned_Narrow_640x480.png
shows complete final snapshot at scroll end; native action bounds are asserted.
This supersedes the earlier button-wrap and below-fold gaps, not the full campaign.
