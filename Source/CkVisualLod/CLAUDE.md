# CkVisualLod

**Purpose:** Budgeted skeletal-mesh visual LOD. Entities in a LOD domain switch between a pooled
SKMC `IskmProxy` (near), a GPU batched crowd member (far, `ACk_Iskm_BatchedCrowd_Actor`), and
hidden — with ranked in-view-first promotion, promote/demote hysteresis, dither crossfades,
promote locks (ragdoll/montage holders), reserved lock budget, and per-domain crowd pools. An
**arbiter** entity per domain owns budgets/pools/view; **member** entities opt in and reference
their domain by gameplay tag (explicit `Request_SetArbiter` overrides).

**Status:** active framework module. Campaign history and remaining manual evidence live in
[PLAN.md](../CkVisualLod/PLAN.md). Design of record: [DESIGN_CkVisualLod.md](DESIGN_CkVisualLod.md).

**Depends on:** `CkCamera` (view info), `CkCore`, `CkEcs`, `CkEcsExt`, `CkIskmRenderer` (the
mechanism APIs this module orchestrates), `CkLog`, `CkPhysics` (planar speed for far-anim),
`CkResourceLoader`.

---

## Key API (Gate 0 surface)

- `UCk_Utils_VisualLodArbiter_UE::Add(InHandle, Params)` — compose an arbiter onto a game-owned
  entity. Params carry a `UCk_VisualLodArbiter_Data` config asset (domain tag, hysteresis band,
  near+lock budgets, ranking knobs, fade, exhaustion policy, N crowd configs).
- `UCk_Utils_VisualLod_UE::Add(InHandle, Params)` — direct-attach a member (arbiter tag, crowd
  index, proxy renderer, promotion mode, initial far anim).
- Member requests: `SetArbiter`, `SetVisibility`, `SetFarAnim`, `SetRenderer`, `Suspend`/`Resume`
  (external ownership handoff); immediate mutators `Request_Acquire/ReleasePromoteLock`.
- Arbiter requests: `SetObserver`/`ClearObserver` (explicit observer wins over local-view discovery),
  `SetRuntimeTuners`/`ResetRuntimeTuners`. Runtime tuners are a complete deferred snapshot seeded
  from the asset; invalid snapshots fail atomically and lowering a budget converges through normal
  arbitration rather than forcibly changing existing promotions or counters. `Get_AreRuntimeTunersValid`
  validates candidate nested edits against the live arbiter's authored shape before they are requested.
- Signals (game/AS binds; the game seam): `OnMemberAcquired` (fires BEFORE the member's first
  visible frame — cosmetics window), `OnPromoted`, `OnDemoteFinishing`, `OnMemberReleased`.
  Member-event payloads carry `(handle, memberIndex)`; read the crowd via `Get_Crowd(handle)` at
  handler time — an actor pointer must not ride a replayable payload.
- Promoted-proxy locomotion is **module-driven by default**: on promote the arbiter puts the proxy
  in sequence pose mode and plays the same anim-collection sequence its far-anim resolves to (Fixed
  index, or the SpeedDriven idle/move), looping at the far rate — so a near proxy walks exactly as
  the far member did, with no AnimBP required. The arbiter re-issues on far-anim change while
  promoted. A game wanting a richer look overrides it inside its `OnPromoted` handler (a
  `Request_PlayAnimation`/`Request_SetAnimInstanceClass` there enqueues after the module's and wins
  by FIFO). No-op for `AlwaysPromoted`/exhaustion proxies (no crowd collection to mirror).
- Pure ranking (`CkVisualLod_Ranking.h`, `ck::visual_lod`): `Select_Flips`, `Get_IsInView`,
  partial rank order — unit-testable without a world.

## Far render bands

Each `FCk_VisualLod_CrowdConfig` may author an ordered `_RenderBands` array. Band 0 normally starts
at distance 0; every later band supplies an outward `_DistanceThreshold`, an inward
`_ReturnHysteresis`, and a soft `UCk_IskmRenderer_Data` profile. Thresholds must be strictly
increasing, hysteresis must be non-negative and smaller than the gap to the previous threshold,
and every profile must use the crowd's exact AnimCollection/default mesh. Invalid arrays fail
closed before a crowd or partial profile set is published.

The arbiter uses its already-computed observer distance to select the greatest threshold not above
the member. Moving outward changes at the threshold; returning inward changes only below
`threshold - hysteresis`. Large teleports may cross several bands in one update. The selected
index is retained on `FFragment_VisualLod_Current` and exposed by `Get_RenderBandIndex`.

Far members migrate between stable `(spatial tile, profile index)` GPU buckets. Their member index,
world transform, animation phase, custom data, visibility ownership, cosmetic registration, and
highlight claims remain member-owned and do not change. A promoted member keeps its selected far
band so it returns to the correct profile on demotion.

Profiles are complete renderer-data states, not deltas. Typical authored bands are:

- full: normal material, shadows, lighting, velocity, and animation cadence;
- reduced: disable contact/dynamic shadows, decals, dynamic-indirect and distance-field lighting,
  ray tracing, and velocity; optionally increase `_FarAnimationUpdateInterval` and force a higher
  minimum mesh LOD;
- terminal: freeze far animation, apply a cheap/unlit `_BaseOverrideMaterials` set, cull at a
  maximum distance, or disable both main and depth passes.

Disabling indirect/distance-field flags does not make a lit material unlit and does not remove its
direct-light shader cost. Use a validated skeletal-compatible cheap/unlit material override for
that optimization. Profile base overrides sit below a whole-crowd material override and above
crowd slot overrides/mesh defaults.

## Crossfade contract (dithered, material-side)

The promote/demote crossfade is **one alpha, two complementary dither masks**. The arbiter owns the
value and writes it to two data channels every fade tick; the masking itself is the materials' job,
and this module never touches either mesh's materials.

- **The value.** `_FadeAlpha`, stepped over `_FadeDuration`. **1 = far crowd member solid,
  0 = near mesh solid.** It reverses mid-flight when the entity re-crosses the band (no proxy or
  slot churn), so both channels must be read as "current", never as "progress from a start".
- **Far channel** — the crowd instance's per-instance custom data at config `_FadeCustomDataSlot`
  (default 13). The crowd material dithers the member IN as the value rises.
- **Near channel** — the promoted mesh's **custom primitive data** at config
  `_FadeNearCustomPrimitiveDataSlot` (default 0), via the proxy's existing
  `UCk_Utils_IskmProxy_UE::Request_SetCustomDataFloat` lane, which mirrors the value to every
  attached outfit submesh (the whole body fades as one) and caches it across SKMC re-acquisition.
  The near material dithers itself OUT as the same value rises. **The renderer asset must declare
  `_NumCustomDataFloat > slot`** or every fade write ensure-fails loudly — that declaration is part
  of the contract, exactly like the material reading the slot.

The two masks are OVERLAPPED complements of one value (`CkUsf_VisualLod_{Far,Near}FadeMask` in
Common.ush, coverage over-driven by `CKUSF_VISUALLOD_FADE_OVERLAP`): a strict per-pixel partition
is only seamless where both renders agree on the surface, and the smooth SKMC pose vs the
30Hz-baked crowd pose guarantees they don't at limbs — under a partition that disagreement reads
as see-through holes (observed in the gym). With the overlap, aligned pixels briefly double-draw
(opaque + depth test = invisible) and misaligned ones stay covered; endpoints remain exact.

The near mesh keeps its **real materials** throughout — no overrides applied or cleared, no generated
master, no editor generate step, and nothing for a game's own `OnPromoted` material work to collide
with. The cost of that is honesty about the contract: a near material that does not read the slot
simply renders solid the whole way, so the flip **pops, loudly and visibly**. That is deliberate —
an unwired material is a content bug that should be seen in the first playtest, not smoothed over.

Steady states are exact, not merely "the last sub-step": the arbiter writes near `0.0` when a promote
fade completes, and again on the mid-life force-end paths (hidden-mid-fade snap, stale-crowd
force-end). Pool hygiene for the slot itself is the renderer's: with the slot declared inside
`_NumCustomDataFloat`, `FProcessor_IskmProxy_Setup` zeroes it on every (re)acquire, so a recycled
mesh never inherits a partway mask — 0.0 is exactly "near solid". Writes enqueued on a proxy that is
released the same tick are cancelled by the request system; that is fine for the same reason.

## Anti-patterns

1. Don't give members a second way to find their arbiter — tag resolution (lazy, cached) with the
   explicit request override is the whole contract.
2. Out-of-view is never a demote trigger (camera spins must not mass-flip); preemption is the only
   way view enters a demote, and it is rate-limited.
3. Don't put viewer discovery in CkVisibleRange (its doc forbids it) — this module resolves the
   viewer.
4. Don't make the crossfade smooth by hiding a representation, swapping in a module-owned material,
   or silently skipping the flip when the near material ignores the fade slot. The contract is the
   two dither channels and nothing else; a pop means unwired content, and it is supposed to show.
