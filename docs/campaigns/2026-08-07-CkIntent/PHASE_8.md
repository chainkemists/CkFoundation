# Phase 8 — gyms (Fighting / Souls / Debugger) + the 40-move bake + gap-closing

> **Status:** 🟡 OPEN (2026-08-09). **Depends on:** Phases 3-6 (✅) for all units; unit 8-4's
> *drive-through* additionally wants unit 7-2 landed (compile-independent — the gym generates
> traffic, the debugger reads recorded state; no code dependency either direction).
> **Gate regime:** scoped per [P2-D4]. Gym visuals are `[EDITOR-VERIFY]` by definition.
> **Scope of record:** PROMPT.md phase-index row 8: gyms (`Fighting`, `Souls`, `Debugger`) +
> autotest gap-closing. Success criteria living here: **6** (40-move bake, headless), **1**'s
> `[EDITOR-VERIFY]` leg (gamepad QCF+Punch, on-screen 0-frame deferral counter), **7** (audit).

## Rulings at phase open

- **[P8-D1] Unit split + order:** 8-1 the 40-move bake AutoTest (criterion 6 — independent of
  everything else and validates the grammar at scale before the gyms lean on notation); 8-2
  `Gym_Input_Fighting`; 8-3 `Gym_Input_Souls`; 8-4 `Gym_Input_Debugger` + the coverage
  gap-close. Script/ serialization law holds: units draft `.as` files to the scratchpad when
  any toolbox run is in flight; the orchestrator installs and gates ([P1A-D2] precedent).
- **[P8-D2] The 40-move set is an asset-shaped AS container** (mimic the battery's asset idiom,
  `CkInput_Assets.as`): ~40 rows of {name, notation string, priority}, covering the motion
  vocabulary breadth-first (QCF/QCB/HCF/HCB/DP-style runs, charge holds, chords incl.
  direction+button, plain taps, lenient + windowed variants). The AutoTest parses + bakes ALL
  rows through the one public `Parse`/`Bake` surface and asserts: zero rejections, compiled-set
  intent count == authored count, spot-checked resolution rows + deferral verdicts (at least:
  one suffix-terminal proven no-defer, one chord-window verdict, one hold verdict). **Criterion
  6's "no hand-written per-move struct construction" is enforced by review-grep of the test +
  asset file** (no `FCk_Intent_Definition` / step-struct construction outside the parser call),
  recorded in the unit's return.
- **[P8-D3] Gym scope caps (v1)** — all three are SM-driven self-asserting stations per
  [P1A-D7] (WHAT/EXPECT/GOT + PASS/FAIL on the panel, colored lines via
  `Update_StationDisplay_Colored`, red = true failure only per [P1A-D8]), registry rows in
  `Script/Common/CkTests_GymRegistry.as`, content faces world -X, floors Z-scale ≥ 0.5:
  - **Fighting:** QCF+Punch with the **on-screen deferral-frame counter** (criterion 1's
    `[EDITOR-VERIFY]` leg — must read 0 on a clean QCF+P); a deferred-vs-instant contrast
    station (chord-ambiguous button vs suffix terminal); a near-miss station (too-slow QCF)
    that names what the debugger's near-miss view should show. Gamepad is the target device;
    keyboard fallback bindings allowed for deskless drive.
  - **Souls:** tap-vs-hold on one shared button (threshold−1 / threshold / threshold+1
    echoed on-panel); a charge-accumulator station (D11); a delivery-loss station (layer
    push mid-hold) demonstrating the D15 DEFAULT pair only (`Cancel` + `RequireRePress`) —
    the opt-in policies are NOT constructible in v1 per [P5-D6] (amended 2026-08-09; the
    original wording contradicted that ruling).
  - **Debugger:** a traffic generator for 7-2's views — stations that produce a failed 236
    scan with `ck.Intent.RecordScanDiagnostics` on (criterion 5's scrub fodder), a deferral
    episode, a layer push/pop, an octant sweep; each station panel names WHICH debugger view
    to watch and what it should show. No new read APIs, no recompute ([P7-D1] applies).
- **[P8-D4] "Autotest gap-closing" = an orchestrator-run coverage audit at phase close**, not
  a speculative unit: criteria 2/3 already have named green tests (Phases 5-6); the audit
  sweeps criterion 7 (C++/BP/AS exercise of every NEW public Utils surface from Phases 1-7)
  and only demonstrated gaps become dispatches.

## Units

**8-1 (CkTests, AS):** the [P8-D2] asset + AutoTest. Gate: scoped `Ck_AutoTest_In`
`--discover-fresh` (+1 row, by name).

**8-2 / 8-3 / 8-4 (CkTests, AS gyms):** per [P8-D3], sequential dispatches (shared gym
infrastructure files — `CkTests_GymRegistry.as`, shared display helpers — make parallel
writes collide). Gate per unit: compile green in the scoped run (gyms add no autotest rows
unless a unit's package says otherwise) + registry row present; visuals →
`[EDITOR-VERIFY]` queue with exact drive steps.

## Exit criteria

- [ ] 8-1's bake test green BY NAME in the scoped gate; review-grep recorded (criterion 6)
- [ ] Three gyms compile, registered, each panel self-asserting per [P1A-D7]
- [ ] Coverage audit run; gaps either closed + gated or recorded as accepted with reasons
- [ ] `[EDITOR-VERIFY]` queue updated (criterion 1 counter, three drive-throughs, criterion
      5 scrub fodder pointing at the 7-2 views)
- [ ] PROGRESS current; comment audit over all new `.as`
- [ ] Full suite NOT run ([P2-D4] — campaign end only)

## NOT in this phase

No production-code changes to CkFoundation (a gap found by the audit that needs production
code STOPs to the orchestrator); no new grammar syntax (D9 revisit clause); no debugger UI
changes (7-2 owns those); no replication; no `.uasset` hand-authoring.
