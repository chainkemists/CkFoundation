# CkPoiDisplayDefinition

**Purpose:** Per-consumer presentation configuration for an entity, plus a parent→child visibility
cascade. A display definition names a `_Consumer` (which projector it is FOR — compass, minimap, a
world-space indicator), a `_Priority`, an `_OffscreenPolicy`, and an OPAQUE soft reference to a
`UCk_Poi_DisplayDefinition_PDA` visual asset. One entity can carry a single definition direct-attach
(`Add`) or several consumer-keyed definitions as child entities under its
`RecordOfPoiDisplayDefinitions` (`Create`). When the owner composes `CkVisibleRange` and goes hidden,
every child definition gains the plain tag `FTag_PoiDisplayDefinition_ParentHidden` (and loses it on
show) — the cascade this module owns.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`,
`CkVisibleRange`. Knows nothing of `CkPoi`, projectors, or "what a viewer is" — it is the display-config
substrate those consumers read, not a POI itself. The `UCk_Poi_DisplayDefinition_PDA` asset and
`ECk_Poi_OffscreenPolicy` enum live HERE (moved out of `CkPoi` — see root `CLAUDE.md` non-negotiable #9).

**Used by:** Gate-4 projectors (compass/minimap presentation layers) via
`TryGet_PoiDisplayDefinition_ByConsumer` + the `Get_IsEffectivelyHidden` read.

---

## Key API

- `UCk_Utils_PoiDisplayDefinition_UE::Add(InHandle, Params)` — direct-attach; composes ONE definition
  onto `InHandle` itself (no child entity). Ensures a valid handle, that the entity does not already
  have the feature, and a valid `_Consumer` tag.
- `Create(InLifetimeOwner, Params) -> FCk_Handle_PoiDisplayDefinition` — creates a definition CHILD
  entity under `InLifetimeOwner`, labelled with its `_Consumer` and connected to the owner's
  `RecordOfPoiDisplayDefinitions`. Also binds the owner's `OnVisibleRange_HiddenChanged` cascade once
  (guarded by `FTag_PoiDisplayDefinition_CascadeBound`) and seeds `ParentHidden` if the owner is
  already hidden at creation.
- `TryGet_PoiDisplayDefinition_ByConsumer(InOwner, InConsumer)` — direct-attach definition first, then
  the first exact-match (`MatchesTagExact`) child in the record; invalid handle if none. The read
  Gate-4 projectors use.
- Getters (on `FCk_Handle_PoiDisplayDefinition`): `Get_Consumer`, `Get_Priority`, `Get_OffscreenPolicy`,
  `Get_DisplayAsset`, `Get_IsParentHidden`, `Get_IsEffectivelyHidden`.
- Consumer tags live under the native root **`Poi.Consumer`** (declared in
  `CkPoiDisplayDefinition_Utils.cpp`).

This module ships **no processors and no signals of its own** — the cascade is a native signal bind to
CkVisibleRange, not a polling processor (the CkTween↔CkTimer precedent). Display definitions are static
config: no `Current` fragment, no requests.

## The cascade contract

Two independent "hidden" concepts, each owned by exactly one module:

- **`FTag_VisibleRange_Hidden`** (owned by `CkVisibleRange`, COUNTED) — the owner's OWN visibility, from
  range state + explicit show/hide. This module NEVER touches it.
- **`FTag_PoiDisplayDefinition_ParentHidden`** (owned HERE, PLAIN) — set on a child definition to mean
  "my parent is hidden". A child has exactly one parent, so this has a single source — a plain tag, not
  counted.

`Create` binds `DoOnOwnerHiddenChanged` to the owner's `OnVisibleRange_HiddenChanged` once. On each
0↔>0 transition CkVisibleRange broadcasts, the handler walks the owner's record and Has-guarded
`Add`/`Remove`s `ParentHidden` on every child. The bind uses `IgnorePayloadInFlight` because `Create`
seeds ground truth itself (a replayed payload would double-apply). A child created under an
already-hidden owner is seeded with the tag at creation so it never flashes visible for a frame.

Gate-4 consumers exclude BOTH `FTag_VisibleRange_Hidden` (owner's own) and
`FTag_PoiDisplayDefinition_ParentHidden` (parent cascade) — `Get_IsEffectivelyHidden` folds both.

## Anti-patterns

1. **Never add a vote to `FTag_VisibleRange_Hidden` from this module.** That counted tag carries only
   CkVisibleRange's own two sources; a parent→child cascade needs its OWN separate tag on the child —
   which is exactly `FTag_PoiDisplayDefinition_ParentHidden`. See `CkVisibleRange/CLAUDE.md`'s consumer
   note (the hard boundary).
2. **No Poi / projector / viewer knowledge here.** This module is display-config + cascade only. What a
   "consumer" means, how a compass or minimap draws, what a viewer is — all live in the consumer, not
   here. `_DisplayAsset` is OPAQUE: never load or dereference it in this module.
3. **`Add` composes ONE definition; `Create` composes per-consumer children.** Don't call `Add` twice on
   one entity expecting two consumers — the second `Add` ensure-fails (one direct-attach definition per
   entity). For several consumer-keyed definitions on one owner, `Create` each.

## See also

- `CkVisibleRange/CLAUDE.md` — the counted hidden tag this module cascades FROM, and its consumer-note
  boundary.
- `CkPoi/CLAUDE.md` — the POI substrate that will reference display definitions by consumer (Gate 3+).
- `CkRecord/Claude.md` — the parent/child record `Create` connects children through.
