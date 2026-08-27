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
mechanism APIs this module orchestrates), `CkLog`, `CkResourceLoader`.

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
- Pure ranking (`CkVisualLod_Ranking.h`, `ck::visual_lod`): `Select_Flips`, `Get_IsInView`,
  partial rank order — unit-testable without a world.

## Anti-patterns

1. Don't give members a second way to find their arbiter — tag resolution (lazy, cached) with the
   explicit request override is the whole contract.
2. Out-of-view is never a demote trigger (camera spins must not mass-flip); preemption is the only
   way view enters a demote, and it is rate-limited.
3. Don't put viewer discovery in CkVisibleRange (its doc forbids it) — this module resolves the
   viewer.
