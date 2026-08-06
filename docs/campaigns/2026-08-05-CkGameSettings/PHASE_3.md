# PHASE 3 — Widget accelerant

**Goal:** the optional UI: settings screen (CodeBuilt/Custom), typed rows with a type→row-class
mapping, keybinding page over CkInput, and `UCk_InputActionWidget_UE` in CkUI. Zero shipped
uassets. This is the judgment-heavy phase — if context passes ~50% mid-phase, checkpoint in
PROGRESS (which step you finished, exact next step) and end the session; the phase is designed to
be resumable at any numbered step.

## Entry criteria

1. Phases 0-2 exit criteria hold.
2. Read (mandatory, in this order): `CkCompassUI_RibbonWidget.h` (the pattern you are mirroring —
   presentation enum, `DoReportMisconfig`, preview, pooling, `BindWidgetOptional`),
   `CkUI/UserWidget/CkUserWidget.h` + `CkActivatableWidget.h`, `CkUI/Layout/CkUI_Layout_Utils.h`,
   `CkUI/CkUI_GameplayTags.h`, `CkUI/Styles/CkCommonButton.h`,
   `CkInput/CkKeyBinding_Utils.h` + `CkKeyBinding_Subsystem.h`, `CkInput/CkKeyIcon_Utils.h`.
3. Build.cs: add `UMG, Slate, SlateCore, CommonUI, EnhancedInput, CkUI, CkInput` to
   CkGameSettings with the annotation comment `// Widget layer only (settings screen + keybinding
   page) — the core registry must never include from these.` (Compass `Build.cs:31-39` precedent.)

## Steps

### 3.1 `UI/CkGameSettingsUI_RowWidgets.{h,cpp}`
Base `UCk_GameSettingsUI_RowWidgetBase : UCk_UserWidget_UE` —
`InjectSetting(const UObject* InWorldContextObject, FName InKey)`: pulls definition + current
value, subscribes `BindTo_OnSettingChanged(InKey, ...)`, unsubscribes in `NativeDestruct`
(rebind-safe like `InjectCompass`). Displays `_DisplayName`, tooltip `_Description`, disabled
state + reason from `_EditConditions`. Subclasses (each with CodeBuilt fallback construction and
`BindWidgetOptional` slots for Custom):
- `UCk_GameSettingsUI_RowWidget_Toggle` (Bool) — checkbox.
- `UCk_GameSettingsUI_RowWidget_Slider` (Int32/Float with range) — slider + numeric readout;
  live-preview writes on drag, but persistence-affecting `Request_SetSettingValue_*` only on
  handle release (AutoSettings' one good slider trick).
- `UCk_GameSettingsUI_RowWidget_Select` (`_Options` present, any type; also Int32 without range) —
  spinner-style prev/next + label, gamepad-friendly.

### 3.2 `UI/CkGameSettingsUI_ScreenWidget.{h,cpp}`
`UCk_GameSettingsUI_ScreenWidget : UCk_ActivatableWidget_UE` (a layer participant — pushed to
`TAG_UI_Layer_Menu` via `UCk_Utils_UI_Layout_UE::PushWidgetToLayer`).
- `ECk_GameSettingsScreen_Presentation { CodeBuilt = 0, Custom }` +
  `CK_DEFINE_CUSTOM_FORMATTER_ENUM` — semantics mirror the Compass enum verbatim (CodeBuilt: WBP
  must NOT author a tree; Custom: `BindWidgetOptional` `_CategoryTabBar` (UCk_TabBarWidget_UE),
  `_RowContainer` (UPanelWidget)).
- Row generation FROM THE REGISTRY: categories from `_CategoryTags` (one tab per top-level
  category tag; settings with no category land in a "General" tab), rows resolved via 3.3, rows
  pooled and `InjectSetting`-ed. Edit-condition-hidden settings are omitted; disabled ones render
  disabled with `_DisabledReason` as tooltip.
- Apply/Cancel strip (CodeBuilt; optional bound buttons in Custom): uses the Phase-1
  pending-changes session; screen activation calls `Request_BeginPendingChanges`, Back/Cancel
  reverts, Apply commits. `_AutoApplyMode` (bool, default true) skips the session entirely and
  writes live (the BB 5.5 behavior).
- Misconfig reporting + design-time preview fake rows through the SAME generation path
  (`DoReportMisconfig` / `_ReportedMisconfig` / preview-count properties — mirror the ribbon).

### 3.3 Type→row mapping
Project settings: `_RowClassOverrides` (TMap<ECk_GameSettings_ValueType,
TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>>) + `_SelectRowClassOverride` (options-present
case). Resolution: per-type override → built-in default. Soft classes async-loaded before first
row spawn (screen shows nothing until loaded; no sync loads in widget code — CkUI doctrine).

### 3.4 `UI/CkGameSettingsUI_KeyBindingPageWidget.{h,cpp}`
`UCk_GameSettingsUI_KeyBindingPageWidget : UCk_UserWidget_UE`. Pure consumer of
`UCk_Utils_KeyBinding_UE`: rows enumerated from `Get_AllRemappableKeys` grouped by
`_DisplayCategory`; per-row current key via `Get_KeyForMapping` + glyph via
`UCk_Utils_KeyIcon_UE::Get_BrushForKey`; capture prompt (modal, keyboard+gamepad key capture via
`NativeOnKeyDown`/`NativeOnAnalogValueChanged`, Escape cancels); on capture:
`Get_HasKeyConflicts` → if conflicts, present swap/overwrite/cancel (use `SwapKeys` /
`UnbindConflictAndRemap`); `RemapKey` with failure-reason surfacing; Reset-row
(`ResetMappingToDefault`) + Reset-all (`ResetAllToDefaults`); `SaveKeyBindings` on page close;
live refresh via `BindTo_OnMappingKeyChanged` (unbind in `NativeDestruct`).
**Fence: zero new API in CkInput.** If something is missing, STOP → PROGRESS blocker.

### 3.5 `UCk_InputActionWidget_UE` → `Source/CkUI/Public/CkUI/CustomWidgets/InputAction/CkInputAction_Widget.{h,cpp}`
Subclass `UCommonActionWidget`; delivers the promise at `CkKeyIcon_Utils.h:30-32`: auto-refresh on
input-device change (CommonInput subsystem's input-method-changed event) AND on remap
(`UCk_Utils_KeyBinding_UE::BindTo_OnMappingKeyChanged`, unbound in `NativeDestruct`). CkUI gains a
`CkInput` dep — annotate it `// UCk_InputActionWidget_UE only`. Keeps the `_UE` suffix (promised
name; CkUI widget bases carry it).

### 3.6 Tests + gym
- In-module spec: `Ck.CkGameSettings.UI.RowClassResolution` (mapping precedence — pure logic).
- CkTests gym (branch `feature/game-settings-tests`): a `GameSettings` gym level/station per the
  gym spec (`Plugins/CkTests/Script/Common/CkGym_CreationSpecification.txt`) that registers a
  demo collection (one setting per row type + both packs if enabled) and pushes the CodeBuilt
  screen — the human visual surface for VALIDATION.md.
- AutoTest: `...GameSettings_ScreenGeneratesRowsFromRegistry` — create the screen headless,
  assert row count/types match a registered demo collection (widget creation works headless;
  rendering is not asserted).

### 3.7 Gate
Phase 0-3 test names green, full suite delta-zero, `--discover-fresh` on first run of new names.
Same STOP branches as prior phases.

## Fences

- Zero uassets committed. `git status` in exit criteria must show no `Content/` changes anywhere.
- No per-frame `NativeTick` work in rows (event-driven via change delegates); the screen may tick
  only if the countdown/preview genuinely needs it — justify in PROGRESS if so.
- No Slate (S-widgets) — UMG C++ only, per the accelerant precedent.
- Rows never cache definition pointers — re-query by key (registry may re-register).
- Do not push the screen anywhere automatically — games own the open/close flow; the gym is the
  only place this campaign pushes it.

## Exit criteria

1. All phase 0-3 test names green; full suite delta-zero vs baseline.
2. `git -C Plugins/CkFoundation status` and CkTests status show source/docs changes only — zero
   `Content/`, zero `.uasset`.
3. `rg -n "CkGameSettings" Source/CkInput` → 0 hits (nothing added to CkInput).
4. Gym registered and boots headless without ensures (run its AutoTest).
5. Committed both repos (no push); PROGRESS updated with per-step checkpoint marks.
