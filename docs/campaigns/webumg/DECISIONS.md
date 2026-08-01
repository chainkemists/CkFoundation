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

## Gate 2 exit — layout threshold — **DECIDED: ±1px ratified** (Adam, 2026-08-01)

"Every element's final rect within ±1px of the Chromium box model at the reference resolution" (brief §10) is now the enforced, measured bar — all 9 L-pages pass it in the default automation suite (884/884 full run). NaN is a hard failure on every node. Recorded deviation at close: CkWebUmg has no BP/AS surface yet — the three-environments rule attaches to the reflected DataAsset API arriving at Gate 4, not to the internal Slate/loader layer; revisit at Gate 4 exit.

## Gate 3 execution decisions (2026-08-01, session 3 — made under Adam's standing "continue and follow your recommendations" / "tackle any issues and additional items")

Each reversible, each measured; revisit at the Gate 3 exit if any reads wrong:

- **Paint strategy = CPU-baked transient textures** (gradients, shadows, per-side border rings): per-pixel browser math in sRGB space + `SRGB=true` texture round-trip. Chosen over material brushes after the gradient bake measured EXACT (P2 0.0000%); the Gate-plan shadow alternatives were consequently not built (could only score worse at higher complexity). Editor-time/test-path only today — the Gate 4 emitter decides what form ships in assets.
- **Harness font mapping = OS faces** (Arial/Segoe UI/Verdana from `C:/Windows/Fonts`; bold cut ≥600; **Black cut ≥800** for Arial — closes Chromium's synthetic-bold divergence, 13.8%→≤2.9%). Rationale: the goldens were rendered by system Chrome with exactly these faces; no other mapping can converge. Machine-local by design; the emitter's shipped font config is Gate 4+ scope.
- **v1 surface widened: per-side border widths AND colors** (was: diagnosed limit, listed as Adam's option in Positions_S8). Done under the blanket "tackle additional items" authorization; extractor B5 diagnostic retired, `borderColors` typed. P1 0.7412%→0.1695%.
- **`<br>` = forced break** folded to a newline in IR `text.content` (whitespace collapses per segment); builder renders it, comparator segments on it. Capability landed; no corpus page exercises it yet.
- **NullRHI lane policy**: pixel suite skips explicitly (`GUsingNullRHI`) instead of failing as a lane artifact; full-suite baseline now **908/908**.

## Gate 3 exit — **DECIDED 2026-08-01** (Adam delegated: "go with the best recommendation grounded in data, not guesses")

1. **Paint threshold = per-class budgets, ENFORCED** in `CkWebUmg_PaintFidelity_Test.cpp`: solids (L*/C*) 0.2%; opaque paint pages (P1/P2/P5/smoke/T*) 0.5%; translucency-bearing pages (P3/P4) 7%. Rationale: a single global number would need >=5.75% to pass P3, gutting the gate for pages that measure 0%; the P3/P4 band IS the sec-8.4 policy (below).
2. **Shadow strategy = baked Gaussian textures, RATIFIED** (sigma=blur/2 over SDF rounded-rect coverage, sRGB layer compositing). Evidence: single black drop exact; residual entirely sec-8.4. Alternatives unbuilt on measured grounds; WebToUMG's parametric-MI shadow choice noted in PriorArt as the unmeasured alternative.
3. **Text tolerance = line-count equality + per-line advance within max(3%, 3px), ENFORCED** in the rect harness's line-run comparator. Dataset: 25 runs, 0 mismatches, mean 0.78%, max 2.9%.
4. **Sec-8.4 translucency policy = option 1** (accept + per-page tolerance band) — implemented as the P3/P4 budget class. Options 2 (extraction-time pre-compositing) and 3 (reject translucency) declined per Positions_S8 reasoning.

**R3 — CLOSED without purchase 2026-08-01**: Adam declined the purchase ("we are not purchasing anything") and supplied WebToUMG's public documentation + full listing instead. The docs answered what the listing withheld and confirmed BUILD on every axis (Chromium 90; stock-panel layout, no flex engine; editable-output + merge reimport = the drift model D3 rejects; fidelity unquantified). Fair-credit capability notes (state capture, animations, material dedup, bundled fonts) recorded in PriorArt for Gates 4-5. The baseline-number question is moot: our harness measures fidelity directly.

Gate proof: 33/33 pattern lane with all gates live; 908/908 full suite. **Gate 4 (Emission) entry awaits Adam's explicit approval** per brief sec-1 rule 2 — draft contract at Plan/Gate_04_Emission.md.

## Gate 4 exit — **CLOSED 2026-08-01** (Adam approved the evidence table)

Delivered per D2/D3: reflected PageAsset (flattened tree, read-only, source-hash stamped), atomic emission (duplicate data-ck-name hard error, proven on H1), idempotence at projection AND package level, CkWebUmgEditor importer, WidgetsByCkName binding surface, and the pixel suite rendering THROUGH the projection with a scoreboard identical to the direct path (the exit criterion, now permanently gated). 931/931 full suite. Deferred to Gate 5 with Adam's visibility: real AS consumption script, editor UX (factory/context-menu). "Also emit WBP?" (D2's re-examination clause) — NOT pursued: the through-projection pixel evidence shows the DataAsset-runtime path meets the fidelity contract without WBP emission; revisit only if Gate 5's real-screen work surfaces a designer-visibility need.

## Standing evidence notes (carried from Phase 0)

- Yoga vendoring cautions: pin `c766885`; `UseWebDefaults` on; never expose inert grid setters; extractor pre-sorts children by computed `order`; fix `pointScaleFactor` for deterministic harness comparisons (PriorArt §3).
- Engine CEF 128 as CDP backend: **probe done 2026-07-31** — feasible: `-cefdebug=<port>` enables CDP remote debugging (`WebBrowserSingleton.cpp:343-347`). Requires a running editor process hosting an offscreen SWebBrowser, so heavier than system Chrome; not adopted for Gate 1. Revisit only if the Node toolchain becomes a real friction (e.g. designer machines without Chrome/Node).
- Chrome-version drift: extraction rides installed Chrome (auto-updates); IR must record browser version + viewport + dpr; determinism claims are per-browser-version (Gate 1 risk table).
