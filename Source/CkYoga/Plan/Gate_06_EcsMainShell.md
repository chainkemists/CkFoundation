# Gate 06: ECS main debugger shell

## Accepted boundary

`SCkDebuggerWindow_Main` now authors its stable three-pane outer splitter and its center tab/body composition from `EcsDebuggerShell.ui.html/.css` and `EcsDebuggerCenter.ui.html/.css`. The two independently retained views keep native bindings disjoint: the outer view owns entity, shared `SCkDebug_PaneHost` center and inspector placement, while the nested center view owns the page-tab and page-body ports. C++ remains authoritative for pages, entity and inspector panels, selection, world/session lifecycle, refresh gating, picker behavior and native startup fallback.

The split is load-bearing. Binding the center pane and its tab/body descendants in one view violates native-binding hierarchy ownership on reload. Independent views preserve the shared PaneHost styling and reload atomically per authored boundary without reconstructing page implementations or their delegates.

## Evidence

The incremental Development Editor build succeeded and fresh real-RHI `Ck.EcsDebugger.AuthoredWindow` passed 1/1 with zero failed, skipped or contaminated tests in `scratch/yoga-ecs-main-shell-final6-20260914.log` (SHA256 `87CB76C877CDA8EEEBA8737CB8D44D7FE206DCE76E7A414C81217F9A6F2ABD9D`). The production window fixture proves:

- authored admission under D3D12 and a three-pane outer splitter with the exact native pane roots mounted;
- exact tab and page-body port mounting inside the nested authored center;
- physical Graph-tab selection replaces only the page body while retaining the tab strip and body container;
- compatible outer and center reloads retain both views, regions, splitter, native panes, selected page body and native port identities;
- missing-port candidates are rejected independently and leave each committed boundary unchanged; and
- window teardown releases both authored views.

The final whole-log scan found no relevant ensure, fatal, authored-resource, CSS/parser, AngelScript or automation failure. Existing missing-development-asset warnings, console-variable performance warnings and a Chromium USB diagnostic remain unrelated startup noise.

## Remaining acceptance

This accepts the ECS main stable-layout row, not the campaign. Production module-tab closure, world/session reset ordering under a live PIE transition, retained native overlay/filter callback lifetime, actual narrow-width scrolling, the all-debugger teardown matrix, ownership gates and representative performance remain open under the recorded CTO dispositions.

The outer-shell census is now 13/27 complete. Fourteen rows remain: AStar, Audio, Crowd, Launcher/Suite, Entity Debug Overlay, Input HUD Overlay, Insights, Intent, Jolt Bake, Jolt, Map, Optimization, Save and SM debugger.

## AStar partial checkpoint

The AStar production window now authors its stable two-splitter grid/stats/history placement and mounts the exact painted grid through a retained `<astar-grid>` adapter. Incremental Development build plus fresh D3D12 `Ck.AStarDebugger.AuthoredShell` passed 1/1 in `scratch/yoga-astar-shell-grid-final8-20260914.log` (SHA256 `C3FA9FC8F4E8DD7E0E4C0266910BC4623DA9C7D295DD3EAF6F66A50BAF8765B6`), covering physical selection, compatible/rejected reload, retained identity, capture-preserving pan continuation, view release and held-subtree input revocation.

This does not advance the census: AStar stats/history presentation remains native, and their equal-count/same-cell refresh plus actual narrow-width scrolling remain open.
