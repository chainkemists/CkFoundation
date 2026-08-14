# CkWidgets

**Purpose:** The reusable UI *primitives* — custom widgets, the `Common*` style assets, the widget
rasterizer, and screen-projection utilities.

**Depends on:** `CkCore`, `CkUICore`, `CkGraphics`, `CkInput`, `CkLog`, `CkThirdParty`.
**Used by:** `CkCueEditor`, `CkEcsDebugger`, `CkGameSettings`, `CkTests`.

Split out of `CkUI` so a custom widget does not drag in the layer-stack/layout framework.
**`CkWidgets` is a sibling of `CkUI` on top of `CkUICore`, not a layer beneath or above it** — nothing
here includes anything from `CkUI`, and `CkUI` includes nothing from here. Keep it that way: the whole
point of the split is that a game can use a `TabBar` or a `CommonButton` style without the layer stack.

`CkGraphics` and `CkInput` moved here from `CkUI` because exactly one widget each needed them —
`CkParametricImage` and `CkInputAction` respectively — so `CkUI` now depends on neither.

---

## Contents

- **Widgets:** `AnimatableRetainerBox`, `ColorWheel`, `FilteredEditableTextBox`, `Flipbook`,
  `InputAction`, `PanBox`, `ParametricImage`, `TabBar`.
- **Styles:** the 29 `UCkCommon*Style_*` / `UCkCommonButton*` CommonUI style assets.
- **`UCk_Utils_WidgetRasterizer_UE`** — render a widget to a texture.
- **`UCk_Utils_Screen_UE`** (`CkScreen_Utils.h`) — screen projection and edge clamping;
  owns `ECk_LinePlaneIntersectionStatus`.

`DialogueBox`, `RichText` and `WidgetStack` deliberately stayed in `CkUI`: `WidgetStack` is
framework-internal (`CkUI_LayerStack.h` is its only consumer), and the other two are narrative/layout
pieces rather than reusable primitives.

---

## Implementation notes

**`UCk_AnimatableRetainerBox::bShowEffectsPreview` must NOT be renamed to match the parent's
`URetainerBox::bShowEffectsInDesigner`.** The parent's is `WITH_EDITORONLY_DATA` (absent in game
builds), so this module needs its own runtime-available toggle; the earlier name
`ShowEffectsInDesigner` collided under AngelScript binding — the binder strips the parent's leading
`b`, so both produced `Get/SetShowEffectsInDesigner` → `asALREADY_REGISTERED` and a failed cook. Keep
the names distinct.

**`UCk_InputActionWidget_UE` exists for resolution, NOT for refresh.** `UCommonActionWidget` already
re-resolves on rebind: `ListenToInputMethodChanged` subscribes to
`UEnhancedInputLocalPlayerSubsystem::ControlMappingsRebuiltDelegate`, and `MapPlayerKey` →
`OnSettingsChanged` → `OnUserSettingsChanged` → `RequestRebuildControlMappings` closes that loop. Do
not re-add a rebind listener believing it is missing. What the parent genuinely cannot do is resolve a
key that no APPLIED Mapping Context supplies — `GetIcon` runs through
`CommonUI::GetFirstKeyForInputType` → `QueryKeysMappedToAction`, which reads applied contexts only —
so it blanks on every rebinding-screen row and on any prompt shown ahead of its context. It is also
slot-blind (first key suiting the current device). The subclass reads
`UEnhancedPlayerMappableKeyProfile::FindKeyMapping` first, which is application-independent and
slot-addressed, then falls back to the parent lookup.

Three details that are easy to get wrong there:
- **The unbound fallback must be `FStyleDefaults::GetNoBrush()`, not `FSlateBrush{}`.** A default brush
  is `DrawAs=Image` with a null resource, so the parent's `DrawAs == NoDrawType` collapse test never
  fires and the prompt stays visible drawing nothing. `UCk_Utils_KeyIcon_UE::Get_BrushForKey` returns
  exactly that default brush on a miss, which is why `ck_input_action_widget::Get_IsDrawableBrush`
  also checks the resource.
- **`Get_ResolvedKey` deliberately calls the same `CommonUI::GetFirstKeyForInputType` the parent's
  icon path uses** for its fallback tier. Querying `QueryKeysMappedToAction` directly and taking `[0]`
  reports a keyboard key while a gamepad glyph is on screen.
- **`OnSettingsChanged` is unsubscribed through a cached `TWeakObjectPtr`, never a re-query.**
  `ReleaseSlateResources` can run after the PlayerController is gone, and the CkInput accessors
  `CK_ENSURE` on a missing controller/subsystem.

**Custom widget provenance.**
- `SCk_ColorWheel` — port of OmegaGameFramework's `SWColorWheel`
  (https://github.com/StudioSyndiCatCaius/OmegaGameFramework), itself derived from the engine's
  `SColorWheel`. Changes against the source: engine-correct selector placement math (the source
  dropped the half-selector offset, leaving the pin off-center), dead tint attributes removed
  (tinting goes through the brushes the UMG wrapper owns), self-invalidation on value changes, and
  `ck::IsValid` guards with core-style brush fallbacks.
- `ECk_WidgetRasterizer_GammaCorrection` defaults to `Enabled` to preserve pre-existing callers.

---

## See also

- `CkUICore/Claude.md` — the widget bases and UI types this module builds on.
- `CkUI/Claude.md` — the layer stack / layout / HUD framework, a sibling of this module.
