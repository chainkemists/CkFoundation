# CkPoi v2 refactor — mission brief (PROMPT.md)

> **Written:** 2026-07-21. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** the campaign ships. On death: delete it, or replace the body with one
> tombstone line ("Superseded by X — kept for history") pointing at the final module `Claude.md`s.

## Goal

One entity can be a single Poi and simultaneously service multiple projectors (Compass, Minimap)
with independently-configured presentation and range culling per projector — without a bespoke
fragment owning fields that only make sense for one projector at a time. After this campaign:
`CkPoi` owns no fragment of its own (a thin meta-feature, like `CkProjectile`); presentation config
and range culling are separate, independently reusable modules; direct-attach onto an existing
entity (e.g. the Player) works with no restriction.

## Success criteria

1. `CkPoi_Fragment.h` has no `FFragment_Poi_Current`, no `FFragment_Poi_Requests`, no
   `FTag_Poi_NeedsSetup`/`FTag_Poi_Disabled`, no `FCk_RepData_Poi` — `UCk_Utils_Poi_UE::Add` adds
   only `ck::FTag_Poi` + composes `CkEntityTag`/`CkLabel`.
2. `CkPoiDisplayDefinition` and `CkVisibleRange` exist as independent modules, each compiling and
   passing their own AutoTests.
3. A Poi's base entity and any of its `PoiDisplayDefinition` children can independently compose
   `VisibleRange`; a parent going hidden forces all children hidden regardless of the children's
   own range (observed in an AutoTest, not just reasoned about).
4. `CkCompass` and `CkMinimap` build and pass their existing AutoTests/gyms against the new modules
   with zero dependency on `CkPoi`'s old fragment shape.
5. A gym demonstrates the Player composed as a Poi via direct-attach (no child entity spawned for
   it).
6. Full toolbox test gate (`ck-tests-authoring-and-running` invocation) green, diffed against the
   baseline captured at campaign start (see PROGRESS.md).

## Constraints & locked decisions

Regenerated from `REFACTOR_MultiProjectorPoi.md` (the design doc this campaign implements) at
campaign start — do not relitigate unless a real problem surfaces during a gate.

| # | Decision | Why |
|---|---|---|
| 1 | `CkPoi` is a meta-feature mirroring `CkProjectile` (`CkProjectile_Utils.cpp:16-32`) — adds `ck::FTag_Poi` + composes `CkEntityTag` category + optional `CkLabel`. No bespoke fragment. | Confirmed precedent; reuse over bespoke (root CLAUDE.md non-negotiable #9) |
| 2 | Direct-attach onto an existing entity is the **primary** mode, unrestricted (includes the Player). Standalone/ping path composes a bare Transform child directly — no `CkTargeting` dependency. | `CkTargetPoint` has no identity data and pulls an unrelated dependency chain |
| 3 | `CkPoiDisplayDefinition` (new module): fragment (`_Consumer` tag, `_DisplayAsset`, `_Priority`, `_OffscreenPolicy`) + `Add`/`Create` (Create = child entity + owner's `RecordOfPoiDisplayDefinitions`, for the multi-consumer case). No processors of its own except the cascade tag (#5). | One Poi, many projector-specific presentations |
| 4 | `CkVisibleRange` (new module): Params (`_MinRange`/`_MaxRange`/`_FadeBandCm`/`_UpdateInterval`), Current (`FCk_Chrono` + `_FadeAlpha` + per-source bookkeeping bools). Reference-counted tag `FTag_VisibleRange_Hidden` (`CK_DEFINE_ECS_TAG_COUNTED`) with **exactly two** internal vote sources: own-range boundary crossing, and an explicit `ECk_VisibleRange_ShowHide` request. **Zero knowledge of Poi/PoiDisplayDefinition/Records** — fully standalone, reusable outside Poi. Fires `OnVisibleRange_HiddenChanged(Handle, bool)` on transition. | Reuse over bespoke; genuinely useful outside Poi (aggro ranges, ambient fade-in) |
| 5 | **Parent→child cascade lives in `CkPoiDisplayDefinition`, not `CkVisibleRange`** (refinement made during gate planning, 2026-07-21 — behavior unchanged from the design doc, only the ownership moved). `CkPoiDisplayDefinition` depends on `CkVisibleRange`, binds `OnVisibleRange_HiddenChanged` on the owning Poi entity (when the Poi itself composes `VisibleRange`), and on transition walks its own `RecordOfPoiDisplayDefinitions` setting/clearing a **separate, plain (non-counted)** tag `FTag_PoiDisplayDefinition_ParentHidden` on each child. Consumers exclude **both** tags in their view (`TExclude<FTag_VisibleRange_Hidden, FTag_PoiDisplayDefinition_ParentHidden>`). | Keeps `CkVisibleRange` fully decoupled from Poi — the cascade is Poi-specific wiring, not a VisibleRange concern. A child needs only one bit of parent-state, not the counted-vote complexity a self-contained VisibleRange instance needs internally |
| 6 | `ck::cadence::ShouldRun(FCk_Chrono&, FCk_Time) -> bool` — new shared primitive, home in `CkCore/Chrono`. Wraps `FCk_Chrono::Tick()` with the "0 = every tick" convention. `CkVisibleRange`'s Update processor uses it. | Replaces the hand-rolled `_TimeSinceUpdate` pattern already duplicated 3+ times |
| 7 | `CkPoi`'s `Register_SaveOnly<FCk_RepData_Poi>` persistence handler is **deleted outright, not replaced** — `CkEntityTag` already has its own (`CkEntityTag_Fragment.cpp:55`), so migrated enable/disable + state tags persist for free. | Verified fact, not an assumption |
| 8 | Gyms updated to add the Player as a Poi via direct-attach. | Acceptance test for constraint #2 |

## Non-goals

- **Retrofitting existing hand-rolled cadence accumulators** (`CkCompass`, `CkMinimap`,
  `CkFogOfWar`, `CkCrowd`, `CkGoap`, `CkCore`'s ReplicationLeakWatch) to use
  `ck::cadence::ShouldRun` — separate follow-up task, not blocking this campaign.
- **Viewer-resolution design** (which camera/local-player a `VisibleRange` instance measures
  distance against) — reuse Compass/Minimap's existing viewer-resolution logic as-is; not
  redesigned here.
- **`CkFogOfWar`** is in scope only if Gate 4 finds it directly touches `CkPoi`'s old fragment
  (unconfirmed as of campaign start — check during that gate, don't assume either way).

## Reading list

- `REFACTOR_MultiProjectorPoi.md` (this module) — the full design this campaign implements; read
  in full before Gate 1.
- Reference modules to mimic: `CkTimer` (canonical small quartet), `CkProjectile`
  (`CkProjectile_Utils.cpp:16-32` — the meta-feature pattern), `CkSubstep`
  (`CkSubstep_Fragment.h:9-10` — tag-swap-on-state-change pattern), `CkPoi`'s own current code
  (what's being replaced — read before deleting, not after).
- `CkEntityTag_Fragment.cpp:55` — persistence handler precedent (constraint #7).
- [PLAN.md](PLAN.md) — gate index.

## Things ruled out — do not re-investigate

| Ruled out | Why | Evidence |
|---|---|---|
| Collapsing Poi's position into `CkTargetPoint`/`CkTargeting` | No identity data on `CkTargetPoint`; `CkTargeting` is an unrelated, heavier dependency (Actor/Provider/Record) | `CkTargetPoint_Utils.h:24-78` |
| VisibleRange as pure data with no processor | Needs to actively maintain the visible/hidden tag over time and cadence | Corrected in the cadence discussion, folded into design doc |
| VisibleRange override-with-fallback (child fully replaces parent state) | User wants Unreal-SceneComponent-style cascade (parent-hidden forces child-hidden even if child's own range says visible) | User directive, this session |
| A single counted tag spanning both VisibleRange's own state and the parent cascade | Couples VisibleRange to Poi/Records, breaking standalone reuse | Gate-planning refinement, decision #5 above |
