# CkVisualLod

**Purpose:** Budgeted skeletal-mesh visual LOD. Entities in a LOD domain switch between a pooled
SKMC `IskmProxy` (near), a GPU batched crowd member (far, `ACk_Iskm_BatchedCrowd_Actor`), and
hidden — with ranked in-view-first promotion, promote/demote hysteresis, dither crossfades,
promote locks (ragdoll/montage holders), reserved lock budget, and per-domain crowd pools. An
**arbiter** entity per domain owns budgets/pools/view; **member** entities opt in and reference
their domain by gameplay tag (explicit `Request_SetArbiter` overrides).

**Status:** under construction — Gate 0 (scaffold + data surface) of the campaign in
[PLAN.md](../CkVisualLod/PLAN.md). Design of record: [DESIGN_CkVisualLod.md](DESIGN_CkVisualLod.md).
The mechanism (pools, flips, fades, ranking) lands in Gate 1; this doc grows per gate and is the
campaign's permanent survivor.

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
- Arbiter requests: `SetObserver`/`ClearObserver` (explicit observer wins over local-view discovery).
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
