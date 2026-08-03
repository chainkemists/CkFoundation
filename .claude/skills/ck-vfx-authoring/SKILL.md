---
name: ck-vfx-authoring
description: "Use when authoring or porting a VFX in the CkParticles/CkUsf code-only pipeline: new behaviors, faithful recreations of reference Niagara effects, look shaders, cadence rows, A/B parity work in the VfxExamples gym. Carries the campaign's hard-won incident lessons."
---

# ck-vfx-authoring — building VFX in the CkParticles/CkUsf pipeline

## Overview

This skill distills the Vefects porting campaign (2026-08, 31 systems → behaviors 7, 17,
18–46) into the rules that survive it. It layers ON TOP of the reference docs — load them
per the table below; nothing they own is restated here. What THIS file owns: the porting
method, the incident-derived rules (every one cost a real red or a real hang), and the
testing/perf discipline for VFX work.

| You need | Read |
|---|---|
| Architecture, DI contract, adding a behavior, template/cadence tables, anti-patterns | `Source/CkParticles/CLAUDE.md` |
| The recipe schema + per-effect index, corpus regeneration command | `Source/CkParticles/Cookbook/README.md` |
| One effect's measured constants, verification walk, known gaps | `Source/CkParticles/Cookbook/NS_<Effect>.md` |
| USF-first material looks (DissolveAdd family etc.) | the CkUsf module + `project_ckusf_usf_materials` patterns |
| The full campaign history (rulings [C-D*]/[P*-D*]/[INS-D*], red cycles, session log) | `docs/campaigns/vefects-porting/PROGRESS.md` |

## The porting method (what actually produced parity)

1. **Corpus first, editor last.** Export the source system with the asset exporter
   (sidecar `_meta.exporterVersion` ≥ 3 — earlier exports MISS the system-state loop stack
   and event handlers; regenerate rather than trust). Write the translation sheet
   (`Cookbook/NS_<Effect>.md`) BEFORE code: every constant in the behavior must be a
   measured source value with a §2/§5 citation, never a tuning.
2. **Corpus caveats that misled us:** `[values]` blocks dump the whole Rapid-Iteration
   store including disabled/removed modules — presence ≠ evidence. `Life Cycle Mode =
   System` makes emitter-level Loop rows INERT (the system stack governs). Lifetime
   Min/Max + a random-range pin: the SELECTED pin drives; overrides parked on the
   unselected pin are inert.
3. **Cadence:** loop = the SYSTEM's loop duration; row lifetime = max over layers of
   (spawn delay + resolved life) — it MAY exceed the loop; bursts = sheet counts;
   rate-only sources state a rate and leave BurstCount 0. Never approximate onto the
   nearest template and never fake cadence with `frac(Age/Cycle)` in the behavior — add a
   row.
4. **Implementation-complete ≠ done.** The bar per port: lanes green + A/B pair staged +
   recipe filled. PARITY is a human verdict at the gym; recipes record §13 known
   differences honestly (dropped light layers, world-vs-local space at a stationary
   pedestal) instead of hiding them.
5. **The parity loop (proven on Slash):** human names the visible difference in plain
   words ("starts from the wrong end", "two swipes", "whole swipe visible at t=0") → you
   translate each phrase into ONE mechanism (temporal read direction; pan wrap vs clamp;
   UV clamp at t=0) → fix, regen, re-verdict. Iterate to "couldn't be closer". Never fix
   by feel; every iteration changes a named mechanism.

## Incident-derived rules — each of these cost a real failure

- **GPU/CPU lockstep is law.** Every `Behavior_*.ush` edit lands the identical math in
  `NDICkParticlesLocal::ExecuteStage_CPU` in the same change. Same ids, same hashing,
  bit-identical 24-bit rand.
- **Regen order: textures → looks → templates.** Row renderers resolve look masters at
  build time. When a batch adds new looks, the FIRST RebuildTemplates run fails BY DESIGN
  — regenerate looks, then rebuild again. A template is trusted only if
  `grep -ac ExecuteStage <template>.uasset` is NON-ZERO (a regen on a non-fork engine
  writes inert templates that load fine and keep every existence test green — it cost a
  real silent regression once).
- **Never restate the roster ceiling or size.** `ck::particles::NumBehaviors` and the
  derived `Get_RosterVisTag_Max()` are the only sources. A test that asserted its own
  ribbon tag `==` the ceiling broke the moment the next port landed (Lightning_Hit
  incident) — assert `>=`/derived, never a literal.
- **No replace-all in shared files.** A batch's DataInterface edit touches ONLY its own
  cases + enumerated insertion points. A replace-all once struck three committed sibling
  cases (C2065 storm). Self-diff-audit before commit: every hunk in shared files must
  belong to YOUR behavior.
- **Delta tables state the FULL inherited pair.** Writing only the changed axis of a
  two-component param (DissolveSpeed `(−0.1, 0)` vs `(−0.1, −0.1)`) caused a wrong port.
- **DissolveAdd semantics (settled):** `Mask = smoothstep(0, DissolveEdge, Noise +
  dissolve)`. A panned shape-U is CLAMPED (off-mesh reveal — wrap makes the whole swipe
  visible at t=0 and doubles the pass); the dissolve-noise pan stays WRAPPED.
- **Hidden particles carry VisTag 0.** `Hidden()` helpers run before tag assignment, so
  hidden particles land on the shared camera-sprite renderer with zero size/color —
  harmless on screen, but a VisTag-bucketing test must bucket WHILE-ALIVE and prove
  never-drawn, or it counts ghosts.
- **Ribbons ride a second emitter** (no per-particle visibility on ribbon renderers):
  seed bank `RibbonSeedBase`, RibbonId via MeshIndex, width via `Size.x`. A dual-emitter
  template greps ~80 ExecuteStage vs ~40 single — only ZERO is failure.
- **Light renderers are CPU-sim-only**; our templates are GPU-sim. Drop light layers as a
  §13 known difference — don't fake them with sprites.
- **A palette twin still needs its own behavior id** (id → one template path is the spawn
  contract) but shares one implementation file and one renderer band (explosion family
  40–43 pattern).
- **`_UsedWithNiagara*` usage flags are independent** — a look drawn by two renderer
  kinds needs the master compiled for both, or one kind renders default-glow.

## Testing discipline for VFX

- Gate shape: `--test --parallel 1 --discover-fresh --no-nullrhi --no-live`, pattern
  `Particles` (not `CkParticles`) / `VfxExamples`. `--no-nullrhi` because Niagara refuses
  to spawn components when the process can't render; tests must skip-with-trace under
  nullrhi, silent at Warning level (the harness escalates Warnings to failures).
- New AS test classes need `--discover-fresh` (discovery is cached; green-with-old-Total
  is stale-green). New C++ `.spec.cpp` additionally needs a relink.
- **Assert corpus-derived values, not rounded restatements.** A "5×" flashbulb assertion
  was red against the true curve value 4.875984; the fix is asserting what the corpus
  chain reproduces to the last float32 digit — never weakening the test.
- **Destroy-in-frame in roster sweeps.** A test that spawns many live systems and lets
  them accumulate collapses frame times (see perf below). Capture validity, then
  `DestroyComponent()` in the same frame — per-pair local spawn lists with per-iteration
  cleanup, not end-of-test sweeps.
- Sustained-sim probes: chain `WaitOneFrame` (each consumes one REAL frame) and bracket
  phases with `ck::Trace` markers — the LOG's timestamps are the wall clock; game-time
  deltas under-report hangs because engine dt is clamped.

## Perf traps

- **Many live systems collapse the editor.** All 31 pairs at once (~370 emitter
  instances) produced minutes-long frames and a 60-70%-CPU unresponsive editor. The
  VfxExamples gym therefore runs ONE pair at a time ([INS-D2]); keep any new gym or test
  in that regime.
- **Do NOT add a dt clamp.** The engine already clamps Niagara sim dt at 0.125 s by
  default (`UNiagaraSettings::bLimitDeltaTime`/`MaxDeltaTimePerTick`, consumed via
  `UNiagaraSystem::MaxDeltaTime`). The "unclamped dt feeds rate emitters" runaway theory
  is DEAD ([INS-D1]) — populations cap at rate × lifetime regardless of frame rate.
- **A cold template's Niagara system compile BLOCKS the game thread for minutes** (the
  "PickupLoop hang", root-caused 2026-08-03). First sustained activation of a template
  whose compiled Niagara data is not in this machine's DDC fires
  `LogNiagara: Compiling System ... PS_CkParticles_Template_<X>` and the game thread
  stalls until it finishes — measured 133 s in one stall (the automation controller
  logged `Ignoring very large delta of 132.92 seconds`; the editor reads as hung at
  60-70% CPU). The result caches to the machine DDC: the identical probe re-run hit
  60-98 fps, and a 60-frame baseline vs 60 frames with the system alive measured
  3.29 s vs 3.44 s — the SIM is ~free; the one-time compile is the whole cost. Two
  traps this creates: (a) spawn-and-destroy tests (roster sweeps, pair-contract tests)
  never keep the system alive long enough to complete/cache the compile, so every green
  lane can leave templates cold; (b) each cold template pays its own stall on first
  activation — a gym walk across fresh regens can "hang" once per pair. After
  regenerating templates or bumping any dependent `.ush` (the DI's compile hash covers
  them, busting EVERY template), PRE-WARM by activating each template until the compile
  completes, or warn the human that first activations stall once and self-resolve.
  Probe pattern: `CkTests Script/CkParticles/CkAutoTest_Particles_PickupLoopSustainedSim.as`
  (baseline-vs-spawned phases with logged markers; use `Print` for log-forensic markers
  and read the FULL editor log — `ck::Trace` is on-screen only, and the toolbox
  `--output` stream is curated, so neither shows them; `Saved/Logs/CkPlugins*.log` does).

## VfxExamples gym (the A/B verification harness)

- One pair EXISTS at a time — the active pair's two stations spawn at fixed spots;
  switching swaps the world in place. Selector: V (list, search, Enter), PgUp/PgDn cycle,
  R restart; `Ck_GymVfxExamples_RestartAll` re-fires both sides in sync (name kept — the
  recipes cite it). Originals resolve by PATH STRING only (`/Game/Vefects/...` then
  `/Vefects/...`) — no package dependency; absent installs show a placard.
- Adding a port's pair = a data edit in `CkVfxExamplesGym_Shared.as` (tags registered in
  the same file's tag asset) — no harness change.
- Keys chosen the hard way: F-keys collide with the editor's rendering debug modes; a
  letter toggle can only OPEN a menu that has type-to-search (it types once inside).

## Rationalization table

| Excuse | Reality |
|---|---|
| "The number looks right, close enough to the corpus." | A rounded restatement was red once. Assert the corpus-chain value to the last digit or re-derive it. |
| "I'll tweak until it looks like the reference." | The parity loop names ONE mechanism per visible difference. Feel-tuning destroys the measured provenance the whole cookbook is built on. |
| "The suite is green, the port is done." | Green lanes are the floor. Parity is a human A/B verdict at the gym, and §13 records what genuinely differs. |
| "This effect's cadence is close to an existing row." | Approximating onto the nearest template is banned — add a row. |
| "I'll clean up the whole switch while I'm in there." | Shared-file edits touch only your cases. The one time this rule was broken it struck three committed behaviors. |
| "Plan says N effects, N done, complete." | Verify completion against the SOURCE inventory, not the plan's tables — the plan itself silently dropped NS_Dash once. |
