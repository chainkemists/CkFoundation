# Phase 3 — CkIntent opens: sampler, frame record, ring buffer, octant + SOCD

> **Status:** ✅ CLOSED (2026-08-08, same session) — unit 3-1 gated 97/97 + 3/3
> (`TickRateTrait`), unit 3-2 gated 101/101 (4 octant/SOCD tests by name; the 5 earlier
> Intent tests staying green = additivity proof). Comment audit clean. One STOP ruled mid-
> phase ([P3-D6], the >60fps loss fork — fixed via the Intent_Collect accumulator). Octant/
> hysteresis defaults are PROPOSALS pending the 0A spike. Full-suite delta-zero deferred per
> [P2-D4]. **Depends on:** Phases 1 (✅), 1b (✅), 2 (✅ closed under [P2-D4]). **Gate regime:** scoped only per [P2-D4] — pattern `Ck_AutoTest_In` (covers the 28
> Input rows + the new Intent rows); full-suite delta-zero deferred to campaign end.
> **Scope of record:** PROMPT.md phase-index row 3: "Sampler on `TickRate = Hz(60)` + clamp
> trait; frame record; ring buffer; octant mapping + hysteresis; SOCD policies." Consumes: the
> conditioned axis state (1b), the routed/arbitrated event stream (1), the ButtonId map (2).
> **0A note:** the hardware spike remains un-run (human); [P1-D3]'s ruling carries — the
> expected-observations table pre-writes a response to every outcome, so 0A refines tuning,
> not these structures.

## Rulings at phase open

- **[P3-D1] Module scaffold.** NEW `Source/CkIntent/` (Runtime, Default loading phase,
  standard Win64/Mac/Linux allowlist), Build.cs inherits `CkModuleRules`. Deps: CkCore, CkEcs,
  CkEcsExt, CkInput, CkLog, CkSettings — nothing else without a STOP. **CkInput must never
  depend on CkIntent** (D1's data-flow direction is one-way). Ships `Claude.md` + its row in
  `Source/CLAUDE.md`'s tier table (T4 band).
- **[P3-D2] D19 clamp trait.** The max-catch-up clamp lands on the SHARED processor base
  exactly as 0H specified it (`PHASE_0_RESEARCH.md` — read the 0H answer and implement THAT
  shape; STOP if it no longer matches `CkProcessor.h`). Default = unlimited, i.e. **zero
  behavior change for every existing processor**; only the sampler opts in. This is the
  phase's high-blast edit — it gets its own review callout and its own AutoTest.
- **[P3-D3] Sampler.** Feature quartet on the input-source entity (opt-in `Add`, no `Create` —
  same shape as InputBias/InputButtonMap). Fixed logic step via the existing `TickRate =
  Hz(60)` trait + the [P3-D2] clamp. Each logic frame appends ONE record row to a
  fixed-capacity ring (capacity in ParamsData, default 120 rows ≈ 2 s; overwrite-oldest).
  Rows are value-type/POD-shaped (D4 preserves the rollback option for free) and carry, v1:
  frame index, per-frame button transitions (pressed/released as ButtonIds) + held set,
  the conditioned values of the configured axis pair, and the [P3-D4] delivery outcomes.
  Utils: `Get_LatestFrame`, `Get_FrameCount`, `TryGet_FrameAtOffset` (0 = latest), plus the
  compose/cast family. NO matching, NO phases, NO intent output (Phases 4-6).
- **[P3-D4] Delivery visibility (executes D15-revised's "per-frame layer/consumption state
  joins the record").** The router ADDITIONALLY retains this frame's routed events with their
  per-event outcome (consumed-by-layer / passed-through / dropped-no-owner) in a per-frame
  fragment on the source, reset at the top of each Route pass. The sampler group runs AFTER
  `FGroup_Input_Route` (and before `FGroup_Gameplay`) and reads it same-frame. Additive only:
  the router's existing arbitration behavior is byte-for-byte unchanged. FLAGGED for
  maintainer review (it is the first structure DESIGN_PollSurface.md's delivery-rows argument
  anticipated).
- **[P3-D6] (ruled at unit 3-1's STOP — the Hz(60)-vs-per-render-frame-retention fork.)**
  The composed [P3-D3]+[P3-D4] design loses button edges above 60 fps (retention cleared on
  render frames the rated sampler skips) and duplicates them below (replayed ticks re-read the
  same retention). Ruling = the unit's option (A), refined for the same-group-ordering law:
  a NEW group `FGroup_Intent_Collect` (`RunAfter FGroup_Input_Route`, `RunBefore
  FGroup_Intent_Sample` — linear splice, no cycle) houses an UNRATED accumulator processor
  that appends each render frame's retention rows into a per-source pending buffer (a CkIntent
  fragment — CkIntent reads CkInput, dependency direction preserved). The rated sampler
  CONSUMES the buffer at each logic tick (consume-clears): >60 fps loses nothing (buffered),
  <60 fps replay duplicates nothing (first replayed tick consumes all, later ones see empty).
  All pending events attribute to the row of the consuming logic frame — sub-hitch timing
  fidelity is inherently degraded during a hitch; documented, not hidden. The sampler's
  raw-axis fallback reads the pending buffer, not the retention. Rejected: (B) unrated
  sampling (loses the fixed-timestep determinism D10 exists for), (C) accepting loss
  (contradicts D18's unconditional recording and makes Phase 5's matcher machine-dependent).
- **[P3-D5] Octant + SOCD (unit 3-2).** Octant mapping over the sampler's configured axis
  pair (default gamepad left stick): 8-way + neutral, with hysteresis so a value sitting on a
  boundary cannot flicker between octants (entering a NEW octant requires clearing the
  boundary by a configured margin; staying requires nothing). SOCD cleaning over a configured
  cardinal ButtonId quad (up/down/left/right), policy enum v1: `Neutral` (opposing pair →
  neutral, the tournament default), `LastInputPriority`, `FirstInputPriority`. Cleaned
  direction + octant join the record row. Defaults and margin values are the unit's proposal,
  documented in `CkIntent/Claude.md`.

## Units (sequential — 3-2 consumes 3-1's record row)

**3-1 (single dispatch):** [P3-D1] scaffold + [P3-D2] clamp trait + [P3-D4] router retention
+ the sampler/record/ring quartet + AutoTests: clamp caps replay after a synthetic hitch
(and default-unlimited preserves current behavior); synthetic inject → button edge appears in
the frame record with its ButtonId on the expected frame; conditioned axis value lands in the
row (bias applied); ring wraps at capacity (oldest evicted, count stable); delivery outcome
recorded for a consumed vs passed-through vs unmatched event.

**3-2 (after 3-1's gate):** [P3-D5] octant/hysteresis/SOCD + AutoTests: octant boundaries at
threshold−ε / threshold / threshold+ε with hysteresis asserted both directions; each SOCD
policy on simultaneous opposing presses; cleaned direction lands in the record row.

## Exit criteria

- [ ] Scoped gate green: pattern `Ck_AutoTest_In`, all pre-existing 28 + every new Intent test
      by name (`--generate` on the first gate — the uplugin gains a module)
- [ ] `Source/CkIntent/Claude.md` written; `Source/CLAUDE.md` tier-table row added;
      `CkInput/CLAUDE.md` gains the one cross-reference the retention fragment needs
- [ ] PROGRESS decision log + dated entries current; comment audit run
- [ ] Full suite: NOT run (deferred, [P2-D4]) — campaign-end diff anchor unchanged

## NOT in this phase

No intent grammar/notation/parser (4), no compiled sets (4), no matcher/charge/arbiter (5),
no intent fragment/signals/decay (6), no debugger lanes (7), no gyms (8), no replication
(D4), no dense button indices (Phase 4 bake), no capture-by-ButtonId (still physical-key
captures), no rollback machinery — the record's POD-ness merely preserves the option.
