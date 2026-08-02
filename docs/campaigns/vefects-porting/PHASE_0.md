# Phase 0 — Exporter ground truth + the two zero-capability ports

**Goal (one sentence):** make the corpus tell the whole truth (system stack, event stacks,
resolved lifetime pins), then prove the port loop on the two effects that need no new pipeline
capability: `NS_Gunshot_Projectile` (behavior **18**) and `NS_Arrow_Projectile` (behavior **19**).

**Entry criteria (verify on disk before starting):**
- The method-proof work is present as COMMITTED history on `feature/particles-cookbook`
  (commits `d02792972`..`fad347d94`, 2026-08-02: DissolveAdd family, Slash re-port, regen,
  recipes, sheets). Any *dirty* files beyond the current unit's own edits are foreign — leave them.
- Lanes baseline: `Particles` 8/8, `CkUsf` 4/4, `VfxExamples` 1/1 (capture fresh counts + log
  paths; record in PROGRESS.md).
- `Saved/CkVfxCorpus/index.json` exists (corpus present; will be regenerated in 0.2).

## Units

### 0.1 — CkAssetExporter: E1 + E2 + [C-D1] resolved-pin export  *(opus; C++, CkFoundation)*

Files: `Plugins/CkFoundation/Source/CkAssetExporter/` (read its Claude.md + the existing Niagara
exporter first; mimic its traversal idioms).

- **E1**: export the SYSTEM-level System State stack (system Loop Behavior / Loop Duration / Life
  Cycle mode) into each system's sidecar — new top-level `systemState` block.
- **E2**: export per-emitter Event Handler stacks (source emitter, event name, spawn count/mode)
  — new `eventHandlers` array per emitter; empty array when none (absence must be
  distinguishable from not-exported: bump `exporterVersion` to 3).
- **[C-D1]**: for `Initialize Particle`'s Lifetime input, export WHICH source actually drives the
  pin — module Min/Max vs rapid-iteration dyn override — as `lifetimeResolved: {source, values}`.
  If the store genuinely cannot disambiguate, emit `lifetimeResolved: {source: "AMBIGUOUS"}` and
  STOP after 0.2 with that finding (editor-fallback branch belongs to the maintainer).
- verify: exporter module compiles; **no behavior/runtime module touched**.

### 0.2 — Corpus re-export + reconciliation sweep  *(orchestrator gates; sonnet for the sweep)*

- Orchestrator: editor closed → build via toolbox → `CK_VFX_CORPUS_EXPORT=1` +
  `--test-pattern ExportVfxCorpus` lane → verify sidecars carry `systemState`, `eventHandlers`,
  `lifetimeResolved`, `exporterVersion: 3` (spot-check NS_BasicAttack: system loop must read
  1.0 s — the human-verified cadence — and NS_Lightning_Range likewise).
- Sweep unit (sonnet): update every sheet's `[unresolved]` cadence/lifetime entries from the new
  fields; flag (do not silently fix) any sheet whose §2 anatomy now contradicts the corpus.
  Output = list of sheets changed + contradictions found. **No invention: a value not in the
  corpus stays `[unresolved]`.**
- Decision gate: contradictions that change a TIER or a capability requirement → STOP,
  orchestrator rules and records `[P0-D*]`.

### 0.3 — Port NS_Gunshot_Projectile (id 18)  *(opus)*

Spec = `Cookbook/NS_Gunshot_Projectile.md` §1–6 (re-read AFTER 0.2 reconciliation) + the per-port
checklist in PORTING_PLAN.md. Notes fixed by this phase: shared cadence row (name it for the
cadence, not the effect — both projectiles use it); looks `PartDisAdd01` (new; delta table per
tripwire 7) + `PartDisAdd04` (exists, bind unchanged); velocity-aligned sprites only. Tests: a
`Test_Particles_GunshotProjectileBehavior.cpp` in the LightningRange/Slash style (curves,
anti-vacuity, layer partition), roster tests pick the id up automatically. Gym: pair row in
`CkVfxExamplesGym_Shared.as` (candidates `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Gunshot_Projectile`
then `/Vefects/...`; credit line). Recipe §7–14 filled.

### 0.4 — Port NS_Arrow_Projectile (id 19)  *(opus)*

Same shape; shares 0.3's cadence row (verify identical post-0.2, else STOP). Its one camera-facing
sprite layer binds via the existing VisTag-0 `User.SpriteMaterial` path (sheet documents this as
the no-C1 route) — do NOT build C1 early for it.

### 0.5 — Gate + A/B  *(orchestrator + maintainer)*

- Orchestrator: editor closed → build → looks lane (CkUsf) → `RebuildTemplateAssets` (env var) →
  `Particles` (expect 8/8 + the two new C++ tests = 10/10; count may differ if tests merge —
  record actuals) → `VfxExamples` → ExecuteStage greps incl. any NEW template.
- `[HUMAN-VERIFY]`: maintainer A/Bs both new pairs in the VfxExamples gym per each recipe's §12.
  Parity verdict per pair recorded in the recipe + PROGRESS.md.

## Exit criteria (all VERIFIED with named evidence)

1. Corpus sidecars carry the three new field groups at `exporterVersion: 3`; NS_BasicAttack
   system loop reads 1.0 s.
2. The [C-D1] lifetime rule is SETTLED (mechanically, or the maintainer's editor ruling recorded
   as `[P0-D*]`) and applied across the sheets.
3. Behaviors 18 + 19 implemented per their sheets; all lanes green in the orchestrator's session;
   both pairs at maintainer-judged parity (or the miss recorded as a finding driving the next
   iteration — parity is the bar for phase CLOSE).
4. PROGRESS.md current (Done evidence, decisions, session log).

## Fences

- Do not touch `DissolveAdd.ush` semantics, the Slash/LightningRange behaviors, or the roster
  tests' derived ceilings.
- Do not add C1/C4/etc. capabilities in this phase — Phase 1 owns them.
- Exporter changes must not write to any asset (read-only export; the corpus lands in `Saved/`).
- Anything not enumerated here → STOP, record in PROGRESS.md Blockers.
