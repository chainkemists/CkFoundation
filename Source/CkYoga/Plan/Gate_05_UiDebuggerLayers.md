# Gate 05: UI Debugger layer list

**Status:** Bounded implementation accepted. This is not acceptance of the full Gate 05 or campaign.

## Scope

Move the native layer hierarchy in `SCkUIDebuggerWindow` to an installed authored view without changing the production layout model or its command bar. Preserve priority ordering, layer and widget state presentation, the existing 16-widget display cap, fuzzy layer-tag filtering, active-layer-only filtering, whole-layer expansion, Expand All, Collapse All, compatible/rejected reload, and owner-release behavior.

Window chrome, summary, search and toggles, name-depth control, refresh controls, layout discovery, layout delegates, and Event History remain in their current ownership.

## Shared tree extension

Extend the existing retained tree rather than flattening hierarchy into a repeat or creating a second hierarchy system:

- `<tree selectable="false">` selects native `ESelectionMode::None`, exposes no selected key or selection callback, and keeps the current selectable default unchanged.
- `<tree projection-field="visible">` names a required Bool field. Projection includes only true nodes whose ancestors are also true, without removing collection nodes, auto-expanding paths, or modifying the user expansion set. It is mutually exclusive with `filter-bind`.
- `<tree expand-on-row-click="true">` toggles only parent nodes on an unmodified left click. The existing native expander remains authoritative when it handles the event first; child rows and modified/right clicks retain existing behavior.
- Compatible reload preserves the retained tree, native tree, node identities, expansion set, projection, and scroll. Rejected declarations preserve the accepted configuration.

The fixed 24 Slate-unit row contract is accepted only if fresh wide/narrow captures show distinct, readable layer headers and widget rows without clipping. A failed visual comparison requires a shared per-node height design; it must not be hidden with a flattened repeat.

## Production projection

Create a separate `UiDebuggerLayers.ui.html/.css` view backed by one `FCkUiTreeCollection`:

- Layer keys use the full gameplay-tag identity, not shortened display text.
- Widget child keys combine their parent layer identity with stable live UObject identity, never display name or list index.
- Every node remains in the collection while filtering. C++ publishes the Bool projection field from the existing `ck::fuzzy::Match` tag predicate intersected with active-only state, with the same value on a visible layer and its displayed children.
- One read-only authored row template uses Bool fields to render distinct layer-header and widget-row branches. C++ supplies snapshot fields, colors, tooltips, active state, priority, input mode, and count.
- Ordinary refresh republishes same-key nodes in place. Structural changes add/remove keys atomically. Expand All and Collapse All call `TrySetExpanded` for layer roots.
- The layer view and data callbacks use weak owner admission and are released before the panel. Resource failure remains visible in the native host.

## Owned files

Shared capability and coverage:

- `Plugins/CkFoundation/Source/CkSlateLayout/Public/CkSlateLayout/CkUiDocument.h`
- `Plugins/CkFoundation/Source/CkSlateLayout/Private/CkUiDocument.cpp`
- `Plugins/CkFoundation/Source/CkSlateLayout/Public/CkSlateLayout/SCkUiTree.h`
- `Plugins/CkFoundation/Source/CkSlateLayout/Private/SCkUiTree.cpp`
- `Plugins/CkFoundation/Source/CkSlateLayout/AUTHORING.md`
- `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkYoga/Test_UiTreeView.cpp`

Consumer and production fixture:

- `Plugins/CkGameplayDebugger/Source/CkUIDebugger/Public/CkUIDebugger/Window/SCkUIDebuggerWindow.h/.cpp`
- `Plugins/CkGameplayDebugger/Resources/UI/UiDebuggerLayers.ui.html/.css`
- `Plugins/CkGameplayDebugger/Source/CkUIDebugger/Private/Tests/CkUIDebugger_HistoryAuthored.spec.cpp`
- `Plugins/CkTests/Script/CkUI/CkUIDebugger_HistoryPie_Assets.as`
- `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkUI/Test_UIDebugger_HistoryAuthoredPie.spec.cpp`

Campaign state:

- `Plugins/CkFoundation/Source/CkYoga/PROGRESS.md`
- `Plugins/CkFoundation/Source/CkYoga/COVERAGE.md`
- `Plugins/CkFoundation/Source/CkYoga/CONTINUATION_PROMPT_HtmlSlateWorkbench.md`
- this plan

## Acceptance

1. Shared parser/runtime tests reject invalid projection declarations atomically and verify all three opt-in tree behaviors while preserving every existing default.
2. Projection-only node updates retain node pointers, native rows where realized, user expansion, and tree/native-tree identity. Hidden subtrees do not auto-expand; clearing projection restores the prior expansion state.
3. Non-selectable trees remain unselected under mouse and keyboard input and never call selection delegates. Parent row clicks toggle exactly once; child, modified, and right clicks do not accidentally toggle.
4. The real source-authored PIE fixture creates at least two production layers and pushes real widgets. Stable layer/child identities survive ordinary event refresh, fuzzy filter hide/restore, active-only hide/restore, and compatible reload; rejected reload preserves the accepted tree.
5. Expand All, Collapse All, header toggle, status colors/pills, the 16-widget cap, and name-depth refresh remain observable. A query matching only a widget class must not match its layer; a fuzzy layer-tag query must show the layer and its children.
6. Held layer view/tree/row state becomes inert after debugger owner release. The fixture tears down before `EndPIE`.
7. Inspect fresh populated wide/narrow captures. Narrow reachability is required; browser breakpoint restacking remains deferred.

## Accepted evidence

Entry baseline `Saved/Logs/BuildTest-UiTreeProjection-Baseline-R1.log` passed `Ck.UiAuthoring` 136/136, zero failed/skipped/contaminated, in 50 seconds, SHA256 `391C9DEC605789C82235843343CCCDE47BA011C3B734CC7139AD340F87AFA002`. The named failing set is empty; archived runtime SHA256 is `0BFC30CE5002A85C09A2D8AF2135BDD13110A15BE33F4E6378332AA2CAD1EA76`. Snapshot: `scratch/baseline_html_slate_ui_debugger_layers_20260910-120509.md`.

Incremental build `Saved/Logs/Build-UiDebuggerLayers-R2-Diagnostics.log` succeeded, SHA256 `70C8C047B7268A8B0854AF0BEF7C8AF9169BC7399C480EEB675D602D1A22D34C`. Broad R2 established 135/136 `Ck.UiAuthoring` plus the new tree runtime and validation coverage green; it also contained unrelated Snapshot failures and a layer class omission, so it is diagnostic evidence rather than an acceptance gate. Diagnostic R3 isolated the authored resource problem to unknown class `ui-layer-row-content`.

The final focused cached gate `Saved/Logs/BuildTest-UiDebuggerLayers-R4-Final.log` passed `Ck.UiAuthoring.UIDebugger.History` and `Ck.UIDebugger.History.PIE` 2/2, SHA256 `7020B25AB933569F11262DA19FD74D0239C679C18569CAD2AF7EAE059C69EE09`.

The production fixture uses AngelScript-authored two layers and a real widget. It covers the 16-widget display cap, exact projection and native actions, compatible/rejected reload, and owner release. Fresh captures at 2026-09-10 12:36:08 local were inspected: `Saved/Automation/UIDebugger/HistoryPie-Wide.png`, SHA256 `1B2C709B5A724090CF46724DD3267A55DFAC4C6CEB4B8113F38ACD0920D4C7E4`, and `Saved/Automation/UIDebugger/HistoryPie-Narrow.png`, SHA256 `8B3501F723BB490AC76640F174C5335F7EBF79F8F7B22940D83FCA4062D82C0C`.

Wide acceptance is limited to two layer headers with priority/input/count, expanded widget children, Active/Inactive pills, and scrolling. Narrow preserves hierarchy and a reachable scrollbar; the native command bar compresses/clips horizontally. Browser-style restacking of that bar remains outside this migration scope.

## Explicit exclusions

This slice does not migrate the whole CkUIDebugger command bar or summary, alter layout-stack behavior, implement browser breakpoint restacking, or establish game/package, real-controller, accessibility, localization, performance, campaign-wide lifetime, Gate 05, or whole-campaign acceptance.
