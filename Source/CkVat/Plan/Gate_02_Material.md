# Gate 2 — Material (VAT decode looks in CkUsf)

> **Status:** see [PLAN.md](../PLAN.md). **Depends on:** Gate 1 code-complete (layout contract in
> [Gate_01_Bake.md](Gate_01_Bake.md)). **Estimate:** 1 session (2026-07-09).

## Goal

After this gate: two CkUsf looks — `VatVertex` and `VatBone` — decode the Gate-1 textures entirely
in the vertex shader (WPO), driven by 12 per-instance custom-data floats and per-collection MID
uniforms, with frame interpolation and 2-state crossfade. Master materials regenerate via
`Ck_Usf_GenerateLooks`.

## Verified facts this gate builds on (2026-07-09, file:line in the digests)

- Look defs are **AS asset blocks** (`Script/CkUsf/CkUsf_Looks_Assets.as`, e.g. Displace :319-350);
  masters are GENERATED artifacts under `/CkFoundation/CkUsf/GeneratedLooks/`.
- WPO entry receives `FCkUsf_VertexInput{Time, UV(TexCoord0), WorldPosition, VertexNormal, VertexColor}`
  (`Common.ush:53-60`); pixel + WPO entries share ONE positional param list (validator
  `CkUsf_LookValidator.cpp:418-436`).
- Per-instance Scalar params map to custom-data slots in **declaration order** (Scalar=1, Vector=3;
  `CkUsf_LookDefinition.cpp:32-62`).
- `CkUsf_SampleTexture2D` is mandatory in WPO (SampleLevel outside PS, `Common.ush:171-187`).
- **UV1/UV2 are not wired into the VS** — generator extension required (5 sites, see work item 1).
- `TransformLocalVectorToWorld(FMaterialVertexParameters, v)` respects `InstanceLocalToWorld` under
  `USE_INSTANCING || USE_INSTANCE_CULLING` (`Engine/Shaders/Private/MaterialTemplate.ush:1702-1709`)
  — the wrapper can hand looks a per-instance local→world basis without new pins.

## PER-INSTANCE FLOAT CONTRACT (12 scalars, declaration order in the look assets)

| Slot | Name | Meaning |
|---|---|---|
| 0-3 | `RowStartA, RowCountA, TimeA, RateA` | active clip: first texture row, row count, start world-time, rate. **When RateA==0, TimeA holds the frozen clip-local time** |
| 4-7 | `RowStartB, RowCountB, TimeB, RateB` | crossfade source clip (same shape; `RowCountB==0` ⇒ no fade source) |
| 8-9 | `TransStart, TransDuration` | crossfade window (world-time); `TransDuration==0` ⇒ instant |
| 10-11 | `LoopA, LoopB` | 0 = loop, 1 = play-once-clamp |

MID uniforms per collection: `BaseColorTex`, `PosVat` (or `BonePosVat`+`BoneRotVat`),
`SampleFrequency`, `TotalRows`, `TexelCount` (width; bone mode: bone count), `BoundsMin`,
`BoundsMax` (FVector), `DecodeNormalized` (0 = High/raw, 1 = Low/bounds-normalized).
Bone-mode weights: shader reads `VertexColor.rgb` = weights 0-2, **weight3 = saturate(1 − r−g−b)**
(VertexInput carries only RGB).

## Work items

1. Generator extension (CkUsf/CkUsfEditor): `FCkUsf_VertexInput` += `UV1, UV2, LocalAxisX/Y/Z`;
   `Build_WpoCustomCode` fills them (UV1/UV2 from new TexCoord pins `CoordinateIndex=1/2`;
   LocalAxis* via `TransformLocalVectorToWorld(Parameters, unit axes)` — Parameters is in scope
   inside Custom-node code, no pins needed); WPO node input list += UV1/UV2; validator reserved
   names += UV1/UV2. Pixel path untouched.
2. `Shaders/CkUsf/Looks/Vat.ush` — `VatVertex` + `VatBone` look pairs (pixel = BaseColorTex sample;
   WPO = full decode: local-time → fractional row (loop/once), 2-row frame interpolation, crossfade
   nlerp/lerp, bone-mode 4-influence quat+translation skinning, local→world via the basis).
3. AS look assets `VatVertex` + `VatBone` appended to `CkUsf_Looks_Assets.as` (param order == the
   contract above; `_WpoFunctionName` set; SurfaceLit).
4. Master regen attempt headless (`Ck_Usf_GenerateLooks`); else [EDITOR-VERIFY].

## Deferred (recorded, not silent)

- **VAT normal consumption in the pixel shader** — the baker already emits `_Nrm`; consuming it
  raises the normal-space question (world-space Normal output / tangent basis under animation).
  VAMP ships this as an advanced feature too. Follow-up gate item; v1 lights with bind-pose normals.
- WPO velocity/TAA behavior (`r.Velocity.EnableVertexDeformation`) — [EDITOR-VERIFY] observation.

## Expected observations

| I will run | I expect | If instead | Response |
|---|---|---|---|
| toolbox `--build` | Succeeded | errors | fix; generator edits are the risk site |
| Iskm suite | 29/0/0 + **no `Angelscript: Error` naming CkUsf_Looks_Assets.as** in the boot log | AS errors | fix the asset-block syntax against the Displace exemplar |
| headless `Ck_Usf_GenerateLooks Vat*` | masters saved under GeneratedLooks | fails headless | [EDITOR-VERIFY]: run the command in-editor; existing looks must also still regenerate (`Ck_Usf_GenerateLooks` full pass) |

## Exit criteria — same commit as last work item

- [ ] Build green; suite 29/0/0; AS clean for the edited file.
- [ ] Layout contract above matches the shipped `.ush` (re-read code).
- [ ] PLAN.md row + PROGRESS.md entry.
