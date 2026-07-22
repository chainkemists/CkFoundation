# CkCompass

**Purpose:** Per-observer 1D projection of world POIs (CkPoi) onto a compass strip — heading + a sorted
list of self-contained `FCk_Compass_Entry` snapshots (bearing, normalized arc offset, clamp state,
distance, elevation delta, priority). UI-agnostic by contract: the module computes; consumers present.

**Depends on:** `CkCamera`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkEntityTag`, `CkLog`, `CkPoi`,
`CkPoiDisplayDefinition`, `CkRecord` (link-level — see its Build.cs comment), `CkUI` (reference
ribbon widget only), `CkVisibleRange`.
**Used by:** game HUDs; the shipped `UCk_CompassRibbon_Widget` reference consumer.

---

## Key API

- `UCk_Utils_Compass_UE::Add(InHandle, InParams)` — composes the Compass directly onto an entity
  (typically the local player's pawn/controller entity; one compass per entity). Observer defaults to
  that same entity; redirect via `Request_SetObserver`.
- `Create(InLifetimeOwner, InParams)` — creates a NEW Compass child entity under `InLifetimeOwner`
  (Compass has no record — no record wiring). Observer defaults to the created child entity itself;
  redirect via `Request_SetObserver`.
- `FCk_Fragment_Compass_ParamsData` — `_ArcDegrees` (essential), `_MaxEntries`, `_CategoryFilter`
  (`FGameplayTagQuery`; **empty accepts everything**), `_HeadingSource`
  (`Auto`/`CameraView`/`EntityTransform`/`Manual`), `_UpdateInterval` (projection throttle; heading is
  NEVER throttled).
- Pull surface (per-frame): `Get_Heading` (0-360, feed this to a ribbon material scalar),
  `Get_Entries` (sorted priority-desc → distance-asc), `Get_ArcDegrees`, `Get_CardinalDirection`
  (pure; reuses CkCore's `ECk_CardinalAndOrdinalDirection`).
- Push surface (membership deltas ONLY): `BindTo_OnEntryAppeared` / `BindTo_OnEntryDisappeared`.
- Requests: `Request_SetCategoryFilter`, `Request_SetManualHeading`, `Request_SetObserver`.
- Pure math: `ck::bearing::` in `CkCore/Math/Bearing/CkBearing_Utils.h` (bearing/arc/cardinal), unit-tested in
  `Tests/CkBearing_Utils.spec.cpp` (`Ck.CkCompass.Bearing.*`).

## The delivery contract (read this before writing a consumer)

1. **Seed from pull, track deltas from push.** Signal replay stashes at most the LAST payload — a late
   binder must populate from `Get_Entries` (bind with `IgnorePayloadInFlight`, the UFUNCTION default
   here) and only then apply Appeared/Disappeared deltas.
2. **Entries are self-contained snapshots.** Every `FCk_Compass_Entry` field is safe to render even if
   the POI died this frame. Any LIVE deref through `Get_Poi()` (state tags, display asset) must be
   `ck::IsValid`-gated.
3. **Positions are data, not events.** Per-frame position/heading is pulled; the signals never fire
   per-frame.
4. Compass state is derived per-client view data: it is **never replicated and never persisted**.
5. Dedicated servers never run the projection (`NetModeRequirement = CosmeticOnly`).
6. Consumers hosted in a `UWidgetComponent` may read one-frame-stale data (component tick order vs the
   ECS pump is not defined within the tick group) — cosmetic only.
7. Projection runs in `FGroup_PostTransform` (final transforms + this frame's composed camera POV, same
   slot as the WorldSpaceWidget projector). A destroyed POI produces exactly one Disappeared at the
   next update; a dying compass drains all entries via its EndPlay processor.
8. The per-POI projection is DATA-PARALLEL (`ParallelFor`, ≥64 POIs; inline below that): workers do pure
   registry reads and write disjoint index slots; sort/truncate/diff/signals — and every
   `Update_Distance` feed — stay on the calling thread. The worker body must never mutate ECS state or
   broadcast.
9. **VisibleRange integration (CkPoi v2 Gate 4).** Range/fade CONFIG is read inline from the POI's
   `CkVisibleRange` params (absent = unlimited) for blip-free same-frame culling; VisibleRange STATE is
   also consumed — base-entity `FTag_VisibleRange_Hidden` (explicit `Request_SetVisibility` or the fed
   range vote) skips the entry per-worker (deliberately NOT a view exclude: hidden POIs must keep
   receiving the distance feed or they could never re-enter range), and a VisibleRange on THIS
   consumer's `PoiDisplayDefinition` child culls only the compass entry (per-consumer restriction,
   one-frame latency). The projector feeds observer distance post-parallel into the base VR and its
   consumer child's VR. Caveat: projectors with DIFFERENT observers sharing one base VR would fight
   over its distance — per-consumer differences belong on the DD children.

## Heading convention

Heading/bearing are world yaws in degrees: **0 = North = +X, 90 = East = +Y** (UE yaw growth). Signed
bearings are shortest-path `[-180, 180]`; normalized offsets are `bearing / (arc/2)` clamped `[-1, 1]`.

---

## Anti-patterns

1. Don't broadcast or expect per-frame position events — pull them. The signals are membership deltas.
2. Don't seed pooled widgets from signal replay (last-payload-only) — seed from `Get_Entries`.
3. Don't persist or replicate compass fragments — recompute on every client from replicated POIs.
4. Don't give the compass its own tick-throttled heading — `_UpdateInterval` gates only the O(POIs)
   projection; heading updates every frame by design (throttled heading = visible ribbon stutter).
5. Don't deref `FCk_Compass_Entry::Get_Poi()` without `ck::IsValid` — the snapshot outlives the POI.

---

## See also

- `CkPoi/Claude.md` — the POI substrate (composition, state tags, persistence, replication).
- `CkUI/Claude.md` — WorldSpaceWidget (in-world indicators; the processor-push house pattern).
- `CkCamera/Claude.md` — the composed-POV pipeline the heading reads.
