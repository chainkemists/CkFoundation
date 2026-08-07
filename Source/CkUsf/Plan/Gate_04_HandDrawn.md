# Gate 4 — HandDrawn: look + subsystem + presets

> **Status:** ✅ Done (2026-08-07) — code-complete + audited; gym visuals maintainer
> `[EDITOR-VERIFY]` (steps in PROGRESS.md Gate-4 entry, incl. the emissive-ceiling A/B).
> **Depends on:** Gate 3 ✅
> **Estimate:** 1–2 sessions — actual: 1 session
>
> **Amendments at exit:** audit ACCEPT-WITH-FIXES, no functional break — fixes were doc-truth
> (inverted-fade = hard cutoff not reverse-fade; highlight ceiling is 2·ColorLevels−1 not the
> named constant; `Enable in Editor Viewports` drop recorded) + one campaign-wide missing guard:
> MID-name resolution arms added to ALL THREE effects' subsystem tests (mutation-proven). P1 was
> a real bug (step(0,0)=1 inked zero-shade pixels at pattern cell centers) — fixed. Deviations
> accepted: strokes share InkColor; Reinhard tone normalize (no exposure knob) with the level-count
> ceiling; AffectSky gates paint+strokes never ink; +StrokeTriplanarSharpness/+SkyDistance.
> Recorded for Gate 5: `_Saturation` ClampMax policy (campaign-wide); Off vs StyleStrength=0
> passthrough distinction documented.

## Goal

After this gate: the StorybookInk preset shows ink contours + hatched shadows + paper grain in
PIE; world-attached strokes stay glued to static geometry under camera motion; every setting is
drivable from C++/BP/AS.

Feature source of truth: Research_yShade_HandDrawn.md. Mostly MECHANICAL — every technique here
has a Gate 2/3 precedent (posterize/tint ≈ palette ops; ink ≈ cel outline detectors; strokes ≈
halftone patterns in either space; paper ≈ noise ops).

## Entry criteria

- [ ] Gate 3 exit checklist re-verified on current HEAD (hash into PROGRESS.md)
- [ ] Baseline test counts re-captured

## Work items

1. **`Looks/HandDrawn.ush`** — PostProcess look at `SceneColorAfterDOF`; scene textures: default
   trio; opt-in WorldPosition (world-attached strokes).
   - Paint: posterize to Color Levels with Color Softness (smooth-quantize), pre-shaping
     Saturation/Contrast, Shadow/Highlight Tint by luminance split with Tint Strength, Affect Sky
     (SceneDepth far-plane test, reuse Gate 3's sky detection).
   - Ink: depth Laplacian + normal-angle detectors (mimic `EdgeOutline.ush`) + color-edge
     detector; Line Variation/Scale = noise-warped sample offsets (organic lines); distance fade
     start/end; color/thickness/opacity.
   - Shadow strokes: `CkUsf_Stylize_StrokePattern` (Gate 1) where luminance < Stroke Shadow
     Threshold; ScreenStable (pixel-space, Stroke Pixel Size) vs WorldAttached (planar-projected
     world pos, Stroke World Size); Strength, Irregularity (hash-jittered pattern phase).
   - Paper: grain (hash noise, Strength/Scale), directional fiber (anisotropic noise), Warmth
     (warm cast lerp). Applied LAST per the source's ordering.
   - Master Style Strength lerp; debug modes (FinalImage/InkMask/ShadowStrokeMask/PaperPattern/
     WorldNormals/SceneDepth) as scalar param.
2. **Params struct** `FCk_Usf_HandDrawn_Params` + enums (`ECk_Usf_HandDrawnStrokePattern`,
   `ECk_Usf_HandDrawnStrokeSpace`, `ECk_Usf_HandDrawn_DebugMode`), house style + formatters.
3. **Preset DA** `UCkUsf_HandDrawnPreset` + `Get_AsParams()`.
4. **Subsystem** `UCkUsf_HandDrawnSubsystem` — Gate 2's recipe verbatim (no stencil, no entity
   API — the source plugin has no per-object path and we add none).
5. **Look + preset assets (AS)** — StorybookInk, SoftPainted, BoldAnimation, DarkGothic,
   PencilWash, Off.
6. **Gym** — "Stylize: Hand-Drawn", Gate 2's gym recipe (preset-selector stations for
   StorybookInk/SoftPainted/BoldAnimation/DarkGothic/PencilWash/Off + shared judge scene + Exec
   commands, Solid Outline gym pattern, cycler registration). Hand-drawn-specific content:
   static architecture-like arrangement (walls/arches from basic shapes) for judging
   world-attached strokes under camera orbit; an animated mover for the screen-stable vs
   world-attached comparison; a high-detail-silhouette object for ink thresholds; stations for
   debug-mode visualization (InkMask / ShadowStrokeMask / PaperPattern).
7. **Tests** — generation; settings round-trip; invalid-input; enable/disable idempotency.

## Expected observations at the gate

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| Toolbox build + tests | Baseline + new green | Regression | A/B-stash; fix or revert |
| [EDITOR-VERIFY] StorybookInk | Contours + hatching + paper grain; strokes glued to static geometry in WorldAttached space under camera orbit | Strokes swim under camera motion | WorldPosition reconstruction wrong — check the opt-in wiring, not the pattern math (screen-space mode should be visibly camera-stable as the control) |
| [EDITOR-VERIFY] paper at two resolutions | Grain scale visually consistent | Grain shifts wildly | Expected per source docs at extreme cases; tune scale param defaults; document the limitation verbatim |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [ ] Observations confirmed; `[EDITOR-VERIFY]` steps listed; evidence in PROGRESS.md
- [ ] `ck-change-control` done-checklist
- [ ] PLAN.md row + this header updated, same commit
- [ ] CkUsf/Claude.md: HandDrawn section added
- [ ] PROGRESS.md dated entry appended
