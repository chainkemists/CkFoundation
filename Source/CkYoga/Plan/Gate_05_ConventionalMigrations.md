# Gate 05: Conventional debugger migrations

**Status:** Gate 04 and the bounded Intent InputHud, Aggro, AI roster, Dialog, Texture page/window-navigation, Save, Object Pool, Style Lab, CkUIDebugger, GOAP, and Visual LOD authored slices recorded in `PROGRESS.md` are verified at their stated boundaries. The current source census does not close the conventional workstream: GOAP/Texture/AI outer-shell scope, the GOAP Inspector gateway, Texture Checker Apply/Restore/PIE, broader consumers, and cross-environment acceptance remain open. Browser-reference parity remains blocked and unwaived.

## First slice: Intent Debugger InputHud controls

Migrate the fixed Slate control panel in `CkIntentDebugger` to an authored CkSlateLayout view while retaining the InputHud preview and the debugger window as native owners.  The existing `SCkIntentDebuggerWindow` is the window and popup parent; the migrated controls must remain mounted there and must not create a second standalone surface.

The slice retains all existing behavior:

| Area | Existing state and migration contract |
|---|---|
| Readout | `MetadataMode` and `FrameNotation` remain user settings. Their setters already sanitize, compare, save, and notify only on a real change. `Reset readout` calls `Reset_ReadoutTuning` only; it must not reset session or project state. |
| Session placement | Mode, scale, opacity, corner, offset X, and offset Y continue through their existing cvar callbacks. A missing console variable is a safe no-op/fallback, never a crash. The placement callback owns corner and offsets and persists them through user-placement settings; mode, scale, and opacity remain session cvars. |
| Project | History cap (3–20), fade seconds (3–30), tap/hold threshold (50–2000), and frame-number visibility remain project settings. SaveConfig and user-settings notification occur only after the effective normalized value differs. |

There are **seven** numeric editors: session scale, opacity, offset X, offset Y; project history cap, fade seconds, and tap/hold threshold. The authored number inputs must use their real committed callback and current clamp/digit contract: finite out-of-range drafts clamp, while malformed drafts do not publish a setting.

## Planned ownership

- `Plugins/CkGameplayDebugger/Source/CkIntentDebugger/Public/CkIntentDebugger/Window/SCkIntentDebugger_InputHudControls.h` and `Public/CkIntentDebugger/Window/SCkIntentDebugger_InputHudControls.cpp`: retained view, bindings, weak-owner actions, existing settings/cvar routing, and no-op admission checks.  Do not recreate preview or painted overlay behavior here.
- `Plugins/CkGameplayDebugger/Resources/UI/IntentInputHudControls.ui.html` and `.ui.css`: responsive authored sections, labels, help text, seven number inputs, cycle controls, frame toggle, and reset action.  Layout and presentation belong in these files; C++ continues to own settings, cvars, preview notification, and availability checks.
- `Plugins/CkGameplayDebugger/Source/CkIntentDebugger/CkIntentDebugger.Build.cs`: add the CkSlateLayout module dependency needed by the authored view.
- `Plugins/CkGameplayDebugger/Source/CkIntentDebugger/Private/Tests/CkIntentDebugger_InputHudControls.spec.cpp`: replace construction-only proof with native interaction coverage.  Amend `SCkIntentDebuggerWindow.cpp` only if the real window host needs an explicit retained region/scroll container; do not change its ownership model otherwise.

## Required acceptance evidence

1. Drive every cycle, boolean, reset, and all seven numeric editors through mounted native controls.  Verify the corresponding setting/cvar/preview notification result and each normalized range/digit behavior.
2. Snapshot and restore every touched user setting, project setting, and console variable.  Prove no-op commits do not save project settings or notify user settings; exercise unavailable cvar paths without a crash.
3. Verify narrow responsive reachability and a readable capture without requiring all controls to fit in one viewport.
4. Exercise a compatible installed-resource reload and a rejected candidate: accepted retained view state remains valid; rejection preserves the prior live view, controls, and callbacks.  Cover focus/owner expiry so held actions become inert after their owning pane/view is released.

## Approved editor and PIE acceptance split

- The existing `CkIntentDebugger_InputHudControls.spec.cpp` remains an editor/Slate fixture. R12 passed 121/121 full authoring tests in 58 seconds with no failed, skipped, or contaminated test. Root independently verified `IntentInputHud-R12-Authoring-Editor.log` has 121 successes and zero relevant diagnostics. The fixture validates the real unavailable-CVar fallback when no local player has registered the session variables, plus readout and project controls, native checkbox toggle, compatible/rejected reload, owner expiry, and narrow capture. Its scoped shared focus repair retains an uncommitted native draft during compatible reload without dismissing the owning popup; normal Enter and ordinary focus-loss commits remain separate live behaviors. If another fixture has already registered the process-global Cvars, it reports that the unavailable scenario is unavailable; it does not unregister or fabricate console variables.
- The CkTests PIE fixture owns active session-CVar routing. It starts one-client PIE on `/Engine/Maps/Entry`, obtains the real local player and its `UCk_InputHud_Subsystem`, then drives mounted mode, corner, scale, opacity, and offsets. It snapshots and restores CVar values and provenance flags together with user placement state before PIE teardown. Manual subsystem initialization and synthetic CVar registration are excluded.
- Project no-op evidence is the unchanged effective setting and `UCk_InputHud_UserSettings` revision after an accepted canonical native commit, supported by source review of the compare-before-`SaveConfig` path. There is no public `SaveConfig` call counter. A checkbox has a native toggle gesture, but no honest native no-op gesture; coverage must not claim one.

The real PIE test at `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkIntent/Test_IntentInputHud_ControlsPie.spec.cpp` passed 1/1 in 31 seconds with exit 0 and no failed, skipped, or contaminated test, using one actual editor. Root independently verified `IntentInputHud-PIE-Editor.log` has one success, zero relevant diagnostics, and the EndPIE marker. It validates active session-CVar routing through the real one-client local-player subsystem; its editor guard, module dependencies, reference scope, and unconditional snapshot cleanup are in place.

## Deferred work

The painted InputHud overlay, ribbon/readout rendering, and other specialized overlay adapters remain a separate migration.  Other conventional debugger consumers are separate slices after this one; this plan does not waive their settings, lifecycle, native-input, or browser-parity obligations.

## Source basis

- `Plugins/CkGameplayDebugger/Source/CkIntentDebugger/Public/CkIntentDebugger/Window/SCkIntentDebugger_InputHudControls.cpp`: current readout/session/project controls and all seven numeric ranges.
- `Plugins/CkGameplayDebugger/Source/CkInputHudOverlay/Public/CkInputHudOverlay/Settings/CkInputHud_Settings.h`, `Settings/CkInputHud_UserSettings.cpp`, and `Subsystem/CkInputHud_Subsystem.cpp`: settings scopes, normalized setters, persistence, and cvar availability/placement routing.
- `Plugins/CkGameplayDebugger/Source/CkIntentDebugger/Public/CkIntentDebugger/Window/SCkIntentDebuggerWindow.cpp`: native debugger-window mount ownership.
- `Gate_04_RepresentativeMigration.md`: Style Lab is the required shared-control proving ground; InputHud’s native shell is not its completion boundary.
