# PROGRESS — Vefects porting campaign

**Read me first on every resume. Trust this file over memory; distrust it enough to spot-check
two Done claims against their cited artifacts.**

## Status board

| Phase | State |
|---|---|
| Pre-campaign (method proof: behaviors 7 + 17, harness, sheets, plan) | **Done** |
| Phase 0 — exporter ground truth + 2 zero-capability ports | **In progress** (unit 0.1 dispatched 2026-08-02) |
| Phase 1 — C1/C3/C4/C9-FlatAdd + 6 M-tier ports | Pending |
| Phase 2 — continuous cadence + lifecycle → Loops/Casts (10) | Chartered |
| Phase 3 — ribbons → trails (4) | Chartered |
| Phase 4 — light/facings/palettes → explosions (3 behaviors / 5 pairs) | Chartered |
| Phase 5 — Lightning_Hit + campaign close | Chartered |

## Done (evidence-backed)

- **NS_BasicAttack (behavior 7) at HUMAN A/B PARITY** — maintainer verdict 2026-08-02
  ("couldn't be closer"), recorded in `Cookbook/NS_BasicAttack.md` completion state; final lanes
  CkUsf 4/4, Particles 8/8, VfxExamples 1/1 (`Saved/Logs/Test-*-PanClamp.log`).
- **NS_Lightning_Range (behavior 17)** implemented + gated; §12 human walk not formally recorded —
  fold into the next maintainer gym session. Note: its dissolve read INVERTED (assembles, not
  erodes) under the corrected family math — §12 row f rewritten accordingly; judge by the original.
- **VfxExamples A/B harness** — pair registry (path-string originals + placard), synced restart,
  `PairStationsSpawn` autotest; loop re-arm via OnSystemFinished.
- **DissolveAdd family semantics settled** — additive dissolve + clamped pan
  (`DissolveAdd.ush`); independent numeric verification in the 2026-08-02 workflow journals
  (`.claude/.../subagents/workflows/wf_df0bc139-496`, `wf_4b86d5bb-92a`).
- **29 translation sheets + PORTING_PLAN.md** in `Cookbook/` (5-batch archaeology, 2026-08-01).
- **512×512 measured texture bakes** — spectra-verified vs corpus paints.

## In-flight

- **Unit 0.1 AUTHORED (Opus, 2026-08-02), not yet gated**: `CkNiagaraExporter.{cpp,h}`,
  `CkAssetExporter_ExportMeta.h` (Niagara sidecar `exporterVersion: 3`), Dispatch comment,
  module Claude.md — dirty in the tree, deliberately uncommitted until the orchestrator builds +
  re-exports the corpus (unit 0.2). Headline: **[C-D1] resolved MECHANICALLY** — Initialize
  Particle's `Lifetime Mode` static-switch pin is the disambiguator (`Direct Set` ⇒ Lifetime pin,
  `Random` ⇒ Min/Max; overrides win only on the DRIVING pin; inert leftovers exported as
  `inertOverrides`/`inertValues`). No editor fallback needed. Caveat for spot-checks: the corpus
  `index.json` has an unrelated `exporterVersion: 3` — read the SIDECAR's `_meta`, not the index.
- Unit 0.2 (orchestrator build + corpus re-export + reconciliation sweep) is next; 0.3–0.5 queued.

## Decisions

- **[C-D1]..[C-D6]** — see PROMPT.md (lifetime rule, bomb mesh, ice palettes, orchestration,
  procedural textures, id allocation). Ruled by maintainer 2026-08-02.
- **[C-D7]** Phases 2–5 chartered in one file, full PHASE_N.md authored at each boundary — decay
  avoidance per ck-methodology §7. (Orchestrator, 2026-08-02.)

## Blockers

- **Remote git operations classifier-blocked in the orchestrator session (2026-08-02):** the
  `--force-with-lease` push of rebased `feature/particles-cookbook` and even plain `git fetch`
  were denied. All commits are LOCAL. Maintainer runs the pushes (commands in the session
  report / Next step) or adds a Bash permission rule; the root submodule-pointer bump waits
  until both submodule tips are confirmed on origin (cross-repo publish guard).

## Next step

1. Maintainer pushes: `git -C Plugins/CkFoundation push --force-with-lease origin
   feature/particles-cookbook` (backup branch `backup/particles-cookbook-pre-rebase` is the undo)
   and `git -C Plugins/CkTests push origin dev`.
2. Orchestrator: verify tips ⊆ origin → root gitlink bump commit → maintainer pushes root `dev`.
3. Unit 0.2: build (editor closed) → corpus re-export → verify v3 sidecar fields (NS_BasicAttack
   system loop = 1.0 s) → commit unit 0.1's files → reconciliation sweep → ports 18/19.

## Open items for the maintainer (non-blocking)

- Commit checkpoint strongly recommended before Phase 0 lands: the uncommitted surface now spans
  2 behaviors, the shader family, the harness, 29 sheets + plan + campaign docs. Say the word and
  the orchestrator prepares scoped commits per submodule for your review.
- LightningRange formal §12 walk (5 min, fold into next gym session).
- Slash open question: family DistortScale 0.1 vs corpus 1.0 on the only live-distortion layer —
  judge the wobble character when convenient; one-token change if too tame.

## Session log

- 2026-08-01/02 — Fable orchestrator: method proof (7+17), harness, 3 A/B iterations to Slash
  parity, 29 sheets + plan, campaign doc set authored, [C-D1..7] ruled, Phase 0 opened, unit 0.1
  dispatched to Opus.
