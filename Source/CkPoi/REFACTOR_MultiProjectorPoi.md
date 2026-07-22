# CkPoi Refactor v2 — Design Doc

**Status:** design only, not implemented. Supersedes `6c98baeb2` (2026-07-20), most of which was
reverted same-day by `159983575` and `7e8347b75`. This doc exists so a future implementation
session doesn't repeat the same undo. Updated after a follow-up discussion on `CkVisibleRange`
cadence/scale — see that section; it corrects the original "no processor" call.

## Motivation

An entity needs to be **one Poi** but visible through **multiple projectors** (Compass + Minimap)
with different presentation per projector (different icon, priority, offscreen behavior, visible
range). The current `FCk_Fragment_Poi_ParamsData` is a single fragment carrying both identity data
and presentation data, one-per-entity — it structurally cannot express "this NPC shows differently
on the compass than on the minimap."

## Root cause of the bloat

Every field on the current fragment already has, or should have, a home elsewhere:

| Field | Current role | New home | Why |
|---|---|---|---|
| `_Category` (FGameplayTag) | filter key, single-valued | **CkEntityTag** — `Poi.Category.*` gameplay tags via `Add_UsingGameplayTag` | Multi-category becomes possible for free; `UCk_Utils_EntityTagQuery_UE` replaces the bespoke `_CategoryFilter` compass logic |
| `_DisplayName` (FText) | label | **CkLabel** — `Add(Handle, Tag)` | CkLabel is already the house one-per-entity role identifier; a second name field is duplication |
| `_Priority` | sort key for crowded projector views | **CkPoiDisplayDefinition** (new, per-consumer) | Priority is inherently per-projector — an NPC can rank high on the minimap and low on the compass |
| `_MaxVisibleRange` / `_MinVisibleRange` / `_RangeFadeBandCm` | range cull + fade | **new module `CkVisibleRange`** | Reusable outside Poi (aggro-range indicators, ambient-trigger fade-in, LOD-style visibility) — exactly the case the project's reuse tenet exists for |
| `_OffscreenPolicy` | Hide vs ClampToEdge | **CkPoiDisplayDefinition** (new, per-consumer) | Presentation behavior, projector-specific — same reasoning as Priority |
| `_RelativeLocation` | owner-space offset | **removed** | See "Position" below |
| `_DisplayAsset` | soft ref to icon/tint/size PDA | **CkPoiDisplayDefinition** (new, per-consumer) | Already documented opaque-to-gameplay; now lives where it's actually consumed instead of transiting through Poi |
| `_StateTags` (Current) | ad hoc gameplay state | **CkEntityTag** | State tags and category tags are the same mechanism; no bespoke container needed |
| `FTag_Poi_Disabled` + `Request_EnableDisable` + `OnPoiEnableDisable` | enable/disable | **CkEntityTag** — add/remove a `Poi.Disabled` tag, consume its existing `OnTagUpdated` signal | One less bespoke request/signal pair for a concept CkEntityTag already owns |

Net result: **CkPoi has no fragment of its own left to justify existing as a feature.** What remains
is composition of existing features plus one new small module for the part that's genuinely
projector-specific presentation data.

## Position: why "Poi becomes CkTargetPoint" needs a caveat

`CkTargetPoint` (`CkTargeting/Public/TargetPoint/CkTargetPoint_Utils.h`) today is **not** a feature
with fragment/identity data — it is purely `Create*(Owner, Transform|Location[, Rotation], Lifetime)`
factory functions that spawn a child entity carrying only `CkEcsExt`'s Transform fragment. It also
lives inside `CkTargeting` (candidate scoring for abilities/AI — deps on `CkActor`, `CkProvider`,
`CkRecord`), a domain that has nothing to do with POIs. Pulling `CkTargeting` into every Poi
consumer to get one factory function is a heavier, semantically-wrong dependency.

The premise is right in spirit but only applies to one of Poi's two existing paths:

- **`Add` (direct-attach, e.g. an NPC that already has a Transform):** no position primitive needed
  at all — the entity's own Transform IS the position. `_RelativeLocation` disappears because there
  is no more owner-relative offset; being a Poi never moves an entity's origin.
- **`Create` (standalone/ping, no owning gameplay entity, e.g. a player-placed map marker):** this
  is exactly `CkTargetPoint::Create_FromLocation`'s job — spawn a bare child entity with a Transform
  at an absolute world location, then tag it as a Poi.

**Recommendation (confirmed):** don't take a `CkTargeting` dependency for this. Have the ping path
compose `CkEcsExt`'s Transform fragment directly on a plain child entity
(`UCk_Utils_EntityLifetime_UE::Request_CreateEntity` + `Transform::Add`) — the same two calls
`CkTargetPoint::Create_FromLocation` makes internally, without inheriting `CkTargeting`'s unrelated
dependency chain.

This is the exception path, not the rule — `Poi::Add` is unrestricted about what it's attached to.
It composes onto **any** entity that already has a Transform, existing or newly spawned. Concretely:
the gyms should add the Player as a Poi (`UCk_Utils_Poi_UE::Add(PlayerHandle, Params)` on the
player's existing entity) as both a real usage example and an acceptance test that direct-attach
works on an entity CkPoi didn't create.

## New shape

### "Is a Poi" — no bespoke identity marker

An entity is a Poi by having **at least one `Poi.Category.*` CkEntityTag** and a Transform. No
`FTag_Poi_*`, no dedicated identity fragment. `UCk_Utils_Poi_UE` (if it survives at all — see Open
Calls) would shrink to thin convenience wrappers: tag-namespace constants and a
`Query_AllPois()`/`Has(Handle)` that delegate straight to `CkEntityTag`.

### `CkPoiDisplayDefinition` (new, small, no processors)

Per-projector presentation config. Fragment + Utils only — it's static data a projector reads, not
something that needs per-tick logic.

```
FCk_Fragment_PoiDisplayDefinition_ParamsData
    _Consumer        FGameplayTag              // e.g. Poi.Consumer.Compass / Poi.Consumer.Minimap
    _DisplayAsset     TSoftObjectPtr<UCk_Poi_DisplayDefinition_PDA>   // unchanged, still opaque to gameplay
    _Priority         int32
    _OffscreenPolicy  ECk_Poi_OffscreenPolicy  // unchanged enum, moves here
```

- `Add(Handle, Params)` — single projector, direct-attach (the common case).
- `Create(LifetimeOwner, Params)` — child entity + `RecordOfPoiDisplayDefinitions` on the owner,
  used exactly when an entity needs more than one (multi-projector case). This is the existing
  Ck composition ritual (`Source/CLAUDE.md` § "Add a feature to an entity") applied unchanged — no
  new pattern invented.
- Compass/Minimap each query the owner's `RecordOfPoiDisplayDefinitions`, filter by their own
  `_Consumer` tag, and read Priority/OffscreenPolicy/DisplayAsset from the matching entry.

### `CkVisibleRange` (new module) — now with a real processor

Correction from the first draft of this doc: range culling needs to actively track state (an
entity crosses in/out of range over time) and expose that state as a **real ECS tag**, not a value
field, so consumers can build a fast filtered view instead of branching per-entity at scale. That
requires a processor after all.

```
FCk_Fragment_VisibleRange_ParamsData
    _MinRange        float      // cm, 0 = none
    _MaxRange        float      // cm, 0 = unlimited
    _FadeBandCm      float      // cm, 0 = hard cut
    _UpdateInterval  FCk_Time   // 0 = every tick; per-entity cadence, see below

FFragment_VisibleRange_Current
    _Chrono          FCk_Chrono  // per-entity cadence accumulator (CkCore/Chrono)
    _FadeAlpha       float       // last computed value, read by consumers
```

**Tag:** `FTag_VisibleRange_Hidden` via **`CK_DEFINE_ECS_TAG_COUNTED`**, not the plain variant —
deliberate choice, see Parent→child cascade below. Absence (count == 0) = visible. Consumers
exclude it in their entt view (`TExclude<FTag_VisibleRange_Hidden>`) instead of
computing/branching per entity every frame — same query shape as the plain-tag case
(`FTag_Poi_Disabled`, `CkPoi_Fragment.h:25`); only the add/remove side is reference-counted.

**Processors:**
- `FProcessor_VisibleRange_Update` — `_TickRate = 0` (per-entity cadence can't be expressed as a
  single uniform processor rate — see cadence discussion below). Per entity: gate the work on
  `ck::cadence::ShouldRun(Current._Chrono, DeltaT)`; when due, compute distance-to-viewer and fade
  alpha; if the in/out-of-range boundary crossed, enqueue a request (mutations stay deferred per
  the framework's requests-are-deferred rule).
- `FProcessor_VisibleRange_HandleRequests` — applies the request: increments/decrements
  `FTag_VisibleRange_Hidden` on the entity itself. **If that entity owns a
  `RecordOfPoiDisplayDefinitions`** (i.e. it's a base Poi with children), it also
  increments/decrements the same tag on every child — one more independent "vote" toward that
  child's hidden count. A child's own boundary-crossing (its own `VisibleRange`, if it has one)
  contributes its own independent vote the same way.

**Explicit show/hide override (a third vote source).** Distance isn't the only reason something
should be hidden — a quest marker deliberately hidden until unlocked, e.g. — so `CkVisibleRange`
also exposes a directly-requested override, house-style as an enum + request rather than a bool
(`ECk_EnableDisable` precedent):

```
UENUM(BlueprintType)
enum class ECk_VisibleRange_ShowHide : uint8 { Show, Hide };

FCk_Request_VisibleRange_SetVisibility : FCk_Request_Base
    _ShowHide  ECk_VisibleRange_ShowHide
```

`UCk_Utils_VisibleRange_UE::Request_SetVisibility(Handle, ECk_VisibleRange_ShowHide)` is a third
independent vote on the same counted `FTag_VisibleRange_Hidden`, alongside the entity's own
range-boundary crossing and (for children) the parent cascade. It cascades identically — an
explicit `Hide` on a base Poi propagates to its display-definition children exactly like an
auto-range hidden state does, since propagation walks `RecordOfPoiDisplayDefinitions` regardless of
which source triggered the change.

With three possible sources, each one must track **whether it currently holds a vote** (e.g.
`_IsOutOfRange`, `_IsExplicitlyHidden`, and — children only — `_IsParentHidden`, all bools on
`FFragment_VisibleRange_Current`) so a repeated identical request doesn't leak an extra increment,
and `Show` clears exactly the one vote that specific source added. The plain-boolean-tag pitfall
from earlier applies per-source now, not just between the two original sources.

**Cadence — a shared primitive, not a `CkVisibleRange`-only concern.** `TProcessorBase::_TickRate`
(`CkEcs/Processor/CkProcessor.h:81,186-199`) was considered and rejected for this: it throttles an
entire processor to one uniform rate, and if set it *catches up* by replaying `DoTick` multiple
times per frame — a fixed-timestep tool, not a work-skipping one. Variable per-entity cadence
(entity A refreshes every frame, entity B every 2s) is inherently per-entity data, not something a
processor-wide knob can express.

Instead: `ck::cadence::ShouldRun(FCk_Chrono& InOutChrono, FCk_Time InDeltaT) -> bool` — one small
free function (home TBD: `CkCore/Chrono/CkChrono_Utils.h` or a new `CkCore/Cadence/`), wrapping
`FCk_Chrono::Tick()` with the "0 = every tick" convention already used elsewhere in this doc.
It replaces the hand-rolled `_TimeSinceUpdate += DeltaT; if (...) return;` block already duplicated
three times in the codebase (`CkCompass_Processor.cpp:178-185`, `CkMinimap_Processor.cpp`, and
`CkCrowd`'s repath/diagnostic-sample accumulators) — a direct instance of non-negotiable #9 (reuse
before bespoke). Retrofitting those three existing call sites to use it is optional and out of
scope for this refactor (see Open Calls).

**Thundering-herd guard:** seed each entity's `_Chrono` with a small phase offset (hash of entity
ID mod `_UpdateInterval`) at `Add`-time, so a wave of Poi's composed in the same frame don't all
recompute — and all mutate the tag — on the same frame forever after. One line at composition
time, no processor change.

**Not resolved here:** which "viewer" position a given `VisibleRange` instance measures against
(local player camera today; which one once split-screen/multiple local players are in scope) is a
separate design question.

**Composable at either level, no restriction:** `VisibleRange::Add` can go on the base Poi entity
(one shared range for every display definition), on an individual `PoiDisplayDefinition` child
(a per-consumer additional restriction), or both.

**Resolution: cascade, like Unreal SceneComponent visibility — not override.** A parent going
out-of-range hides every child regardless of the child's own range; a child can independently be
*more* restrictive than its parent (its own range hides it while the parent is still visible), but
a child can never be *less* restrictive — it cannot show itself when the parent says hidden. This
is why the tag is reference-counted rather than plain: "hidden" has up to two independent sources
(the entity's own range, and — for children — the parent's), and either one alone must be enough to
hide it, while both need to clear before it's visible again. A plain boolean tag would let one
source's "clear" wipe out the other source's still-active "hidden," which is exactly wrong here.

Two gotchas for whoever implements this (not open design questions, just easy to miss):
- **Seeding at `Create` time:** a `PoiDisplayDefinition` child created under a Poi that is
  *already* hidden must start with the inherited vote pre-applied, not wait for the next boundary
  crossing — otherwise it's briefly visible for up to one `VisibleRange` cadence interval after
  creation.
- **Cleanup if the Poi's `VisibleRange` is removed** (not just toggled hidden/visible, but the
  fragment itself goes away): the propagation source must actively withdraw its vote from every
  child, or they're permanently stuck with one extra hidden-count they can never clear.

### `CkPoi` — after: a meta-feature, mirroring `CkProjectile`

Confirmed precedent: `UCk_Utils_Projectile_UE::Add` (`CkProjectile_Utils.cpp:16-32`) owns **no
bespoke fragment data** — it adds one identity tag (`InHandle.Add<ck::FTag_Projectile>()`) and
composes existing features (`Velocity::Add`, `Acceleration::Add`, `AutoReorient::Add`, then starts
their integrators). `CkPoi` follows the same shape — an accelerant, not a feature:

```cpp
auto UCk_Utils_Poi_UE::Add(FCk_Handle& InHandle, const FCk_Fragment_Poi_ParamsData& InParams) -> FCk_Handle_Poi
{
    InHandle.Add<ck::FTag_Poi>();                                              // identity tag — cheap membership test, no CkEntityTag lookup needed
    UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(InHandle, InParams.Get_Category());
    if (InParams.Get_Label().IsValid())
    { UCk_Utils_GameplayLabel_UE::Add(InHandle, InParams.Get_Label()); }        // optional
    return Cast(InHandle);
}
```

`ck::FTag_Poi` is a genuine `CK_DEFINE_ECS_TAG`, separate from the `Poi.Category.*` CkEntityTag
tags — it answers "is this entity a Poi at all" via cheap view membership, the same role
`FTag_Projectile` plays for CkProjectile; category answers "which kind," via CkEntityTag as
already designed. `PoiDisplayDefinition`/`VisibleRange` composition stays a separate explicit call
(`UCk_Utils_PoiDisplayDefinition_UE::Add`/`Create`) — `Poi::Add` doesn't force either, since a bare
Poi with no display definition yet (or ever, e.g. a gameplay-only marker) is valid.

**Primary mode is direct-attach onto an entity that already exists** — this is not a fallback path,
it's the common case (an NPC, or the player). Composing a standalone/detached Poi (a ping with no
owning gameplay entity) is the minority case; see Position below.

Deleted entirely: `FFragment_Poi_Current`, `FFragment_Poi_Requests`, `FTag_Poi_NeedsSetup`,
`FTag_Poi_Disabled`, all four `FCk_Request_Poi_*` structs, both `OnPoi*` signals,
`FProcessor_Poi_Setup`, `FProcessor_Poi_HandleRequests`, `FCk_RepData_Poi`, `RecordOfPois`.
`CkPoi_DisplayDefinition.h` (the `UCk_Poi_DisplayDefinition_PDA` data asset) is unchanged and moves
under the new `CkPoiDisplayDefinition` module.

## Composition example (the motivating case)

```cpp
// NPC is already a Transform-bearing entity (normal actor-bridge composition).

UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(NpcHandle, TAG_Poi_Category_Npc); // "is a Poi", filterable

UCk_Utils_PoiDisplayDefinition_UE::Create(NpcHandle, {
    .Consumer = TAG_Poi_Consumer_Compass, .DisplayAsset = NpcCompassIcon,
    .Priority = 5, .OffscreenPolicy = ECk_Poi_OffscreenPolicy::ClampToEdge });

UCk_Utils_PoiDisplayDefinition_UE::Create(NpcHandle, {
    .Consumer = TAG_Poi_Consumer_Minimap, .DisplayAsset = NpcMinimapIcon,
    .Priority = 3, .OffscreenPolicy = ECk_Poi_OffscreenPolicy::Hide });

// Range culling only where a given consumer needs it — composed on that display child, not the NPC:
UCk_Utils_VisibleRange_UE::Add(CompassDisplayChildHandle, { .MinRange = 0, .MaxRange = 5000, .FadeBandCm = 200 });
```

One entity, one set of category tags, two independently-configured projector views. Naming (`Add`
vs `Create` argument shapes) is illustrative, not final API.

Direct-attach onto an existing entity (the Player, in a gym) — no child entity, no CkTargetPoint:

```cpp
UCk_Utils_Poi_UE::Add(PlayerHandle, { .Category = TAG_Poi_Category_Player });
UCk_Utils_PoiDisplayDefinition_UE::Add(PlayerHandle, {
    .Consumer = TAG_Poi_Consumer_Minimap, .DisplayAsset = PlayerBlip, .Priority = 10 });
UCk_Utils_VisibleRange_UE::Add(PlayerHandle, { .MinRange = 0, .MaxRange = 0 }); // unlimited — always shown to self
```

## Module/dependency impact

| Module | Before | After |
|---|---|---|
| `CkPoi` | Core,Ecs,EcsExt,Label,Log,Record,Settings,Timer | Core,Ecs,EntityTag,Log (or removed — see Open Calls) |
| `CkPoiDisplayDefinition` (new) | — | Core,Ecs,EcsExt,Log,Record,Settings |
| `CkVisibleRange` (new) | — | Core,Ecs,EcsExt,Log (EcsExt for reading the entity's Transform in the distance calc) |
| `CkCompass` | Camera,Core,Ecs,EcsExt,Log,Poi,UI | swaps `Poi` dep for `EntityTag`+`PoiDisplayDefinition`(+`VisibleRange` if it reads range itself) |
| `CkMinimap` | Camera,Core,Ecs,EcsExt,Label,Log,Poi,Record,UI | same swap as Compass |

## Resolved (2026-07-21 discussion)

1. **`CkPoi` survives, as a meta-feature** — mirrors `CkProjectile` (`CkProjectile_Utils.cpp:16-32`),
   confirmed to own no bespoke fragment data itself; it adds one identity tag and composes existing
   features. See "`CkPoi` — after" above.
2. **Ping/standalone Transform-child path is the exception, not the rule.** Direct-attach onto an
   existing entity (including entities CkPoi didn't create — the Player, in a gym) is the primary,
   unrestricted mode. No `CkTargeting` dependency either way.
3. **Persistence is already solved — no new work needed.** `CkEntityTag` already has a registered
   handler: `FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_SaveData_EntityTags>`
   (`CkEntityTag_Fragment.cpp:55`). Once Poi's enable/disable and state tags move onto `CkEntityTag`,
   they're saved/restored automatically — nothing to build. `CkPoi`'s own
   `Register_SaveOnly<FCk_RepData_Poi>` handler (`CkPoi_Fragment.cpp:29`) is deleted outright, not
   replaced.
4. **`VisibleRange` two-level resolution: parent→child cascade, not override.** Composable at both
   the base Poi and any `PoiDisplayDefinition` child, no restriction. A reference-counted
   `FTag_VisibleRange_Hidden` (`CK_DEFINE_ECS_TAG_COUNTED`) carries independent "hidden" votes from
   the entity's own range and (for children) its parent's — matching Unreal SceneComponent
   visibility semantics: parent-hidden forces child-hidden regardless of the child's own range;
   a child can independently be more restrictive but never less. See `CkVisibleRange` above for the
   mechanism and its two implementation gotchas (creation-time seeding, cleanup on removal).

## Open calls (confirm before implementing)

None currently — all four prior open calls resolved above.

## Follow-ups (not blocking this refactor)

- **Cadence retrofit, broadened per full-codebase audit.** Once `ck::cadence::ShouldRun` exists,
  retrofit every hand-rolled "don't run every frame" accumulator to use it:
  - `CkCompass_Processor.cpp` (`_TimeSinceUpdate`)
  - `CkMinimap_Processor.cpp` (`_TimeSinceUpdate`)
  - `CkFogOfWar_Processor.cpp` (`_TimeSinceUpdate`)
  - `CkCrowdAgent_BlockDetect_Processor.cpp` (`_SampleAccumulatorSec`)
  - `CkGoap_Action_Processor.cpp` (`_SecondsSinceLastReplan`, replan throttle)
  - `CkReplicationLeakWatch_WorldSubsystem.cpp` (`_SampleIntervalSeconds`, CVar-driven)

  **Explicitly excluded** — same superficial shape (`FCk_Time` accumulator + compare) but different
  semantics (timeout/stall detection, fire-once-and-escalate, not recurring skip-then-reset):
  `CkReplicatedFragmentContainer_Processor.cpp` and `CkPersistenceHydration_Processor.cpp`
  (`_PendingForSeconds`), `CkEntityReplicationDriver_Processor.cpp` (`_Seconds`, FireGateStall).
  Don't fold these into the cadence primitive — converting a timeout detector into a throttle-and-
  reset helper would change its behavior.
- **Bucketed-cadence scheduler primitive (design recorded 2026-07-21, not scheduled).** Replace the
  per-entity cadence poll (`ck::cadence::ShouldRun` ticked for every entity every frame) with cadence
  buckets: quantize `_UpdateInterval` at `Add` into a fixed bucket set (quantize toward faster),
  tag each entity with its bucket (`FTag_<Feature>_Cadence_<Bucket>`), and run one sub-processor per
  bucket with `TProcessorBase::_TickRate` set to the bucket interval — the scheduler's empty-view
  skip makes vacant buckets near-free, and the per-entity `FCk_Chrono` + `ShouldRun` path disappears.
  Scope it as a FRAMEWORK primitive (it subsumes the cadence-retrofit list above), not a
  CkVisibleRange-local change. Two prerequisites before building it:
  1. A phase-offset knob in `TProcessorBase` — all entities in a bucket evaluate on the same tick,
     losing the natural per-entity stagger the `.Complete()` Add-seed provides today, so big buckets
     become frame spikes (300 entities in a 1s bucket = 300 evaluations one frame, 0 for ~59).
     `_RemainingDeltaTFromLastFrame` is private with no offset support (`CkProcessor.h:82`).
  2. A benchmark at representative Poi counts proving the poll registers at all — per skipped entity
     it costs one chrono tick over packed storage, and the due-work (fade + range compare) is barely
     bigger, so the justification is ecosystem-wide cleanup + large-N headroom, not measured perf.
  Also account for the `_TickRate` catch-up loop (`CkProcessor.h:194-200`) replaying `DoTick` after a
  hitch — pure waste for sampling (non-integrating) processors; cap it or accept it.
- Update the gyms to add the Player as a Poi (see composition example above) — both documentation
  and an acceptance test for the direct-attach-onto-existing-entity path.
