# DECISIONS.md — CkWebUmg campaign

## DECISION 0 — Build vs. buy — **DECIDED: BUILD** (Adam, 2026-07-31)

Phase 0 recommendation accepted as presented ([PriorArt.md](PriorArt.md) § Build vs. buy): build per the brief's phase plan with scope reductions R1 (CDP extraction), R2 (vendor Yoga if DECISION 1 → C/D), R3 (buy one WebToUMG license as measured baseline — **Adam's purchase, still open**; feeds the Gate 2 threshold argument). The full pre-decision option analysis and movers are preserved in the Phase 0 version of this file at `D:\tmp\ckstyle-phase0\DECISIONS.md` and in PriorArt.md — not restated here.

## Naming — **DECIDED: CkWebUmg family** (Adam, 2026-07-31)

`CkWebUmg` (Runtime) + `CkWebUmgEditor` (UncookedOnly); CLI `ckwebumg-extract`; campaign dir `docs/campaigns/webumg/`. Chosen over CkCss/CkMockup/CkFlexUi. Reason `CkStyle` was unavailable: collides with `namespace CkStyle` (`Source/CkEditorTools/Public/CkEditorTools/Style/CkStyle.h:32`) and `CkStyleSettings.h`; `CkEditorStyle` module also exists (`CkFoundation.uplugin:370`). Harness module placement (CkTests plugin vs dedicated) is a Gate 2 decision — no `*Harness` module precedent exists.

## DECISION 1 — Layout runtime — **DECIDED: option C** (Adam, 2026-07-31, per Gate_01_Decision_Packs recommendation)

Yoga-backed Slate flex panel for Gate 2; lowering pass (option D) is the stated evolution path, deferred until a real hand-editability demand exists. Revisit triggers recorded in the pack: Gate 2 prototype failing text-measure integration at ±1px, or R3 baseline showing native lowering already within tolerance.

## DECISION 2 — Output form — **DECIDED: DataAsset-runtime primary** (Adam, 2026-07-31)

Runtime construction from the IR is the primary output (the Gate 2 harness forces the runtime builder into existence regardless). "Also emit WBP?" re-examined at Gate 4 entry with observed designer-visibility costs.

## DECISION 3 — Regeneration model — **DECIDED: read-only regeneration, ratified** (Adam, 2026-07-31)

Generated output strictly read-only; hand-authoring in subclasses/bound C++; no merge-back. Corpus addendum adopted: duplicate `data-ck-name` is a **hard emit error**, not a warning.

## DECISION 4 — v1 CSS surface — **DECIDED: implemented allowlist ratified** (Adam, 2026-07-31)

The corpus-tested `SUPPORTED_PROPERTIES`/`SUPPORTED_SHORTHANDS`/`VALUE_RULES` in `Tools/ckwebumg-extract/src/extract.mjs` are the v1 surface of record, with the four diagnostic classes (name / value / pseudo-element / computed-divergence). Surface grows by evidence (diagnostic reports from real screens), not speculation. Schema v1 approved same date.

## Gate 2 structural decision — harness home — **DECIDED: CkTests plugin — with a recorded Gate 2 deviation** (2026-07-31)

Target home stays CkTests (owns test infrastructure; no `*Harness` module precedent). **Gate 2 deviation:** at execution time, CkTests' `Source/CkTests/CkTests.Build.cs` was dirty with a sibling session's uncommitted work (their CkParticles test edits) — adding the CkWebUmg dependency line there cannot be committed without publishing their WIP, which shared-worktree discipline forbids. The rect-diff harness therefore lands **inside CkWebUmg** (`Source/CkWebUmg/Private/`) for Gate 2. **Migration trigger:** the sibling's CkTests changes land on `dev` → move the harness to CkTests, delete it from CkWebUmg, re-run the gate observations. Follow-up owned by the campaign, noted in PROGRESS open items.

## Standing evidence notes (carried from Phase 0)

- Yoga vendoring cautions: pin `c766885`; `UseWebDefaults` on; never expose inert grid setters; extractor pre-sorts children by computed `order`; fix `pointScaleFactor` for deterministic harness comparisons (PriorArt §3).
- Engine CEF 128 as CDP backend: **probe done 2026-07-31** — feasible: `-cefdebug=<port>` enables CDP remote debugging (`WebBrowserSingleton.cpp:343-347`). Requires a running editor process hosting an offscreen SWebBrowser, so heavier than system Chrome; not adopted for Gate 1. Revisit only if the Node toolchain becomes a real friction (e.g. designer machines without Chrome/Node).
- Chrome-version drift: extraction rides installed Chrome (auto-updates); IR must record browser version + viewport + dpr; determinism claims are per-browser-version (Gate 1 risk table).
