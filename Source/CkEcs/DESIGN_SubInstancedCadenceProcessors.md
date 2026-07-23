# Sub-Instanced Cadence Processors — Design & Implementation Plan

**Status:** implementation plan locked 2026-07-22 (see "Implementation plan" at the bottom);
per-phase status is tracked there. The benchmark prerequisite (#2 below) is `[EDITOR-VERIFY]` —
agents cannot run PIE; the exact manual steps are recorded at the bottom. This is a
**framework primitive** for CkEcs (it subsumes the per-feature cadence retrofit list), not a
CkVisibleRange-local change. Origin: the `CkPoi` v2 refactor follow-up
(`CkPoi/REFACTOR_MultiProjectorPoi.md` → "Bucketed-cadence scheduler primitive", recorded 2026-07-21).

## The primitive it replaces

The shipped cadence primitive is `FCk_Chrono::Tick(DeltaT, ECk_Chrono_OverflowPolicy::Wrap)`
(`CkCore/Chrono/CkChrono.h`) — the `Wrap` overflow policy turns `Tick` into a recurring-interval gate
over a per-entity `FCk_Chrono` (`Done` = fire this tick; the default `Clamp` policy latches for a
one-shot). A processor that wants per-entity variable cadence (entity A re-evaluates every frame,
entity B every 2 s) runs every scheduler tick and, per entity, calls
`Current._CadenceChrono.Tick(DeltaT, Wrap)` to decide whether this is a due tick. `CkVisibleRange` is
the current driver; the follow-up retrofit list (Compass / Minimap / FogOfWar) would join it.

**Why not `TProcessorBase::_TickRate`?** (`CkEcs/Processor/CkProcessor.h:81,186-199`) It throttles the
*whole* processor to one uniform rate, and on a hitch it *catches up* by replaying `DoTick` multiple
times per frame — a fixed-timestep tool, not a work-skipper. Variable per-entity cadence is per-entity
data; a processor-wide knob cannot express it.

**The cost the poll pays:** `Tick(dt, Wrap)` ticks an `FCk_Chrono` for **every** entity **every**
frame, even when almost none are due — O(N) chrono ticks/frame over the feature's whole population.

## Proposal — cadence buckets ("sub-instanced" processors)

1. **Quantize** each entity's `_UpdateInterval` at `Add` into a fixed bucket set, quantizing toward
   **faster** (an entity must never update slower than requested).
2. **Tag** the entity with its bucket: `FTag_<Feature>_Cadence_<Bucket>`.
3. Run **one sub-processor per bucket**, each with `TProcessorBase::_TickRate` set to that bucket's
   interval, iterating only its bucket's tagged entities. The scheduler's empty-view skip makes vacant
   buckets near-free.

"Due-ness" stops being a per-entity chrono poll and becomes **membership in a `_TickRate`'d
sub-processor's view**. The per-entity `FCk_Chrono` + `Tick(dt, Wrap)` path disappears for bucketed
features.

Proposed bucket set (geometric, tune against the benchmark): `{ 0 (every tick), 0.1s, 0.25s, 0.5s,
1s, 2s, 4s }`. Quantize a requested interval down to the nearest bucket `<=` it.

## Prerequisites — BOTH required before building

### 1. A phase-offset knob on `TProcessorBase`

A `_TickRate`'d sub-processor fires its **whole** view on the same tick, losing the natural per-entity
stagger the current `Tick(dt, Wrap)` poll gives for free (entities added on different frames reset at
different times). A 1 s bucket holding 300 entities becomes 300 evaluations on one frame, 0 for the
next ~59 — a frame spike.

Add an **optional** `_TickPhaseOffset` (default 0 → **zero behavior change for every existing
processor**) so bucket sub-processors fire on different frames. `_RemainingDeltaTFromLastFrame` is
private with no offset support today (`CkProcessor.h:82`); the offset seeds the initial accumulator.

### 2. A benchmark proving the poll registers at all — `[EDITOR-VERIFY]` (agents cannot run this)

Per skipped entity the poll costs **one** `FCk_Chrono::Tick` over packed storage; the due-work
(`Compute_FadeAlpha` + `Compute_IsInRange`) is barely larger. The justification for this whole
primitive is **ecosystem-wide cleanup + large-N headroom, not measured perf** — so measure first.
Profile `FProcessor_VisibleRange_Update` at representative counts (e.g. 1k / 5k / 20k VisibleRange
entities) in PIE via Unreal Insights / `stat`, before and after. If the poll does not register at the
counts the project actually hits, the bucketing is not worth its complexity — keep `Tick(dt, Wrap)`.

## `_TickRate` catch-up caveat

The `_TickRate` catch-up loop (`CkProcessor.h:194-200`) replays `DoTick` after a hitch. For
**sampling** (non-integrating) processors — which cadence buckets are — re-sampling the same distance
N times is pure waste. Either cap catch-up to 1 for bucket sub-processors, or accept it. Recommend a
"no catch-up" trait carried by the bucket mixin.

## Bucketing mechanics

- **At `Add`:** `bucket = Quantize(_UpdateInterval)`; `Add<FTag_<Feature>_Cadence_<bucket>>()`.
- **Mutable interval** (VisibleRange's is immutable today): re-bucket = remove old tag, add new.
- **Sub-processors:** one per bucket, `_TickRate = bucket`, `_TickPhaseOffset =` bucket-index-derived
  stagger; each iterates `FTag_<Feature>_Cadence_<bucket>` ∩ the feature's fragments.
- **Immediate-first-eval** (today the `.Complete()` Chrono seed in `UCk_Utils_VisibleRange_UE::Add`): a
  newly-added entity must evaluate on its bucket's NEXT fire, not wait a full bucket interval. Keep a
  lightweight `FTag_<Feature>_NeedsFirstEval` consumed once by an every-tick pass, or seed the entity
  into a transient "eval-now" set. Preserve the shipped immediate-first-eval semantics.

## Migration

1. Land the framework primitive: the `_TickPhaseOffset` knob + a reusable CkEcs bucketing helper
   (a `CK_DEFINE_CADENCE_BUCKETS`-style macro, or a `TCadenceBucketed<...>` processor mixin that emits
   the per-bucket sub-processors and the `Add`-time tagging).
2. Migrate `CkVisibleRange` (the many-entity driver) first, as the reference. **Benchmark before/after.**
3. Few-entity throttles (Compass / Minimap / FogOfWar — one instance per player-ish) buy **nothing**
   from buckets. They should stay on `Tick(dt, Wrap)`; the scheduler is for MANY-entity features.

## Interim (pre-scheduler): the thundering-herd stagger

Independent of the scheduler, the per-entity poll can stagger steady-state recompute by seeding each
entity's `_CadenceChrono` with a per-entity phase offset (`GetTypeHash(Handle)` mod interval) at `Add`,
while preserving immediate-first-eval via a `_NeedsFirstEval` flag — the `.Complete()` seed alone
cannot do both (Complete → all same-frame adds fire and reset in lockstep). This is the poll-side
equivalent of prerequisite #1's phase knob. Same benchmark caveat: it is unproven the herd registers,
so it is gated on the same measurement.

## Non-goals

- Not a general fixed-timestep replacement — that is `_TickRate`'s integrating use.
- Not for few-entity throttles — those stay on `Tick(dt, Wrap)`.

## See also

- `CkCore/Chrono/CkChrono.h` — `FCk_Chrono::Tick` with `ECk_Chrono_OverflowPolicy::Wrap`, the cadence
  gate this would supersede for bucketed features.
- `CkVisibleRange/CLAUDE.md` — the first/reference consumer.
- `CkPoi/REFACTOR_MultiProjectorPoi.md` — where this design was first recorded.

---

# Implementation plan (locked 2026-07-22, grounded against real code that day)

## Ground truth this plan is built on (all verified by reading the code)

- `TProcessorBase` (`CkEcs/Processor/CkProcessor.h:51-102,161-223`): `_TickRate` is a private
  `TimeType` with `CK_PROPERTY(_TickRate)` (fluent `Set_TickRate`). **Zero call sites in all of
  Source/** — every shipped processor runs the `_TickRate == ZeroSecond()` fast path; the catch-up
  `while (AdjustedTickRate >= _TickRate)` loop (`:194-200`) is live code with no user. The
  accumulator `_RemainingDeltaTFromLastFrame` (`:82`) is private with no seed hook.
- Registration (`CkEcs/Scheduler/CkProcessorRegistration.h:89-95`): `CK_REGISTER_PROCESSOR(T)` and
  `CK_REGISTER_PROCESSOR_WITH_FACTORY(T, ...)` build one `FProcessorDescriptor` keyed by
  `Get_ProcessorCanonicalName<T>()` = `entt::type_name<T>` (`CkProcessorTraits.inl.h:23-29`).
  **Multi-instance therefore requires distinct TYPES** — template instantiations
  (`FProcessor_X_Bucket<3>`) get distinct canonical names for free.
- Traits are harvested from the type via `requires` probes (`BuildDescriptor`,
  `CkProcessorTraits.inl.h:209-322`): `Group`, `RunAfter`, `MarkedDirtyBy`, `PumpPolicy`,
  `EmptyViewPolicy`, ... Inherited aliases are found on the derived type, so a CRTP cadence base
  can supply them.
- Empty-view skip (`CkProcessorScheduler.cpp:139-189`): a provably-empty view **skips the whole
  dispatch including `Tick`**, so a `_TickRate`'d processor's accumulator FREEZES while its view is
  empty. Filter tags participate in the include set (`ViewIncludeOf`, `CkProcessorTraits.inl.h:133-146`
  strips only `TExclude`/`TIgnoreInEditor`) — a per-bucket tag makes a vacant bucket provably empty.
  Eligibility requires the template-generated `DoTick` (unshadowed) — the cadence base must NOT
  shadow `DoTick`.
- `FCk_Time` default-constructs to 0.0s (`CkCore/Time/CkTime.h:76`), so `_TickRate` defaults to
  zero and the fast path is the universal default.
- The pump path `Pump()` (`CkProcessor.h:206-222`) **bypasses `_TickRate` entirely** — a nonzero-rate
  bucket processor must never be pump-eligible (no `MarkedDirtyBy` on buckets 1..N).
- AutoTests run sequentially in a **shared PIE world** (`CkTests/Script/Common/CkAutoTest_Base.as`,
  "shared PIE world" cleanup comment); each test's entity subtree is destroyed at finish.

## Test-contract analysis (why wake-alignment is load-bearing)

`Ck_AutoTest_VisibleRange_CadenceGatesUpdates` (CkTests, unmodifiable from this repo) pins, for
`_UpdateInterval = 0.5s`: (1) first eval on the first tick after `Add`; (2) NO re-eval within 0.1s
of that first eval; (3) re-eval by ~0.7s after it. Under bucket-grid cadence these hold **because
of the empty-view-skip freeze**: bucket 3 (0.5s) is empty until this test's entity joins, so its
accumulator is frozen at 0 and the first bucket fire lands ~0.5s after the entity's arrival —
entity-aligned phase, deterministically inside (0.1s, 0.7s). This is a REAL dependency of the green
gate, so it is promoted to a documented contract: **a bucket's phase aligns to the moment its view
last became non-empty**. Consequences, deliberately accepted:
- Cadence bucket processors get **no default `_TickPhaseOffset`** — a nonzero seed would break
  wake-alignment (first fire would land `TickRate - Offset` after wake). The knob still ships on
  `TProcessorBase` (prerequisite #1) for consumers that want explicit stagger; the cadence mixin
  simply doesn't use it by default.
- If `_EnableEmptyViewMainPassSkip` (ECS project settings, default ON) is turned off, bucket phase
  becomes world-aligned and the 2nd assertion above becomes ~20% flaky. Accepted: the gate runs
  default config; hardening (an Add-time phase-reset hook for empty buckets) is a recorded
  follow-up, not built now.
- A bucket that empties mid-phase and later re-wakes inherits its frozen mid-phase accumulator
  (first fire < interval after re-wake). Only observable if two tests in one PIE session occupy the
  same nonzero bucket; today only CadenceGates uses one (0.5s — the other three VisibleRange-suite
  tests all use interval 0 = bucket 0).

The other baseline tests are cadence-insensitive: `_OwnRangeBoundaryCrossing`,
`_ExplicitOverrideIsIndependentVote`, and `Ck_AutoTest_Minimap_MaxVisibleRange_Culls` all compose
with `_UpdateInterval = 0` → bucket 0 (every tick), semantics identical to today.

## Phase 1 — `TProcessorBase` knobs (opt-in, default = byte-identical behavior)

File: `CkEcs/Processor/CkProcessor.h` only.

1. `enum class ECk_ProcessorTickCatchUp : uint8 { ReplayMissedTicks, SampleLatestOnly }` at global
   scope (matching the sibling scheduler-policy enums `ECk_ProcessorPumpPolicy` /
   `ECk_ProcessorEmptyViewPolicy`). Consumed by `TProcessorBase::Tick` via a
   `requires { DerivedType::TickCatchUpPolicy; }` probe (precedent:
   `Get_EmptyViewPolicyAllowsSkip`, `CkProcessorTraits.inl.h:197-205`). Absent ⇒
   `ReplayMissedTicks` ⇒ the existing loop verbatim. `SampleLatestOnly` ⇒ consume every whole
   tick-rate multiple but fire `DoTick` ONCE with the summed elapsed time (phase remainder
   preserved, time conserved, no replay). Guarded: non-static or wrongly-typed declarations are
   static_assert failures, not silent fallbacks.
2. Compile-time tick rate — REFINED during implementation from the originally-planned
   `static constexpr double CadenceIntervalSeconds` to a typed literal (a raw double neither
   self-documents its unit nor resists misuse). The author-facing trait is ONE line:
   `static constexpr auto TickRate = ck::Hz{4};` (or `ck::Seconds{0.25}`) — `ck::Hz`/`ck::Seconds`
   are consteval literal types over a shared `ck::FTickRate` carrier (`CkProcessor.h`), constexpr-legal
   where `FCk_Time` (reflected USTRUCT, no constexpr ctor) is not. The public `Get_TickRate()` (a
   normal member fn — the DOUBLE is the constexpr artifact, the `FCk_Time` is materialized per call)
   resolves trait-else-`_TickRate`; `Tick` materializes it once and uses it for the fast-path check
   and both catch-up loops. Trait-absent processors read the same default-zero member ⇒ fast path
   byte-identical. Misuse fails to COMPILE: zero/negative rate (consteval ctor poison), raw-number /
   `FCk_Time` / type-alias / non-static / non-constexpr spellings (static_asserts in
   `Get_TickRate`), and `Set_TickRate` on a trait-declaring processor (static_assert in its body —
   instantiated only if called). Known residual, documented in the header: TWO bases both declaring
   `TickRate` make lookup ambiguous, which the requires-probe reports as absent (silent every-tick
   degrade) — don't stack cadence mixins.
3. `_TickPhaseOffset` (`TimeType`, default 0) + `CK_PROPERTY_GET` + a hand-written fluent
   `Set_TickPhaseOffset` that records the value AND seeds `_RemainingDeltaTFromLastFrame`.
   Nothing existing calls it ⇒ nothing changes. (Cadence buckets deliberately do NOT use it —
   wake-alignment above — so it stays a runtime knob with no compile-time twin until a consumer
   with a per-type fixed offset exists.)

Gate: full editor build + VisibleRange 4/4, Chrono 3/3, Poi 46/46 (baseline counts from HEAD).

## Phase 2 — the bucketing primitive (`CkEcs/Processor/CkProcessor_CadenceBuckets.h`, new)

All in `namespace ck` (+ nested `ck::cadence` for the value helpers):

- Bucket set: `cadence::BucketCount = 7`, intervals `{0, 0.1, 0.25, 0.5, 1, 2, 4}` s, stored as
  constexpr doubles (`FCk_Time` is a reflected USTRUCT with no constexpr ctor — `_TickRate` itself
  must stay the runtime member every processor already has, or the opt-in guarantee breaks).
  `cadence::Get_QuantizedBucketIndex(FCk_Time)` — largest bucket `<=` requested (toward faster);
  `<= 0` ⇒ bucket 0.
- Tag families: `TTag_CadenceBucket<T_CadenceKey, T_BucketIndex>` (bucket membership; the view
  filter; compile-time range-checked) and `TTag_CadenceFirstEval<T_CadenceKey>` (pending immediate
  first eval). `T_CadenceKey` = the feature's typesafe handle type (unique per feature; it is also
  the `T_HandleType` the bucket processor passes to `ck_exp::TProcessor` — one type, two roles,
  collapsed during implementation to cut a template param). The compile-time rate source of truth
  is `detail::TCadenceBucketRateTraits<T_BucketIndex>`: buckets 1..N declare
  `static constexpr auto TickRate = ck::Seconds{BucketIntervalsSeconds[N]};` (the Phase-1 trait);
  the `<0>` specialization declares NOTHING, so bucket 0 hits the exact ZeroSecond every-tick fast
  path. No ctor bridging, no `Set_TickRate` anywhere.
- **First-eval folds into bucket 0** (no separate processor): `cadence::AddCadenceTags<Key>(Handle,
  Interval)` quantizes, adds `TTag_CadenceBucket<Key, N>`, and for `N != 0` ALSO adds
  `TTag_CadenceBucket<Key, 0>` + `TTag_CadenceFirstEval<Key>`. The every-tick bucket-0 processor
  evaluates the newcomer on its next tick (and same-frame via pump — see below);
  `cadence::TryConsume_FirstEval<Key>(Handle)`, called at the top of the shared eval body, removes
  both transient tags (`Has<FirstEval>` ⇒ the bucket-0 tag was transient — a native bucket-0
  entity never gets the FirstEval tag). Removing a view-include tag mid-iteration is the shipped
  `MarkedDirtyBy` consumption pattern (`CkVisibleRange_Processor.cpp:66`); pools are
  `in_place_delete` so it is iteration-safe.
- `TCadenceBucketProcessor<T_Derived, template<int32> T_DerivedTemplate, T_BucketIndex,
  T_HandleType, T_Fragments...>` : `ck_exp::TProcessor<T_Derived, T_HandleType,
  T_Fragments..., TTag_CadenceBucket<T_HandleType, T_BucketIndex>>`. Supplies:
  - the compile-time `TickRate` trait via the `TCadenceBucketRateTraits<T_BucketIndex>` mixin base
    (nothing for bucket 0; no phase offset — wake-alignment);
  - `TickCatchUpPolicy = SampleLatestOnly` (re-sampling after a hitch is waste);
  - `RunAfter` chain bucket N → bucket N-1 via `T_DerivedTemplate` (orders the shared read-write
    fragment across the 7 instantiations WITHOUT tripping the write-write-conflict Permissive
    auto-edges + warnings; bucket 0 gets an empty `TDepList<>`);
  - bucket 0 additionally: `MarkedDirtyBy = TTag_CadenceFirstEval<T_CadenceKey>` so a fresh `Add`
    drains its first eval in the same frame's pump passes (nonzero buckets must NOT be
    pump-eligible — `Pump()` bypasses `_TickRate`).
  Consumer supplies `Group` and the (static) `ForEachEntity`.
- `CK_REGISTER_CADENCE_BUCKET_PROCESSORS(_ProcessorTemplate_)` — registers `<0>..<6>`, with a
  `static_assert(BucketCount == 7)` so a bucket-set change forces the macro update.
- `TCadenceBucketDepList<template<int32> T, ExtraDeps...>` — a `TDepList` over all 7
  instantiations (+ extras) for downstream `RunAfter` (e.g. HandleRequests).

Not built (YAGNI until a consumer needs it, recorded here so it isn't re-litigated): mutable-interval
re-bucketing (`VisibleRange`'s interval is immutable at `Add`); the Add-time phase-reset hardening;
the interim per-entity poll stagger (superseded by this primitive for the only shipped poll user).

## Phase 3 — migrate `CkVisibleRange` (reference consumer)

- `CkVisibleRange_Fragment.h`: drop `_CadenceChrono` from `FFragment_VisibleRange_Current` (and its
  `CK_DEFINE_CONSTRUCTORS` — no essential remains; `FFragment_VisibleRange_Requests` is the
  no-ctor-macro precedent). Friend declarations follow the processor rename.
- `CkVisibleRange_Utils.cpp::Add`: replace the `.Complete()` Chrono seed with
  `cadence::AddCadenceTags<FCk_Handle_VisibleRange>(InHandle, InParams.Get_UpdateInterval())`.
  Public API (C++/BP/AS) unchanged — quantization is internal to `Add`.
- `CkVisibleRange_Processor.h/.cpp`: `FProcessor_VisibleRange_Update` (no external references —
  swept CkGameplayDebugger/CkTests/CkApplication 2026-07-22) becomes
  `template <int32 T_BucketIndex> class FProcessor_VisibleRange_Update_Bucket` on the cadence base;
  the eval body (fade alpha + boundary-crossing request enqueue, `Tick(dt, Wrap)` gate deleted,
  `TryConsume_FirstEval` prepended) is a template definition in the .cpp above the registration
  macro. `HandleRequests`' `RunAfter` becomes the bucket dep list.
- `CkVisibleRange/CLAUDE.md`: rewrite the Cadence section (chrono poll → buckets, the wake-alignment
  contract, quantize-toward-faster).
- `FCk_Chrono`'s `Wrap` overflow policy STAYS (public API; no other Source callers after this, but
  it is the documented recurring-cadence primitive for few-entity throttles per Non-goals above).

Gate: full build + the same three suites at baseline counts. Comment audit
(per the 2026-07-21 doctrine) as the closing step.

## Benchmark — `[EDITOR-VERIFY]`, exact manual steps

Agents cannot run PIE; a human must run this before any perf claim is made for the migration.

1. Check out the commit BEFORE Phase 3 (chrono poll) and the final commit (buckets), building each.
2. In PIE, spawn N entities composed with VisibleRange at a mix of `_UpdateInterval` values
   (suggested: 70% at 1s, 20% at 0.25s, 10% at 0 — matching a Poi-heavy scene) at N = 1k, 5k, 20k.
   A throwaway AS gym that `Add`s VisibleRange to N fresh entities and feeds `Update_Distance`
   per frame is sufficient.
3. Measure per build: `stat ckprocessors` (STATGROUP_CkProcessors — `FProcessor_VisibleRange_Update`
   before / the seven `FProcessor_VisibleRange_Update_Bucket<N>` rows after), or Unreal Insights
   with the `cpu` trace channel (each processor is a named scope on the scheduler track,
   `CkProcessorScheduler.cpp:202-212`).
4. Compare the summed VisibleRange Update cost per frame at each N, plus frame-time variance
   (the bucket design trades steady per-frame polling for burstier per-fire work — check for
   spikes at bucket boundaries; the 1s bucket firing 700 evals in one frame is the herd risk
   prerequisite #1 tracks).
5. Record the numbers in this file; if the poll does not register at project-realistic N, the
   Non-goals section's guidance (keep `Tick(dt, Wrap)` for few-entity features) extends to
   everything and the migration is optics-only — say so.

## Phase status

- Phase 1 (TProcessorBase knobs): DONE 2026-07-22 — with the TickRate-literal interface refinement
  described in the plan (`ck::Hz`/`ck::Seconds` consteval literals instead of a raw double trait).
- Phase 2 (bucketing primitive): DONE 2026-07-22 — `CkProcessor_CadenceBuckets.h`; one deviation
  from plan discovered by the build: the tag templates must derive `ck::TTag<Self>` (the registry's
  empty-tag static_assert, `CkRegistry.h:486`).
- Phase 3 (VisibleRange migration): DONE 2026-07-22 — gate: VisibleRange 4/4 (incl.
  CadenceGatesUpdates), Chrono 3/3, Timer 36/36, Poi at baseline.
- Specs: DONE 2026-07-22 — pure quantize/literal spec `Ck.CkEcs.CadenceBuckets.*`
  (`CkProcessor_CadenceBuckets.spec.cpp`); hermetic runtime pins
  `CkTests.UnitTests.CkEcs.Processor.{TickRateTrait_*, CadenceBuckets_*}`
  (CkTests `Test_Processor_TickRateTrait.cpp`) covering rated-vs-unrated fire counts, both catch-up
  policies, immediate-first-eval, and the empty-view accumulator-freeze wake-alignment.
- Benchmark: OPEN — `[EDITOR-VERIFY]` above. No perf claims made; the migration's perf value is
  unproven until a human runs it.
