# Gate 1 — DECISION packs (1–4)

> **Written:** 2026-07-31. Input to Adam's Gate 1 exit rulings. Each pack: options, the evidence now on file, a recommendation, and what would flip it. Brief §1 rule 3: these stop the campaign until chosen.
> Evidence base: 20-page corpus + goldens (`Tools/ckwebumg-extract/corpus/`), PriorArt.md, VERIFIED.md.

---

## DECISION 1 — Layout runtime

**The corpus sharpened the options.** The IR carries *both* the used-value boxes (absolute rects at the reference viewport) *and* the full flex inputs (direction/justify/align/alignContent/wrap/grow/shrink/basis/gap, order pre-sorted) — so every option below is implementable from the same IR without re-extraction.

| Option | Corpus evidence |
|---|---|
| **A — bake rects into UCanvasPanel** | Trivially satisfiable: every corpus node already has its border-box rect. Exact at 1920×1080 by construction; zero reflow. `inset.authored` even gives honest anchors for the absolute cases. |
| **B — lower to native panels** | Concrete lossy set, now enumerable from real pages: L4's `align-content: space-between` and `wrap-reverse` have no UWrapBox equivalent; L3's weighted `flex-shrink` (220/140/240 split) has no native analog; L2's grow ratios map to Slate FillSize only when siblings are all-fill; `gap` needs per-slot padding synthesis. B alone cannot pass the L-corpus at ±1px without per-case hacks. |
| **C — Yoga-backed panel** | Yoga verified to cover every layout feature the corpus uses (gap incl. percent, wrap+reverse, align-content incl. space-between/evenly, grow/shrink/basis — PriorArt §3); `order` handled by extraction pre-sort. Slate two-pass reconciliation evidenced at source level (VERIFIED § Slate). **The L-corpus goldens are, as committed, a ready-made Yoga fixture suite**: feed IR inputs to Yoga, compare against `box` rects. |
| **D — C + lowering pass** | = C's fidelity plus B's hand-editability for trivial containers; the brief's own table marks it highest maintenance. |

**Recommendation: C for Gate 2, with D as the stated evolution path (lowering pass deferred until a real hand-editability demand exists).** Gate 2's harness can then measure Yoga-vs-Chromium divergence numerically before any emitter exists. **Flips if:** the Gate 2 Yoga prototype shows text-measure integration failing at real ±1px (the compiled proof PriorArt §3 still owes), or if R3's WebToUMG baseline shows native-panel lowering already lands within tolerance on the L-corpus — then B/D's cost calculus changes.

## DECISION 2 — Output form

**The corpus reframed this decision: the Gate 2 harness must render IR → widgets → pixels to diff against goldens, so a runtime builder is *required infrastructure regardless of the ruling*.** The genuine question is only which *durable artifact* Gate 4 emits.

- **Runtime construction from a DataAsset holding the IR** — the harness path, hot-swappable (pairs with Phase 5 live-reload), no codegen, byte-stable regeneration for free (corpus IRs are byte-deterministic). Cost: invisible in the UMG designer; BP authors interact only via named `data-ck-*` hooks.
- **Generated `UWidgetBlueprint`** — designer-inspectable; but DECISION 3's read-only posture removes the "designers edit it" benefit, leaving inspectability; costs the fiddly asset-generation layer and binary diffs.
- **Generated C++** — text-diffable; least editor-integration risk; still not designer-visible; adds a codegen layer.

**Recommendation: rule DataAsset-runtime as the primary output now (it gets built for the harness anyway); defer the "also emit WBP?" question to Gate 4 entry, when the runtime path's designer-visibility cost is observable in practice rather than argued.** **Flips if:** Adam wants designers opening converted screens in the UMG designer as a hard requirement — then WBP emission moves back into Gate 4 scope proper.

## DECISION 3 — Regeneration model

**Recommendation: ratify as proposed** — generated output strictly read-only; hand-authoring in subclasses/bound C++; no merge-back ever. Corroboration: WebToUMG independently ships exactly this (stable names, logic preserved, layout edits overwritten, merge "on roadmap" ⟨V⟩). Corpus addendum: H1 shows duplicate `data-ck-name` currently emits both nodes named `Duplicate` after diagnosing — the emitter must treat that diagnostic as a **hard error** (refuse to emit), not a warning, or regeneration stability breaks. **Flips if:** nothing plausible; a merge scheme would need its own conflict-resolution design first per the brief.

## DECISION 4 — v1 CSS surface

**Recommendation: ratify the implemented allowlist as v1** — it is no longer a proposal but a tested artifact: `SUPPORTED_PROPERTIES`/`SUPPORTED_SHORTHANDS`/`VALUE_RULES` in `src/extract.mjs`, exercised by 20 pages. Deltas discovered versus the brief's sketch, all evidence-driven:
- **Value-level rules exist** (`display` ∉ {grid,…}, `position` ∉ {sticky,fixed}) — property-name allowlisting alone let `display:grid` through silently (bug B6, fixed).
- **Four diagnostic classes** (name, value, pseudo-element, computed-divergence) — H1 fires all four; the computed-divergence class (elliptical radii, per-side border colors) wasn't in the brief but is required by "no silent drops" once real pages hit it.
- **Recorded-not-parsed escapes**: gradients and box-shadows are captured verbatim-computed pending Gate 3's typed parsers — recorded in SCHEMA.md as temporary, with the Gate 3 exit closing them.
- `:hover/:active/:disabled` in scope and working (forced-pseudo-state diffs); `disabled` semantic attribute captured.
**Flips if:** the first real project screen (Phase 5 target) leans on something out-of-surface — the diagnostic report will say so with file:line, and the surface grows by evidence, not speculation.

---

**Requested from Adam to close Gate 1:** (a) schema review of `Tools/ckwebumg-extract/SCHEMA.md`; (b) rulings on the four packs above. Everything else on the Gate 1 exit checklist is done and committed on `feature/webumg-campaign`.
