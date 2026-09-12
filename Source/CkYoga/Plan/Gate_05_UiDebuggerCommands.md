# Gate 05: UI Debugger command shell

**Status:** Accepted bounded command-shell slice (2026-09-11). Browser breakpoint parity and whole Gate 05/campaign acceptance remain open.

## Accepted checkpoint (R27-R28)

R27 incremental build `Saved/Logs/Build-UiDebuggerCommands-R27.log` succeeded, SHA256 `2F74C65F8EA1C75144BFD88BE0C47D44F0CCB97B92A757C26BDD92F01FED4628`. The final cached real-RHI R28 gate `Saved/Logs/Test-UiDebuggerCommands-R28-Final.log` passed `Ck.UiAuthoring.UIDebugger.History` and `Ck.UIDebugger.History.PIE` 2/2 in 33 seconds, zero failed/skipped/contaminated, SHA256 `B970725E5076B0D52008A9387F820BA222C360ABB48DE03DBA85BD9A63ADB2DD`. Runtime archive `Saved/Logs/UiDebuggerCommands-R28-Editor.log`, SHA256 `E2E9F4AD9ACB2693F815FD03D8E6ADD1E48C84EAD89410D17F011514FC937C40`, contains zero latent timeout lines and no relevant automation, ensure, fatal, AngelScript, parser, or script diagnostics; the only noted line is unrelated Chromium USB.

Mounted physical Clear History recorded `enabled=1 revision=2 input-suspensions=0`, valid `SCkUiStyledButton` capture, `ui-clear-history` tag, and a target-containing pointer path. The previously deferred post-transition predicate now passes with `[Push]` History, one primary-layer child, three mounted visible tree nodes, and matching repeat/History counts. Fresh inspected captures: wide 1206x766 SHA256 `CA8DEACDE0F28B357214D032864FA7A5BED6C7A831EB2DCDB17761497B5084A0`; narrow 426x426 SHA256 `AB281184091EBF04EFCDF156A382578EFC16E521EC2D2329E44B41FA2D1771F7`. They show readable hierarchy and narrow command reachability without overlap.

## Historical checkpoint (R8-R23)

The authored `commands` resources and their production integration are complete. The R8 cached two-test gate, `Saved/Logs/Test-UiDebuggerCommands-R8-Final.log` (SHA256 `234974538C972F2E87BDD839A4FE99B2E515126CC31C47F964669DE8984743CA`), was 1/2: the authoring coverage passed, while the PIE coverage retained Clear History reconciliation and the 101-push retention-cap failures. R11 remained 1/2 with those aggregate Clear/Cap failures.

The fixture was then split so physical target/down/up routing is observed separately from reconciliation, and its cap observation now uses `TickUntil`. Build R12 is green: `Saved/Logs/Build-UiDebuggerCommands-R12.log`, SHA256 `1EF269B47373F39CF66F407D6EB6B288141A1F80AA2CE6471658A8DEC78AC264`.

R13 used one cached start and remained 1/2: `Saved/Logs/Test-UiDebuggerCommands-R13-Final.log`, SHA256 `F4A4B5099B30B300D34AA42F1DCDF85C9134410159CF2351ECFA07E2B7DC334B`. The cap now passes. The precise remaining symptom is Clear History target-path true, pointer-down handled, pointer-up unhandled, and action not fired; the run has one 15-second timeout. A production or resource defect is not established.

Fresh captures at 2026-09-11 00:19:22 are visually acceptable for readable hierarchy and narrow command reachability, with unchanged hashes: `Saved/Automation/UIDebugger/HistoryPie-Wide.png` `FB333A119DE682C7460DBF0A65B691B4318A4B3E47BC58BD94866AFA9D2135A1` and `HistoryPie-Narrow.png` `AB281184091EBF04EFCDF156A382578EFC16E521EC2D2329E44B41FA2D1771F7`. They are not final acceptance evidence.

R15's cached two-test diagnostic remained 1/2 with one 15-second timeout: the exact Clear button had no capture after app-level down, while the pre-up hit path still contained the target. R17 added actual cursor-user captor evidence: `CkPlugins.log` reported `valid=0/type=<none>/tag=None/path-contains-target=0`; it was again 1/2 with one timeout. No production or authored-resource defect is established.

R18's build failure was diagnostic-only misuse of private `FCkUiView::CanDispatchEvents`; it was corrected and is not a product failure. R19 then built successfully with public observations instead: Clear button `IsEnabled` before down and the command-view revision. Evidence: `Saved/Logs/Build-UiDebuggerCommands-R19.log`, SHA256 `9FD778967DC7903E3F735C0F7543CD5026048CE0D3F4F8DD5F9FFB5030553E78`.

R20 was 1/2 with `enabled=1` and revision `2`, but no captor. R21 built successfully, SHA256 `8775C7B281534DACF94662D2A7E6641D2EA2E8422333CD6D1BD2B3433E3A6E05`.

R22 functionally passed 2/2: the exact input record was `enabled=1 revision=2 input-suspensions=0 captor SCkUiStyledButton tag ui-clear-history path contains target`. Its log SHA256 is `F57C341777CC62C33571766BB5C23552B0A5302D01B8AFCDC635C4586D5306BC`. The wide/narrow capture hashes are unchanged: `FB333A119DE682C7460DBF0A65B691B4318A4B3E47BC58BD94866AFA9D2135A1` / `AB281184091EBF04EFCDF156A382578EFC16E521EC2D2329E44B41FA2D1771F7`. One anonymous 15-second wait timeout remains, so R22 is not accepted.

R23 incremental build succeeded and compiled named wait-start diagnostics: SHA256 `EE2242BDB46C2F78E56F0019EB7FD3E560BF6145410C71A027F7CFE628665A0C`; the log reports `Result: Succeeded`, build success, and no matched errors, ensures, or script errors. The next gate is one cached `UIDebugger.History` run to identify the exact wait across both focused tests, followed by the minimal correction and final zero-timeout acceptance gate. The migration remains unaccepted until a clean 2/2 run with zero timeout/error diagnostics and fresh wide/narrow capture review. No acceptance is claimed.

## Scope

Replace the three UI-specific native `SCkDebug_WindowChrome` command groups with one installed retained CkSlateLayout region named `commands`, loaded from `UiDebuggerCommands.ui.html/.css`. The authored region owns the layer filter, filter clear, active-layer-only toggle, force refresh, expand all, collapse all, clear history, and name-depth presentation.

Native C++ continues to own all state, projection, command effects, refresh gating, layout discovery, event binding, file polling, and lifetime admission. The existing authored Summary, Layers, and History regions remain separate retained views.

## Decisions

`[G5-UI-COMMANDS-D1]` Retain `SCkDebug_WindowChrome` and its shared refresh-policy/rate and authority-world-speed controls. Those are suite-level host controls, not UI Debugger feature commands. Remove only the `LayerView`, `LayerFilter`, and `LayerActions` command groups from the Chrome configuration.

`[G5-UI-COMMANDS-D2]` Do not add a world or layout selector. The production window currently discovers the first Game/PIE local-player layout implicitly; a selector would introduce new target identity, rebinding, persistence, and multi-local-player semantics rather than migrate existing behavior.

`[G5-UI-COMMANDS-D3]` Author name depth from ordinary buttons plus a live text binding, preserving `SCkDebug_NameDepthCycler`'s exact Full(0) / 1..Max cycle. Do not retain it through a `<native>` port and do not create a one-consumer adapter.

`[G5-UI-COMMANDS-D4]` Use visible labelled action buttons rather than hiding the core commands in a popup menu. The strip may wrap at narrow widths. This preserves direct reachability, reduces menu-session risk, and improves the current clipped native command-bar behavior without claiming general browser breakpoint parity.

## Owned files

- `Plugins/CkGameplayDebugger/Resources/UI/UiDebuggerCommands.ui.html`
- `Plugins/CkGameplayDebugger/Resources/UI/UiDebuggerCommands.ui.css`
- `Plugins/CkGameplayDebugger/Source/CkUIDebugger/CkUIDebugger.Build.cs`
- `Plugins/CkGameplayDebugger/Source/CkUIDebugger/Public/CkUIDebugger/Window/SCkUIDebuggerWindow.h/.cpp`
- `Plugins/CkGameplayDebugger/Source/CkUIDebugger/Private/Tests/CkUIDebugger_HistoryAuthored.spec.cpp`
- `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkUI/Test_UIDebugger_HistoryAuthoredPie.spec.cpp`
- this plan, `PROGRESS.md`, `COVERAGE.md`, and the continuation prompt

## Verification contract

1. Installed resources load through the real CkDebugger plugin path and mount inside the real `SCkUIDebuggerWindow` while native WindowChrome remains the owner.
2. Mounted production input drives search, filter clear, active-only, force refresh, expand all, collapse all, clear history, and both name-depth directions with command-specific state observations.
3. Compatible reload preserves retained search identity/draft and advances the accepted revision. Rejected reload preserves the accepted view, controls, state, and revision.
4. Owner release makes externally held authored controls inert and releases the command view before the other retained views.
5. The final one-start cached real-RHI gate reruns `Ck.UiAuthoring.UIDebugger.History` and `Ck.UIDebugger.History.PIE` on the final binary, stays 2/2 relative to the recorded empty failing set, and has zero latent timeout lines.
6. Fresh wide/narrow captures visibly show readable hierarchy and narrow reachability without overlap. Browser breakpoint parity remains deferred.

## Baseline

Entry snapshot: `scratch/baseline_html_slate_ui_debugger_commands_20260910-222423.md`. The immediately preceding final gate is 2/2 with an empty failing set, but contains one non-failing 15-second latent timeout; eliminating that debt is part of this slice.

## Explicit exclusions

World/layout selection, the shared WindowChrome refresh/world-speed implementation, browser breakpoint parity, exact browser parity, game/package, controller, accessibility, localization, performance, broad multi-user behavior, and whole-campaign acceptance remain outside this slice.
