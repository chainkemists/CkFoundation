# CkMinimap

**Purpose:** Per-observer 2D projection of world POIs (CkPoi) onto a map frame — the HUD minimap AND the
fullscreen world map are the same projector with different params — plus the **FogOfWar** exploration-grid
data layer. UI-agnostic by contract: the module computes; consumers present.

**Depends on:** `CkCamera`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkEntityTag`, `CkLabel`, `CkLog`, `CkPoi`,
`CkPoiDisplayDefinition`, `CkRecord`, `CkUI` (reference widget only), `CkVisibleRange`.
**Used by:** game HUDs; the shipped `UCk_MinimapFrame_Widget` reference consumer.

**Two features, one module (deliberate):** fog's only consumers today are map projectors. Extraction
trigger: the first NON-map gameplay consumer of exploration state → extract `CkFogOfWar` into its own module.

---

## Fragment shape (2026-08-06 — spec-fragment-granularity P6)

The Spec is the authoring payload and is fully unpacked at `Add`; it is NOT retained wholesale.

- `ck::FFragment_Minimap_Params` — retained immutable residue ONLY: `_ProjectionMode`, `_FrameShape`,
  `_FixedBounds`, `_MaxEntries`, `_UpdateInterval`.
- `ck::FFragment_Minimap_Current` — everything request-mutable: the three seeded from the Spec at
  `Add` (`_ViewExtent`, `_RotationMode`, `_CategoryFilter`) plus the derived view state.

**Why `_CategoryFilter` moved.** It used to live in the Params fragment and be mutated *there* by
`Request_SetCategoryFilter` (`InParams.Set_CategoryFilter(...)`) — which is why HandleRequests took
Params `TReadWrite`. That made "Params" a mutable fragment in all but name, and left the module with
two contradictory policies: `_ViewExtent`/`_RotationMode` were copied into Current and their Spec
copies went stale, while `_CategoryFilter` was mutated in place. All three now have exactly one home
(Current), Params is genuinely immutable, and HandleRequests takes it `TReadOnly`. The authoring
ensures (positive ViewExtent / valid FixedBounds) moved to `Add`, where the Spec still exists.

---

## Minimap feature

- `UCk_Utils_Minimap_UE::Add(InHandle, InParams)` — composes the Minimap feature DIRECTLY onto `InHandle`
  (no child entity; one direct minimap per entity). The owner needs NO Transform — only the observer does.
- `Create(InLifetimeOwner, InParams)` — creates a NEW Minimap child entity under `InLifetimeOwner`,
  connected to the owner's RecordOfMinimaps. An owner hosts MULTIPLE minimaps this way (HUD minimap +
  world map = two children; children carry no label — no ByTag lookups, enumerate via `ForEach_Minimap`).
  The observer defaults to `InLifetimeOwner` (unlike the compass, which defaults to the created child).
- `FCk_Minimap_Spec` — `_ViewExtent` (world cm center→edge = the zoom; **ObserverCentric
  only** — FixedBounds projects through `Get_BoundsToFrame` and never reads it, so it is neither
  required nor ensured there. `_FixedBounds` is what FixedBounds actually requires),
  `_ProjectionMode` (`ObserverCentric` minimap / `FixedBounds` world map — IMMUTABLE post-Add, destroy +
  re-Add to change; `_FixedBounds` must be valid for FixedBounds), `_RotationMode`
  (`NorthLocked`/`RotateWithObserver`; FixedBounds is always north-up), `_FrameShape` (Rectangle/Circle —
  affects edge-clamp math only), `_MaxEntries`, `_CategoryFilter` (empty accepts everything), `_UpdateInterval`.
- Pull surface: `Get_Entries` (sorted priority-desc → distance-asc), `Get_ViewOrigin`/`Get_ViewYawDegrees`
  (as of the LAST update), `Get_ViewExtent`, `Get_FrameToWorld` (map-click → world XY; callers pick Z).
- Push surface (membership deltas ONLY): `BindTo_OnEntryAppeared` / `BindTo_OnEntryDisappeared`.
- Requests: `Request_SetViewExtent` (zoom; > 0 or rejected), `Request_SetCategoryFilter`,
  `Request_SetObserver`, `Request_SetRotationMode`, `Request_SetFogOfWar` (invalid handle = no fog culling).
  Every one of these forces an immediate reprojection — changes never wait out the throttle.
  Every one also takes its REQUEST STRUCT (`FCk_Request_Minimap_SetViewExtent{5000.0f}`), never a loose
  value — see the `Request_*` rule in [Source/CLAUDE.md](../CLAUDE.md).
- Pure math: `ck::minimap::` in `CkMinimap_Math.h`, unit-tested in `Tests/CkMinimap_Math.spec.cpp`
  (`Ck.CkMinimap.Math.*`).

## The delivery contract (read this before writing a consumer)

1. **Seed from pull, track deltas from push.** Signal replay stashes at most the LAST payload — a late
   binder must populate from `Get_Entries` (bind with `IgnorePayloadInFlight`, the UFUNCTION default here)
   and only then apply Appeared/Disappeared deltas.
2. **Entries are self-contained snapshots.** Every `FCk_Minimap_Entry` field is safe to render even if the
   POI died this frame. Any LIVE deref through `Get_Poi()` must be `ck::IsValid`-gated.
3. **Positions are data, not events.** Per-frame positions are pulled; the signals never fire per-frame.
4. Minimap state is derived per-client view data: **never replicated, never persisted** (the TRANSIENT
   record is an intent label). FogOfWar is the opposite — see below.
5. Dedicated servers never run the projection (`NetModeRequirement = CosmeticOnly`).
6. Consumers hosted in a `UWidgetComponent` may read one-frame-stale data — cosmetic only.
7. Projection runs in `FGroup_PostTransform` (final transforms + this frame's composed camera POV). A
   destroyed POI produces exactly one Disappeared at the next update; a dying minimap drains all entries
   via its EndPlay processor.
8. **`_UpdateInterval > 0` staleness is WHOLESALE** — view origin/yaw AND every entry position go stale
   together (unlike the compass, which refreshes its heading every frame). Default is 0 (every frame).
9. **The default observer is the LIFETIME OWNER** (the entity `Create` was called on) — the compass's
   `Create` defaults to the created child entity itself. `Request_SetObserver` with an invalid handle
   resets to the lifetime owner.
10. **Fog outside its bounds = explored.** Fog gates only the world rectangle it covers; a mis-sized fog
    fails VISIBLE (POIs show) instead of silently hiding them forever.
11. The per-POI projection is DATA-PARALLEL (`ParallelFor`, ≥64 POIs; inline below that): workers do pure
    registry reads — EntityTag/VisibleRange/DisplayDefinition state, owner transforms, the fog grid —
    and write disjoint index slots; sort/truncate/diff/signals — and every `Update_Distance` feed —
    stay on the calling thread. The worker body must never mutate ECS state or broadcast.
12. **VisibleRange integration (CkPoi v2 Gate 4).** Same contract as the compass (its point 9): inline
    max-range cull from VR config; base `FTag_VisibleRange_Hidden` as a per-worker skip (NOT a view
    exclude — hidden POIs must keep receiving the distance feed); per-consumer culls via a VR on the
    minimap's DD child; distance fed post-parallel; shared-VR multi-observer caveat applies.

## Frame-space / heading conventions

World yaw: **0 = North = +X, 90 = East = +Y** (matches CkCompass). Frame space: center-origin,
**+X = screen right, +Y = screen DOWN** (UMG), unit = half-frame → visible frame is [-1, 1]².
North-up mapping: `Frame = (ΔY/E, -ΔX/E)` — a POI due North renders screen-up. `RotateWithObserver`
rotates the world delta by -viewYaw first (observer's facing = screen up). Widgets place blips at
`_MapPosition x half-widget-size`.

## FogOfWar feature

Exploration is GAMEPLAY state — persists via the v3 handler (`Register_SaveOnly<FCk_RepData_FogOfWar>`,
phase 4 flips to net+save for co-op shared maps), accumulates on the authority too (processors declare
`NetModeRequirement = All`). Direct-attach fragment; one grid per map-area entity.

- `UCk_Utils_FogOfWar_UE::Add(InHandle, InParams)` — `_Bounds` (essential, must be valid), `_CellSize`,
  `_RevealRadius`, `_UpdateInterval` (reveal-sampling throttle, default 0.25s).
- Requests: `Request_AddRevealer`/`Request_RemoveRevealer` (entities whose Transforms auto-reveal),
  `Request_RevealLocation` (scripted), `Request_RevealAll`, `Request_Reset`, `Request_SetExplored`
  (bulk restore — the hydration vehicle; cell-count mismatch = ensure + keep the fresh grid).
  Reveal stamps are center-in-radius, but the cell CONTAINING the reveal point is ALWAYS revealed —
  a radius under half the cell diagonal (`CellSize·√2/2`) near a cell corner would otherwise reveal
  nothing, and "reveal this location" must never be a no-op inside the bounds.
- Queries: `Get_IsLocationExplored` (outside bounds = TRUE), `Get_ExploredFraction`, `Get_CellCounts`,
  `Get_ExploredData` (pull-seed for painters).
- Signals: `BindTo_OnCellsRevealed` (BATCHED cell indices per update — payload struct
  `FCk_FogOfWar_RevealedCells`), `BindTo_OnReset`. Painters seed from `Get_ExploredData`, then apply
  deltas — same last-payload-replay caveat as every Ck signal. A `Request_Reset` handled in the same
  drain DISCARDS the reveals batched before it: those cells are void, and painters re-seed off `OnReset`
  instead of receiving a CellsRevealed batch the grid no longer backs. A no-op reset (empty or
  fully-unexplored grid) never broadcasts.
- Fog processors run in a group BEFORE `FGroup_PostTransform` so the minimap's fog cull never reads a
  same-frame half-written grid. Only recipe-rebuildable fog persists (same coverage contract as CkPoi —
  fog composed on a bare-created entity orphans its payload on load).
- Fog VISUALS are out of scope here: consume `OnFogOfWarCellsRevealed` + `Get_CellUVRect` and paint into a
  CkRenderTarget (reference widget deliberately skips fog rendering).

## Map imagery

`UCk_Minimap_MapLayer_PDA` — `_Bounds` (DATA: feeds FixedBounds projection + fog grids) + `_MapTexture`
(OPAQUE to the data layer, presentation resolves it — same doctrine as CkPoi's `_DisplayAsset`). A baker
editor module (scene capture → these assets) is planned, not shipped.

---

## Reference widget (`UCk_MinimapFrame_Widget`)

Shipped as the worked example of the delivery contract, not as production HUD furniture. Design notes
that used to live in its class comment:

- **Pull-in-NativeTick is the documented exception.** CkUI doctrine routes per-frame widget updates
  through processors (see WorldSpaceWidget). This widget is the edge-consumer of the minimap PULL API —
  all math is precomputed by `FProcessor_Minimap_Update`, and the tick only writes UMG layout for its own
  pooled children. Games wanting processor-pushed HUDs consume the same API from their own processor.
- Appeared/Disappeared are bound (`IgnorePayloadInFlight`) and re-surfaced as Blueprint events purely to
  demonstrate the membership-delta contract; positioning never depends on them (blips are index-pooled
  against `Get_Entries` every frame).
- **Rectangle frames only.** A circular mask is presentation polish (material or retainer box); the DATA
  layer's clamp math already supports `ECk_Minimap_FrameShape::Circle`. Fog is likewise not painted here —
  real games seed from `Get_ExploredData`, apply `OnCellsRevealed` deltas, and composite via CkRenderTarget.
- **[EDITOR-VERIFY]** The base widget class carries `meta=(DisableNativeTick)`; class metadata is not
  inherited and is editor-only, so this subclass is expected to tick. Verify `NativeTick` actually runs in
  PIE on first integration; if it does not, drive the refresh from a timer or drop the base meta expectation.

---

## Anti-patterns

1. Don't broadcast or expect per-frame position events — pull them. The signals are membership deltas.
2. Don't seed pooled widgets from signal replay (last-payload-only) — seed from `Get_Entries`.
3. Don't persist or replicate Minimap fragments — recompute on every client from replicated POIs.
   (FogOfWar is the persistent one.)
4. Don't deref `FCk_Minimap_Entry::Get_Poi()` without `ck::IsValid` — the snapshot outlives the POI.
5. **Don't drive gameplay from entry membership at the MaxEntries cap** — a POI oscillating around rank N
   fires Appeared/Disappeared repeatedly (accepted flap; no hysteresis by design). Entries are UI data.
6. Don't call `Add` twice on the same entity — it composes directly (one direct minimap per entity) and the
   second call is rejected. Use `Create` for multiple maps on one owner (HUD minimap + world map).
7. Don't hand the minimap a fog handle from another world/registry — `Get_IsLocationExplored` ensures and
   degrades to "explored".

---

## See also

- `CkPoi/Claude.md` — the POI substrate (composition, state tags, persistence, replication).
- `CkCompass/CLAUDE.md` — the 1D sibling projector (this module mirrors its delivery contract).
- `CkRenderTarget` — where fog painting belongs (no doc yet).
- `CkSnapshot/Claude.md` — persistence-handler authoring recipe.
