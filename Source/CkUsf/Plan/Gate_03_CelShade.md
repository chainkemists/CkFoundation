# Gate 3 — CelShade: look + subsystem + stencil per-object + entity API

> **Status:** ✅ Done (2026-08-06) — code-complete + audited; gym visuals + the illumination
> reconstruction are maintainer `[EDITOR-VERIFY]` (steps in PROGRESS.md Gate-3 entry; prewritten
> pivot `_QuantizeFinalColor` if bands track albedo).
> **Depends on:** Gate 2 ✅
> **Estimate:** 2–3 sessions — actual: 1 session
>
> **Amendments at exit:** audit (ACCEPT-WITH-FIXES) found+fixed: stale Applied cache lost cel
> patterns after an outline round-trip (4th processor + disjoint _Remove views); gym entity/stencil
> rows misaligned; entity test was registry-only (now actor-backed incl. the regression sequence);
> stencil base <1 unguarded; cascade bypassed the outline-exclusion; dead Specular pin removed
> (master regenerated); two math-contradicting comments fixed. Decisions: clear = DISABLE not
> restore (matches outline precedent, documented in 3 places); cel↔outline mutually exclusive per
> entity (one stencil byte; outline-after-cel wins, cache drops); Strength governs bands/pattern
> only, Enabled is the passthrough; no deferred request path exists (immediate mutation mirrors
> Request_ApplyOutline — item 8's Failed_Cancelled conditional was vacuous). Known: Midpoint is in
> pre-exposed units; float32 world-pos degrades ~1e6 uu; world-space patterns slide on movers
> (documented limitation). DESIGN_EntityOutlines.md cited by docs does NOT exist in checkout —
> pattern reconstructed from code (stale-doc follow-up). Toolbox trap: `--discover-fresh` required
> after adding tests without a rebuild (stale test list = fake green).

## Goal

After this gate: the CleanAnime preset produces banded cel shading with halftone band transitions
in PIE; a mesh at Custom Stencil base+1 renders RoundDots while neighbors render the global
pattern and base−1 suppresses transitions; `Request_SetCelPattern(Handle, Pattern, Scope)` drives
the same per-entity.

Feature source of truth: Research_yShade_CelDither.md §Pass A. This is the HIGH-unknown gate —
the stencil path and entity API are new infrastructure; the illumination reconstruction is an
approximation to be judged in the gym.

## Entry criteria

- [ ] Gate 2 exit checklist re-verified on current HEAD (hash into PROGRESS.md)
- [ ] Baseline test counts re-captured
- [ ] `SolidOutline.ush` + outline stencil LUT mechanics + `DESIGN_EntityOutlines.md` +
      `CkUsf_Outline_Fragment.h`/`_Processor.cpp` read; the sync-processor pattern named
- [ ] Gate 1's GBuffer-read evidence re-checked against the blendable location chosen here
      (`SceneColorAfterDOF`)

## Work items

1. **`Looks/CelShade.ush`** — PostProcess look at `SceneColorAfterDOF`; scene textures:
   SceneColor, SceneDepth, SceneNormal, BaseColor, Metallic, Roughness, Specular, CustomStencil;
   opt-in WorldPosition (triplanar world pattern space).
   - Illumination = `SceneColor.rgb / max(BaseColor.rgb, eps)` (locked decision); band quantize
     (Bands, Midpoint, Band Offset, Distribution [exponent], Band Softness, Shadow Lift,
     Strength); Quantize Final Color mode quantizes SceneColor luminance instead.
   - Halftone transitions at band boundaries: `CkUsf_Stylize_HalftonePattern` (Gate 1), world
     (triplanar + sharpness) or screen space; strength/contrast/sizes/distance scaling/octave
     range/scroll speed.
   - Color treatment (Shadow/Light Tint, Saturation, Minimum Albedo, Affect Unlit [skip pixels
     where illumination ≈ 0 or unlit shading model], Quantize Final Color).
   - Sky (SceneDepth ≥ Sky Distance → separate bands/strength/pattern/scale).
   - Metallic response (Metallic ≥ threshold → own bands/strength/pattern strength).
   - Stepped specular (Steps/Threshold/Intensity/Roughness Cutoff) from
     illumination highlights gated by Roughness.
   - Rim light (Power/Threshold/Softness/Intensity/Color/Follows Lighting) from
     SceneNormal · view direction (view dir derived from reconstructed WorldPosition − camera).
   - Outline (Thickness, quality [tap count], color, blend mode, opacity, depth/normal/albedo
     thresholds, distance fade) — mimic `EdgeOutline.ush` Laplacian/normal-angle detectors,
     extended with an albedo (BaseColor) detector.
   - Stencil per-object: CustomStencil == Base−1 → suppress transitions; Base+0..9 → force that
     pattern index; else global pattern. NO allocation — direct-value contract.
   - Debug modes 0–9 as scalar param (band index, outline mask, illumination, albedo, normals,
     pattern threshold, pattern coords, stencil; motion-offset mode reports black — stabilization
     is a non-goal).
2. **Params struct** `FCk_Usf_CelShade_Params` + enums (`ECk_Usf_CelPattern` — ORDER IS THE
   STENCIL CONTRACT, `ECk_Usf_CelPatternSpace`, `ECk_Usf_CelShade_DebugMode`), house style +
   formatters.
3. **Preset DA** `UCkUsf_CelShadePreset` + `Get_AsParams()`.
4. **Subsystem** `UCkUsf_CelShadeSubsystem` — Gate 2's subsystem recipe; additionally validates
   the stencil base range `[Base−1, Base+9]` does not intersect the outline subsystem's
   `[StencilMin, StencilMax]` (loud ensure + clamp-reject per non-negotiable #3).
5. **Entity API** — `FCk_Handle_UsfCelPattern`-free minimal shape mirroring entity outlines:
   `ck::FFragment_Usf_CelPatternTarget` (+ Applied fragment), `UCk_Utils_Usf_CelPattern_UE::
   Request_SetCelPattern(Handle, ECk_Usf_CelPattern, Scope)` / `Request_ClearCelPattern(Handle)`,
   actor-path sync processor in CkUsf setting per-component CustomDepth/stencil
   (`ECk_Usf_OutlineScope` reuse for Entity vs EntityAndDependents). Renderer-module extensions
   (ISM/ISKM) recorded as follow-ups, NOT built here — outline precedent distributes those to the
   renderer modules later.
6. **Look + preset assets (AS)** — CleanAnime, Balanced, ComicHalftone, InkCrosshatch, SoftToon,
   Off. (Work items 7–8 below consume these.)
7. **Gym** — "Stylize: Cel Shade", Gate 2's gym recipe (preset-selector stations + shared judge
   scene + Exec commands, Solid Outline gym pattern, cycler registration). Cel-specific stations
   beyond the 6 presets: STENCIL row — three meshes with RenderCustomDepth at base−1 / base+1 /
   base+5 beside an untagged control (per-object suppress/RoundDots/Crosshatch vs global);
   ENTITY row — an entity-script station driving `Request_SetCelPattern`/`Request_ClearCelPattern`
   (mirror `CkUsfOutlineGym_EntityStations.as`); a skeletal/translating mover demonstrating the
   documented world-space pattern-sliding limitation. Judge scene adds a metallic sphere
   (Metallic group), a glossy sphere (stepped specular), and a rim-lit backlit figure.
8. **Tests** — generation; settings round-trip; invalid-input (incl. stencil-range collision
   rejection); entity request tests (apply → fragment present + stencil set on proxy component;
   clear → restored; destroy-mid-request completes `Failed_Cancelled` per house request contract
   if the deferred path is used — mirror whatever `Request_ApplyOutline` does, verified at entry).

## Expected observations at the gate

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| Toolbox build + tests | Baseline + new green | Regression | A/B-stash; fix or revert |
| Look generation | ~50-input master generates + compiles (Gate 1 proved the count) | Compile failure only at THIS look | Diff against the probe's param mix; isolate the offending input type |
| [EDITOR-VERIFY] CleanAnime in gym | Visible flat bands + halftone at transitions; stable under camera motion (pre-TAA placement) | Shimmering edges | Confirm `SceneColorAfterDOF` actually active on the generated master; re-check blendable-location doctrine |
| [EDITOR-VERIFY] illumination approximation | Bands follow lighting on lit dielectrics; metals handled by Metallic group | Bands driven by albedo, not light | The reconstruction failed — STOP, write addendum; candidate pivot: quantize luminance of SceneColor (Quantize Final Color mode) as default |
| [EDITOR-VERIFY] stencil mesh at base+1 / base−1 | RoundDots on that mesh / suppressed on that mesh | No per-object change | `r.CustomDepth 3` project setting + mesh RenderCustomDepth — verify before suspecting the shader |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [ ] Observations confirmed; `[EDITOR-VERIFY]` steps listed; evidence in PROGRESS.md
- [ ] `ck-change-control` done-checklist
- [ ] PLAN.md row + this header updated, same commit
- [ ] CkUsf/Claude.md: CelShade section + stencil contract documented
- [ ] PROGRESS.md dated entry appended
