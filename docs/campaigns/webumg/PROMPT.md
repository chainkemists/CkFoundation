# CkWebUmg — mission brief (PROMPT.md)

> **Written:** 2026-07-31. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** the campaign ships and `Source/CkWebUmg/Claude.md` + `Source/CkWebUmgEditor/Claude.md` become the permanent record. On death: delete, or replace body with a tombstone line.
> **Source of truth for scope:** [CampaignBrief.md](CampaignBrief.md) (Adam's original brief — §ref numbers below cite it). This file adapts it to house structure; on conflict the brief wins.

## Goal

A toolkit inside CkFoundation that converts an HTML+CSS mockup into UMG with **measured** fidelity: a designer iterates in a browser, runs a conversion, and gets UMG whose every element rect is within the agreed tolerance of the Chromium box model — enforced by a pixel/rect harness, never by eyeball. "Faithful" is a number (brief §10), or the tool fails loudly.

## Success criteria (campaign-level; per-gate criteria live in the gate files)

1. A 20+-page golden corpus extracts deterministically (byte-identical `*.ckui.json` across runs) with reference PNGs (brief §9 Phase 1).
2. Layout-only corpus pages render in UMG within the agreed layout tolerance (proposed ±1 px, brief §10) at reference resolution, verified by the harness in CI (Phase 2).
3. Paint corpus pages (backgrounds/borders/radii/gradients/shadows) hit threshold; text pages hit the separate text tolerance (Phase 3).
4. Convert → open in editor → render → diff round-trip stays within threshold; regeneration is idempotent (Phase 4).
5. A real screen from an actual project converts and is wired to gameplay data via `data-ck-*` (Phase 5).
6. Zero silent property drops: every unsupported CSS property appears in the conversion report with source `file:line`, verified against a deliberately hostile page (Phases 1/6).

## Constraints & locked decisions

| Decision | Choice | Why | Where recorded |
|---|---|---|---|
| DECISION 0 — build vs buy | **BUILD** (Adam, 2026-07-31) | No existing solution meets §10; both architectural bets survived source-level recon | [DECISIONS.md](DECISIONS.md) |
| Module family name | **CkWebUmg** / **CkWebUmgEditor** (Adam, 2026-07-31) | `CkStyle` collides with existing `CkStyle::` namespace (`CkEditorTools/Style/CkStyle.h:32`); no `*Harness` module precedent — harness placement decided at Gate 2 | [DECISIONS.md](DECISIONS.md) |
| Extractor | Headless Chromium via CDP (Puppeteer-core driving installed Chrome), editor-time only | The browser is the specification; removes cascade/specificity/inheritance/calc()/var() from scope (brief §3); CDP methods verified stable ([PriorArt.md](PriorArt.md) §5) | brief §3 |
| Layout engine candidate | Yoga @ `c766885` (MIT), vendored CkThirdParty-style, **if** DECISION 1 → C/D | 12.4k LOC, zero deps, C++20; measure-callback model reconciled against Slate source ([PriorArt.md](PriorArt.md) §3) | PriorArt §3 |
| Runtime dependency policy | Nothing web-related ships in a cooked build; Node/Chrome is authoring-time only | brief §3 "Cost of that choice" | brief §3 |
| Regeneration posture (proposed, ratify at DECISION 3) | Generated assets read-only; hand-authoring in subclass/bound C++ | Merge-back is unsolvable in general (brief §4 D3); WebToUMG independently converged on this | brief §4 |

Open decisions [DECISION 1–4] (layout runtime, output form, regeneration, v1 CSS surface) are **due at Gate 1 exit with evidence**; they stop the campaign until Adam chooses (brief §1 rule 3).

## Non-goals

- CSS Grid, inline formatting contexts with mixed inline boxes, pseudo-elements, transitions/animations, `filter`, `clip-path`, `mix-blend-mode` — out of v1 (brief §4 DECISION 4 proposal); must still produce diagnostics, never silence.
- Runtime HTML/CSS parsing in the engine — the IR contains no CSS syntax (brief §5).
- Embedding a browser in shipped UI — rejected with engine-source evidence; do not relitigate ([PriorArt.md](PriorArt.md) §6).
- Responsive multi-viewport extraction in v1 — extraction viewport is a recorded input (brief §6 `%`/`vw`/`vh` row); revisit post-v1.

## Operating rules (from brief §1 — non-negotiable, all phases)

1. Never assume, always verify — engine claims need `path:line`. 2. Gate every phase; Adam approves. 3. Tradeoffs presented, not silently resolved. 4. Fix bugs on contact in CkFoundation code. 5. No silent property drops. 6. Anti-sycophancy. 7. Every phase updates DECISIONS.md + VERIFIED.md.

## Reading list

- [CampaignBrief.md](CampaignBrief.md) — full scope, IR sketch (§5), mapping table (§6), hard problems (§8), phase plan (§9), fidelity definition (§10).
- [PriorArt.md](PriorArt.md) — per-target verdicts; §3 (Yoga/Slate reconciliation) and §5 (CDP contract) are load-bearing for Gates 1–2.
- [VERIFIED.md](VERIFIED.md) — claim ledger; append, never rewrite history.
- Gate index: [PLAN.md](PLAN.md).
- Reference modules for mimicry when the C++ modules start (Gate 2+): `CkTimer` (quartet shape), `CkThirdParty` (vendoring shape: EnTT/Jolt/fmt), `CkVat`/`CkUsf` (material-backed rendering modules), `CkPmg` (procedural drawing).

## Things ruled out — do not re-investigate

| Ruled out | Why | Evidence |
|---|---|---|
| Buy WebToUMG instead of building | No fidelity contract, single viewport, no diagnostics, UNKNOWN source access | PriorArt §1 |
| litehtml as extractor/layout | Box model ≠ the golden screenshot's engine; no Grid/gap/calc/transforms/box-shadow | PriorArt §4 |
| StyledWidgets as a base | Styling-only; zero layout, zero HTML | PriorArt §2 |
| Embedded browser for shipped UI | Texture-blit output, no console path in engine build files, per-element §10 comparison inexpressible | PriorArt §6 |
| In-engine CSS parser | Chromium-as-extractor removes the entire cascade problem space | brief §3; PriorArt §5 |
| `CkStyle` as module name | Symbol + filename collision | DECISIONS.md |
