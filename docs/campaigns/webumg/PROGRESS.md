# CkWebUmg — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-07-31 (uncommitted):** Gate 0 ✅. Gate 1 work items 1 (CLI ✅), 3 (extraction core ✅ on smoke page), 4 (diagnostics mechanism ✅), 5 (`data-ck-*` ✅ extraction + spec; duplicate-name + unknown-attribute diagnostics still open), 7 (determinism ✅ on smoke page — full corpus pending). Open: 2 (schema review), 6 (20-page corpus — 1/20), 8 (CEF probe), 9 (DECISION packs), assets/fonts blocks.
**Baseline being diffed against:** n/a — no engine-code changes yet; toolchain baseline in Gate_01 entry criteria.
**Next action:** author the corpus (L1–L8 layout, P1–P5 paint, T1–T4 text, C1–C2 controls, H1 hostile) and run the determinism proof across all of it. Corpus authoring is executor-shaped work — a candidate for an Opus session with `/execute-phase` against Gate_01.
**Blocked on:** nothing for current work (DECISIONS 1–4 block Gate 1 *exit*). Open Adam-side item: R3 WebToUMG purchase.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-07-31 | DECISION 0 = BUILD | PriorArt evidence chain; Adam approved | Never (gate passed) |
| 2026-07-31 | Module family = CkWebUmg | Adam picked; CkStyle collides | Never |
| 2026-07-31 | Extractor rides system Chrome via puppeteer-core (no bundled Chromium download) | Chrome present; smaller toolchain; version recorded per-extraction | If version drift bites (Gate_01 risk 1) |
| 2026-07-31 | CLI lives at `Tools/ckwebumg-extract/` (plugin root), outside `Source/` | UBT must never see it; brief calls it "outside engine" | If plugin packaging complains |

## Dated entries (append-only, newest first)

### 2026-07-31 (later) — Extractor working end-to-end on smoke page
- Built `Tools/ckwebumg-extract/` (Node ESM, puppeteer-core@24 on system Chrome; no Chromium download). `npm install` clean, 0 vulnerabilities.
- Ran: `node src/extract.mjs corpus/smoke.html` → IR + golden PNG. Confirmed by direct assertion script against the JSON: children re-sorted by computed `order` (0,1,2 ⇒ HEALTH/STAMINA/CONFIRM, matches golden pixels); `:hover` state diff captured via `CSS.forcePseudoState` (`{"hover":{"background-color":"rgb(70, 115, 195)"}}`); `data-ck-name/bind` extracted; `data-ck-ignore` subtree absent (string-searched whole IR); `%` width resolved absolute (62% → 223.36px).
- Confirmed: diagnostics fire with exact file:line — `backdrop-filter: blur(4px)` reported at `smoke.html:25`, verified against the file with `sed -n '25p'`. Two bugs found+fixed on contact: (1) injected neutralization CSS was self-diagnosed as author CSS → moved injection to inspector-origin `CSS.createStyleSheet` (harvester reads `regular`-origin only); (2) inline-`<style>` line numbers were sheet-relative → rebased via stylesheet header `startLine`.
- Confirmed: determinism — two independent extractions of smoke.html: IR byte-identical (`diff` empty) AND golden PNG byte-identical (`cmp` clean). Chrome/150.0.7871.188.
- Inferred (unconfirmed): PNG byte-determinism may not hold across machines/GPUs — Gate_01 risk row stands; per-machine goldens acceptable since the Gate 2 harness compares UMG↔golden on the same machine.
- Specs written: `Tools/ckwebumg-extract/SCHEMA.md` (IR v1 as implemented, incl. post-transform box caveat + temporary `computed` escapes for gradient/shadow pending Gate 3 parsers), `DATA_CK_SPEC.md`.

### 2026-07-31 — Campaign scaffolded; Gate 1 opened
- Gate 0 closed: PriorArt.md / VERIFIED.md / DECISIONS.md landed (Phase 0 originals archived at `D:\tmp\ckstyle-phase0\`); brief copied in as CampaignBrief.md.
- Confirmed: Node v24.14.0, npm 11.9.0, Chrome at `C:\Program Files\Google\Chrome\Application\chrome.exe` (commands run this session).
- Doc set created per ck-methodology (PROMPT/PLAN/PROGRESS/Gate_01).

## Open items
| Item | Status | Next step |
|---|---|---|
| Gate 1 work items 1–9 | in progress | see Gate_01_IR_Extraction.md |
| R3: buy WebToUMG (~$56) for baseline | waiting on Adam | Adam purchases; corpus run against it lands in Gate 2 evidence |
