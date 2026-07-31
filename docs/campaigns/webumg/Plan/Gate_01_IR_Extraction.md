# Gate 1 — IR + extraction

> **Status:** ✅ Done (2026-07-31)
> **Depends on:** Gate 0 ✅ (DECISION 0 = BUILD, 2026-07-31)
> **Estimate at entry:** 2–4 sessions · **Actual:** 2 sessions (same day)

## Goal

After this gate: running `ckwebumg-extract <page.html>` produces a schema-versioned `*.ckui.json` (computed absolute values only, zero CSS syntax) plus a golden PNG, **byte-identically across repeated runs**, for a 20-page corpus; every CSS property outside the proposed v1 surface appears in the IR's `unsupported` array with `file:line`-grade provenance; and Adam has the evidence packs to rule on DECISIONS 1–4.

## Entry criteria (ran 2026-07-31)

- [x] Gate 0 exit re-verified: DECISION 0 recorded (DECISIONS.md), PriorArt/VERIFIED committed to campaign dir.
- [x] Toolchain baseline: Node v24.14.0, npm 11.9.0, system Chrome present at `C:\Program Files\Google\Chrome\Application\chrome.exe` (observed this session — versions recorded per-extraction in the IR, since Chrome auto-updates).
- [x] CDP contract verified stable for the four core methods (PriorArt §5).

## Work items

1. **CLI scaffold** — `Tools/ckwebumg-extract/` (Node, `puppeteer-core` on system Chrome). Outside `Source/` so UBT never sees it. NEW INFRASTRUCTURE — but thin: CDP plumbing is puppeteer's.
2. **IR schema v1** — brief §5 sketch made concrete (`schema: 1`; `source` gains `browser` version field; box/layout/paint/text/states/children/unsupported/ck). Spec doc: `Tools/ckwebumg-extract/SCHEMA.md`.
3. **Extraction core** — fixed viewport (1920×1080 @ dpr 1), animations/caret/scrollbars neutralized, per-element computed style + box model, absolute values resolved, children pre-sorted by computed `order` (Yoga lacks `order` — PriorArt §3), `Page.captureScreenshot` golden.
4. **Diagnostics mechanism** — v1-surface allowlist (DECISION 4 proposal); everything else → `unsupported[]` with property, value, and best-available provenance via `CSS.getMatchedStylesForNode` (rule source file:line when a stylesheet rule set it; `computed` marker when only the computed value diverges from initial).
5. **`data-ck-*` spec** — `name`/`widget`/`bind`/`ignore`/`slot` per brief §7; spec doc + extraction into the `ck` object.
6. **Corpus** — 20 pages, tiered: L1–L8 layout-only (flex axes, grow/shrink/basis, wrap, gap, nesting, absolute, overflow, min/max), P1–P5 paint (radius, gradients, shadows, borders, opacity/transform), T1–T4 typography (wrap, ellipsis, letter-spacing/line-height, mixed weights), C1–C2 controls (button states, inputs), H1 hostile (deliberately unsupported properties — must produce diagnostics, used again at Gate 6).
7. **Determinism proof** — run extraction twice per corpus page; byte-diff IR (must be empty) and pixel-diff goldens (report; see risk row).
8. **Probe (timeboxed ~half day):** can the engine's CEF 128 serve CDP for extraction (kills Node dependency)? Outcome is a note in PROGRESS.md, not a commitment.
9. **DECISION packs** — one page per open DECISION with the Gate-1 evidence, presented to Adam.

## Expected observations at the gate — and branches

| I will run | I expect | If instead | Prewritten response |
|---|---|---|---|
| `ckwebumg-extract` twice on each corpus page | IR byte-identical | JSON differs | find the volatile field (timestamps, object-key order, float jitter), eliminate at source — sorted keys, fixed precision; never post-hoc diff-masking |
| golden PNG diff across runs | pixel-identical on same machine+Chrome | AA/subpixel jitter | record as known per-machine variance; harness (Gate 2) compares UMG↔golden, not golden↔golden — but document it |
| hostile page H1 | every out-of-surface property in `unsupported[]` with provenance | silent drop | blocker — the mechanism is the brief's rule 5; fix before exit |
| box model vs computed `transform` | quads are post-transform geometry; IR records both + computed transform | mismatch elsewhere | re-read CDP semantics; adjust extractor, note in VERIFIED.md |
| `%`/`vw`/`em` values in corpus | all resolved to absolute px by Chromium | any CSS syntax leaks into IR | schema violation — extend resolver; the IR contains no CSS syntax (brief §5) |

## Risks

| Risk | Sizing |
|---|---|
| Chrome auto-update shifts extracted values between sessions | Real but bounded — IR records browser version; corpus re-extraction is cheap; goldens re-bake with version bump |
| Per-node CDP round-trips slow on big pages | Accept for v1; `DOMSnapshot.captureSnapshot` (experimental) noted as optimization, not adopted |
| Provenance (file:line) not available for all properties | `getMatchedStylesForNode` gives rule origins; inline styles/UA styles get marker provenance — degrade loudly, never silently |

## Exit criteria — ALL land in the same commit as the last work item

- [x] All expected observations confirmed; evidence in PROGRESS.md (2026-07-31 session-2 entry: 20/20 pages byte-deterministic IR+PNG; H1 fires all diagnostic classes; no CSS syntax leaks after B3 fix)
- [x] Schema reviewed by Adam; DECISIONS 1–4 made by Adam ("Go with your recommendations", 2026-07-31 — recorded in DECISIONS.md)
- [x] `data-ck-*` spec exists (DATA_CK_SPEC.md) and extraction honors it (smoke + H1 assertions)
- [x] PLAN.md status row AND this header updated, same commit
- [x] PROGRESS.md dated entries; VERIFIED.md appended (Gate 1 additions section)

**Deviation from plan, recorded:** T4 (mixed inline weights) and C2 (inputs) were folded into T3/C1 by the corpus author; corpus is 20 pages counting smoke. The inline-formatting hard limit (brief §8.5) gets its dedicated fixture at Gate 3 when text tolerance is defined.
