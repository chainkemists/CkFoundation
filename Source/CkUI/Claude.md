# CkUI

**Purpose:** UI *framework* — layer stack and primary game layout, HUD, and extension points.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkSettings`, `CkThirdParty`, `CkUICore`.
**Used by:** `CkDynamicEditor`, `CkUIDebugger`, `CkUIEditor`, BusterBlock.

The widget base classes, shared UI types, per-player UI policy (cursor lock, navigation config, input
mode, named slots), input suspension and screen fade moved out to **`CkUICore`**, so that deriving a
widget no longer drags in this layout framework. The custom widget primitives, the `Common*` styles,
the widget rasterizer and the screen-projection utils moved out to **`CkWidgets`**.
`CkUICore`, `CkUI` and `CkWidgets` are **siblings on the base, not a stack** — a custom widget must
never pull in the layer stack. `CkGraphics` and `CkInput` left with `CkWidgets`: one widget each
(`CkParametricImage`, `CkInputAction`) was their only consumer.

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

- `ACk_HUD_UE::Refresh_Context` — explicit rebuild-completion refresh for the primary layout and active Game-layer
  HUD root. It bypasses first-push `OnlyIfMissing` only at that owned root; unrelated menu layers keep their context.

---

## Pattern

Widgets receive their entity through the ECS ContextReceiver on `UCk_UserWidget_UE` — the layer stack
injects context on push, and the widget reads it in `OnValidContextInjected`. Layout is data-driven:
a `UCk_UI_LayoutConfigAsset` declares the layers and their HUD elements, and the Layout Subsystem
registers those elements as extension-point contributions.

---

## Implementation notes

**Custom widget provenance.**
- `UCk_DialogueBox_UserWidget_UE` — derived from https://github.com/redxdev/UnrealRichTextDialogueBox.
  `UCk_DialogueTextBlock::RebuildWidget` mirrors `URichTextBlock::RebuildWidget`, re-implemented only
  to capture the `FSlateTextLayout` / `FRichTextLayoutMarshaller` the typewriter effect needs.

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

- `CkUICore/Claude.md` — the widget bases, UI types and per-player UI policy this module builds on.
- `CkWidgets/Claude.md` — the custom widget primitives and `Common*` styles, a sibling of this module.
- `CkWorldSpaceWidget/Claude.md` — the entity-driven world-space widget feature that used to live here.
