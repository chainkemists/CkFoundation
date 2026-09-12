## Data pane retention verified (2026-09-08)

GalleryDataPaneRetention-R3 builds and passes 2/2 in 38s. Its archived actual `CkPlugins_2.log` as `GalleryDataPaneRetention-R3-Editor.log` has 2 successes and zero relevant diagnostics; editor exit was 0 and PID 10824 is terminal. Across initial, R2, and R3, six editor starts were used. The first two attempts rejected an impossible full-visible assertion because the error banner leaves a 292 viewport for a 320 splitter and cached end shifts 253 to 323; this was not a product defect.

R3 uses ScrollToEnd, visible-intersection splitter drag, and bottom-scrollbar reachability. Compatible reload retains the native adapters, horizontal offset 48, dragged splitter coefficient, and table selection. Exact Menus focus is asserted before Data navigation. Root reviewed `Data-Narrow-CompatibleReloadScrollSplitter.png`: outer scrolling intentionally clips the top while the divider and table scrollbar remain reachable. Only the test changed.
## Native slot scope and two-view batch reload verified (2026-09-08)

BuildTest-NativeSlotScope.log passes 115/115 in 47s, GalleryBatchReload-R2.log passes 2/2 in 36s, and NativeSlotScope-ResourceInspector.log passes 10/10 in 44s. Their archived actual editor logs are `NativeSlotScope-Editor.log` from `CkPlugins_2.log`, `GalleryBatchReload-R2-Editor.log` from `CkPlugins.log`, and `NativeSlotScope-ResourceInspector-Editor.log` from `CkPlugins.log`; each has the stated success count and zero relevant diagnostics. Editor exit was 0 and all processes are terminal. Four additional editor starts were used, six total including the original failed batch gate.

The gallery now demonstrates an atomic main-plus-native-preview reload with accepted/rejected narrow captures. The shared path scopes native bindings to each custom slot, recognizes committed child native ports, keeps rejected validation from mutating live scope with a scratch FCustomSlot, and rejects cross-slot native movement. Regression coverage uses a foreign SBox parent and proves a rejected candidate can be followed by a valid callback. Root reviewed both batch captures and confirmed readable accepted/rejected text, status, revisions, and action with preserved accepted content after rejection.

Next inspectable supported-state slice: horizontal Data scroll and splitter retention through accepted reload at `gallery-dialog/content/records-scroll` and `gallery-dialog/content/data-splitter`. Variable per-item row heights remain unsupported.
## Active-dialog owner release verified (2026-09-08)
BuildTest-GalleryActiveDialogRelease.log builds and passes2/2 in43s, editor exit0. Actual CkPlugins_2.log archived as GalleryActiveDialogRelease-Editor.log after2success-record check; zero Error/ensure/fatal/AngelScript-warning/error/MissingResource matches. Two editor starts, process terminal. Existing Forms focused-owner release retained. Fresh installed gallery model/window helper explicitly selects Compose, scrolls/focuses native opener, opens dialog and verifies exact body Cancel focus before releasing sole model pointer. Held root/body/close buttons survive; weak model expires, owned focus clears without invoker restoration, held callbacks cannot invoke external close counter. No production/resource changes. Root caught hidden-tab presence assumption before gate; setup is now actual native path.
Bounded Gate03 audit identified next concrete multi-view batch rollback example, optional-slot/custom-factory lifecycle, variable-height collections, horizontal scroll/splitter retention and zero-content/text cases. Reject redundant stale-action gap: acceptedreload already proves old ordinary action inert; current active-owner case adds owner-expired action evidence. Existing registered dialog satisfies required content/body custom-slot example; arbitrary optional factory still separate. Crossenvironment controller/multi-user/localization/package/performance remain later, unsupported nestedcollections require shared design.
Next batch design recommendation (not implemented): private gallery-owned second FCkUiView, own owner gate/bindings and tiny preview document, mounted via native binding in Menus; successful two-view TryReloadBatch then malformed second candidate must preserve both accepted revisions/roots/text and live callbacks. Avoid coupling standalone GalleryStarter registry/data. Verify per-view release and actual mounting before implementation. Full campaign active; preserve dirty work, no commit/push/merge.
## Main-page scroll and dialog focus verified (2026-09-08)
BuildTest-GalleryScrollDialogFocus.log builds and passes2/2 in41s, editor exit0. Actual CkPlugins_2.log archived as GalleryScrollDialogFocus-Editor.log after2success-record check; zero Error/ensure/fatal/AngelScript-warning/error/MissingResource matches. Two editor starts (discovery and native acceptance), process terminal. Test-only helper checks all six main page native scrolls at640x480 with nonzero offset32, away/back routed native header clicks, same widget/offset, then resets offsets/restores wide Compose.
Existing registered-dialog test now explicitly focuses native composition-open before open, asserts first body Cancel focus, focuses/clicks Confirm and verifies exact invoker restoration, reopens and cancels with same restoration. FindButton resolves actual SButton leaves rather than measured tags. No production or resource change in this slice. Audit confirms owner destruction differs: clears owned dialog focus without restoring invoker; current gate does not claim active-dialog owner-destruction coverage. Terra tests/contract audit/ledger cleanup, root reviewed and gated.
GalleryCoverage current rows were reconciled for earlier numeric/select/tree/sort/reload and registered content/body slots. Current scroll/dialog-focus cells now closed by this checkpoint. Remaining supported-state/adapter lifecycle and full scale variants require scoped review before representative migration; controller/multi-user/localization/package remain cross-environment work. Nested collection support remains separate shared design. Full campaign active; preserve dirty work; no commit/push/merge.
## Numeric and empty-select states verified (2026-09-08)
FINAL BuildTest-GalleryNumericSelect-R3.log builds and passes2/2 in38s, editor exit0. Actual CkPlugins.log archived as GalleryNumericSelect-R3-Editor.log after2success-record check; zero Error/ensure/fatal/AngelScript-warning/error/MissingResource matches. Root inspected refreshed Forms-Narrow-InvalidNumber.png and Forms-Narrow-EmptySelect.png: complete cards show accepted24/live proposal12/native error and empty0/requestedcompact/status. Recovered42 capture also generated. Six editor starts total across initial/R2/R3; all processes terminal.
Initial1/2 exposed real gallery authoring mistake: NumberChanged wrote accepted _Number for each valid typing prefix1/12, so final malformed12junk rejection left model12. Separated _NumberProposal from committed _Number and exposed both; no shared native parser changes. Invalid draft has no commit/count/status change; valid42 clears error/commits; native baseline24 restored. R2 passed2/2 but capture viewport clipped state labels; R3 only changed scroll targets to full number/select cards. Native Enter may restore authoritative display24 while error remains; do not claim invalid draft remains displayed after commit.
Empty/restore options actions publish through TrySetRecords first, preserve requestedcompact, display native placeholder/count/key/status, and emit no select callback; restore silently resolvescompact, then native Down emits exactly one callback. Reset requires successful options publication before reset scalars change. Separate live numeric proposals and committed values are the intended gallery model contract. Tests restore24/compact before later pages. New helper avoids oversized RunTest, Terra implementation/audit/test ownership and root review/gates.
Next: main-page scroll retention across native tab changes and existing registered-dialog body-focus/dismiss/reopen lifecycle. Audit confirms FCkUiDialog is already a retained registered custom tag with content/body slots; do not invent redundant generic adapter merely to fill custom-slot example. Arbitrary factory/optional-slot/batch failure gallery cells remain separate. Full campaign and migrations remain active. No commit/push/merge; dirty work preserved.
## Accepted compatible reload verified (2026-09-08)
BuildTest-GalleryAcceptedReload.log builds and passes2/2 in35s, editor exit0. Actual test lane is CkPlugins_2.log, archived as GalleryAcceptedReload-Editor.log after2success-record check; CkPlugins.log was discovery only. Zero Error/ensure/fatal/AngelScript-warning/error/MissingResource matches. Two editor starts, process terminal. Root reviewed Menus-Narrow-CompatibleReload.png: changed description and accepted status readable, native controls remain accessible. After capture root shortened outdated panel heading from Rejected reload lab to Reload lab; static validator passes, no behavior change or extra editor gate for that text-only correction.
RunAcceptedReload reads installed pair and changes only commands-description in memory, preserves IDs/bindings and never writes resources. Setup failure is distinct and never invokes reload. Real lastresult/live revision drive status. Extracted helper verifies +1 revision, retained tabs/table/navigation/tree-lab/repeat/scroll and native list/tree identity, preserved model/selection/expansion, exact Menus header focus and nonzero32 scroll offset, visible changed authored text, held ordinary dialog action cannot alter dialog/status, fresh dialog open/dismiss, and reacquired current text editor for ownerrelease. Ordinary buttons/editors are rebuilt; no false retained-editor promise. Terra implementation/test/audit, root reviewed callback/focus/offset contracts and integrated.
Next supported gallery priorities from bounded audit: numeric invalid/recovery and empty select state; main-page scroll offset retained across tab switches; explicit custom-slot lifecycle example. Verify registered dialog already supplies custom-slot coverage before inventing duplicate adapter. GalleryCoverage old row7 keyed-tree gap and row11 rejected-reload gap are superseded by current dated checkpoints; remaining custom-factory/ownership/batch failures stay open. Full campaign including nested collection design, representative and conventional migrations, specialized/cross-environment acceptance remains active. No commit/push/merge; dirty work preserved.
## Independent tree mutation lab verified (2026-09-08)
FINAL BuildTest-GalleryTreeLab-R2.log builds and passes2/2 in42s, editor exit0. GalleryTreeLab-R2-Editor.log archives actual CkPlugins.log after2success-record check; zero Error/ensure/fatal/AngelScript-warning/error/MissingResource matches. Root reviewed Repeats-TreeLab-Narrow.png: five controls readable, expanded Root with reversed Gamma/Beta/Alpha rows and Alpha selection, native scroll reachable. Two editor starts total. Initial compile failed before any editor on duplicate NavigationResult local and C4883 oversized InstalledResources function; corrected descriptive locals and extracted unchanged tree coverage into RunTreeLabAcceptance. No compiler suppression or coverage removal.
Independent gallery-tree-lab uses atomic TrySetNodes with Root/Alpha/Beta baseline, Gamma insertion, reversal, selected-leaf removal, reinsertion and reset. Model metadata changes only after successful publication. Existing page navigation and global Reset remain unchanged. Native keyboard expansion, actual row clicks, realized row Y-order, surviving identities/selection/expansion, removal callback clearing/detachment, fresh reinsertion without auto-selection and reset retained identities/collapsed root/empty selection all verified. Reset explicitly clears model selection after successful native clear to handle no-op callback behavior. Prebuild review corrected false fresh-node reset assumption and outer-scroll geometry placement. Terra model/resources/tests ownership plus independent review; root integrated/gated.
Next: accepted compatible reload lab with visible revision advancement and appropriate retained native identity/focus/scroll evidence. Remaining gallery state cells, nested collection design, representative complex migration, conventional migrations and specialized/cross-environment acceptance remain open. Full campaign active; preserve dirty work; no commit/push/merge.
## Native table sorting verified (2026-09-08)
BuildTest-GallerySortLab.log builds and passes2/2 in38s, editor exit0. GallerySortLab-Editor.log archives actual CkPlugins.log after2success-record check; zero Error/ensure/fatal/AngelScript-warning/error/MissingResource matches. Two editor starts (discovery and native acceptance); process terminal. Test-only extension uses the existing sortable Name column, actual STableColumnHeader button pointer input and SListView projected items. Full12-key ascending/descending order, selected record-00005, model selection and retained record identity verified. Sorting runs after existing Data mutation assertions; no guessed third-click reset, production code or resource changes. Existing gallery/starter coverage stays passing.
Terra independently reconciled stale current GalleryCoverage rows for Forms, flat repeats and nested tabs; historical checkpoint prose remains dated evidence. Next tree slice must use separate gallery-tree-lab collection: existing _Navigation selection drives six-page SetPage and must remain intact. Tree selection clears on removal; surviving root expansion and nodes retain identity; reinsertion does not auto-reselect. Implement inspectable insert/reorder/remove/reinsert/reset through atomic TrySetNodes with native tree coverage. Then accepted compatible reload and remaining gallery cells, followed by approved migration order. Full campaign active, dirty work preserved, no commit/push/merge.
## Nested tabs lab verified (2026-09-08)
FINAL BuildTest-GalleryTabsLab-R4.log builds and passes2/2 in39s, editor exit0. GalleryTabsLab-R4-Editor.log archives actual CkPlugins.log after2success-record check; zero Error/ensure/fatal/AngelScript-warning/error/MissingResource matches. Root inspected Compose-NestedTabs-LongLabel-640x480.png: all three nested headers including long Details label, scenario controls and retained draft are readable inside native scroll.
Compose now has ordinary nested Overview/Details/History tabs, bound draft, enable/long-label/reset actions. Native coverage proves pointer selection, retained pane/editor identity and draft, focus-only arrows/Home/End, Space activation and callback count, model-owned Overview fallback when active Details is disabled, native focus evacuation, blocked disabled selection, reenable, external focus preservation and dialog continuity. No shared runtime changes. Static validator now counts only direct gallery-pages children and balances template scope; independent review confirms menu declarations have separate ID namespace.
Initial compile stopped at incorrect SOverlay include (zero editors); R2 1/2 rejected missing required committed attribute; R3 1/2 exposed invalid test equality between measured wrapper and retained native tabs. Corrected production-required draft callback and source-proven test containment assertion, not runtime workarounds. Six editor starts total across R2/R3/R4; all processes terminal. Dirty work preserved; no commit/push/merge. Full campaign remains active.
Next bounded slice: actual native table-header ascending/descending sorting with complete row-order assertions and selected-key retention. Existing markup already declares sort-field; use production STableColumnHeader pointer routing and SCkUiTable GetList projection. Then keyed tree mutation and accepted compatible reload; remaining GalleryCoverage and shared nested-collection design stay open before migrations.
## Rejected reload state update (2026-09-08)
GalleryReloadLab builds2/2; actual final editor2successes and clean relevantdiagnostics. Four installed-source in-memory failures are inspectable on Menus: malformedmarkup, missingbinding, missingaction, unsupportedstyle. Tests prove failure diagnostics, unchanged revision/nativeidentity/modeldata and working accepted dialogcallbacks aftereachrejection. Resource hashes unchanged; narrow diagnostic capture reviewed. Setupfailures are distinct. This does not cover custom-factory failure, arbitrary ownership failure, multi-view rollback or every lifecycle state.
## Forms text-validation update (2026-09-08)
GalleryValidation-R2 builds2/2, actual editor2successes with clean relevant diagnostics. Native65-character typing/commit preserves prior accepted value with visible error;64-character recovery commits exact value and hides error. Invalid-load/recover/explicit-commit controls use same setter; visible draft length/committed value/lastaction expose state. Native readonly/disabled behavior and sibling numeric/select interactions remain covered. Narrow invalid/recovered captures visually reviewed; scenario buttons are scroll-reachable. This is text validation, not numeric-invalid/select-empty/controller/full Forms scale acceptance.
## Flat repeat lifecycle update (2026-09-08)
GalleryRepeatLab-R2 builds and passes2/2, actual editor2successes with clean relevant diagnostic scan. Authored repeat reverse/update/remove/reinsert/reset and status expose lifecycle in the Repeats page. Native test verifies actual order/text change; same-key widget retention; expansion independence; removed held-button rejection; fresh reinserted native identity with restored model state; repeat-only reset. Repeats-Wide-Scrolled-Lab.png visually inspected. Nested collections remain unsupported/pending design, and repeat keyboard/focus/lifecycle state matrix is not complete.
## Nested collection boundary (2026-09-08)
Nested repeats are currently rejected, not merely missing gallery examples: CkUiDocument.cpp ValidateFieldScope rejects repeat/table/tree/native inside repeat items, and SCkUiSurface.cpp repeats that runtime validation. CkUiCollection.h permits Text/Number/Bool/Color/Image fields, with no child-collection field contract. Adding useful parent-specific nested collections therefore requires an explicit binding/identity/publication/lifetime design; deleting the parser guard would not establish support. Keep this as pending shared-runtime capability work for the full campaign. The active flat repeat lifecycle lab covers only supported per-key mutation and retention.
## Layout lab update (2026-09-08; supersedes older layout-scale pending statements below)
BuildTest-GalleryLayout-R7.log2/2 and GalleryLayout-R7-Editor.log2successes, clean relevant diagnostics. Supported direction/grow/shrink/width/min-max/gap/padding/alignment/wrap/ellipsis examples are authored. Native geometry and top/scrolled captures cover Layout at wide/narrow and100/150/200percent application scale. Actual-window containment and>=64logical scroll height are asserted; narrow200 measured65 with compact Repeats/Menus/Compose labels. Root visual review confirms reachable text and ellipsis. Other pages' scale/state variants remain open. No margin, percentage-length or flex-basis CSS support is implied; these are parser boundaries, not missing examples of supported syntax. Nested composition, controls, lifecycle, reference parity and cross-environment acceptance remain open.
## Initial native acceptance checkpoint (2026-09-08)
CapabilityGallery-R9.log passes 2/2 through installed resources: six native tab headers, disabled/read-only text behavior, text/number commits, select, independent record expansion, dialog cancel/confirm, rejected reload and owner release; starter loads, projects/selects an actual mounted table row, and preserves the accepted region/table on missing search binding. GalleryLifetime-ResourceInspector.log passes 10/10, including gallery/inspector host switching. BuildTest-GalleryLifetime-R8.log passes 115/115 authoring, including held-root focus release, foreign focus preservation and callback redirect preservation. Archived actual logs match counts and have zero Error:/ensure/fatal/AngelScript warning-error matches. R10 gallery 2/2 proves the verified Checkerboard brush without missing-resource diagnostics.

This is initial behavior evidence, not exhaustive gallery completion. The R10
checkpoint proves the initial six-page authoring load, two gallery cases and ten
consumer cases; twelve wide/narrow captures were reviewed. The title separator
mojibake was corrected and verified in the later collection-lab captures.
Application-scale variants remain open; complete style/state variants, larger
collections, localization, controller sessions and the remaining rows below stay
open. Reusable standalone GalleryStarter resources and exact native binding
contract live in CkTests/Resources/CapabilityGallery/Templates. Template reuse
remains document-local.

# Authored capability gallery coverage

This is the inspectable-page checklist for the HTML/CSS-to-Slate workbench. A
focused unit or consumer test proves behavior, but it does not create a gallery
example. Each row below therefore has two independent columns: existing evidence
and the page/state still required. A screenshot of the full Resource Inspector is
not a substitute for an isolated example.

## Current schema boundary

The authored document currently declares these node kinds in
`CkSlateLayout/Public/CkSlateLayout/CkUiDocument.h`: `row`, `column`, `text`,
`button`, `native`, `search`, `image`, `scroll`, `splitter`, `custom`, `table`,
`table-column`, `overlay`, `tree`, `tabs`, `tab`, `menu-button`, and `repeat`.
The same document model carries table selection, typed field bindings, tab keys,
repeat item actions, named regions, templates/uses and menu declarations. The
registry additionally exposes typed text/image/number/bool/string/color bindings,
typed events/actions, custom styles, retained custom widgets, named slots,
`CanDispatchEvents`, Slate-user ownership and transient capture release
(`CkUiWidgetRegistry.h`, `SCkUiSurface.h`).

The gallery must only claim properties accepted by the current parser and registry.
Unsupported browser CSS, bespoke painted widgets and graph internals require an
explicit registered adapter page or remain outside the authored gallery.

## Current static resource inventory

`Plugins/CkTests/Resources/CapabilityGallery/CapabilityGallery.ui.html` now
contains six authored page keys: `layout-page` (`key="layout"`),
`forms-page` (`forms`), `data-page` (`data`), `collections-page`
(`collections`), `commands-page` (`commands`), and `composition-page`
(`composition`). The validator reports all six required keys and no duplicate
IDs outside template scope. The resource also provides these concrete examples:

- Layout/style: `layout-scroll`, `layout-content`, `layout-bound-text`,
  `layout-committed-text`, `layout-selection`, `layout-preview`,
  `gallery-color-probe`, `gallery-image`, `layout-disabled-overlay`.
- Controls: `gallery-edit-text`, `gallery-read-only`, `gallery-number`,
  `gallery-slider`, `gallery-select`, `gallery-query`.
- Collections/navigation: `data-splitter`, `gallery-navigation`,
  `gallery-records`, `record-name`, `record-text`, `records-empty`,
  `gallery-repeat-items`, `repeat-card`, `repeat-toggle`, `repeat-expanded`,
  `gallery-pages`.
- Commands/composition: `commands-menu`, `commands-reset`, `commands-dialog`,
  `commands-reject`, `commands-status`, `composition-open`,
  `gallery-dialog`, `gallery-dialog-body`, `dialog-dismiss`,
  `dialog-confirm`, and the `bound-metric` template used by `layout-bound-text`
  and `layout-committed-text`.

This remains a static parser/inventory result. Runtime evidence is recorded in
the R10 checkpoint above and is limited to the cases named there; it does not
turn this inventory into exhaustive proof of layout, bindings, interaction,
reload, ownership, scale, or screenshot fidelity.

## Collection lab checkpoint

`GalleryCollections-R3.log` passes 2/2 in 31 seconds. The corresponding editor
log has zero `Error`, ensure, fatal, AngelScript warning/error, or missing
resource matches. Proven cells are: counts 0/1/12/1000/10000; live-row bounds
greater than zero and below 128 for the large cases; native filter; actual row
order and text mutation; selection-count transitions; removal/reinsert; and
three independent repeat items. The earlier R2 reachability gate proved the
vertical scroll/min-height path, and the final data narrow scrolled capture
shows a reachable table with an ASCII-correct title. The initial narrow viewport
itself remains unusable, so this does not prove general narrow layout.

## Page groups

| Page | Inspectable examples and required state variants | Existing unit/consumer evidence | Gallery gap/status |
| --- | --- | --- | --- |
| 1. Layout primitives | `layout-scroll`, `layout-content`, `layout-bound-text`, `layout-selection`, `layout-preview`, `gallery-color-probe`, `gallery-image`, `layout-disabled-overlay` | `Test_UiDocument.cpp`, `Test_UiFlexWrap.cpp`, `Test_UiCustomMeasurement.cpp`, `Test_UiVerticalScroll.cpp`, `Test_UiTextWrapping.cpp`; Resource Inspector and StyleLab geometry consumers; GalleryEmptyLayout-R2 | Supported native gallery scope is complete. Layout R7 plus EmptyLayout-R2 prove supported geometry and zero-content toggle states: collapsed content/read-only overlay are not visible, desired height decreases, outer minimum remains 150, and scroll/outer survive navigation. Empty/recovery and populated geometry are captured wide and narrow 1x. No physical node removal or full empty-state scale coverage is claimed. Margin and flex-basis are unsupported parser boundaries |
| 2. Text, image and style | `layout-bound-text`, `layout-committed-text`, `gallery-color-probe`, `gallery-image`, `gallery-slider`, `description`, `title` | `Test_UiTextMetadata.cpp`, `Test_UiTextWrapping.cpp`, `Test_UiCustomStyles.cpp`, `Test_UiLocalizedLabels.cpp`, `Test_UiButtonWrapping.cpp`; GalleryEmptyLayout-R2, GalleryStyledSlider | Supported native gallery scope is complete. Zero-content messaging/recovery, wrapping/overflow/ellipsis, text styling, and named gallery style cases have current evidence. GalleryStyledSlider verifies the existing registered slider `-ck-*` color/length properties from `CkUiSlider.cpp`: six paint colors, 18px thumb, 6px bar, routed keyboard/pointer input, and disabled rejection. Full localized and environment-scale matrices remain later work |
| 3. Basic actions | `commands-reset`, `commands-reject`, `composition-open`, `dialog-dismiss`, `dialog-confirm`, `toggle-enabled` | `Test_UiButtonEnabled.cpp`, `Test_UiButtonWrapping.cpp`, Resource Inspector menu/button consumers; dated gallery action/reload/owner-release gates | Supported native gallery scope is complete. Named enabled/disabled, stale-action/revision, dialog, long-label, and owner-release cases are covered by the current gallery gates. Controller, multi-user, localization, package, and performance acceptance remain separate |
| 4. Input controls | `gallery-query`, `gallery-edit-text`, `gallery-read-only`, `gallery-number`, `gallery-slider`, `gallery-select` | `Test_UiTextInput.cpp`, `Test_UiTextEvents.cpp`, `Test_UiCheckbox.cpp`, `Test_UiNumberInput.cpp`, `Test_UiNumberInteraction.cpp`, `Test_UiSlider.cpp`, `Test_UiSelect.cpp`, `Test_UiSelectPopup.cpp`; GalleryValidation-R2, GalleryNumericSelect-R3, GalleryStyledSlider, dated owner-release gates | Supported native gallery scope is complete. Native text validation, readonly/disabled, numeric proposal/commit rejection and recovery, empty/select restore, query, slider interaction/style, and owner release have named coverage. Controller, multi-user, localization, package, and performance remain later acceptance |
| 5. Scrolling and panes | `layout-scroll`, `records-scroll`, `data-splitter`, `layout-preview` | `Test_UiScrollOverlay.cpp`, `Test_UiVerticalScroll.cpp`, `Test_UiSplitter.cpp`, `Test_UiSplitterValidation.cpp`, Resource Inspector/Texture captures | Supported native gallery scope is complete. Vertical gallery scroll retention and Data horizontal `records-scroll` plus `data-splitter` retention through accepted reload are proven: native adapters, horizontal offset 48, dragged splitter coefficient, and table selection persist. Variable per-item row heights remain unsupported; broader cross-environment resize/controller states are later work. |
| 6. Collections | `gallery-records`, `record-name`, `record-text`, `records-empty`, `gallery-query` | `Test_UiCollection.cpp`, `Test_UiKeyCollectionBindings.cpp`, `Test_UiTableView.cpp`, `Test_UiTableSort.cpp`, `Test_UiTableSelectable.cpp`, `Test_UiTableInteraction.cpp`, `Test_UiTableLifecycle.cpp` | **Supported native gallery scope complete. R3 proven cells:** 0/1/12/1000/10000 counts, bounded live rows, native filter, order/text mutation, selection transitions, removal/reinsert, and three independent repeats. Native Name-header ascending/descending interaction and selected-record identity now verified by GallerySortLab. Larger composition and cross-environment scale/style/controller variants are later work |
| 7. Trees and repeated subtrees | `gallery-navigation`, `gallery-repeat-items`, `repeat-card`, `repeat-toggle`, `repeat-expanded`, `gallery-tree-lab` | `Test_UiTreeView.cpp`, `Test_UiTreeMutation.cpp`, `Test_UiTreeCollection.cpp`, `Test_UiRepeat.cpp`, `Test_UiViewBatch.cpp`, Resource Inspector pinned/activity and SurfaceLighting consumers; GalleryRepeatLab-R2, GalleryTreeLab-R2 | Supported native gallery scope is complete. Flat repeat and keyed native-tree labs prove reverse/update/remove/reinsert/reset, retained surviving identities/state, detached removed controls, fresh reinsertion, selection/expansion reconciliation, and reset. Variable per-item row heights remain unsupported. Broader cross-environment focus/lifecycle coverage is later work. Nested repeats remain explicitly unsupported as a shared-runtime boundary for future work |
| 8. Tabs and navigation | `gallery-pages`, `layout-page`, `forms-page`, `data-page`, `collections-page`, `commands-page`, `composition-page`, `capability-nested-tabs` | `Test_UiTabs.cpp`, `Test_UiTabsParser.cpp`, `Test_UiTabsInteractions.cpp`, `Test_UiTabsMeasurement.cpp`, `Test_UiTabsPanelState.cpp`, `Test_UiTabsUsers.cpp`, Resource Inspector tabs; GalleryTabsLab-R4 and GalleryScrollDialogFocus | Supported native gallery scope is complete. Main-page native scroll retention and nested Overview/Details/History selection, retained draft/pane identity, disabled Details repair, long label, keyboard header navigation/Space activation, focus evacuation, and dialog continuity are covered. Controller/multi-user/localization/package acceptance remains later work |
| 9. Menus and dialogs | `commands-menu`, authored table/tree context-menus, `commands-dialog`, `dialog-dismiss`, `dialog-confirm`, `gallery-dialog`, `gallery-dialog-body`, `commands-accept-reload`, `commands-reject` | `Test_UiMenus.cpp`, `Test_UiMenusNested.cpp`, `Test_UiMenuSession.cpp`, `Test_UiContextMenuParser.cpp`, `Test_UiContextMenuRuntime.cpp`, `Test_UiDialog.cpp`, GalleryAcceptedReload, GalleryContextMenus-R2, Resource Inspector dialog consumer | Supported native gallery scope is complete. Compatible/rejected reload diagnostics and routed dialog open/dismiss/focus restoration are covered. GalleryContextMenus-R2 verifies native Actions `menu-button` pointer toggle/reopen, plus table/tree right-click popup opening, exact context keys, bound labels/status, and action counts; it does not claim selection retention. The historical nested-reopen uncertainty remains unresolved. Controller/multi-user/localization/package acceptance remains later work |
| 10. Composition and ownership | `bound-metric` template, `layout-bound-text`, `layout-committed-text`, retained `gallery-dialog` with `content`/`body` slots, `composition-open`, `gallery-slot-probe` with required `content`/optional `details` | `Test_UiTemplates.cpp`, `Test_UiCustomSlotsParser.cpp`, `Test_UiCustomSlotsRuntime.cpp`, `Test_UiRetainedRegistry.cpp`, `Test_UiRetainedCapture.cpp`, `Test_UiOwnerContext.cpp`, `Test_UiViewDispatchScope.cpp`; GalleryAcceptedReload, GalleryActiveDialogRelease, GalleryOptionalSlots-R4 | Supported native gallery scope is complete. Document templates, registered dialog, native preview slot, and gallery-only custom factory provide inspectable composition. Scoped native bindings, child ports, rejected-validation atomicity, cross-slot rejection, optional-slot lifecycle, qualified getter, focus cleanup, stale action, and owner expiry are covered. Arbitrary factory breadth and cross-environment capture/focus acceptance remain later work |
| 11. Failure and lifecycle lab | `commands-accept-reload`, `commands-reject`, `records-empty`, `layout-disabled-overlay`, `gallery-dialog`, `gallery-slot-probe` | `Test_UiView.cpp`, `Test_UiViewBatch.cpp`, `Test_UiViewDispatchScope.cpp`, `Test_UiRetainedCapture.cpp`, `Test_UiTableLifecycle.cpp`, `Test_UiTabsInteractions.cpp`; GalleryAcceptedReload, GalleryReloadLab, GalleryActiveDialogRelease, GalleryOptionalSlots-R4 | Supported native gallery scope is complete. Compatible/rejected reload candidates preserve accepted revision, native identity, model data, and callbacks. Atomic main-plus-preview acceptance/rejection, cross-slot rejection, optional `PrepareReload` rejection absent/present, generic presence resynchronization, stale actions, and owner teardown are covered. Broader factory breadth and cross-environment lifecycle acceptance remain later work |
| 12. Resource Inspector reference | No Resource Inspector-specific gallery page; the capability resource's `data-page` and `gallery-records` are the smaller analogues | Installed Resource Inspector consumer; current-binary `ResourceInspector-Row12-R1` 10/10; `Workbench.reference.html` is the visual target | **Native reference scope complete.** Installed loading/error, pins, dialog, and long-label consumer states pass on the shared intrinsic-region binaries. Long labels preserve exact `NSLOCTEXT` identity/default restoration for the search hint and Name header, selected/query/note state, and separate narrow horizontal access; fresh `LongLocalizedLabels-Narrow.png`, `Loading.png`, `Error.png`, and `Dialog.png` were reviewed. This is not full culture/localization or browser parity. Browser-reference visual parity remains policy **BLOCKED** and unwaived |

## State coverage rule

Every gallery page must expose the state variants that can change behavior or
layout: empty, populated, large, loading/error where applicable; enabled,
disabled, readonly, invalid and validated for controls; focused, unfocused,
selected and unselected for navigable controls; accepted and rejected reload;
owner expiry; removal/reinsert; and narrow/wide or scale-sensitive geometry.
Controller and multi-user states belong on the relevant control/navigation pages,
not in a single aggregate screenshot.

## Proposed small-page delivery groups

The twelve rows above can be implemented as six reviewable resource pages:

1. **Layout and text lab** — rows 1–2.
2. **Controls and validation lab** — rows 3–4.
3. **Scroll, splitter and collection lab** — rows 5–6.
4. **Tree, repeat and tabs lab** — rows 7–8.
5. **Menus, dialog and ownership lab** — rows 9–10.
6. **Failure/lifetime and reference workbench** — rows 11–12.

Each page should have a stable authored root, explicit model bindings, state
buttons or selectors, and one production-path test named after the page. Existing
focused tests can be reused as assertions, but the page must remain inspectable
in the native workbench. The current six authored page keys are `layout`, `forms`,
`data`, `collections`, `commands`, and `composition`; their resource inventory is
validated separately by `Plugins/CkTests/Resources/CapabilityGallery/validate_gallery.py`.
The remaining blockers are browser-reference visual verification, complete
localized/control variants, controller and multi-user acceptance, and the wider
debugger/package/lifetime campaign.

## Template reuse boundary

Current reuse is document-local: `<template name="...">` declarations are
collected into the current document's template map and `<use template="...">`
references resolve against that map. `CkUiDocument.cpp` performs this during
`Parse` and `CompileNode`; the authoring guide calls the mechanism “document-local
authored templates.” No cross-resource import, include, or shared template
library mechanism is present in the current parser, so examples requiring reuse
across separate resource files remain outside this gallery's supported claim.
