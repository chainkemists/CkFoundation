# Gate 1 — Foundation: generator extension + pattern library + param-count proof

> **Status:** ✅ Done (2026-08-06)
> **Depends on:** — (first gate)
> **Estimate:** 1–2 sessions — actual: 1 session (entered + exited 2026-08-06)
>
> **Amendments at exit** (full evidence: PROGRESS.md 2026-08-06 entries):
> - Work item 4's probe is PERMANENT (`StylizeParamCount` + `Looks/StylizeProbe.ush`), not deleted.
> - Work item 6 (unplanned, forced by discovery): the shader-compile gate was mutation-tested
>   toothless, so `Validate_LookShaders` became the exported `Validate_LookShaderCompile` reading
>   real resource compile errors, with a DESTRUCTIVE opt-in synchronous force used only by tests.
>   Generation-path behavior for shipped looks is unchanged.
> - "Byte-identical" is enforced as exact Custom-node Inputs + Code equality on a controlled
>   subject plus absence-of-new-inputs across the regenerated PP roster (raw .uasset bytes churn
>   on every regeneration regardless).

## Goal

After this gate: a PostProcess look can declare GBuffer scene textures (BaseColor / Metallic /
Roughness / Specular) and an opt-in depth-reconstructed `In.WorldPosition`; `StylizeCommon.ush`
provides every procedural pattern the three effects need; a ~50-param PostProcess look is PROVEN
to generate and compile; and every pre-existing look regenerates byte-identically.

## Entry criteria (pre-flight — run these, don't assume them)

- [x] Baseline captured 2026-08-06 (CkFoundation `9967963b4`, CkTests `fd26b553`): Usf pattern,
      10/10 passed, names in PROGRESS.md current-state block.
- [x] Engine facts verified against the checked-out source (D:/Repositories/UnrealEngine-Angelscript
      5.7.4), file:line evidence in PROGRESS.md (2026-08-06 entry). All three CONFIRMED, with:
      GBuffer UB bound at EVERY blendable location (pre-TAA is for temporal stability, not
      availability); after-tonemap WorldPosition is dynamic-res scaled (WorldPosition consumers
      stay at AfterDOF); PPI_SceneColor is REJECTED in PP domain — SceneColor stays
      PPI_PostProcessInput0, never add a PPI_SceneColor row.
- [x] Generator + validator sources read end-to-end; insertion points named in PROGRESS.md
      (2026-08-06 entry): `Get_SceneTextureWiring` table `CkUsf_Generator.cpp:162`, PP branch
      `:496-560`, default-trio invariant `:175`, `kReservedParamNames` `CkUsf_LookValidator.cpp:15`,
      non-PP warning shape `:404-411`.

## Work items

1. **`ECk_Usf_SceneTexture` extension** — add `BaseColor`, `Metallic`, `Roughness`, `Specular`;
   wire generator (SceneTexture expressions → Custom-node inputs → `In.<Name>` fields appended to
   `FCkUsf_SurfaceInput`) and validator. Pattern: the existing CustomDepth/CustomStencil opt-in
   rows (generator + `Common.ush` struct comments). EMPTY `_SceneTextures` still emits exactly the
   historical default trio — byte-identical invariant.
2. **Opt-in PP world position** — new LookDefinition bool (default off; PostProcess-only; name it
   per the `_PixelDataChannels` precedent) wiring `UMaterialExpressionWorldPosition` into the pixel
   Custom node as `In.WorldPosition` for PP looks. Validator errors when set on non-PP domains
   (mirror the `_ParticleColor` domain check).
3. **`Shaders/CkUsf/StylizeCommon.ush`** — NEW INFRASTRUCTURE (pure HLSL, no engine unknowns).
   Contents (each a small pure function, house naming `CkUsf_*`):
   Bayer 2/4/8 threshold matrices; white-noise hash (reuse `CkUsf_Hash21`); procedural blue-noise
   approximation; the 10 cel halftone patterns (Bayer, RoundDots, SquareDots, Lines, Crosshatch,
   DiagonalLines, ConcentricCircles, Triangles, ClusteredNoise, Spiral) behind one
   `CkUsf_Stylize_HalftonePattern(int Index, float2 P, ...)` dispatcher; the 4 hand-drawn stroke
   patterns (DiagonalPencil, Crosshatch, LooseScribble, Stipple) behind
   `CkUsf_Stylize_StrokePattern(...)`; luminance + palette-quantization helpers
   (steps quantize, custom-palette nearest, monochrome). Builds ON `Common.ush`
   (`CkUsf_IGN`, `CkUsf_Remap`, hashes) — re-declares nothing.
4. **Param-count proof** — a temporary `StylizeProbe` PostProcess look with 50 params (mix scalar/
   vector) + the new scene textures; generate + force-compile via the generator. Record verdict.
   On failure → STOP; the LUT fallback decision (PROMPT.md ruled-out table) reopens with evidence.
   Delete the probe look + asset after recording (its lesson lives in PROGRESS.md).
5. **Byte-identical negative test** — CkTests generation test asserting a pre-existing look's
   regenerated master is unchanged by items 1–2 (pattern: the `NiagaraSpriteContract` negative).

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| Toolbox build | Compiles clean, 0 new warnings | UHT/codegen errors on enum extension | Fix forward; the enum is additive — check generator switch exhaustiveness first |
| Generation of ALL existing looks | Byte-identical masters (negative test green) | Any diff | The new wiring leaked into the default path — find the non-gated expression emit; do NOT relax the test |
| StylizeProbe generation + compile | Generates, force-compiles clean at 50 inputs | Generation error / HLSL failure at N inputs | Record exact N + error; reopen LUT fallback (PROMPT.md) and present "[params] vs [LUT]" with evidence |
| CkUsf test suite | Baseline counts unchanged + new negative test green | New failures | A/B-stash to prove ownership; fix or revert before gate exit |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [x] Every expected observation confirmed; evidence in PROGRESS.md (2026-08-06 entries): build
      clean; 12/12 `-nullrhi` vs 10/10 baseline; 6/6 real-RHI serial CkUsf unit run; mutation test
      FAILS with the named HLSL error post-strengthening; 0 latent roster failures; byte-identical
      negative green.
- [x] Probe verdict recorded — kept permanent per the exit amendment above.
- [x] `[EDITOR-VERIFY]`: none required for this gate (no visual claims made).
- [x] PLAN.md status row AND this Status header updated — both, same commit.
- [x] CkUsf/Claude.md: SceneTexture GBuffer entries, `_PostProcessWorldPosition`, StylizeCommon.ush,
      and the strengthened compile gate documented.
- [x] PROGRESS.md dated entries appended.
