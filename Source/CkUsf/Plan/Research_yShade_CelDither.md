# Research: yShade "Easy Cel Dither Shading" — capability inventory

> **Written:** 2026-08-06, from a full crawl of https://yureka.games/yShade/cel-dither and its five
> sub-pages (settings, getting-started, runtime-api, packaging, troubleshooting). Fab listing was
> bot-gated (HTTP 403) and not read. Re-implementation source of truth for Gates 2 (dither) and 3
> (cel). **This doc dies when:** the Stylize campaign ships (permanent residue: CkUsf/Claude.md).
> **Key source limitation:** the settings reference is group-level tables (control-name lists) —
> no per-setting types, defaults, or ranges are published, except `Pattern Stencil Base` = 200 and
> the C++ examples' implied types (`Bands` int e.g. 3, `ColorSteps` int e.g. 6, `Weight` float
> e.g. 0.65f). Everything else is our choice, tuned in the gym.

## What it is

TWO independent full-screen passes. Original implementation: SceneViewExtensions created
automatically per world + global shaders; no content assets, no MPCs, no PP materials/volumes.

- **Pass A — Cel/Halftone**: requires deferred + SM5+ (reads GBuffer); Substrate not fully
  supported; no forward/mobile.
- **Pass B — Screen Dithering**: final-screen pass; only needs ES3.1+ (works more broadly).

## Pass A — Cel/Halftone features

- **Quantized lighting bands**: Bands count, Midpoint, Band Offset, Distribution, Band Softness,
  Shadow Lift, Strength.
- **Halftone transition patterns** at band transitions. 10 patterns (ORDER IS CONTRACTUAL — it
  defines the stencil mapping): Bayer, Round Dots, Square Dots, Lines, Crosshatch, Diagonal Lines,
  Concentric Circles, Triangles, Clustered Noise, Spiral. Pattern space: world (surface-attached,
  triplanar, has triplanar-sharpness) or screen (camera-aligned). Controls: Enable Pattern,
  Pattern, Pattern Space, Strength, Contrast, world/pixel size, triplanar sharpness, distance
  scaling, octave range, scroll speed.
- **Color**: Shadow Tint, Light Tint, Saturation, Minimum Albedo, Affect Unlit, Quantize Final
  Color (quantize lighting vs final scene color).
- **Sky**: Enable Sky, Sky Distance, Bands, Strength, Pattern, Pattern Strength, Pattern Scale —
  separately tuned response for distant sky pixels.
- **Metallic**: Metallic Bands, Strength, Pattern Strength — distinct quantized response for
  metallic surfaces.
- **Specular**: Enable Specular, Steps, Threshold, Intensity, Roughness Cutoff — stepped stylized
  highlights.
- **Rim light**: Enable Rim Light, Power, Threshold, Softness, Intensity, Color, Follows Lighting.
- **Outline**: Enable Outline, Thickness, quality, color, blend mode, opacity,
  depth/normal/albedo thresholds, distance fade — screen-space ink lines.
- **Per-object via Custom Stencil**: Enable Stencil Patterns, Pattern Stencil Base (default 200).
  Mesh renders CustomDepth in stencil mode (`r.CustomDepth 3`); stencil = base+0..base+9 selects
  the pattern in enum order; base−1 (199) SUPPRESSES transitions on that mesh.
- **Moving-object stabilization**: EXPERIMENTAL temporal-history correction for world-space
  pattern sliding on translating movable/skeletal meshes; "may create trails" at silhouettes /
  disocclusions; rotation and newly revealed surfaces cannot be stabilized. (CUT from our port —
  needs temporal history a material blendable does not have.)
- **Debug modes** (0-9): final image, band index, outline mask, illumination, albedo, normals,
  pattern threshold, pattern coordinates, motion offset, stencil.

## Pass B — Screen Dithering features

- **Pattern**: Enabled, Pattern (Bayer 2x2 / Bayer 4x4 / Bayer 8x8 / Interleaved Gradient Noise /
  White Noise / procedural Blue Noise), Pixel Scale, Strength, Animate, Animation Period.
- **Color**: Palette Mode, Color Steps, Palette (custom palettes), Color Space, Monochrome,
  Monochrome Tint, Weight, Saturation, Contrast, Pre-Gamma.
- **Downsampling**: Box Filter Downsample, Stabilize Grid (pixelation with aligned grid).
- **Debug modes** (0-4): final image, pattern, quantization error, quantized-without-dither,
  downsampled input.

## Presets

- Cel: Balanced, Clean Anime, Comic Halftone, Ink Crosshatch, Soft Toon, Off.
- Dither: Balanced, Subtle Color, Retro Pixel, 4-Color Handheld, Animated Grain, Off.

## Runtime API (the shape we mirror, Ck-ified)

- `UCelShadingSubsystem` / `UDitheringShadingSubsystem` (world subsystems), each:
  `SetEnabled`/`IsEnabled`, `SetSettings`/`GetSettings` (`FCelShadingSettings` /
  `FDitheringShadingSettings`), `ResetToProjectDefaults`.
- Game-thread only; view extension copies effective settings at view-family setup.
- Actor components = scoped world-settings overrides, "do not render per-actor". (Dropped.)
- CVars, `-1` = resume settings-driven: `r.YShade.Cel.{Enabled,Bands,Midpoint,Pattern,
  PatternStrength,PatternSpace,Outline,Debug}`; `r.YShade.Dither.{Enabled,Pattern,ColorSteps,
  PixelScale,Strength,Weight,Monochrome,Debug}`.

## Docs' recommended tuning order

1. lock exposure/lighting → 2. cel bands/midpoint/distribution/shadow-lift/strength → 3. pattern
choice + scale → 4. outlines, specular, rim, metallic, sky → 5. screen dithering last, after the
tonemapped look is stable → 6. test motion, temporal upscaling, resolutions, packaged builds.

## Troubleshooting facts worth keeping

- Dither blurred → set Pixel Scale 1 while diagnosing; check TAA/upscaler/dynamic-res ordering;
  "avoid later effects that deliberately blur the final image".
- World pattern space is camera-stable but slides on translating skeletal meshes (the
  stabilization feature existed for this; we cut it and document the limitation instead).
