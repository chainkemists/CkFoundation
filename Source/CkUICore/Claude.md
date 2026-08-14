# CkUICore

**Purpose:** The UI base layer — the widget base classes every Ck/game widget derives from, the shared
UI types, per-player UI policy (cursor lock, navigation config, input mode, named slots), input
suspension, and screen fade.

The `UI.Layer.*` / `UI.ExtensionPoint` native gameplay tags deliberately stay in `CkUI`: they name
layer-stack and extension-point concepts this module knows nothing about, and nothing here uses them.
(`UE_DECLARE_GAMEPLAY_TAG_EXTERN` also emits no export macro, so an `FNativeGameplayTag` global does
not resolve across a DLL boundary — moving them here broke the CkUI link.)

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkThirdParty`.
**Used by:** `CkCompass`, `CkEditorToolbar`, `CkGameSettings`, `CkMinimap`, `CkUI`, `CkUIDebugger`,
`CkWatermark`, `CkWorldSpaceWidget`, `CkTests`.

Extracted from `CkUI` so that deriving a widget does not drag in the layer-stack/layout framework.
`CkUI` (layout, HUD, layer stack, extension points) and `CkWidgets` (custom widget primitives and
styles) are **siblings on top of this module, not stacked on each other**.

`CkUI` kept its name because it kept the asset-facing half — content records class paths like
`/Script/CkUI.Ck_UI_LayerStack_ContextProvider_UE`, whereas this base layer is overwhelmingly
*derived from* (including by 15+ BusterBlock AngelScript subclasses of `UCk_ActivatableWidget_UE`),
and derivation costs nothing to re-point.

---

## Key API

- `UCk_UserWidget_UE` / `UCk_ActivatableWidget_UE` — the two widget bases. Both carry an ECS
  ContextReceiver; a widget reads its entity in `OnValidContextInjected`, never at construction.
- `ICk_UI_LayerParticipant` — contract a widget implements to participate in a layer stack. It lives
  here, not in `CkUI`, because `UCk_ActivatableWidget_UE` implements it: the base declares the
  contract and the framework above calls it.
- `UCk_Utils_UI_UE::Request_LockCursorToWidget` / `Request_UnlockCursor` — confine the cursor to a
  `UWidget`'s screen bounds. Slate owns the lock afterwards (the rect follows layout changes and
  auto-releases when the widget leaves the screen), so there is no per-frame upkeep and no teardown
  unlock. Returns `ECk_UI_CursorLock_Result` — the platform silently drops a lock when the widget's
  window is not foreground, and that case is reported rather than swallowed.
- `UCk_Utils_UI_UE::Request_SetNavigationConfig` / `Get_NavigationConfig` — read/replace Slate's
  navigation policy via `FCk_UI_NavigationConfig`. Needed by any game that binds a navigation key as
  a gameplay/UI input: `FNavigationConfig::GetNavigationDirectionFromKey` consumes Tab and the arrow
  keys before the event can bubble to the game viewport, so a CommonUI action-router binding on Tab
  never fires while Escape and gamepad face buttons do (gamepad routes through the
  `FCommonAnalogCursor` *preprocessor*, which runs ahead of navigation). `FSlateApplication` is
  **process-global** — a caller that narrows navigation for gameplay must restore defaults on EndPlay
  or the editor's own UI keeps the narrowed config after PIE.
- `UCk_UI_Input_Subsystem_UE` — handle-based input suspension. Suspensions stack; input is only
  restored when every `FCk_Handle_InputSuspension` has been resumed.
- `UCk_ScreenFade_Subsystem_UE` / `UCk_Utils_ScreenFade_UE` — per-player screen fade.

---

## Implementation notes

**Cursor lock goes through the platform cursor, not `FSlateUser`.** `FSlateUser::LockCursor` /
`UnlockCursor` / `GetCursor` all sit below `SLATE_SCOPE` in `SlateUser.h`, which expands to
`protected` outside the Slate module — that API is Slate-internal. `Request_LockCursorToWidget`
therefore recomputes the clip rect `FSlateUser::LockCursorInternal` would have produced, including
the fullscreen display-distortion correction `FSlateUser` applies.

**The input and screen-fade subsystems are deliberately separate `ULocalPlayerSubsystem`s.** They
were one class that shared no member state. `Initialize`/`Deinitialize` exist only to arm the editor
modal-loop hook, so they belong to the input half; the fade subsystem needs neither. Keeping them
apart is also what lets `CkUI_Types.h` stop forward-declaring a subsystem, which is what made the
old ScreenFade ↔ Subsystem include cycle possible.

**`CkUI_Utils.h` includes the input subsystem rather than forward-declaring the handle.**
`SuspendInput` is a UFUNCTION returning `FCk_Handle_InputSuspension` **by value**, so UHT needs the
complete type; a forward declaration compiles only for as long as something else happens to pull the
definition in first.

**Headers here must include `CkFormat.h` / `CkIsValid.h` directly** when they use
`CK_DEFINE_CUSTOM_FORMATTER_INLINE` or `CK_DEFINE_CUSTOM_IS_VALID_INLINE`. Those macros live in
`CkCore/Format/` and `CkCore/Validation/`, **not** in `CkMacros.h` — a header that relies on its
includers to have pulled them in breaks the moment it is included first in some translation unit.

---

## See also

- `CkUI/Claude.md` — the layer stack, layout, HUD and extension-point framework above this.
- `CkWidgets/Claude.md` — the custom widget primitives and Common* styles, a sibling of `CkUI`.
- `CkWorldSpaceWidget/Claude.md` — entity-driven world-space widgets; consumes this module only.
