# Research: yShade "Easy Hand-Drawn Shading" — capability inventory

> **Written:** 2026-08-06, from a full crawl of https://yureka.games/yShade/hand-drawn and its five
> sub-pages (settings, getting-started, runtime-api, packaging, troubleshooting). Fab listing was
> bot-gated (HTTP 403) and not read. This is the re-implementation source of truth for Gate 4.
> **This doc dies when:** the Stylize campaign ships (its permanent residue is CkUsf/Claude.md).
> **Key source limitation:** the docs publish NO default values and NO numeric ranges for any
> setting — defaults/ranges in our port are our own choices, tuned in the gym.

## What it is

One coordinated full-screen pass, four treatments (docs' recommended order: paint → ink → shadow
strokes → paper last), plus a master `Style Strength` lerp between original scene and stylized
result. Fully procedural — the plugin ships no Blueprints, materials, volumes, or textures.
Original implementation: SceneViewExtension + global shaders; deferred-only, opaque surfaces only
(no translucency), Windows/SM5+, no forward, no mobile.

## Treatments

1. **Paint** — color-region reduction (posterize to N levels + transition softness), pre-shaping
   saturation/contrast, dual tint (shadow tint on dark regions, highlight tint on bright) with a
   shared strength, optional sky inclusion.
2. **Ink** — three independent edge detectors (depth discontinuity/silhouette, normal orientation,
   scene-color change), each with own threshold; line-variation noise (scale + amount) for organic
   contours; distance fade (start/end); color, thickness, opacity.
3. **Shadow strokes** — procedural hatching where luminance < threshold. 4 patterns:
   Diagonal Pencil, Crosshatch, Loose Scribble, Stipple. Stroke space: screen-stable
   (camera-relative; recommended for animated scenes) or world-attached (surface-projected;
   recommended for static environments — slides on animated geometry). Separate pixel-size /
   world-size controls; irregularity control.
4. **Paper** — procedural grain (strength + scale), directional fiber variation, warm cast.
   Docs warn: resolution/upscaler-sensitive; tune at target resolution.

## Full settings inventory (names verbatim; types inferred; NO published defaults)

Master: `Enabled` (bool), `Style Strength` (float), `Enable in Editor Viewports` (bool).

Paint: `Simplify Color` (bool), `Color Levels` (int), `Color Softness` (float), `Saturation`
(float), `Contrast` (float), `Shadow Tint` (color), `Highlight Tint` (color), `Tint Strength`
(float), `Affect Sky` (bool).

Ink: `Enable Ink` (bool), `Ink Color` (color), `Ink Thickness` (float), `Ink Opacity` (float),
`Depth Threshold` (float), `Normal Threshold` (float), `Color Edge Threshold` (float),
`Line Variation` (float), `Line Scale` (float), `Ink Fade Start Distance` (float),
`Ink Fade End Distance` (float).

Shadow strokes: `Enable Shadow Strokes` (bool), `Stroke Pattern`
(enum: DiagonalPencil/Crosshatch/LooseScribble/Stipple), `Stroke Space` (enum:
ScreenStable/WorldAttached), `Stroke Strength` (float), `Stroke Shadow Threshold` (float),
`Stroke Pixel Size` (float), `Stroke World Size` (float), `Stroke Irregularity` (float).

Paper: `Enable Paper` (bool), `Grain Strength` (float), `Grain Scale` (float), `Fiber Strength`
(float), `Paper Warmth` (float).

Debug: `Debug Mode` (enum: FinalImage default / InkMask / ShadowStrokeMask / PaperPattern /
WorldNormals / SceneDepth).

## Presets (write ordinary settings; remain editable)

Storybook Ink, Soft Painted, Bold Animation, Dark Gothic, Pencil Wash, Off. Plus factory reset.

## Runtime API (the shape we mirror, Ck-ified)

- `UHandDrawnShadingSubsystem` (world subsystem): `SetEnabled`/`IsEnabled`,
  `SetSettings(FHandDrawnShadingSettings)`/`GetSettings()`, `ResetToProjectDefaults`.
- Settings struct example fields: `bEnabled`, `StyleStrength`, `InkThickness`,
  `StrokePattern` (`EHandDrawnStrokePattern::Crosshatch`).
- Game-thread only; render thread receives a locked settings copy at view-family setup.
- Actor component = scoped WORLD settings override with restore-on-deactivate; explicitly
  "does not create a per-actor rendering effect". (We drop this shape — presets + subsystem
  calls cover it.)
- No MPCs, no post-process materials/volumes anywhere.

## Per-object / masking

Strictly full-screen. No stencil/custom-depth usage anywhere in this plugin. Masking is implicit:
`Affect Sky` toggle, translucency untouched, ink distance fade, stroke luminance threshold.

## Docs' recommended tuning order

1. lighting/exposure → 2. paint simplify + tint → 3. ink thickness + thresholds → 4. shadow
threshold, then stroke strength/scale → 5. paper at final output resolution → 6. test animated
objects + packaged builds.
