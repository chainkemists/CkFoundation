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

The outer-shell census is now 14/27 complete. Thirteen rows remain: Audio, Crowd, Launcher/Suite, Entity Debug Overlay, Input HUD Overlay, Insights, Intent, Jolt Bake, Jolt, Map, Optimization, Save and SM debugger.

## AStar outer-shell acceptance

The AStar production window authors its stable two-splitter grid/stats/history placement, mounts the exact painted grid through a retained `<astar-grid>` adapter and independently authors Stats and SearchHistory presentation. Incremental Development build plus fresh D3D12 `Ck.AStarDebugger.AuthoredShell` passed 1/1 in `scratch/yoga-astar-shell-complete-final4-20260914.log` (SHA256 `0709A065ED82ECBB1ACD3113544DAC5D3E4D71D7D4D036A283BF9543CEA7772F`), covering physical grid selection and copy actions, same-cell stats refresh, equal-count history replacement, compatible/rejected reload, capture-preserving pan continuation, actual narrow overflow, view release and post-reload held-action/input revocation.

This advances the census to 14/27. Broader production-host acceptance remains outside this focused row, and the known collector ordering defect can still record zero grid-derived path/cost because completion is tracked before the test-grid overlay is applied.

## Audio outer-shell and runtime-lifecycle checkpoints

`SCkAudioDebuggerWindow` now mounts its stable tabs/stats/filter/page-body stack through retained `AudioDebuggerShell.ui.html/.css` while keeping the exact six-page native switcher and shared WindowChrome. Incremental Development build `scratch/yoga-audio-shell-final-build-20260914.log` succeeded (SHA256 `F32B4A9B04008368703CA1C02106349D9505C574343C32EC3D533F0ED5DC6BCF`); fresh serial D3D12 `Ck.AudioDebugger.AuthoredShell` passed 1/1 in `scratch/yoga-audio-shell-final-test-20260914.log` (SHA256 `3B1436DA86FF59A79E42EFD8D9210B32AFC0149CF55A513C039B11E714641C69`). The fixture proves physical non-default page selection, compatible/rejected reload, narrow/short reachability, release and invalid-startup fallback recovery.

A later incremental Development build and fresh serial D3D12 `Ck.AudioDebugger.AuthoredShell` passed 1/1 in `scratch/yoga-audio-lifecycle-test-realrhi-final-20260914.log` (SHA256 `21BC3E0EB1DB7B484362B5ED951FF3DBFC3CEE2474763F1FEEAFEAD9211C055A`). The production fixture proves identity-keyed same-name replacement, empty/director active counts, actual physical Overlay mutation through real ECS handles, stale held-control revocation, independent world/session invalidation and module pre-exit ownership release. The preceding NullRHI physical/layout cluster failure is not acceptance evidence.

The next slice moves all four live summary cards into `AudioDebuggerShell.ui.html/.css`. The superseding incremental Development build and fresh serial D3D12 `Ck.AudioDebugger.AuthoredShell` passed 1/1 in `scratch/yoga-audio-authored-stats-style-final5-20260914.log` (SHA256 `CEE3DBDE770FAB740E030B5E5633AAB70AAA0DF70EC09AE7DA6AE205B9630274`), proving authored labels, native-subtree exclusion, `0 / 0` to `1 / 4` projection, compatible/rejected reload, bounded native-fallback recovery and live Style Lab font/card geometry parity.

The dedicated Crossfade page then moved its separator, title, subtitle, exact plot placement and live legend into `AudioDebuggerCrossfade.ui.html/.css`, retaining the production dual-series `SCkDebug_Sparkline` through one native binding while leaving the compact Tracks-page lane unchanged. Incremental Development build plus fresh D3D12/SM6 `Ck.AudioDebugger.AuthoredShell` passed 1/1 in `scratch/yoga-audio-crossfade-final2-20260914.log` (SHA256 `8CCFB22BA16258D2972B133F2E152F51183A7A03F8FAA710DF1A08F3CE050549`), covering physical selection, exact shared series/plot identity, two production collector samples, compatible/rejected reload, independent fallback recovery, style revision and teardown.

The Spatial attenuation block then moved its heading and six live arithmetic/detail rows into `AudioDebuggerAttenuation.ui.html/.css`, retaining the exact production `SCkAudioDebugger_FalloffCurve` through one native binding and one shared in-place spatial model. Incremental Development build plus fresh D3D12/SM6 `Ck.AudioDebugger.AuthoredShell` passed 1/1 in `scratch/yoga-audio-attenuation-final2-20260914.log` (SHA256 `82ABA2F56E8F2ED03EBBFB722ADC47E80BEA17B9F63F7811A9D0798F3BD0F19E`), covering physical Spatial and track selection, exact live arithmetic, compatible/rejected reload, independent fallback recovery, style revision, 2D/removal/invalidation clearing and teardown.

The Events toolbar then moved its stable four-control placement and sampling caveat into `AudioDebuggerEventsToolbar.ui.html/.css`, retaining the exact four production `SCkDebug_ToggleSurface` instances through native ports while leaving the exact native `SCkDebug_EventLog` as a sibling. Incremental Development build plus fresh D3D12/SM6 `Ck.AudioDebugger.AuthoredShell` passed 1/1 in `scratch/yoga-audio-events-toolbar-final3-20260914.log` (SHA256 `585165FB33DF1343A456CF01E845338958506160C047225F756EB8E45A635525`), covering physical Events selection and all four preference inputs without a world, future State-event suppression/enabling and exact rendered history, compatible/rejected reload, independent fallback recovery, populated-history retention, invalidation, style revision and teardown. Recording flags affect future appends and do not filter retained history.

The main shell filter row then moved its ordinary search/toggle placement into `AudioDebuggerShell.ui.html/.css`, retaining the exact production `SCkDebug_SearchBar` and four filter toggle surfaces through native ports. Incremental Development build plus fresh D3D12/SM6 `Ck.AudioDebugger.AuthoredShell` passed 1/1 in `scratch/yoga-audio-filters-final1-20260914.log` (SHA256 `8B0459283CDD8E9B586EA5898AD2D4302215B62BD38AA09B7C17702D7C08D931`), covering physical text and toggle input, compatible state-preserving reload, atomic rejection for every missing port, native startup fallback behavior, detach-before-recovery transfer and weak-owner teardown. The specialized tab widget remains native pending its own adapter/design slice.

This is not Audio acceptance and does not change the 14/27 census. Tabs plus Directors, Tracks, the rest of Spatial, full Events log/control presentation if required and Overlay remain incomplete, Radar still needs a retained adapter, and the broader production-host every-debugger teardown matrix remains open.
