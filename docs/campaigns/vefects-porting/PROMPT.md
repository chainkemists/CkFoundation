# Campaign: Vefects → CkParticles porting — parity on every effect

**Freshness:** authored 2026-08-02 (Fable orchestrator session). Death condition: campaign complete
(VALIDATION.md satisfied) or superseded by a successor doc — tombstone here if so.

## Goal / done-definition

Port every remaining `Anime_VFX/Shared/Skills` Vefects system (27 behaviors + 2 ice palettes = 29
originals) into CkParticles at **human-judged A/B parity** in the VfxExamples gym, following the
method proven on `NS_BasicAttack` (behavior 7) and `NS_Lightning_Range` (17). Done =
VALIDATION.md's protocol passes: every pair judged at parity by the maintainer, all test lanes
green, zero `/Game/Vefects` package dependencies, every recipe's §7–14 filled with evidence.

**Non-goals:** the `Anime_Stylized_VFX` sibling pack (different, parameterized systems); world-space
emitter capability (C12 — recorded as §13 known-difference per sheet; stationary A/B pedestals are
unaffected); importing ANY pack art (meshes, textures) into plugin content.

## The method (proven; do not improvise a different one)

Per effect: translation sheet (already written, `Plugins/CkFoundation/Source/CkParticles/Cookbook/NS_<X>.md`)
→ implement its §6 → gates → human A/B in VfxExamples → fill §7–14 honestly. The per-port
checklist is `Cookbook/PORTING_PLAN.md` § "Per-port checklist". PORTING_PLAN.md also owns the
capability matrix (C1–C12), tier census, and wave order — this campaign executes it.

## Standing decisions (recorded — never re-litigate)

- **[C-D1]** Lifetime ambiguity (~28 emitters): resolve MECHANICALLY via the Phase-0 exporter
  improvement (dump the resolved pin binding). Only if the exporter cannot see it → maintainer
  checks DebuffLoop's `Arrow_Green`/`Arrow_Purple` in the editor once; the ruling generalizes.
  Never silently assume either reading. (Maintainer, 2026-08-02.)
- **[C-D2]** Bomb prop mesh: procedural stylized stand-in (sphere + fuse) with the toon-banded
  material; shape gap recorded in §13. No mesh import. (Maintainer, 2026-08-02.)
- **[C-D3]** Ice explosions: palette variants — TWO behaviors (ExplosionGround, ExplosionOmni),
  each with a fire/ice per-layer palette table selected by a parameter; the VfxExamples gym still
  gets four pairs (each palette vs its own original). (Maintainer, 2026-08-02.)
- **[C-D4]** Orchestration: Opus 5 executes phases; ALL toolbox gates are re-run by the
  orchestrator session (main loop), never trusted from a sub-agent; Fable-tier (or best-available
  fresh-context) audit at each phase boundary. (Maintainer directive, 2026-08-02.)
- **[C-D5]** Textures are procedural-only, parameterized from MEASURED characteristics of corpus
  PNGs (spectra, ridge counts, contrast percentiles) — derive numbers, never copy pixels.
  (Maintainer, 2026-08-01, reaffirmed for the campaign.)
- **[C-D6]** Behavior ids: allocated at port time in wave order, one ordered pass (next free id at
  campaign start = 18). Sheets never carry ids.

## Constraints & tripwires (each one has burned a session — treat as law)

1. **Gates**: every lane `--parallel 1 --discover-fresh --no-nullrhi --no-live`, test-only lanes
   never pass `--config`; pattern `Particles` not `CkParticles`. Trustworthy = fresh full-size
   lane log (~540 KB) + tally read from the log, in the ORCHESTRATOR'S session.
2. **Sub-agent builds/lanes DIE with the agent's session.** A result like "build running" or
   "waiting for lane" means NOT RUN. Executors author; the orchestrator gates.
3. **No builds while any editor holds `Saved/Logs/CkPlugins.log`** (hook enforces). No source/AS
   edits while a lane is in flight. Machine is shared with a sibling session — check for foreign
   builds before lanes.
4. **Regen order: CkUsf looks BEFORE templates** (row renderers resolve masters at build time).
   After regen, `grep -ac ExecuteStage` on every `PS_CkParticles_Template*.uasset` must be
   non-zero.
5. **GPU/CPU lockstep** is non-negotiable: every `Behavior_*.ush` change mirrors exactly in
   `CkParticles_DataInterface.cpp`. Curves = clamped-key lerps verbatim from the sheet; velocity
   decay via closed form, not integration.
6. **DissolveAdd family semantics (settled 2026-08-02, do not regress):** dissolve mask =
   `smoothstep(0, Edge, Noise + dissolve)` (channel ADDS to noise); the panned shape-U is
   CLAMPED (off-mesh = invisible reveal, not a wrapping rotation); dissolve-noise tiling stays
   wrapped. Behavior 17 depends on both.
7. **Delta tables state the full inherited pair**, never just the changed axis (the SlashDisAdd04
   `(−0.1, 0)` bug). When reading a sheet's §4, absent = inherits the REFERENCE, not zero.
8. **Corpus caveats:** `[values]` blocks include disabled/removed module params (presence ≠
   evidence); emitter-level Loop rows are INERT under `Life Cycle Mode = System` (Phase 0 fixes
   the exporter to dump the system stack); sibling-pack discriminator = `userParameters: []` +
   `M_VFX_DisAdd_*` materials (emitter counts do NOT discriminate).
9. **New AS autotests** need `--discover-fresh`; new C++ automation tests need a relink before
   discovery. AutoTest harness escalates any Warning to a failure — missing-content paths must be
   log-silent.
10. **Scope hygiene:** never blanket-stage; do not commit/push/reset/clean — the maintainer ships.
    Preserve all dirty work you did not author. No `/Game/Vefects` reference in code or content —
    `LightningRangeAuthoring` asserts it and every port's authoring test must too.

## File inventory (the load-bearing files)

- `Plugins/CkFoundation/Source/CkParticles/Cookbook/` — PORTING_PLAN.md + 29 `NS_<X>.md` sheets +
  the two COMPLETE exemplar recipes (NS_BasicAttack.md — read §13/§14 for every trap above in long
  form; NS_Lightning_Range.md).
- Naming header (`CkParticles_ScriptDefinition_Naming.h`): roster, `Get_TemplateSpecs()` cadence
  table + `RendererOverrides`, `Get_BehaviorLookName`, `Get_RosterVisTag_Max()` (derived — never
  restate a ceiling).
- `Source/CkParticles/Shaders/CkParticles/Behaviors/*.ush` + `CkParticles_DataInterface.cpp` (CPU
  mirror), `CkParticlesEditor` generators (Texture/Mesh/TemplateBuilder), CkUsf `DissolveAdd.ush` +
  `Script/CkUsf/*_Assets.as` looks, `Plugins/CkTests/Script/CkVfxExamples/` (gym pair registry),
  `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkParticles/` (tests).
- `Saved/CkVfxCorpus/` — machine-local evidence; regenerate via `CK_VFX_CORPUS_EXPORT=1` +
  `Ck.AssetExporter.ExportVfxCorpus`.

## Executor session-start ritual (verbatim, every session)

1. Read `Plugins/CkFoundation/docs/campaigns/vefects-porting/PROGRESS.md`; spot-check TWO Done claims against cited
   artifacts (open the file:line / re-run the grep / read the log tail).
2. Read the current PHASE_N.md; verify its entry criteria hold on disk.
3. Load skills: `ck-methodology` (CkFoundation-scoped variant), `build-test`,
   `ck-tests-authoring-and-running`; this PROMPT.md's tripwires section.
4. Capture the baseline: run the three lanes (`Particles`, `CkUsf`, `VfxExamples`) and record
   counts before changing anything.
5. Execute units in order; STOP on any unenumerated observation; record decisions/blockers in
   PROGRESS.md immediately, not at session end.
