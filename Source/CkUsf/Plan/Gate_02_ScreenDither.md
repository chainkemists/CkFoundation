# Gate 2 — ScreenDither: look + subsystem + presets + tests

> **Status:** ⏳ Pending
> **Depends on:** Gate 1 ✅
> **Estimate:** 1–2 sessions — re-date at entry; record actual at exit

## Goal

After this gate: `UCkUsf_ScreenDitherSubsystem::Apply_Preset(RetroPixel)` produces a pixelated,
palette-reduced final frame in PIE; every dither setting is drivable at runtime from C++/BP/AS;
the look+subsystem+preset+test recipe is proven end-to-end for Gates 3–4 to copy.

Feature source of truth: Research_yShade_CelDither.md §Pass B. This is the LOW-unknown gate:
SceneColor only, `AfterTonemapping`, no stencil, no GBuffer.

## Entry criteria

- [ ] Gate 1 exit checklist re-verified on current HEAD (hash into PROGRESS.md)
- [ ] Baseline test counts re-captured
- [ ] Outline subsystem read end-to-end (`CkUsf_OutlineSubsystem.cpp` — the DoEnsure_ViewEffect /
      MID-caching / reaping mechanics this gate replicates); insertion points named in PROGRESS.md

## Work items

1. **`Looks/ScreenDither.ush`** — PostProcess look, `AfterTonemapping`, SceneColor only.
   Pattern selection (Bayer2x2/4x4/8x8, IGN, WhiteNoise, BlueNoise — from StylizeCommon), pixel
   scale, strength, animate + period (Time-driven threshold offset); palette reduction (palette
   mode: steps vs custom palette [fixed max 8 vector params + count], color space toggle,
   monochrome + tint, weight lerp, saturation/contrast/pre-gamma); pixelation (UV grid snap,
   optional 4-tap box filter, grid stabilization); debug modes 0–4 as a scalar param.
   Pattern to mimic: `EdgeOutline.ush` (PP look shape), buffer-UV discipline per the traps
   cookbook (`CkUsf_ViewportUVToBufferUV`, `CkUsf_BufferTexelSize`).
2. **Params struct** `FCk_Usf_ScreenDither_Params` — house ParamsData style (private `_Members`,
   `CK_PROPERTY*`, `CK_GENERATED_BODY`); enums `ECk_Usf_DitherPattern`, `ECk_Usf_PaletteMode`,
   `ECk_Usf_DitherColorSpace`, `ECk_Usf_ScreenDither_DebugMode` (+ `CK_DEFINE_CUSTOM_FORMATTER_ENUM`
   each). Defaults = our "Balanced" preset values.
3. **Preset DA** `UCkUsf_ScreenDitherPreset : UDataAsset` — public UPROPERTY fields mirroring the
   params struct (`UCkUsf_OutlinePreset` precedent), + `Get_AsParams()`.
4. **Subsystem** `UCkUsf_ScreenDitherSubsystem : UWorldSubsystem` — mirror
   `UCkUsf_OutlineSubsystem` mechanics (hidden view actor + `UPostProcessComponent` + one cached
   MID from the ScreenDither look master); API: `Get_ScreenDitherSubsystem`, `Request_SetEnabled` /
   `Get_IsEnabled`, `Apply_Preset`, `Request_SetSettings` / `Get_Settings`,
   `Request_ResetToDefaults`. Param writes only for changed values. Validation per
   non-negotiable #3 (hoisted condition + ensure + separate early-out; null preset/look = loud
   reject, zero mutation).
5. **Look asset + preset assets (AS)** — `Script/CkUsf/CkUsf_ScreenDitherLook_Assets.as` (the
   LookDefinition, `_Parameters` order = the .ush signature) +
   `CkUsf_ScreenDitherPresets_Assets.as` (Balanced, SubtleColor, RetroPixel, FourColorHandheld,
   AnimatedGrain, Off).
6. **Tests (CkTests)** — generation test for the look; subsystem settings round-trip
   (Set→Get equality); invalid-input tests (null preset, null world context → ensure fired, no
   state change, no crash); enable/disable idempotency.
7. **Gym** — "Stylize: Screen Dither", patterned on the Solid Outline gym
   (`CkUsfOutlineGym_GameMode/​_PlayerController.as` + `CkTests_GymRegistry.as` registration,
   alphabetical): `ACk_UsfStylizeDitherGym_GameMode` + PlayerController. Because the effect is
   VIEW-WIDE, stations are preset SELECTORS, not spatial showcases: one station per preset
   (Balanced, SubtleColor, RetroPixel, FourColorHandheld, AnimatedGrain, Off) whose activation
   applies that preset via the subsystem; station text names what to look for. Shared judge
   scene spawned once: gradient ramp wall (banding/palette readability), dielectric + metallic +
   rough/smooth spheres, a rotating mover (temporal stability), sky visibility. Exec commands:
   restart, cycle preset, cycle debug mode (mirror `Ck_GymOutline_RestartAll`).

## Expected observations at the gate

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| Toolbox build + full CkUsf tests | Baseline + new tests green | Regression | A/B-stash; fix or revert |
| Look generation | Generates + force-compiles; validator clean | Param-order mismatch | Fix the AS `_Parameters` order — the .ush signature is authoritative |
| [EDITOR-VERIFY] "Stylize: Screen Dither" gym: RetroPixel station | Pixelated palette-reduced frame; Off station restores the clean frame losslessly | Dither invisible | Check blendable weight + AfterTonemapping placement; check TAA isn't smearing (yShade troubleshooting: Pixel Scale 1 while diagnosing) |
| [EDITOR-VERIFY] FourColorHandheld | 4-color reduction + visible ordered dither | Banding without dither | Pattern threshold not applied pre-quantize — check quantize/dither order |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [ ] Observations confirmed; evidence in PROGRESS.md; `[EDITOR-VERIFY]` items listed with exact
      steps for the maintainer (agents cannot PIE)
- [ ] `ck-change-control` done-checklist for a new-feature change
- [ ] PLAN.md row + this header updated, same commit
- [ ] CkUsf/Claude.md: ScreenDither section added
- [ ] PROGRESS.md dated entry appended
