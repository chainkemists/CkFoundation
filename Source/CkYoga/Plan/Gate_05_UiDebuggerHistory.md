# Gate 05: UI Debugger Event History

**Status:** Focused production PIE and layer-action lifetime acceptance complete. The installed authored card, live layout-event publication, ordering, 100-event cap, Clear History behavior, retained refresh, teardown, non-empty wide/narrow geometry, and owner-release safety of the four native layer-action buttons are verified through a fully source-authored fixture. This does not complete the rest of CkUIDebugger, Gate 05, or the campaign.

## Scope

Replace only the native Event History card/list in `SCkUIDebuggerWindow` with one retained CkSlateLayout view loaded from installed CkGameplayDebugger resources. Keep `SCkDebug_WindowChrome`, the command bar, summary, layer list, layout discovery, and all layer/widget rendering native.

The authored document owns the history card presentation, collapsed/expanded body, empty state, vertically repeated event rows, wrapping, maximum visible height, and wide/narrow styling. C++ continues to own layout delegate subscription, event descriptions, the 100-event cap, ordering, refresh scheduling, and teardown.

## Data and lifetime contract

- Project history events into stable keyed records. Keys are monotonic within the window generation and are never derived from array indices.
- Preserve newest-first ordering and the existing 100-record cap.
- Do not clear or recreate the retained view during ordinary refreshes. Publish collection changes through the existing model binding.
- A compatible resource reload preserves the view, collection identity, and expanded state. A rejected reload preserves the accepted document and current history.
- Invalidate dispatch/lifetime admission before detaching the view. Held History widgets and the Force Refresh, Expand All, Collapse All, and Clear History callbacks must become inert after owner release.
- Empty history presents `No events yet.` through the authored document. Clearing history remains the existing command-bar action and must not affect layer state.

## Owned files

- `Plugins/CkGameplayDebugger/Source/CkUIDebugger/Public/CkUIDebugger/Window/SCkUIDebuggerWindow.h/.cpp`
- `Plugins/CkGameplayDebugger/Source/CkUIDebugger/CkUIDebugger.Build.cs`
- `Plugins/CkGameplayDebugger/Resources/UI/UiDebuggerHistory.ui.html/.css`
- `Plugins/CkGameplayDebugger/Source/CkUIDebugger/Private/Tests/CkUIDebugger_HistoryAuthored.spec.cpp`
- `Plugins/CkTests/Script/CkUI/CkUIDebugger_HistoryPie_Assets.as`
- `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkUI/Test_UIDebugger_HistoryAuthoredPie.spec.cpp`
- `Plugins/CkFoundation/Source/CkUI/Public/CkUI/Layout/CkUI_PrimaryGameLayout.cpp`

No CkFoundation capability extension is expected. If the existing repeat/collection/visibility/expanded-state contracts are insufficient, stop and record the shared gap rather than adding a CkUIDebugger-only workaround.

## Acceptance

1. Mount the real `SCkUIDebuggerWindow` history view and verify the installed resource loads through `FCkDebug_UiRegistry` and `FCkUiView`.
2. Exercise the real layout-event publication path where feasible; prove empty state, at least two ordered descriptions, and the 100-event cap without injecting a fake partial view state.
3. Verify the existing Clear History command empties the authored collection without mutating the layer list.
4. Verify compatible and rejected reload behavior, stable native/view identity, expansion retention, and held-owner release.
5. Capture and inspect wide and narrow mounted geometry. Narrow reachability is required; breakpoint restacking is not.
6. Run one incremental build/focused real-Slate gate. Add a PIE start only if the real layout delegate path cannot be established honestly in an editor fixture; state that extra evidence before launching it.

## Accepted evidence and remaining boundary

AngelScript supplies the gameplay-tag asset, layout-config data-asset literal, and concrete CommonUI widget class, so no manual editor asset is a prerequisite. Incremental build `Saved/Logs/Build-UiDebuggerHistory-Authored-R5.log` succeeded. The original cached one-start gate `Saved/Logs/BuildTest-UiDebuggerHistory-Authored-R6-Final.log` passed `Ck.UIDebugger.History.PIE` 1/1 with zero failed, skipped, or contaminated tests in 32 seconds. The lifetime closeout build `Saved/Logs/Build-UiDebuggerHistory-Lifetime-R2.log` succeeded, and the one-start combined final gate `Saved/Logs/BuildTest-UiDebuggerHistory-Lifetime-R3-Final.log` passed `Ck.UiAuthoring.UIDebugger.History` and `Ck.UIDebugger.History.PIE` 2/2 with zero failed, skipped, or contaminated tests in 31 seconds. Its archived runtime log is `Saved/Logs/CkPlugins-UiDebuggerHistory-Lifetime-R3-Final.log`. Fresh inspected captures are `Saved/Automation/UIDebugger/HistoryPie-Wide.png` and `HistoryPie-Narrow.png`.

The focused fixture accepts production local-player layout create/push/pop/clear, stable record/item identity across compatible refresh, exactly one `ClearLayer` event after removal of the duplicate broadcast, mounted Clear History pointer down/up with collection/repeat empty reconciliation, the newest consecutive 100 records after 101 pushes, and teardown before `EndPIE`. Source inspection verifies weak-owner gates for all four native layer-action callbacks. The fixture retains those buttons across owner release and observes panel expiry plus an unchanged externally held non-empty History collection after activation. Other native callbacks and the rest of the native CkUIDebugger surface remain outside this fixture.

## Explicit exclusions

Whole-window CkUIDebugger migration, callbacks beyond the four named layer actions, browser parity, the deferred narrow breakpoint/restacking track, navigation `+`, game/package, real controller sessions, accessibility, localization, performance, and campaign-wide lifetime acceptance remain open. The next bounded migration boundary is layer-list authoring.
