# DESIGN — CkVisualLod: budgeted visual-representation LOD

## 2026-08-30 approved extension — range render profiles

The original near/far representation decision remains intact. Far members additionally select an
ordered renderer-data profile from camera distance. Each profile boundary has an outward threshold
and an inward return hysteresis; the current band is sticky inside that interval.

- `UCk_IskmRenderer_Data` is the shared authored profile for pooled SKMCs and GPU cluster
  primitives. Existing render/cull/light fields become active; material overrides, animation
  update interval/freeze, velocity, ray tracing, and minimum LOD extend that same asset.
- One `ACk_Iskm_BatchedCrowd_Actor` retains the authoritative member array. GPU render views are
  buckets keyed by `(spatial tile, profile index)`; moving between buckets never changes the public
  member index or reconstructs its animation/custom-data state.
- Profile preparation is atomic. All profile assets load and validate against the crowd's exact
  animation collection/default mesh before the crowd is published. A failed required profile
  rejects the crowd rather than running a partial profile set.
- Profile migration creates/configures the destination bucket before ownership moves, preserves
  the entire instance payload, zeros velocity for the one primitive-change frame, then rebuilds
  each affected bucket once. This follows `CkDebugScene`'s staged bucket-reconcile precedent.
- Primitive-wide profile buckets are mandatory: a material custom-data bit may alter pixels, but
  cannot remove an instance from the shadow, velocity, custom-depth, or other primitive passes.
- A zero-pass/frozen terminal profile supplies distance culling without adding another VisualLod
  representation state. True per-instance render-pass filtering and a separate low-poly
  shadow-only skeletal representation are outside this gate.

**Date:** 2026-08-27 · **Status:** presented for maintainer sign-off (all §Decisions confirmed in-session)
**Spec-by-example:** BusterBlock `Script/ECS/NpcVisualLod/` + `Script/ECS/AmbientNpc/` (read in full,
including the uncommitted, compile-UNVERIFIED view-ranked promotion change — treated as design intent,
not proven behavior; its logic lands here re-derived in C++ with its own tests).

## Goal

A T4 CkFoundation module that owns skeletal-mesh visual LOD as a first-class capability: it knows
every rendering technique (pooled SKMC `IskmProxy`, GPU batched crowd member via
`ACk_Iskm_BatchedCrowd_Actor`, hidden, always-promoted), holds all tuning options, and intelligently
switches per entity — generalizing BusterBlock's two near-duplicate AngelScript systems
(`UBb_Processor_NpcVisualLod_Flip`, `UBb_Processor_AmbientNpc_VisualLod`) so they become thin
adopters and their duplicated logic is deleted.

## Decisions (maintainer-confirmed, 2026-08-26/27)

| Question | Decision | Why |
|---|---|---|
| Name / split | **CkVisualLod**, one module | Names what it arbitrates (visual representation) without naming a technique; survives VAT/imposters. A mechanism/policy split has nothing to put in the second half — policy *data* is config, policy *code* stays game-side. |
| Policy seam | **Game-agnostic mechanism in C++; game-specific behavior in game AngelScript**, hooked via signals + config | Maintainer's words. No per-frame script callback exists — far-anim is config + per-entity override request. |
| Options surface | **Per-arbiter config data asset** (`UCk_DataAsset_PDA`) | Policy knobs + N crowd configs per LOD domain; BB = two assets (roster, ambient) with naturally independent budgets. |
| Domain reference | **Gameplay tag, lazily resolved; explicit handle request wins** | Spawn paths only need a tag constant (`VisualLod.Roster`) — zero handle plumbing; `Request_SetArbiter(handle)` for tests/direct wiring. Duplicate tag ⇒ ensure. |
| Viewer | **Observer-wins**: arbiter `Request_SetObserver`; unset → new `UCk_Utils_Camera_UE::TryGet_LocalViewInfo` | Framework norm (CkCompass) preserved and split-screen-proof; the discovery util discharges the pending "upstream TryGet_LocalCamera into CkCamera" chip. No view ⇒ arbiter no-ops (dedicated server / editor world). |
| Persistence | **None** — no persistence handler registered | v3 rebuild+hydrate: all LOD state is derived; entities rebuild from spawn recipes and re-earn promotion. |
| Exhaustion policy | **Config per arbiter**: `FallbackPromote` \| `StayFar_Ensure` | Unifies the roster/ambient disagreement as a choice instead of a fork. |
| Budget accounting | **Fixed**: near / lock / unbudgeted counted separately | BB defect: exhaustion-fallback + PromotedAlways promotes permanently shrank the near-16. |
| Ranking scope | **Whole arbiter query per tick** | BB defect: per-batch ranking with a global budget. |
| Near+locked charge routing | Keep the uncommitted simplification (charges lock budget) | Deliberate; locked NPCs still compete via ranking when the lock budget is full. |

## Entity model

Two features in one module, each a standard quartet surface:

### Arbiter — one entity per LOD domain

- `FCk_Handle_VisualLodArbiter`; params carry the domain **tag** + the **config asset**.
- **Config asset** `UCk_VisualLodArbiter_Data : UCk_DataAsset_PDA`:
  - Policy: promote/demote distances (hysteresis band), near budget, reserved lock budget,
    lock-promote max distance, fade duration (`FCk_Time`), view-cone margin, always-in-view radius,
    preempt distance margin, max preempts/tick, exhaustion policy, fade custom-data float slot
    (default 13), park Z.
  - `TArray<FCk_VisualLod_CrowdConfig>`: anim collection, pool size, tile size, speed-driven
    far-anim params (idle/walk sequence indices, authored walk speed, move-speed threshold, rate
    clamp). Crowd id = array index; the game assigns it per entity at spawn (generalizes
    male/female to N).
- `ck::FFragment_VisualLodArbiter_Current`: per-crowd pools (weak crowd actor, free list, owner
  handles), promoted set, the three budget counters, observer handle, resolved view for the tick,
  and a runtime-tuner snapshot seeded from the authored asset at setup.
- Requests: `SetObserver`, `ClearObserver`, `SetRuntimeTuners`, `ResetRuntimeTuners`. Runtime
  tuner changes are deferred, all-or-nothing snapshots: malformed finite/range/budget/enum inputs
  complete `Failed` without changing the live values; reset copies the authored decision knobs.
  Lowering a budget does not force demotions or rewrite counters; ordinary arbitration converges.

### Member — any entity opting into LOD

- `FCk_Handle_VisualLod`; params: arbiter domain tag (or nothing — set via request), crowd id,
  proxy renderer (`TSoftObjectPtr<UCk_IskmRenderer_Data>`), `PromotedAlways`, initial hidden,
  initial far-anim mode.
- `ck::FFragment_VisualLod_Current`: cached arbiter handle, member index, weak crowd, promoted /
  via-lock, fade phase + alpha, preempt-demote flag, hidden latch, promote-lock counter, cached
  seq/rate, visual scene-node handle, `FCk_Handle_IskmProxy`, suspended flag.
- Tag `ck::FTag_VisualLod_Promoted` (maintained by the flip) so consumers/debugger get cheap views.
- Requests (all with completion delegates per the request contract): `SetArbiter(handle)`,
  `SetHidden(ECk_EnableDisable)`, `AcquirePromoteLock` / `ReleasePromoteLock`,
  `SetFarAnim(SpeedDriven | Fixed{seq, rate})`, `SetRenderer(soft ptr)`, `Suspend` / `Resume`.

**Suspend/Resume** generalizes ambient's reaction-HFSM ownership handoff: while suspended the module
does not touch the entity's representation (slot stays retained); `Resume` fail-closes to the far
representation (also generalizes ambient's `Recover_NearFailure`).

## Processors

| Processor | Duty |
|---|---|
| `FProcessor_VisualLod_HandleRequests` | Drain member requests (locks are counter bumps; hidden latch; anim mode; suspend). |
| `FProcessor_VisualLodArbiter_HandleRequests` | Observer wiring. |
| `FProcessor_VisualLodArbiter_Update` | The flip driver. Per arbiter: resolve view (observer → `TryGet_LocalViewInfo` fallback) → per-member pass (stale-crowd invalidation, hidden handling, lazy crowd stand-up + slot acquisition, fade ticking, distance-only demote, far transform-per-frame / anim-on-change) → ranked flip pass across the **whole** member set (budget spend + rate-limited preemption). |
| `FProcessor_VisualLod_EndPlay` | Deterministic slot release + budget refund on member destruction — replaces BB's sweep-reliant reclamation and the `_LockedPromoteCount` clamp hack. An amortized sweep remains as reconciliation (Resilience Tenets: pools must converge from arbitrary state). |

All BB fade/flip gotchas are contract here: `PreemptDemote` suppresses the near-side fade reversal;
a lock taken mid-fade overrides and clears it; hidden-mid-fade snaps the member out and leaves the
fade slot clean at 1.0; member-vanished-mid-fade resolves the promote state; demote-begin writes
fade=0 **before** first visible frame; slot recycling rewrites the fade slot; out-of-view is never a
demote trigger; preempted slots free at fade end (challenger wins on a later tick).

**Ranking** (`Select_Flips`, in-view cone test, partial selection sort, worst-incumbent search) ports
as pure free functions in a ck namespace — unit-tested in C++ automation, which is where the
unverified AS ranking change gets its real verification.

## Game seam — signals

`CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE`, fired on the member handle at transition points; the
game (BB AngelScript) binds:

| Signal | Timing contract | BB binds |
|---|---|---|
| `OnVisualLod_MemberAcquired(crowd, index)` | **before** the member's first visible frame | manifest slice + skin-RGB floats, far-cosmetic registration |
| `OnVisualLod_Promoted(proxy)` | after proxy build, before end of tick | wardrobe `Apply`, skin MIDs, weapon-hand rebind |
| `OnVisualLod_DemoteFinishing(crowd, index)` | before proxy destroy | cosmetic follower detach, weapon park, far-cosmetic re-register |
| `OnVisualLod_MemberReleased(crowd, index)` | slot returned | cosmetic cleanup |

⚠️ Open verification item (Gate 1): the acquire-signal must observably fire before the visibility
write reaches the crowd. If the signal machinery cannot guarantee synchronous in-tick delivery, the
fallback is an explicit bindable "configure member" delegate on the arbiter — flagged to the
maintainer before building it.

What stays in BB AS: CharacterCustomizer integration, weapon rebind/park, crowd-manifest float
composition, skin MIDs, flyer-run detection (drives `Set_FarAnim(Fixed)`), schedule-driven hide
calls, the ambient reaction HFSM (now over Suspend/Resume).

## Style/doctrine compliance notes

- Durations are `FCk_Time`; on/off surfaces are `ECk_EnableDisable`; no `b` bool prefixes
  (`bHidden` → `_Hidden`). Crowd actor refs are `TWeakObjectPtr` (observed, BB-proven);
  renderer data is soft-ptr params + rooted-batch on load if the module loads it.
- Requests deferred via `_Requests` fragments; every `Request_*` carries the completion delegate,
  fires exactly once, idempotent no-ops report `Succeeded`.
- Three environments: full surface verified C++ / BP / AS.
- Reuse check (non-negotiable #9): CkVisibleRange was considered and rejected as the mechanism —
  it is per-entity caller-fed distance banding with no viewer, no budgets, no arbitration; its own
  doc forbids viewer knowledge and anticipates a caller like this module. CkVisualLod resolves the
  viewer itself and may *feed* VisibleRange consumers later, but does not build on it.

## Testing

- **C++ automation**: ranking/selection (budget spend, in-view-first, preempt margin + rate limit,
  worst-incumbent), budget accounting (three-counter invariants, exhaustion paths, lock refund on
  destroy), pool recycle invariants (double-free, foreign-owner, stale-crowd).
- **CkTests AS autotests + gym**: flip lifecycle end-to-end (acquire → promote → fade → demote →
  release), hidden round-trip, lock behavior, suspend/resume, signal firing order.
- **BB parity gate (adoption)**: BB's existing NpcVisualLod/AmbientNpc autotests stay green after
  both systems become adopters; every §2 mechanic of the continuation prompt survives or is
  consciously dropped with sign-off.

## Gates (index in PLAN.md)

0. Scaffold + data surface (module, fragments, config asset, handles) — compiles all-ways.
1. Mechanism (pools, flip, fades, budgets, locks, ranking) + C++ automation tests.
2. Signals + `TryGet_LocalViewInfo` (CkCamera) + full request surface + CkTests coverage.
3. Debugger surface (CkGameplayDebugger bridge: per-arbiter budget/pool panel, view-cone +
   per-entity state overlay).
4. BB adoption (separate host repo): both systems to thin adopters, parity gates, delete duplicates.

## Non-goals

- New rendering techniques (VAT, imposters) — the arbiter's technique set is designed to grow, not
  grown now.
- Cosmetic/wardrobe generalization — stays game-side behind the signals.
- Replication — client-local by construction (no view ⇒ no-op), like the BB systems.
- CkVisibleRange integration — separate module, separate concern.

## Ruled out — do not re-investigate

| Ruled out | Why |
|---|---|
| Policy remaining BB script long-term | Maintainer mandate: "should have been part of CkFoundation". |
| Viewer discovery in CkVisibleRange | Its doc forbids viewer knowledge. |
| Out-of-view as demote trigger | Camera spin mass-demotes; decided during the BB ranking work. |
| Same-tick promote-into-preempted-slot | Slot frees at fade end by design; keeps accounting simple. |
| Mechanism/policy module split | Nothing to put in the policy half (data is config, code is game-side). |
| BNE strategy-object policy seam | Invites per-frame script calls; signals + config cover the need. |
