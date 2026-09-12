# Gate 05: UI Debugger summary

**Status:** Bounded summary slice accepted. This does not accept Gate 05, the whole CkUIDebugger, or the campaign.

## Scope

Replace the native one-line layout summary with one installed retained CkSlateLayout region, `summary`, from `UiDebuggerSummary.ui.html/.css`. The resource presents either the existing no-layout message or three compact metrics: `ACTIVE`, `INPUT`, and `LAYERS`.

The visible layout branch binds scalar values `summary-active-tag`, `summary-input-mode`, and `summary-layer-count`, gated by `summary-has-active-layout`. The no-layout branch binds `summary-no-active-layout` and is gated by `summary-no-active-layout-visible`, rather than becoming an empty placeholder. C++ continues to own layout discovery, values, all commands, state transitions, reload policy, and lifetime admission.

## Visual intent

At wide sizes, the summary is a compact outlined card with three readable metric cells and a distinct active treatment. It uses only the existing row/column/text CSS contract: padding, gap, background, border, radius, bounded typography, letter spacing, wrapping, and color tokens. It intentionally uses styled text rather than a `debug-status` active pill because no additional foreground/background binding contract is required.

At narrow sizes, metric cells may wrap safely while preserving all three values. Browser-style breakpoint restacking is explicitly deferred.

## Owned files

- `Plugins/CkGameplayDebugger/Resources/UI/UiDebuggerSummary.ui.html`
- `Plugins/CkGameplayDebugger/Resources/UI/UiDebuggerSummary.ui.css`
- `Plugins/CkGameplayDebugger/Source/CkUIDebugger/Public/CkUIDebugger/Window/SCkUIDebuggerWindow.h/.cpp`
- `Plugins/CkGameplayDebugger/Source/CkUIDebugger/Private/Tests/CkUIDebugger_HistoryAuthored.spec.cpp`
- `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkUI/Test_UIDebugger_HistoryAuthoredPie.spec.cpp`
- this plan

## Accepted production-path evidence

The incremental build `Saved/Logs/Build-UiDebuggerSummary-R3.log` succeeded, SHA256 `3A61966C85CE2CBE97FB62233E02BF88F56E2B4BD0E0928FE094FCA1A1DF0BF5`.

The one-serial-cached-real-RHI gate `Saved/Logs/Test-UiDebuggerSummary-R4-Final.log` passed `Ck.UiAuthoring.UIDebugger.History` and `Ck.UIDebugger.History.PIE` 2/2 with exit 0 and zero failed, skipped, or contaminated, SHA256 `D3F7079740C8D8C9FAE68D080FD2F33BEFACBB6529EE592341976F98562A4097`. The final log has zero `LogAutomationController` errors, ensures, fatals, AngelScript errors, or script errors. It is not timing-clean: one non-failing `FCk_Latent_WaitForCondition` advanced after a 15-second timeout, which remains fixture timing debt.

Fresh inspected captures are `Saved/Automation/UIDebugger/HistoryPie-Wide.png`, SHA256 `8D40C364F0CD2E91F8171DBB26DC9689110EAC19C2A68E964BAB6A1C7801644A`, and `Saved/Automation/UIDebugger/HistoryPie-Narrow.png`, SHA256 `2499E3A35C9791FCC7439F9CF0135B99355B13EA3B0947F86FE94823DCBA55F6`. Wide shows distinct ACTIVE/INPUT/LAYERS cards. Narrow safely wraps and remains reachable; it does not establish browser breakpoint parity.

## Explicit exclusions

Native WindowChrome, all commands, layer search, active-only toggle, layer actions, name-depth control, refresh/global selectors, world-speed controls, layout-stack behavior, the layer tree, Event History, responsive browser restacking, browser parity, game/package, controller, accessibility, localization, performance, and campaign-wide lifetime acceptance remain outside this slice.
