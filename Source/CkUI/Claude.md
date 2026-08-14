# CkUI

**Purpose:** UI *framework* — layer stack and primary game layout, HUD, and extension points.

**Depends on:** `CkCore`, `CkEcs`, `CkGraphics`, `CkInput`, `CkLog`, `CkSettings`, `CkThirdParty`, `CkUICore`.
**Used by:** `CkCueEditor`, `CkDynamicEditor`, `CkUIDebugger`, `CkUIEditor`, BusterBlock.

The widget base classes, shared UI types, per-player UI policy (cursor lock, navigation config, input
mode, named slots), input suspension and screen fade moved out to **`CkUICore`**, so that deriving a
widget no longer drags in this layout framework. `CkUICore`, `CkUI` and `CkWidgets` are **siblings on
the base, not a stack** — a custom widget must never pull in the layer stack.

This module kept the name because it kept the asset-facing half: content records class paths such as
`/Script/CkUI.Ck_UI_LayerStack_ContextProvider_UE`, while the base layer is overwhelmingly derived-from
and therefore free to re-point.

The world-space widget feature moved out to `CkWorldSpaceWidget` — it was the only ECS feature quartet
here, and the only reason this module depended on `CkEcsExt`.

Extension points live here rather than in their own module on purpose: `UCk_UI_Layout_Subsystem_UE`
*owns* `TArray<FCk_UI_ExtensionHandle>` and drives registration. Layout does not *use* the extension
system, it *drives* it — one feature, no seam.

---

## Key API

- `ECk_LinePlaneIntersectionStatus` — result of 3D projection queries.
- `ACk_HUD_UE::Refresh_Context` — explicit rebuild-completion refresh for the primary layout and active Game-layer
  HUD root. It bypasses first-push `OnlyIfMissing` only at that owned root; unrelated menu layers keep their context.
- `UCk_InputActionWidget_UE` — `UCommonActionWidget` that resolves its glyph through the player's mappable
  key profile instead of the applied Mapping Contexts, so it still shows a binding on a rebinding screen
  where no gameplay context is applied. Adds `_Slot` (secondary bindings), `_UnboundPolicy` (the parent
  hard-collapses on a missing glyph), and `Get_ResolvedKey` / `Get_ResolvedKeyDisplayName` /
  `OnResolvedKeyChanged` for a key-name text fallback. Seed from `Get_ResolvedKey`, then track
  `OnResolvedKeyChanged` — the first broadcast happens during construction, before a subclass can bind.

---

## Pattern

Widgets receive their entity through the ECS ContextReceiver on `UCk_UserWidget_UE` — the layer stack
injects context on push, and the widget reads it in `OnValidContextInjected`. Layout is data-driven:
a `UCk_UI_LayoutConfigAsset` declares the layers and their HUD elements, and the Layout Subsystem
registers those elements as extension-point contributions.

---

## Implementation notes

**`UCk_AnimatableRetainerBox::bShowEffectsPreview` must NOT be renamed to match the parent's
`URetainerBox::bShowEffectsInDesigner`.** The parent's is `WITH_EDITORONLY_DATA` (absent in game
builds), so CkUI needs its own runtime-available toggle; the earlier name `ShowEffectsInDesigner`
collided under AngelScript binding — the binder strips the parent's leading `b`, so both produced
`Get/SetShowEffectsInDesigner` → `asALREADY_REGISTERED` and a failed cook. Keep the names distinct.

**Custom widget provenance.**
- `SCk_ColorWheel` — port of OmegaGameFramework's `SWColorWheel`
  (https://github.com/StudioSyndiCatCaius/OmegaGameFramework), itself derived from the engine's
  `SColorWheel`. Changes against the source: engine-correct selector placement math (the source
  dropped the half-selector offset, leaving the pin off-center), dead tint attributes removed
  (tinting goes through the brushes the UMG wrapper owns), self-invalidation on value changes, and
  `ck::IsValid` guards with core-style brush fallbacks.
- `UCk_DialogueBox_UserWidget_UE` — derived from https://github.com/redxdev/UnrealRichTextDialogueBox.
  `UCk_DialogueTextBlock::RebuildWidget` mirrors `URichTextBlock::RebuildWidget`, re-implemented only
  to capture the `FSlateTextLayout` / `FRichTextLayoutMarshaller` the typewriter effect needs.
- `ECk_WidgetRasterizer_GammaCorrection` defaults to `Enabled` to preserve pre-existing callers.

**`UCk_InputActionWidget_UE` exists for resolution, NOT for refresh.** `UCommonActionWidget` already
re-resolves on rebind: `ListenToInputMethodChanged` subscribes to
`UEnhancedInputLocalPlayerSubsystem::ControlMappingsRebuiltDelegate`, and `MapPlayerKey` →
`OnSettingsChanged` → `OnUserSettingsChanged` → `RequestRebuildControlMappings` closes that loop. Do not
re-add a rebind listener believing it is missing. What the parent genuinely cannot do is resolve a key
that no APPLIED Mapping Context supplies — `GetIcon` runs through `CommonUI::GetFirstKeyForInputType` →
`QueryKeysMappedToAction`, which reads applied contexts only — so it blanks on every rebinding-screen row
and on any prompt shown ahead of its context. It is also slot-blind (first key suiting the current
device). The subclass reads `UEnhancedPlayerMappableKeyProfile::FindKeyMapping` first, which is
application-independent and slot-addressed, then falls back to the parent lookup.

Three details that are easy to get wrong here:
- **The unbound fallback must be `FStyleDefaults::GetNoBrush()`, not `FSlateBrush{}`.** A default brush is
  `DrawAs=Image` with a null resource, so the parent's `DrawAs == NoDrawType` collapse test never fires and
  the prompt stays visible drawing nothing. `UCk_Utils_KeyIcon_UE::Get_BrushForKey` returns exactly that
  default brush on a miss, which is why `ck_input_action_widget::Get_IsDrawableBrush` also checks the
  resource.
- **`Get_ResolvedKey` deliberately calls the same `CommonUI::GetFirstKeyForInputType` the parent's icon path
  uses** for its fallback tier. Querying `QueryKeysMappedToAction` directly and taking `[0]` reports a
  keyboard key while a gamepad glyph is on screen.
- **`OnSettingsChanged` is unsubscribed through a cached `TWeakObjectPtr`, never a re-query.**
  `ReleaseSlateResources` can run after the PlayerController is gone, and the CkInput accessors
  `CK_ENSURE` on a missing controller/subsystem.

**HUD layout config is a soft ref by design.** A subclass default carries only a path, async-loaded
in `DoInitializeUI` at BeginPlay. A CDO-time hard ref cannot block-load safely in a packaged client
(it can run on a worker thread during async load) and resolved to null, leaving the HUD with no UI.

**HUD rebuilds a surviving layout instead of adopting it.** On a new world, if the Layout Subsystem
still holds a layout, `ACk_HUD_UE` destroys it and rebuilds. The survivor comes from the previous
world's HUD never tearing it down — its `DoShutdownUI` ran after the PlayerController died — so HUD
shutdown deliberately tolerates a missing PlayerController/LocalPlayer (the subsystem is
LocalPlayer-scoped; the `Verbose` logs on those early-outs exist so the leak stays attributable).
`HandleLayoutConfigLoaded` is where the next world detects and destroys it. Returning silently left a
stale layout on screen with dead bindings AND suppressed `OnLayoutReady`, so game HUD subclasses
never re-injected the fresh world's context. Adopt-and-rebind is unsafe: the surviving widgets hold
dead entity handles.

---

## See also

- `CkWorldSpaceWidget/Claude.md` — the entity-driven world-space widget feature that used to live here.
- `CkGameSession/Claude.md` — session state drives some UI visibility.
- `CkGraphics/Claude.md`.
