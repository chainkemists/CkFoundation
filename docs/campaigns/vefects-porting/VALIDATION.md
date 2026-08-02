# VALIDATION — definition of done for the Vefects porting campaign

The campaign is complete when every row below is checked with named evidence.

## Per-effect acceptance (applies to every ported behavior; recorded in its recipe)

1. Recipe `NS_<X>.md` §7–14 filled; §13 lists every known difference honestly; delta tables state
   full inherited values.
2. Focused C++ behavior test green (curves vs sheet keyframes, layer partition, anti-vacuity);
   GPU/CPU lockstep reviewed on the final diff.
3. VfxExamples pair row present (path-string originals, both mount candidates, credit); pair
   autotest green; no Warning on the missing-content branch.
4. No `/Game/Vefects` package dependency (authoring-test assertion, LightningRangeAuthoring
   style).
5. **`[HUMAN-VERIFY]` A/B parity**: maintainer judges the pair in the VfxExamples gym per the
   recipe's §12 criteria. Parity verdict + date recorded in the recipe's completion state. A miss
   is a finding that drives a measured iteration (the Slash pattern: diagnose numerically → fix →
   regen → re-judge) — not a shrug, not a silent accept.

## Campaign-level gates (run at every phase boundary and at close)

- Lanes, orchestrator-run, `--parallel 1 --discover-fresh --no-nullrhi --no-live`:
  `Particles` / `CkUsf` / `VfxExamples` — all green, tallies read from full-size fresh logs.
- `grep -ac ExecuteStage` non-zero on every `PS_CkParticles_Template*.uasset`.
- Roster invariants derived, never restated: `Get_NumBehaviors()`, `Get_RosterVisTag_Max()`.
- Behaviors 7 and 17 still at parity after any shared-shader/generator change (numeric
  inertness proof for default-inert changes; re-A/B if a deliberate change lands).
- `git status` across CkFoundation + CkTests shows only campaign-authored changes; nothing
  committed/pushed without the maintainer's explicit instruction.

## Close-out

- PORTING_PLAN.md tier census updated to final state; PROGRESS.md status board all-green with the
  final session log; stale campaign docs tombstoned per ck-methodology §7.
- Maintainer sign-off on the full VfxExamples gym walk (all pairs, one session).
