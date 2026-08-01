# Gate 3 — Paint layer

> **Status:** ✅ Done (2026-08-01) — exit calls delegated by Adam ("go with the best recommendation grounded in data, not guesses"); thresholds ENFORCED in the harnesses, 33/33 with gates live.
> **Depends on:** Gate 2 ✅ (±1px layout ratified; 884/884 suite)
> **Estimate at entry:** 4–8 sessions (Phase 0 estimate, re-dated 2026-08-01)

## Goal

After this gate: the paint corpus (P1–P5) renders through the builder within an agreed pixel threshold of the Chromium goldens, measured by a **pixel-diff harness** (FWidgetRenderer → render target → per-pixel compare vs golden PNG); text pages (T1–T3, C1) pass a **separate text tolerance** (§8.1/§10 — per-glyph-run boxes, not pixel diff); and the §8 written positions due before Phase 4 exist (text metrics strategy, shadow strategy, DPI contract).

## Entry criteria (2026-08-01)

- [x] Gate 2 exit re-verified (commit `e4edd5952` tip; 884/884).
- [x] Baseline: 884/884 is the new suite baseline to diff against.
- [x] Slate-native capabilities inventoried before inventing materials (work item 0 — verify against engine source, cite path:line): `FSlateRoundedBoxBrush` (per-corner radii + outline) for radius/borders; `SComplexGradient`/`SSimpleGradient` for axis-aligned gradients; shadows have NO native soft path — strategy decided inside this gate with pixel evidence.

## Work items

0. ✅ **Slate paint inventory** — cited (FSlateRoundedBoxBrush, gradient widgets; shadows have no native soft path).
1. ✅ **Pixel harness** — landed with text-rect masking, per-page scores, diff-artifact dumps; pixel lane requires `--no-nullrhi` (default lane skips explicitly via `GUsingNullRHI` guard; full suite 903/903).
2. ✅ **Radius + borders** — `FSlateRoundedBoxBrush` per node; per-side widths/colors later baked in-surface (miter-classified ring textures, B5 diagnostic retired). P1 0.7412% → **0.1695%** (AA-only residual).
3. ✅ **Typed gradients** — EXCEEDED the plan: extractor types stops + radial center/radius absolutely; builder BAKES all linear (any angle) and radial gradients to per-node sRGB textures with browser math. P2 31.86% → **0.0000%** (maxDelta 1). SComplexGradient path deleted.
4. ✅-implemented **Shadow strategy** — baked Gaussian textures (σ=blur/2, SDF coverage, sRGB layer compositing). Single black drop exact; P3 → 5.7471% where the ENTIRE residual is §8.4 compositing-space, not shadow geometry. Alternatives not built (could only score worse at higher complexity). **Adam ratifies at exit.**
5. ✅ **Text styling + tolerance regime** — OS font mapping (Arial/Segoe/Verdana + Black cut for ≥800; px→pt 0.75 factor), letter-spacing/text-transform/`<br>` modeled, line-run comparator (line boxes from Range.getClientRects, greedy-wrap replay, fallback-run merging, nowrap honored). Final dataset: 25 runs, 0 wrap mismatches, mean 0.78%, max 2.9%. Tolerance proposal for exit: ±3% advance, ±2px line height.
6. ✅ **Asset import (P5)** — browser-normalized `ckui-assets/` bundles + `ImportFileAsTexture2D` with SRGB. P5 0.0000%.
7. ✅ **§8 written positions** — [Positions_S8.md](../Positions_S8.md): compositing space (with options), text metrics, DPI contract, stacking contexts, shadows, per-side borders.

## Expected observations — and branches

| I will run | I expect | If instead | Prewritten response |
|---|---|---|---|
| Pixel harness on L-pages (solid colors only) | near-zero pixel diff (sanity: rect-green must imply pixel-green for solids) | large diffs | renderer/golden mismatch (sRGB/gamma, DPI) — fix the harness before touching paint |
| P1 radius/borders via rounded-box brush | anti-aliased edges differ slightly from Chromium | edge-band failures only | tolerate via failing-pixel budget, never via raising the channel threshold |
| P2 gradients (typed) | axis-aligned linear within threshold | banding/interpolation gap | check sRGB vs linear interpolation space — Chromium interpolates in sRGB by default |
| P3 shadows | strategy-dependent | hard-edge approximation fails pixel diff badly | that IS the evidence for the shadow decision — record scores per option |
| T-pages glyph-run compare | systematic advance/baseline offsets | chaos beyond systematic offset | font mapping wrong — fix mapping before tuning tolerance |

## Exit criteria — same-commit rule as always

- [x] P-corpus at ratified paint threshold — **per-class budgets enforced in PaintFidelity**: solids (L/C) 0.2%, opaque paint pages 0.5%, translucency-bearing (P3/P4) 7% = the §8.4 band (policy option 1). Measured floors: P1 0.1695% / P2 0.0000% / P3 5.7471% / P4 4.3886% / P5 0.0000%.
- [x] Text tolerance ratified and **enforced in the line-run comparator**: line-count equality + per-line advance within max(3%, 3px). Dataset: 25 runs, 0 mismatches, mean 0.78%, max 2.9%.
- [x] Shadow decision: **baked-Gaussian ratified** (single black drop exact; residual entirely §8.4).
- [x] §8 positions written (text/DPI/stacking/compositing/shadows) — [Positions_S8.md](../Positions_S8.md)
- [x] Extractor gradient escape closed; goldens re-extracted; SCHEMA.md in step
- [x] Suite baseline diff: 908/908 confirmed (884 preserved + rect/text/hostile additions + 19 explicit NullRHI-lane skips)
- [x] PLAN.md + this header, same commit; PROGRESS dated entry
