# Gate 1 — Bake (in-editor VAT baker)

> **Status:** see [PLAN.md](../PLAN.md) (single home). **Depends on:** Gate 0 ✅ (`f69f9095a..8649f328c`).
> **Estimate:** 1 session (2026-07-09).

## Goal

After this gate: pressing Bake on a `UCk_VatCollection_Data` (via `UCkVat_BakerSubsystem`) produces —
in sibling packages, overwrite-in-place — a `UStaticMesh` (bind-pose geometry + lookup UV channel(s)
+ bone-mode weights/indices), mode-appropriate VAT textures, and writes the serialized clip table +
bounds back onto the collection (`_IsBaked = true`). Everything below "TEXTURE LAYOUT CONTRACT" is
the spec Gate 2's shader decodes — change it only with a dated edit here.

## Entry criteria

- [x] Gate 0 exit re-verified: committed on `feature/vat-feature`, Iskm suite 29/0/0 on final binaries.
- [x] 5.7 engine APIs verified at file:line against `D:\Repositories\UnrealEngine-Angelscript` (research
      agent, 2026-07-09): FMeshDescription authoring (`FbxStaticMeshImport.cpp:717-718` two-call UV-channel
      idiom), `CreateMeshDescription`/`CommitMeshDescription`/`Build` (`StaticMesh.h:1620/1648`), build-settings
      overrides required (`EngineTypes.h:2717/2721/2748` — defaults are all true), editor source read via
      `GetImportedModel()->LODModels[0]` + `FSkelMeshSection::SoftVertices` (`SkeletalMeshLODModel.h:61`,
      `FSoftSkinVertex` `SkeletalMeshTypes.h:56-72`, influences **uint16**, BoneMap-relative), package ceremony
      precedent `FMeshUtilities::ConvertMeshesToStaticMesh` (`MeshUtilities.cpp:542-754`), `TSF_RGBA16F` +
      `TC_HDR` (`TextureDefines.h:357/399`).

## Work items

1. Collection additions: `_PositionBoundsMin/_Max` baked fields (Low-precision decode range; also
   exposed to the Gate-2 material) + editor-only `ApplyBakeResults(FCk_Vat_BakeResults)` mutator.
2. `ck::vat_editor::Bake_VatCollection(...)` in CkVatEditor: sampling via `ck::anim_bake` (Gate-0 core),
   both encoders, static-mesh build, package/save ceremony (mimics `CkParticles_TextureGenerator::Bake`
   for packages + `ConvertMeshesToStaticMesh` for the mesh ceremony).
3. `UCkVat_BakerSubsystem : UEditorSubsystem` (mirror `UCkParticles_GeneratorSubsystem`) with a
   BlueprintCallable `Bake(Collection)` — AS/BP-callable in editor.
4. Build.cs: CkVatEditor += CkAnimation, MeshDescription, StaticMeshDescription, AssetRegistry, EditorSubsystem.

## TEXTURE LAYOUT CONTRACT (Gate 2 decodes this)

Common: row = global frame (row 0 = reference pose ⇒ vertex-mode offsets are zero); rows =
`FrameLayout.TotalFrameCount` (ensure ≤ 8192); clips occupy contiguous row ranges per the serialized
clip table. Textures: `SRGB=false`, `TMGS_NoMipmaps`, `TF_Nearest`. Precision High =
`TSF_RGBA16F` + `TC_HDR` (raw values); Low = `TSF_BGRA8` + `TC_VectorDisplacementmap` (normalized).

**Vertex mode** (texture width = LOD0 render-vertex count, ensure ≤ 4096):
- Position texture: RGB = component-space offset from bind pose (`skinned - bind`); High raw f16,
  Low normalized `(offset - BoundsMin) / (BoundsMax - BoundsMin)`. A = unused (1).
- Normal texture: RGB = skinned unit normal in the vertex's BIND-POSE TANGENT frame
  (`(dot(n,T), dot(n,B), dot(n,N))`, B reconstructed `cross(N,T) * sign` matching the engine's
  GenerateYAxis), encoded `n * 0.5 + 0.5` (BOTH precisions). A = unused. Tangent-space is
  load-bearing: it feeds the material Normal pin directly and is invariant under the per-instance
  transform — the PS has no instance basis to transform a local/world normal with (2026-07-10).
- Mesh lookup UV (`_LookupUVChannel`): `U = (vertexIndex + 0.5) / width`, `V = 0` (shader computes row V).
- Skinning per vertex: strongest ≤ 4 influences from the editor model (uint16 weights / 65535,
  renormalized; > 4 influences keeps the strongest 4 and logs ONE per-bake summary at Display —
  not an ensure/warning: standard Mannequin content carries 5-influence vertices, and the
  automation harness escalates warnings to failures. Changed 2026-07-10).

**Bone mode** (texture width = render-bone count from `ck::anim_bake::BuildSkeletonData`):
- BonePosition texture: RGB = translation of `ShaderMatrix = RefPoseInverse * ComponentSpace` (the
  bind-relative bone transform); High raw, Low bounds-normalized (same bounds fields). A = unused.
- BoneRotation texture: RGBA = normalized quaternion of ShaderMatrix rotation; High raw f16, Low
  `q * 0.5 + 0.5`. Bone SCALE is not baked (v1 limitation — ensure fires if a sampled bone matrix
  carries non-unit scale beyond tolerance? NO — silent perf cost unacceptable; documented limitation only).
- Mesh carries per-vertex skinning data: vertex COLOR = 4 weights (renormalized, strongest 4);
  UV `_LookupUVChannel` = (renderBoneIdx0, renderBoneIdx1), UV `_LookupUVChannel + 1` =
  (renderBoneIdx2, renderBoneIdx3) as raw float indices. Bone U = `(idx + 0.5) / width` in-shader.
- Influence chain: soft-vertex influence (BoneMap-relative) → `Section.BoneMap[i]` (mesh bone) →
  `GetSkeletonBoneIndexFromMeshBoneIndex` → `SkeletonBoneToRenderBone[]` (ensure ≠ INDEX_NONE).

Static mesh (both modes): bind-pose positions/normals/tangents/UV0 from the editor model;
`bRecomputeNormals/Tangents = false`, `bGenerateLightmapUVs = false`, `bUseFullPrecisionUVs = true`,
`bRemoveDegenerates = true`; material slots copied from the source mesh (Gate 3 overrides with the
VAT look MID); Nanite off.

## Expected observations — and branches

| I will run | I expect | If instead | Response |
|---|---|---|---|
| toolbox `--build` | Succeeded | UHT/link errors | fix; editor-module deps are the usual suspect (Gate 0 lesson) |
| Iskm suite (unchanged code) | 29/0/0 | delta | my build broke shared code — A/B and fix |
| [EDITOR-VERIFY] Bake on a real collection (human): create `UCk_VatCollection_Data` for a BB character mesh + 2 clips, call `UCkVat_BakerSubsystem::Bake` | assets appear beside the collection; clip table + `_IsBaked` populated; position texture row 0 ≈ 0.5-gray (Low) / black (High); re-bake overwrites without duplicates | bake ensure fires / wrong texel data | read the ensure; verify against the layout contract before touching the shader |

## Exit criteria — same commit as the last work item

- [ ] Build green; Iskm suite still 29/0/0 (bake code is editor-only — runtime untouched this gate).
- [ ] [EDITOR-VERIFY] steps above listed for the human (the agent cannot press Bake in a live editor).
- [ ] PLAN.md row flipped; PROGRESS.md dated entry with evidence.
- [ ] Layout contract above matches the shipped encoder (re-read the code, not memory).
