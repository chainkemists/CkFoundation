# Sub-Instanced Cadence Processors — Design & Implementation Plan

**Status:** design only, NOT implemented. Both prerequisites below are hard gates. This is a
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
