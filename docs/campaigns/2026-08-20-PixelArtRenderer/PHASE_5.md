# PHASE 5 — The PixelArt look (outline + toon banding + palette, at internal res)

> Entry: Phase 4 done (the subsystem applies looks); Phase 2 running (visual verification needs
> the stabilized low-res raster). Load `ck-vfx-authoring` (the CkUsf-looks porting rules: dead
> `Const*` pins, `_Group` on every param, `_Defines` wiring) and read
> `Source/CkUsf/Claude.md` §Authoring loop + §Parameter contract + §Traps before writing HLSL.
> Every reference algorithm is captured VERBATIM in [RESEARCH_Technique.md](RESEARCH_Technique.md)
> §C (outline kernels, de-doubling, grazing-angle scaling, band-shift edge rule) and §D (banding
> math, OKLab posterize) — port from there, cite the section in the .ush header comment.

## Executable spec

This phase's deliverable is visual; the executable half is the generator/validator contract:

1. After authoring the look asset + .ush, `Ck_Usf_GenerateLooks` must produce
   `/CkFoundation/CkUsf/GeneratedLooks/M_CkUsf_Look_PixelArt` with ZERO validator errors
   (the validator output is the machine-checkable spec — paste its clean run into PROGRESS.md).
2. CkTests C++ contract test (mimic `Test_Usf_StylizeContract.cpp`):
   `Test_Usf_PixelArtLookContract.cpp` — asserts the LookDefinition asset's `_Parameters` list
   matches the .ush entry signature positionally (count + names), `_Domain == PostProcess`,
   `_BlendableLocation == SceneColorAfterDOF`, and `_SceneTextures` covers
   SceneColor/SceneDepth/WorldNormal.
3. The visual rubric (scored in Phase 6's gym, `[EDITOR-VERIFY]`): silhouettes darken 1 texel /
   creases brighten 1 texel / no doubled creases / no ground-plane staircase at grazing angle /
   band-shift mode picks palette-adjacent colors.

## Steps

1. **Files** (look lives in CkUsf — CkUsf owns looks; CkPixelArt applies it):
   - `Source/CkUsf/Shaders/CkUsf/Looks/PixelArt.ush` — entry
     `CkUsf_PP_PixelArt(FCkUsf_SurfaceInput In, <params in asset order>)`.
   - `Script/CkUsf/CkUsf_PixelArtLook_Assets.as` — `asset ... of UCkUsf_LookDefinition`
     (mimic `CkUsf_ScreenDitherLook_Assets.as` incl. the PaletteColor0..7 + count trick).
   - `Script/CkUsf/CkUsf_PixelArtPresets_Assets.as` — at least two presets ("Crisp16" —
     16-color palette + band-shift edges; "SoftRamp" — 5 bands, flat edge colors).
2. **Shader structure** (one look, self-contained — two pre-TAA looks don't compose,
   `CkUsf/Claude.md:453`):
   - Stage 1 — edge classification (RESEARCH §C.1/C.2): 4-tap plus kernel; silhouette = one-sided
     depth discontinuity with grazing-angle-scaled threshold
     (`z_thresh = base * (1 + t01 * AngleScale)`); crease = opposed-pair normal contrast
     (`max(|d_up−d_down|, |d_left−d_right|)`) with the de-doubling bias dot + shallower-pixel
     gate; convex-vs-concave = the view-facing comparison (§C.3).
   - Stage 2 — banding (§D): wrap N·L is not available in a PP pass — band the LUMINANCE of
     scene color instead (CelShade's proven approach; reuse
     `StylizeCommon.ush` quantize/dither helpers), `_Bands` + `_ThresholdGradientSize` +
     optional Bayer dither at band edges.
   - Stage 3 — palette (§D): `ECk_Usf_PaletteMode`-style ladder (ColorSteps / CustomPalette /
     LuminanceSteps) — mimic `ScreenDither.ush:200-230`, reuse its encode/decode around the
     quantizer.
   - Stage 4 — edge application: `EdgeMode` param — 0 = flat colors (`_LineDarken`,
     `_CreaseBrighten` multipliers), 1 = band-shift (±1 band in the quantized space BEFORE
     palette snap, §C.3 — the t3ssel8r signature).
   - Neighbour taps: viewport→buffer UV ONCE via `CkUsf_ViewportUVToBufferUV` +
     `CkUsf_BufferTexelSize()` (the documented CkUsf trap).
3. **Parameter list** (positional contract; every param gets `_Group` + `_SortPriority`;
   groups: "Outline", "Banding", "Palette", "Edges"): `EnableOutline`, `DepthThreshold`,
   `AngleZCutoff`, `AngleZScale`, `NormalSmoothLow`, `NormalSmoothHigh`, `LineDarken`,
   `CreaseBrighten`, `EdgeMode`, `Bands`, `ThresholdGradientSize`, `DitherStrength`,
   `PaletteMode`, `PaletteCount`, `PaletteColor0..7`. Mirror these into
   `FCk_PixelArt_LookParams` (declared in Phase 4) and write the subsystem's
   `DoWrite_ChangedParams`-style MID projection (diff against `TOptional<> _WrittenLook`,
   full-write first time, enums as index — the CelShade shape).
4. **Wiring**: the Phase 4 subsystem, when `_ApplyLook` is enabled, creates the MID via
   `UCk_Utils_Usf_UE::Create_MID_ForLook` and applies it on its own hidden
   view actor + unbound `UPostProcessComponent` (CelShade `.cpp:338-383` lazy-creation shape).
   Stencil-range validation is NOT needed (this look claims no stencil values) — note that in
   the module doc.
5. **Generate + iterate**: `Ck_Usf_GenerateLooks` → fix validator errors → `recompileshaders
   changed` loop for .ush-only edits. Regen order rule from `ck-vfx-authoring` applies if any
   texture params get added later.
6. Contract test green; commit (CkUsf look commit; CkPixelArt wiring commit; CkTests commit).

## Exit criteria

- Validator-clean generation (pasted output) + contract test green.
- In the Phase-2 debug scene: enabling the look via `Apply_Preset` visibly produces 1-texel
  outlines and banded palette colors at internal res `[EDITOR-VERIFY]` (full rubric scoring
  happens in Phase 6's gym).
- `rg -n "_Group" Script/CkUsf/CkUsf_PixelArtLook_Assets.as` — every parameter grouped.
- Full suite delta-zero vs baseline.

## Fences

- ONE look. Do not split outline and banding into two looks — pre-TAA looks do not compose.
- Do not modify `CelShade.ush`, `ScreenDither.ush`, or `StylizeCommon.ush` beyond ADDING new
  shared helpers (additive only; a replace-all in a shared .ush once struck three sibling
  behaviors — `ck-vfx-authoring`).
- Do not read gameplay light direction in the shader for edge highlighting — the band-shift
  rule is geometric (view-facing comparison); the light-gated variant is UNVERIFIED upstream
  and out of scope.
- No raw `.Sample()` on scene textures in a lit path — follow the CkUsf traps cookbook.
- The look must never reference the pixel-art renderer state — it must render (as an ordinary
  full-res stylization) even with the renderer off; the pairing is composition, not coupling.
