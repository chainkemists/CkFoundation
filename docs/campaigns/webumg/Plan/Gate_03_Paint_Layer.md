# Gate 3 — Paint layer

> **Status:** 🟡 In progress (entered 2026-08-01)
> **Depends on:** Gate 2 ✅ (±1px layout ratified; 884/884 suite)
> **Estimate at entry:** 4–8 sessions (Phase 0 estimate, re-dated 2026-08-01)

## Goal

After this gate: the paint corpus (P1–P5) renders through the builder within an agreed pixel threshold of the Chromium goldens, measured by a **pixel-diff harness** (FWidgetRenderer → render target → per-pixel compare vs golden PNG); text pages (T1–T3, C1) pass a **separate text tolerance** (§8.1/§10 — per-glyph-run boxes, not pixel diff); and the §8 written positions due before Phase 4 exist (text metrics strategy, shadow strategy, DPI contract).

## Entry criteria (2026-08-01)

- [x] Gate 2 exit re-verified (commit `e4edd5952` tip; 884/884).
- [x] Baseline: 884/884 is the new suite baseline to diff against.
- [x] Slate-native capabilities inventoried before inventing materials (work item 0 — verify against engine source, cite path:line): `FSlateRoundedBoxBrush` (per-corner radii + outline) for radius/borders; `SComplexGradient`/`SSimpleGradient` for axis-aligned gradients; shadows have NO native soft path — strategy decided inside this gate with pixel evidence.

## Work items

0. **Slate paint inventory** — cite what exists (rounded box brush, gradients, box element rounding) before any material work; pick per-feature: native Slate / material / diagnostic-only.
1. **Pixel harness** — `FWidgetRenderer` renders the built tree at the recorded viewport to a render target; compare vs golden PNG: per-pixel channel delta with (a) a global threshold + failing-pixel budget, (b) text-region masks (from IR text-leaf rects) excluded from the pixel metric and measured separately. Per-page score reported; runs in the default suite (`CkTests.UnitTests.CkWebUmg.PaintFidelity.*`).
2. **Radius + borders** — `FSlateRoundedBoxBrush`-backed painting in the builder (per-corner radii from IR; uniform outline; per-side width/color divergence → already diagnosed by extractor).
3. **Typed gradients** — extractor closes the Gate-1 `computed` escape: parse `linear-gradient`/`radial-gradient` computed strings into typed stops in the IR (schema addition, goldens re-extracted); builder emits axis-aligned linear via Slate gradient widgets; arbitrary angles/radial per work-item-0 decision (material or diagnostic).
4. **Shadow strategy [DECISION-shaped]** — options with measured pixel scores: (a) SDF material brush (build UMaterial programmatically in an editor module), (b) 9-slice baked texture approximation, (c) diagnostic-only in v1. Presented with numbers, Adam picks at gate exit.
5. **Text styling + tolerance regime (§8.1 position)** — font mapping config (corpus uses Arial; decide the bundled/mapped face), apply weight/size/letter-spacing/line-height to STextBlock; implement the per-glyph-run box comparison; propose the numeric text tolerance from measured data (Gate 2 already logged 200×200-vs-131×38-class gaps).
6. **Asset import (P5)** — `img`/background-image textures: load PNG at build time for the harness path (UTexture2D from file); editor-time import pipeline is Gate 4 scope.
7. **§8 written positions** — text metrics strategy (from 5), DPI contract (verify UMG DPI-curve application site with path:line), stacking contexts scoping. Due before Gate 4 per the brief.

## Expected observations — and branches

| I will run | I expect | If instead | Prewritten response |
|---|---|---|---|
| Pixel harness on L-pages (solid colors only) | near-zero pixel diff (sanity: rect-green must imply pixel-green for solids) | large diffs | renderer/golden mismatch (sRGB/gamma, DPI) — fix the harness before touching paint |
| P1 radius/borders via rounded-box brush | anti-aliased edges differ slightly from Chromium | edge-band failures only | tolerate via failing-pixel budget, never via raising the channel threshold |
| P2 gradients (typed) | axis-aligned linear within threshold | banding/interpolation gap | check sRGB vs linear interpolation space — Chromium interpolates in sRGB by default |
| P3 shadows | strategy-dependent | hard-edge approximation fails pixel diff badly | that IS the evidence for the shadow decision — record scores per option |
| T-pages glyph-run compare | systematic advance/baseline offsets | chaos beyond systematic offset | font mapping wrong — fix mapping before tuning tolerance |

## Exit criteria — same-commit rule as always

- [ ] P-corpus at agreed paint threshold (number ratified by Adam at this gate, like ±1px was)
- [ ] Text tolerance defined, measured, ratified; T-pages pass it
- [ ] Shadow decision made by Adam from measured options
- [ ] §8 positions written (text/DPI/stacking)
- [ ] Extractor gradient escape closed; goldens re-extracted; SCHEMA.md in step
- [ ] Suite baseline diff (884 + new tests, zero newly failing)
- [ ] PLAN.md + this header, same commit; PROGRESS dated entry
