# CkWebUmg — §8 written positions (due before Gate 4)

> **Written:** 2026-08-01 (Gate 3). Each position states what we measured, what we claim, and what
> decision (if any) belongs to Adam at the Gate 3 exit. Evidence lives in
> [VERIFIED.md](VERIFIED.md); numbers below are from the 2026-08-01 session-3 scoreboard
> (28/28 pixel+rect, 903/903 full suite).

## §8.4 Compositing space — THE platform divergence (decision: Adam)

**Measured:** the browser composites translucent content in **sRGB space**; UE/Slate composites in
**linear space**. Exhibits, all probe-verified:

| Case | Browser (sRGB blend) | UE (linear blend) | Delta |
|---|---|---|---|
| rgba(255,0,0,0.5) over #0c0e12 (P5, historical) | R=133 | R=188 | 55 |
| Blue glow rgba(70,110,200,·) over body (P3) | B=53 | B=97 | 44 |
| Black shadow over dark body (P3 card 1) | (7,9,11) | (7,9,12) | ≤1 |

The last row is the control: dark-over-dark agrees in both spaces, which is why pure-black shadows
pass exactly while colored glows and mid-alpha overlays fail. One mechanism explains every failing
and every passing region of P3 (5.7471%) and P4 (4.3886%); it also contributes to AA edges in
P1/smoke.

**No per-element fix exists in Slate:** blending happens in the compositor in linear space; Slate
material brushes cannot read the destination; the global gamma-space render posture was tried and
broke every solid page at 100% (Slate quantizes vertex colors through an sRGB encode regardless —
session-2 finding, reverted per pre-stated rule).

**Options for the Gate 3 exit call:**
1. **Accept + per-page tolerance band** for translucency-bearing pages (P3/P4 gate at their
   measured §8 floor + margin). Cheapest; honest; the divergence is visible only in side-by-side
   comparison, not in isolation.
2. **Extraction-time pre-compositing** where the underlying pixels are static (shadow halos over a
   known page background bake the blend result as opaque texels). Exact for the corpus, but breaks
   the moment a shadow overlaps a dynamic sibling; adds an extractor mode that must know what is
   "static". Not recommended as the default.
3. **Reject translucency from the v1 surface** (diagnose like grid/sticky). Rejected by
   recommendation: opacity/shadows/AA are core visual vocabulary; the divergence is bounded and
   quantified.

Recommendation: option 1.

## §8.1 Text metrics (position + remaining work item 5)

**Measured:** Chromium and Slate disagree on text extents at the font level, before any tolerance
question — e.g. a text leaf recorded 200×200 by Chromium measured 131×38 under Slate's default
font (Gate 2 measure-callback logs). Rendered glyph rasters obviously differ too.

**Position:** pixel-diffing glyphs is the wrong contract. The harness **masks IR text-leaf rects
out of the pixel metric** (masked-pixel counts reported per page — T1/T3 at 0.0003% residual with
masking proves the mask boundary is tight). Text fidelity gets its own regime (work item 5, still
open): map the corpus font at emitter config level, apply size/weight/letter-spacing/line-height
to `STextBlock`, and compare **per-glyph-run boxes** (position + advance) rather than pixels; the
numeric tolerance is proposed from measured data at the Gate 3 exit, like ±1px was for layout.
Until item 5 lands, text pages gate only on their non-text pixels (currently ≤0.0003%).

## §8.2 DPI contract

**Position:** the reference-viewport contract. Extraction pins dpr=1
(`Tools/ckwebumg-extract/src/extract.mjs:16` — `deviceScaleFactor: 1`, plus
`--force-device-scale-factor=1` launch arg); the harness renders at Slate scale 1.0 with the RT
sized exactly to the recorded viewport (`Source/CkWebUmg/Private/CkWebUmg_PaintFidelity_Test.cpp`,
`FWidgetRenderer::DrawWindow` path); the flex panel sets Yoga `pointScaleFactor` = 1. UMG's
runtime DPI curve (`UUserInterfaceSettings` application-scale) sits ABOVE the widget tree we
build and scales the whole tree uniformly — fidelity is defined at scale 1 and survives uniform
scaling by construction. Non-uniform per-widget DPI behavior is out of the v1 contract.

## §8.6 Stacking contexts

**Position:** v1 implements **flat sibling z-ordering**: children sort by computed `order` at
extraction (IR bakes it), and `SCk_WebUmgFlexPanel::OnArrangeChildren` stable-sorts paint order by
`zIndex` decoupled from layout order. This closed L6 (2.89% → 0.0000%). Full CSS stacking-context
semantics (contexts created by `opacity<1`, `transform`, `isolation`, interleaving of negative
z-index with backgrounds) are NOT modeled; the corpus does not exercise them beyond L6's flat
cases. If a real mockup needs true stacking contexts, that is a Gate 4+ schema addition
(per-context grouping), not a panel patch — recorded so it isn't rediscovered.

## §8.5 Shadows (implemented; ratification: Adam)

**Implemented and measured** (Gate 3 work item 4): baked Gaussian textures — SDF rounded-rect
coverage → separable Gaussian σ = blur/2 (the CSS/Skia model) → per-layer source-over compositing
in straight-alpha sRGB space → transient texture; outset halos ride a negative-padding overlay
slot around the final widget (render transform/opacity apply to the shadow, as CSS requires);
inset paints between background and content clipped to the rounded silhouette. Result: single
black drop shadow **exact** (probe delta ≤1); P3 6.0603% → 5.7471% with maxDelta 145 → 52, and
the entire residual is §8.4, not shadow geometry. The alternatives from the gate plan (SDF
material brush / 9-slice approximation / diagnostic-only) were not built: the baked path already
hit the platform ceiling, so their scores could only be worse or equal at higher complexity.
Ratify or redirect at exit.

## P1 per-side borders — RESOLVED 2026-08-01 (surface widened under Adam's blanket authorization)

Originally a listed option; Adam's "tackle any issues and additional items" authorized it. Per-side
widths AND colors are now in-surface: the extractor types `borderColors [t,r,b,l]` (the B5
diagnostic retired), the builder bakes a miter-classified ring texture (outer-minus-inner rounded
SDF, penetration-ratio side ownership). Measured: P1 0.7412% → **0.1695%**, matching the
arithmetic attribution — the residual is rounded-edge AA only (n3's r=110 circle dominates).
