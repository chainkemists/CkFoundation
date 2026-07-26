# CkUsf

**Purpose:** Author materials as plain HLSL functions instead of material-editor node graphs. A
*look* = one `.ush` entry point + one `UCkUsf_LookDefinition` data asset; the editor-side generator
(CkUsfEditor) assembles a master `UMaterial` around a Custom node, validates the asset↔HLSL
contract, force-compiles the shaders, and saves it under `/CkFoundation/CkUsf/GeneratedLooks/`.
Runtime code applies looks via `UCk_Utils_Usf_UE` (master lookup, MID creation, post-process
attach). Also home to the Shadertoy-style multi-pass renderer and the Custom-Stencil outline
subsystem.

**Depends on:** `CkCore`, `CkEcs`, `CkGraphics`, `CkLog`.
**Editor twin:** `CkUsfEditor` (generator, validator, console command, save-hook).

---

## Key API

- `UCkUsf_LookDefinition` (`LookDefinition/CkUsf_LookDefinition.h`) — the look asset: ush
  include/function names, domain, blend/shading/translucency-lighting overrides, `_Defines`,
  usage flags, params. Per-instance slot layout queries: `Get_PerInstanceSlotOf`,
  `Get_NumPerInstanceFloats`.
- `UCk_Utils_Usf_UE` (`Apply/CkUsf_Utils.h`) — `Get_LookMasterMaterial`, `Create_MID_ForLook`,
  `Apply_PostProcess_ToCamera/Component`, `Set_Scalar/Vector/Texture`.
- `UCkUsf_MultiPassRenderer` (`MultiPass/`) — BufferA-D + Image passes, double-buffered feedback.
- `UCkUsf_OutlineSubsystem` (`Outline/`) — per-preset Custom-Stencil outlines + params LUT.
- `UCk_Utils_Usf_Outline_UE` (`Outline/CkUsf_Outline_Utils.h`) — ENTITY-level outlines:
  `Request_ApplyOutline(Handle, Preset, Scope)` / `Request_RemoveOutline`. Per-renderer sync processors
  apply it (actor path here; shadow ISM in CkIsmRenderer; SKMC custom depth in CkIskmRenderer Plan-1;
  batched Plan-2 members are not entities — use `UCk_Utils_IskmBatched_UE::Set_CrowdMemberOutline`).
  Design + mechanisms: `DESIGN_EntityOutlines.md`.
- `/CkUsf/Common.ush` — the input/output structs + the shader stdlib (sampling, normals, parallax,
  triplanar, flow maps, SDFs, color ops, dithering).
- Generation (editor): `ck::usf_editor::Generate_AllLookMaterials()` / `Generate_LookMaterial(Def)`
  (`CkUsfEditor/Generator/CkUsf_Generator.h`); validation in `CkUsf_LookValidator.h`.

## Authoring a new look (the loop)

1. **Write the shader** — `Source/CkUsf/Shaders/CkUsf/Looks/<Name>.ush` (or your own module's
   mapped shader dir; BB maps `/BusterBlock` in `BusterBlock.cpp`):
   ```hlsl
   #pragma once
   #include "/CkUsf/Common.ush"

   FCkUsf_SurfaceOutput CkUsf_Look_<Name>(FCkUsf_SurfaceInput In, float3 Tint, float Speed)
   {
       FCkUsf_SurfaceOutput O = CkUsf_DefaultSurface();
       O.BaseColor = Tint * (0.5 + 0.5 * sin(In.Time * Speed));
       return O;
   }
   ```
2. **Declare the asset** — AS `asset <Name> of UCkUsf_LookDefinition { ... }`
   (`Script/CkUsf/CkUsf_Looks_Assets.as` is the exemplar file) or an editor data asset.
   `_Parameters` order MUST match the function's params after `In` — the validator enforces it.
3. **Generate** — save the asset in-editor (auto-regens via the package-save hook), or run the
   console command `Ck_Usf_GenerateLooks [LookName]`, or call the `UCkUsf_GeneratorSubsystem`
   functions. Generation fails LOUDLY (log + result errors) on any contract violation.
4. **Iterate on the .ush only** — no regeneration needed: run `recompileshaders changed` in the
   editor console (the generated master references the include; the engine re-reads it). Set
   `r.ShaderDevelopmentMode=1` in ConsoleVariables.ini for retry-on-error dialogs.
5. **Apply** — `Create_MID_ForLook` + assign to a mesh slot, or the post-process helpers.

## The parameter contract (enforced by the validator at generation)

The generator passes params to the look function **positionally, in `_Parameters` declaration
order**. Asset↔HLSL mapping: `Scalar → float`, `Vector → float3` (the param node's float4 is
`.rgb`-swizzled), `Texture2D → Texture2D <Name>, SamplerState <Name>Sampler`, `TextureCube`
likewise. Count/type/order mismatch = generation **error**; HLSL param-name drift = warning.
A WPO entry point (`_WpoFunctionName`) takes `FCkUsf_VertexInput` + the SAME param list.

Param names may not be: HLSL keywords, duplicates (material param names are case-insensitive), or
the reserved Custom-node input/output/local names (`Time`, `UV`, `WorldPosition`, `CameraVector`,
`VertexNormal`, `VertexTangent`, `PixelDepth`, `VertexColor`, `Scene*`, `Custom*`, `EmissiveColor`,
`Roughness`, `Metallic`, `Specular`, `AmbientOcclusion`, `Normal`, `Opacity`, `OpacityMask`,
`Refraction`, `SubsurfaceColor`, `ClearCoat`, `ClearCoatRoughness`, `In`, `O`). The entry point
must be defined DIRECTLY in `_UshIncludePath` (not a nested include / macro).

Two further asset-side rules the validator enforces before it ever reads the .ush: `_PerInstance` is
legal only on Scalar/Vector params (textures cannot ride per-instance custom data), and
`_UshIncludePath` must resolve through the registered shader source directory mappings
(`AddShaderSourceDirectoryMapping`) — an unresolvable virtual path is an error, not a silent skip.

## Traps cookbook

- **Never raw `.Sample()` in a lit or WPO'd look** — lit looks compile ray-tracing hit permutations
  where `Sample()` is an illegal opcode, and the WPO node compiles the include in the VS. Use
  `CkUsf_SampleTexture2D/Cube` (frequency-safe) or explicit `SampleLevel`.
- **Post-process neighbour taps: convert viewport→buffer UV once** (`CkUsf_ViewportUVToBufferUV`)
  and sample everything in buffer space, else the effect drifts off-object under screen percentage
  / window resize. Texel size = `CkUsf_BufferTexelSize()`.
- **The Normal pin is TANGENT-space.** Normal maps feed it directly (`CkUsf_SampleNormalMap`);
  world-space procedural normals must go through `CkUsf_WorldToTangentNormal(In, N)`.
- **Per-instance slots are never counted by hand** — writers query
  `LookDefinition->Get_PerInstanceSlotOf(Name)` / `Get_NumPerInstanceFloats()` (Scalar = 1 float,
  Vector = 3). The generator derives node `DataIndex` from the same functions.
- **Hand-edits to a generated master are wiped by the next regeneration.** Everything must come
  from the LookDefinition (usage flags, defines, blend/shading). That is the point.
- **`_Defines`** (`"NAME"` / `"NAME=VALUE"`) are the static-switch equivalent — they feed the
  Custom nodes' `AdditionalDefines`, e.g. retuning a `#ifndef` default without touching the .ush.
  A define change needs regeneration (it changes the material, not just the include).
- **Lit translucency reads flat by default** — set `_TranslucencyLighting = SurfacePerPixel` on
  glass-like looks (engine default is volumetric non-directional).
- **`_PixelDataChannels` is opt-in because interpolators cost** — only looks whose PIXEL stage decodes
  mesh data channels need TexCoord1/TexCoord2 wired (e.g. CkVat's normal-texture lookup). The WPO entry
  point always receives UV1/UV2; it lives on its own VS-safe Custom node because the pixel node reads
  pixel-only inputs.
- **UV is TexCoord0** (surface) or screen position (post-process). Other UV channels are not wired;
  raw escape hatch: `Parameters`/`View.*`/`GetPrimitiveData()` inside the .ush work but are
  version-fragile — prefer asking for the input to be wired properly.
- **Generation validates shaders by force-compiling** — but only permutations the generating
  session can compile. `Get_LookMasterMaterial` re-checks at load and names the look if its master
  would render as the checkered default.
- **Multi-pass looks** declare the magic uniforms (`iResolution` Vector, `iFrame`/`iTimeDelta`
  Scalars, `iChannelN` Texture2D) as ordinary params; the renderer binds them by name each tick.

## Anti-patterns

- Don't hand-author materials that reference `/CkUsf/` includes — the generator owns that wiring.
- Don't `git add` regenerated `Content/CkUsf/GeneratedLooks/*.uasset` blindly alongside unrelated
  work; regeneration timestamps churn them.
- Don't name a look param after a Custom-node output — the generated signature lists the name
  twice → "redefinition" (the validator now rejects this before HLSL ever sees it).

## Implementation notes

### Blendable location (`_BlendableLocation`)

The pre-TAA locations (`SceneColorAfterDOF` / `SceneColorBeforeDOF`) run at *rendering* resolution
BEFORE TSR/TAA, so the look's output is temporally accumulated like ordinary geometry. Anything
derived from Custom Depth/Stencil **requires** one of them: those buffers are rendered with the
TAA-jittered projection every frame, so a look placed after tonemapping thresholds that jittered
mask with no temporal resolve ever seeing it — its edges shimmer even on a stationary camera.
Trade-off: pre-TAA is also pre-tonemap, so output colors are scene-referred linear (the tonemapper
remaps them and bloom sees them), and TSR may slightly ghost the output behind fast movers.

### Per-instance slot layout

`Get_PerInstanceSlotOf` / `Get_NumPerInstanceFloats` on the LookDefinition are THE source of truth,
shared by the generator (Custom-node `DataIndex`) and runtime writers (CkIsmRenderer
`SetCustomDataValueById` / `NumCustomDataFloats`). Slots accrue over `_Parameters` in declaration
order — per-instance Scalar = 1 float, per-instance Vector = 3 (rgb). Explicit `_PerInstanceSlot`
values do not advance the auto counter; both kinds resolve through the same query.

The generator emits one `PerInstanceCustomData(3Vector)` node per per-instance param (`DataIndex` = the
slot, `ConstDefaultValue` = the param default), so a non-instanced mesh renders the default and the look
stays safe everywhere. Set `_PerInstanceSlot` explicitly when the instance custom-data layout is owned
elsewhere — e.g. CkIskm batched crowds reserve `[0]`/`[1]` for frame bits and start game data at `[2]`
(`CkIskm_BatchedClusterComponent::SendRenderDynamicData_Concurrent`).

### Shading-model wiring

Each exotic `_ShadingModel` is generated together with the G-buffer outputs it requires: `Subsurface`
wires SubsurfaceColor (and Opacity drives the scatter), `ClearCoat` wires ClearCoat + ClearCoatRoughness.
`Inherit` keeps the domain default (SurfaceLit → DefaultLit, everything else → Unlit).

### Generated-master invariants (CkUsfEditor)

- An empty `_SceneTextures` falls back to the historical default trio (SceneColor / SceneDepth /
  SceneNormal), so PostProcess looks authored before the field existed regenerate byte-identically.
- Refraction is wired only for **lit** translucency (glass): unlit translucent looks stay
  byte-unchanged and no unlit-translucent + refraction permutation is compiled.
- Usage flags are re-baked from the LookDefinition on every regeneration, and surface masters
  additionally force the instanced + skeletal permutations on. The renderers auto-enable those at
  runtime, but that path falls back to the default material in a packaged build.

### Entity outlines

Each renderer module's sync processor reacts to (`ck::FFragment_Usf_OutlineTarget` + its own proxy
fragment) and records what it applied in a module-local `...OutlineApplied` fragment, so removal /
EndPlay can undo without the Target fragment. Cascade-derived targets are stamped on lifetime
dependents by `ECk_Usf_OutlineScope::EntityAndDependents`; dependents spawned after the request are
not retro-outlined.

`UCkUsf_OutlineSubsystem`'s stencil allocation (`Get_OrAllocate_StencilFor` / `Release_StencilFor`) is
public and refcounted precisely so those other renderer modules can share a preset's Custom-Stencil value
without depending on CkUsf's own component-apply path. The whole outline design mirrors the reference
SolidOutlineSystem, `ECk_Usf_OutlineType` included.

### Outline preset follow-ups

`_UseFillTexture` samples the subsystem's single *shared* fill texture. Per-preset fill textures are
a known follow-up — they would need a texture atlas or array.

## See also

- `CkGraphics/Claude.md` (MID utilities), `CkIsmRenderer/Claude.md` (per-instance data writers).
- Tests: `CkTests/Source/CkTests/Private/UnitTests/CkUsf/` (generation, multi-pass, outline);
  gyms: `CkTests/Script/CkUsf/`.
