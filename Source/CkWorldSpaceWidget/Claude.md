# CkWorldSpaceWidget

**Purpose:** ECS feature that drives a UMG widget from an entity's world transform — screen projection, viewport clamping, distance scaling, fading, and occlusion. Also hosts `UCk_WidgetComponent_UE`, the world-component render path.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkUI`, `CkLog`, `CkSettings`, `CkThirdParty`.
**Used by:** `CkEcsDebugger`.

Extracted from `CkUI` — it was the only true ECS feature quartet living inside that module, and the only reason `CkUI` depended on `CkEcsExt`.

---

## Key API

- `UCk_Utils_WorldSpaceWidget_UE::Add` — compose the feature onto an entity.
- `FCk_Fragment_WorldSpaceWidget_ParamsData` — location / scaling / fading / occlusion sub-structs.
- Reconfiguration requests: `FCk_Request_WorldSpaceWidget_SetLocationInfo` / `SetScalingInfo` /
  `SetFadingInfo` / `SetOcclusionInfo`.
- `ECk_WorldSpaceWidget_RenderMode` — `Viewport` (wrapper widget) vs `WorldComponent` (`UWidgetComponent`).

---

## Anti-patterns

Don't update widget positions from `Tick` in a `UUserWidget` subclass — route through the processor so updates are batched.

---

## Implementation notes

**Legacy-plugin provenance.** All inherited from the legacy WorldSpaceWidgets plugin:
`ECk_WorldSpaceWidget_Clamping_Policy` collapses its two interacting bools (`bShouldClampToViewport` +
`bShouldClampByBounds`) into one policy; `ECk_WorldSpaceWidget_Occlusion_Policy::HideWhenOccluded`
mirrors `bShouldBeOccluded`; `ECk_WorldSpaceWidget_RenderMode::WorldComponent` matches the legacy
`/Script/UMG.WidgetComponent` callout; `FCk_WorldSpaceWidget_OcclusionInfo::_TraceChannel` defaults to
`ECC_Camera` because that is the channel the legacy plugin traced on; `_DrawAtDesiredSize` carries
`UWidgetComponent::bDrawAtDesiredSize` semantics, which is what legacy WidgetComponent callouts authored.

**Gotchas.**
- `WorldComponent` mode hands the content-widget INSTANCE (`Params._Widget`) to
  `UWidgetComponent::SetWidget`, and only after `RegisterComponentWithWorld`. Do not rely on the
  component's own `InitWidget`/`SetWidgetClass` instantiation — unreliable for runtime-created
  components, and `GetWidget()` comes back null.
- The reconfiguration requests each overwrite the matching sub-struct on the live Params fragment
  through the deferred `HandleRequests` processor. `SetScalingInfo` additionally flips
  `FTag_WorldSpaceWidget_NeedsUpdateScaling` so distance-scaling can be toggled at runtime — the
  legacy parity gap was that the tag was granted only at creation.
- EndPlay must remove the WRAPPER from the viewport, not the content widget: the wrapper is what
  `Request_WrapWidget` added, and removing it takes its content child with it. Removing only the
  content leaked the wrapper into the viewport forever.
- Enabled/disabled is the `FTag_WorldSpaceWidget_Disabled` tag, not a fragment field —
  `Get_EnableDisable` derives from tag presence and both PostTransform processors `TExclude` it, so a disabled
  widget is never iterated. Two consequences: the widget-went-away-destroy-the-entity safety net in
  `UpdateLocation`/`UpdateScaling` does not run while disabled (it fires on re-enable instead), and
  the `WorldComponent` transform push stops — `Request_SetEnabled` pushes the transform once on
  re-enable so an anchor that moved while hidden does not pop for a frame.

**Profiling.** Counters live under `STATGROUP_CkWorldSpaceWidget` (`stat CkWorldSpaceWidget`). They
were under `STATGROUP_CkUI` before the extraction.

---

## See also

- `CkUI/Claude.md` — the widget base class and per-player UI policy this module sits on.
- `CkEcsExt/Claude.md` — Transform / SceneNode, the source of the world position this feature reads.
