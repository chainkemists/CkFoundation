# CkPoi

**Purpose:** World-anchored points of interest as a **meta-feature** (the `CkProjectile` shape): `Add`
composes one identity tag (`ck::FTag_Poi`) + a `CkEntityTag` category (+ optional `CkLabel`) onto an
entity that already has a Transform. CkPoi owns **no bespoke fragment machinery** — identity/category/
state live in `CkEntityTag`, the display name in `CkLabel`, per-projector presentation in
`CkPoiDisplayDefinition`, range/fade culling in `CkVisibleRange`. Projectors (CkCompass now;
CkMinimap, world-map, world-space indicators) read those homes directly.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkEntityTag`, `CkLabel`, `CkLog`.
**Used by:** `CkCompass`, `CkMinimap` (projection), `CkEcsDebugger`/`CkMapDebugger` (inspection).

**Named `Poi`, not `Marker`:** `Marker` is taken — `CkOverlapBody`'s `FCk_Handle_Marker`/`utils_marker`
is an overlap-collision primitive, unrelated to HUD markers.

---

## Key API (the whole surface)

- `UCk_Utils_Poi_UE::Add(InHandle, Params)` — direct-attach onto ANY Transform-bearing entity
  (including entities CkPoi didn't create — the Player is the canonical example). One POI per entity.
  Params: `_Category` (essential, `Poi.Category.*`) + `_Label` (optional `CkLabel` role tag).
  There is deliberately NO `Create` — the standalone/ping path is caller-side composition
  (`Request_CreateEntity` + `Transform::Add` + `Poi::Add`), see Recipes.
- `Has` / `Cast` / `<AsPoi>` autocast — keyed on `ck::FTag_Poi`.
- `Get_WorldLocation(Poi)` — the POI entity's own Transform location. The one position API every
  projector reads. (No relative offset — being a Poi never moves an entity's origin.)
- `Get_CategoryTags(Poi)` — the entity's `CkEntityTag` set filtered to `Poi.Category` descendants.
  Empty for one pump right after `Add` (EntityTag adds are deferred) — callers settle first.
- Native tags (declared in `CkPoi_Utils.cpp`): `Tag_Poi_CategoryName` = `"Poi.Category"` (picker
  root), `Tag_Poi_DisabledName` = `"Poi.Disabled"` (the disable convention, below).

## Conventions replacing the old bespoke API

| Old (deleted in v2 Gate 3) | Now |
|---|---|
| `Request_EnableDisable` + `FTag_Poi_Disabled` + `OnPoiEnableDisable` | `CkEntityTag` `"Poi.Disabled"`: disabled iff `Has_UsingGameplayTag(Entity, Tag_Poi_DisabledName)`. Adds/removes are DEFERRED one pump and COUNTED (N disables need N enables). Signal: EntityTag `BindTo_OnGameplayTagUpdated` with a RelevantTags filter (fires on 0↔1 presence flips only). Projectors skip disabled POIs per-entity in their update loop (an EntityTag cannot be an entt view exclude). |
| `_StateTags` + `Request_Add/Remove/SetStateTags` + `OnPoiStateChanged` | Plain `CkEntityTag` gameplay tags + `OnGameplayTagUpdated`. No replace-all verb exists — add/remove individually. |
| `_DisplayName` (FText) | `CkLabel` (`_Label` param, optional). |
| `_Priority` / `_OffscreenPolicy` / `_DisplayAsset` | `CkPoiDisplayDefinition` (per-consumer; `TryGet_PoiDisplayDefinition_ByConsumer`). Projector defaults when absent: Priority 0, OffscreenPolicy Hide. |
| `_Max/MinVisibleRange` / `_RangeFadeBandCm` | `CkVisibleRange` params on the entity. Absent = unlimited. (Gate 3: projectors read the CONFIG and keep their own distance math; the Update_Distance/hidden-tag flow is Gate 4.) |
| `_RelativeLocation` | Deleted. Position = entity Transform. |
| `FCk_RepData_Poi` + `Register_SaveOnly` handler | Deleted outright. Category/state/disabled persist via `CkEntityTag`'s own `Register_SaveOnly<FCk_SaveData_EntityTags>` (`CkEntityTag_Fragment.cpp`). **Pre-Gate-3 saves' Poi enable/state payloads no longer restore** (accepted, PROMPT decision #7). |

## Recipes

- **POI on an existing entity (primary mode):** `UCk_Utils_Poi_UE::Add(Entity, Params)` — nothing else
  required. Compose `PoiDisplayDefinition`/`VisibleRange` separately per projector need.
- **Level-placed static POI:** `ACk_EntitySpawner_UE` referencing `UCk_Poi_EntityScript` (set
  `_PoiParams`; the spawner injects `_SpawnTransform`). This script is also the save-rebuild recipe:
  its replayed Construct re-composes Transform + Poi; EntityTag's handler hydrates category/state.
- **Ping / standalone marker:** create a bare child (`Request_CreateEntity`), `Transform::Add` at the
  world location, `Poi::Add`; TTL via a `CkTimer` that destroys the host (see
  `CkAutoTest_Poi_Create_TtlExpires.as`). Route multiplayer pings through a CkCue.
- **Multi-projector presentation:** one `PoiDisplayDefinition` child per consumer
  (`Poi.Consumer.Compass` / `Poi.Consumer.Minimap` — native tags in the projector modules).

## Anti-patterns

1. Don't add fields to `FCk_Fragment_Poi_ParamsData` — if a datum isn't intrinsic to "is a Poi at
   all", it belongs in a composable module (root CLAUDE.md non-negotiable #9; this module is the case
   study).
2. Don't read `Get_CategoryTags`/disabled state in the same tick as the mutation — EntityTag is
   deferred one pump. Settle first (`WaitOneFrame` in tests).
3. Don't compose a POI on a transform-less entity — `Add` ensures against it.
4. Don't try to view-filter disabled POIs with an entt exclude — the disable convention is an
   EntityTag, not an ECS tag; skip per-entity in the update loop (see `CkCompass_Processor.cpp`).

## See also

- `CkPoiDisplayDefinition/CLAUDE.md`, `CkVisibleRange/CLAUDE.md`, `CkEntityTag/CLAUDE.md` — the homes.
- `CkCompass/CLAUDE.md`, `CkMinimap/CLAUDE.md` — the projectors.
- `REFACTOR_MultiProjectorPoi.md` — the v2 design this shape implements (historical).
