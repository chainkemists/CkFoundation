# Gate 04: representative Style Lab ControlsPane migration

Status: Gate 04 native focused evidence complete, 2026-09-08. InputHud R8 native authoring built and passed 120/120 in 53s with two editor starts; the root-verified editor log has 120 successes and zero Error, ensure, fatal, AngelScript warning/error, or MissingResource diagnostics. The existing construction gate then passed 9/9 in 33s with one editor start; PID 36860 is terminal and `StyleLabInputHud-R8-Construction-Editor.log` has nine successes and zero relevant diagnostics across five legacy Ck.StyleLab and four authoring cases. Browser-reference parity remains blocked and unwaived. This completes Gate 04 native focused evidence only; the full campaign and wider representative migration remain open.

## Purpose and source boundary

[SCkStyleLab_ControlsPane.cpp](../../../../CkGameplayDebugger/Source/CkStyleLabDebugger/Public/CkStyleLabDebugger/Widgets/SCkStyleLab_ControlsPane.cpp) now loads [StyleLabControls.ui.html](../../../../CkGameplayDebugger/Resources/UI/StyleLabControls.ui.html) for all 24 reflected generic axes in six groups, seven retained sample previews, and the authored InputHud document. The existing profile resource remains independent. The verified generic-axis, native InputHud authoring, and construction evidence completes the Gate 04 native focused scope; it does not close the wider ControlsPane migration campaign.

The target covers all 24 generic axes across all metadata groups and all seven previews, plus the InputHud group: six cycles, 17 numeric settings, five custom-color settings, and visual reset. One axis group, cycles without numeric/color editing, or an authored outer shell around permanent native controls does not meet this target.

## Ownership boundary

HTML/CSS owns the document structure: grouped inspector bodies, row placement, labels, descriptions, action placement, responsive wrapping, spacing, visibility, and presentation. Add a dedicated controls resource pair under `Plugins/CkGameplayDebugger/Resources/UI/`; do not destabilize the existing profile resource merely to combine documents.

C++ retains behavior that has a real runtime contract:

- reflection and metadata validation for `FCkDebuggerStyleSelection`;
- typed enum cycling, unknown-config recovery, `SaveConfig`, `NotifyChanged`, Custom-profile transition, selection notification, and preview rebuilds;
- InputHud user/project setting mutation, numeric range and commit behavior, and project persistence;
- color-picker opening/commit semantics and palette/custom-color behavior;
- retained `SCkDebug_InspectorPanel` expansion treatment and the native sample/overlay preview lifetime.

The InputHud slice uses reusable authoring controls, with the live composition and bindings in [SCkStyleLab_ControlsPane.cpp](../../../../CkGameplayDebugger/Source/CkStyleLabDebugger/Public/CkStyleLabDebugger/Widgets/SCkStyleLab_ControlsPane.cpp) and native integration coverage in [CkStyleLab_InputHudAuthored.spec.cpp](../../../../CkGameplayDebugger/Source/CkStyleLabDebugger/Private/Tests/CkStyleLab_InputHudAuthored.spec.cpp). The deleted `SCkStyleLab_InputHudControls` shell is retired historical implementation, not a current compatibility dependency.

## Natural slices

1. **Generic-axis document — verified.** `SCkStyleLab_ControlsPane.{h,cpp}` publishes records, bindings, and weak actions for every reflected axis and group; the controls HTML/CSS pair keeps inspectors and `SCkStyleLab_SamplePane` previews in retained native composition. BuildTest-StyleLabControls-R6.log passed 117/117 in 46s, exit 0 (Toolbox PID 6200 terminal); StyleLabControls-R6-Editor.log has 117 successes and zero relevant diagnostics. StyleLabControls-R6-ResourceInspector.log then passed 10/10 in36s, exit0 (PID7820 terminal); its single cached consumer lane contains all10 unit/functional tests, and StyleLabControls-R6-ResourceInspector-Editor.log has10 successes and zero diagnostics. The focused test routes an action from every group, exercises compatible/rejected reload and owner lifetime, and captures the mounted narrow scroll viewport. That capture is partial scrolled content, not complete layout acceptance.

2. **InputHud document and reusable controls — native authoring verified.** The HTML/CSS owns all InputHud sections, rows, labels, descriptions, authored previous/next cycle buttons, reset action, visibility, wrapping, and status. The registered `number-input` supplies only the 17 editor internals and the shared registered `color-picker` only the five swatch/picker internals; neither control owns row or section layout. Each event calls the existing user/project setting mutation path, including the distinct project-level Hold bar max persistence path. R8 native authoring and construction coverage are green.

3. **Integrated acceptance — native focused evidence complete.** Exercise wide, narrow, and scaled layout; an action from every generic group; InputHud user and project mutations; color and numeric commit/rejection paths; preview notifications; compatible and rejected reload; and held-action/owner lifetime. Inspect captures for wrapping, clipping, scroll reachability, and retained inspector expansion. Extend the existing Style Lab authored and axis-metadata tests, or add a focused ControlsPane authored spec.

## Affected files

Primary implementation and tests:

- `Plugins/CkGameplayDebugger/Source/CkStyleLabDebugger/Public/CkStyleLabDebugger/Widgets/SCkStyleLab_ControlsPane.h/.cpp`
- new `Plugins/CkGameplayDebugger/Resources/UI/StyleLabControls.ui.html/.css`
- `Plugins/CkGameplayDebugger/Source/CkStyleLabDebugger/Private/Tests/CkStyleLab_ProfilesAuthored.spec.cpp`
- `Plugins/CkGameplayDebugger/Source/CkStyleLabDebugger/Private/Tests/CkStyleLab_AxisMetadata.spec.cpp`
- `Plugins/CkGameplayDebugger/Source/CkStyleLabDebugger/Private/Tests/CkStyleLab_InputHudAuthored.spec.cpp`

## Next slice note: InputHud authored-control contract

- `number-input` already supplies retained draft freeze, Escape restoration, finite-float parsing, Enter/user-focus-loss commit, commit-time clamp, integer rounding, and no callback on invalid input. Bind `committed`, not optional `changed`, so typing never persists settings. The current 17 settings are float-safe; a future double-valued setting needs an explicit wider contract.
- Preserve current 0/1/2 digit presentation by adding a generic validated `fractional-digits` option (0 through 6); the present control uses `.9g`, while `SCkDebug_NumericEditor` uses per-row fixed formatting. A model handler must ignore no-op commits before calling a setter or `NotifyChanged`, especially for project `HoldBarMaxPx` persistence. The shared control's visible finite-number error is an intentional validation policy decision, replacing the old editor's silent invalid rejection only if accepted by the slice.
- Register `color-picker` in the shared CkSlateLayout authoring registry, with required `value` `ColorBinding` and `committed` color event, and optional `enabled`, `read-only`, and `alpha`. Its retained implementation is a native swatch/button plus an owned `SColorPicker` popup (clamped, mouse-up commit); authored HTML/CSS owns every label and row. The planned host is an owned native-child `SWindow`, not global `OpenColorPicker` (which uses singleton state) or an `IMenu` (whose focus-loss policy dismisses the picker after native hex Enter). It retains the explicit host Slate user, weak owner/view dispatch, validated opening/commit, and generation invalidation before teardown so late callbacks are inert.
- The six existing InputHud cycles use authored previous/next buttons mapped to their existing setters. The visual-reset action remains explicitly scoped to `Reset_VisualTuning`; it must not imply a readout or project Hold bar reset.

## Explicit later work

The painted overlay is separate: `CkInputHudOverlay/Widgets/SCkInputHud_Root.cpp` and `SCkInputHud_Ribbon.cpp` need retained overlay/ribbon composition, not an incidental ControlsPane rewrite. `CkIntentDebugger/Window/SCkIntentDebugger_InputHudControls.cpp` is a separate settings consumer and must not be folded into this representative migration.

## Exit evidence

A focused final gate must prove the complete target above through real native input and installed resources, with fresh log inspection. It may claim only focused Style Lab ControlsPane coverage. Controller, multi-user, localization, package, performance, browser-reference parity, other debugger consumers, and the full campaign remain outside this gate.

