# CkUI

**Purpose:** UI utilities — widget math helpers (line/plane intersection for 3D-to-screen projection), UI entity lifecycle, and bridge between UE's `UMG` and the ECS world.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkGameSession`, `CkLog`, `CkSettings`, `CkThirdParty`.
**Used by:** `CkCueEditor`, `CkDynamicEditor`, `CkEditorToolbar`, `CkWatermark`.

---

## Key API

- `ECk_LinePlaneIntersectionStatus` — result of 3D projection queries.
- UI entity lifecycle (show/hide, bind to entity position).
- `UCk_Utils_UI_UE::Request_LockCursorToWidget` / `Request_UnlockCursor` — confine the mouse cursor
  to a `UWidget`'s screen bounds. Slate owns the lock afterwards (rect follows layout changes, auto-
  releases when the widget leaves the screen), so there is no per-frame upkeep and no teardown unlock.
  Returns `ECk_UI_CursorLock_Result` — a lock is silently dropped by the platform when the widget's
  window is not foreground, and that case is reported rather than swallowed.
- `ACk_HUD_UE::Refresh_Context` — explicit rebuild-completion refresh for the primary layout and active Game-layer
  HUD root. It bypasses first-push `OnlyIfMissing` only at that owned root; unrelated menu layers keep their context.
- `UCk_Utils_UI_UE::Request_SetNavigationConfig` / `Get_NavigationConfig` — read/replace Slate's navigation policy
  via `FCk_UI_NavigationConfig` (mirrors the tunable public fields of `FNavigationConfig`). Needed by any game that
  binds a navigation key as a gameplay/UI input: `FNavigationConfig::GetNavigationDirectionFromKey` consumes Tab and
  the arrow keys before the event can bubble to the game viewport, so a CommonUI action-router binding on Tab never
  fires while Escape and gamepad face buttons do (gamepad routes through the `FCommonAnalogCursor` *preprocessor*,
  which runs ahead of navigation). `FSlateApplication` is **process-global** — a caller that narrows navigation for
  gameplay must restore defaults on EndPlay or the editor's own UI keeps the narrowed config after PIE.

---

## Pattern

Attach a UI entity to a gameplay entity; the UI processor updates the widget's screen position each frame from the entity's world transform.

---

## Anti-patterns

Don't update widget positions from Tick in a `UUserWidget` subclass — route through the UI processor so updates are batched.

---

## Implementation notes

**Cursor lock goes through the platform cursor, not `FSlateUser`.** `FSlateUser::LockCursor` /
`UnlockCursor` / `GetCursor` all sit below `SLATE_SCOPE` in `SlateUser.h`, which expands to
`protected` outside the Slate module — that API is Slate-internal. `Request_LockCursorToWidget`
therefore recomputes the clip rect `FSlateUser::LockCursorInternal` would have produced, including
the fullscreen display-distortion correction `FSlateUser` applies.

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

**WorldSpaceWidget — legacy-plugin provenance.** All inherited from the legacy WorldSpaceWidgets
plugin: `ECk_WorldSpaceWidget_Clamping_Policy` collapses its two interacting bools
(`bShouldClampToViewport` + `bShouldClampByBounds`) into one policy;
`ECk_WorldSpaceWidget_Occlusion_Policy::HideWhenOccluded` mirrors `bShouldBeOccluded`;
`ECk_WorldSpaceWidget_RenderMode::WorldComponent` matches the legacy `/Script/UMG.WidgetComponent`
callout; `FCk_WorldSpaceWidget_OcclusionInfo::_TraceChannel` defaults to `ECC_Camera` because that is
the channel the legacy plugin traced on; `_DrawAtDesiredSize` carries
`UWidgetComponent::bDrawAtDesiredSize` semantics, which is what legacy WidgetComponent callouts
authored.

**WorldSpaceWidget gotchas.**
- `WorldComponent` mode hands the content-widget INSTANCE (`Params._Widget`) to
  `UWidgetComponent::SetWidget`, and only after `RegisterComponentWithWorld`. Do not rely on the
  component's own `InitWidget`/`SetWidgetClass` instantiation — unreliable for runtime-created
  components, and `GetWidget()` comes back null.
- The reconfiguration requests (`FCk_Request_WorldSpaceWidget_SetLocationInfo` / `SetScalingInfo` /
  `SetFadingInfo` / `SetOcclusionInfo`) each overwrite the matching sub-struct on the live Params
  fragment through the deferred `HandleRequests` processor. `SetScalingInfo` additionally flips
  `FTag_WorldSpaceWidget_NeedsUpdateScaling` so distance-scaling can be toggled at runtime — the
  legacy parity gap was that the tag was granted only at creation.
- EndPlay must remove the WRAPPER from the viewport, not the content widget: the wrapper is what
  `Request_WrapWidget` added, and removing it takes its content child with it. Removing only the
  content leaked the wrapper into the viewport forever.

---

## See also

- `CkGameSession/Claude.md` — session state drives some UI visibility.
- `CkGraphics/Claude.md`.
