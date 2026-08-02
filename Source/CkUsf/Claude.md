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
- **Niagara sprite looks need BOTH the usage flag and the inputs.** `_UsedWithNiagaraSprites` alone
  gets you a master a sprite renderer will accept; without it the renderer silently falls back to the
  DEFAULT material in a packaged build. The particle data is separate opt-in (below).

## Niagara sprite contract (opt-in)

A look that renders on a Niagara **sprite** renderer declares it on the LookDefinition. Every field
defaults off, so looks that predate the contract regenerate byte-identically — the
`NiagaraSpriteContract` test asserts exactly that negative.

| Field | Effect |
|---|---|
| `_UsedWithNiagaraSprites` | bakes `bUsedWithNiagaraSprites` (engine `Material.h:721`) onto the generated master |
| `_UsedWithNiagaraMeshParticles` | bakes `bUsedWithNiagaraMeshParticles` — the same contract for a MESH-particle renderer |
| `_ParticleColor` | wires `UMaterialExpressionParticleColor` → `In.ParticleColor` (float4) |
| `_ParticleDynamicParameter` | wires `UMaterialExpressionDynamicParameter` (index 0) → `In.DynamicParameter` (float4) |
| `_ParticleDynamicParameterNames` | up to 4 channel names, for readability in the generated master |

Both inputs are **Surface-domain only** — the generator wires them nowhere else, and the validator
errors rather than leaving them silently inert. Reading them on a non-particle mesh is safe:
`In.ParticleColor` defaults to opaque white and `In.DynamicParameter` to zero.

Sprite and mesh-particle usages exist and are INDEPENDENT: the flag decides which renderer accepts the
master, not what the shader may read, so a look reading `ParticleColor` needs whichever one matches its
renderer. Ribbon usage is still deliberately absent — add it when a look actually needs it.

Three engine facts the generator depends on (verified against the checked-out 5.7 source):

- `UMaterialExpressionParticleColor`'s output 0 is **`RGB` — a float3**, with alpha as a separate `A` output
  (`MaterialExpressions.cpp:9869`). The generator therefore wires TWO pins (`ParticleColor` + `ParticleAlpha`)
  and assembles the float4 in generated code. Connecting the node's default output instead yields
  `cannot convert from 'float3' to 'float4'` inside generated `Material.ush` — a message that names no look,
  so trace it back through the `SHADER FAILED TO COMPILE` line that does.

- `UMaterialExpressionDynamicParameter` exposes **four SCALAR outputs**, not a float4. The generator
  therefore declares four `DynParam0..3` pins and assembles the float4 in generated code, rather than
  building an `AppendVector` chain (which has failed to compile under SM6 in this codebase).
- Those outputs are named from `ParamNames` only inside `GetOutputs()`
  (`MaterialExpressions.cpp:9965`). `ConnectMaterialExpressions` matches the RAW `Outputs` array, so
  **`GetOutputs()` must be called after setting `ParamNames`** or every by-name connect silently
  no-ops — a failure that produces a working-looking material with dead inputs.

Consumer examples: CkParticles behavior 17 (`RingDissolveAdd`, sprite) — recipe in
`CkParticles/Cookbook/NS_Lightning_Range.md` — and behavior 7's five `DissolveAdd` looks (four mesh-particle
+ one sprite) in `CkParticles/Cookbook/NS_BasicAttack.md`.

**Three Vefects material FAMILIES live here, and each is ONE shader with N parameterizations** — the source
ships one parent graph and one material instance per emitter, so a look is a set of parameter defaults, never
a second copy of the math:

| Family | Shader | Look assets | Notes |
|---|---|---|---|
| `DissolveAdd` | `/CkUsf/Looks/DissolveAdd.ush` | `Script/CkUsf/CkUsf_SlashLooks_Assets.as` (the family helper `Usf_DissolveAddParams` + the NS_BasicAttack/projectile looks), `CkUsf_HitLooks_Assets.as` (the hit/impact looks) and `CkUsf_CastLooks_Assets.as` (the cast/spawn looks) | 23 looks. Additive-reading unlit translucency: a shape mask tinted by Particle Color, eroded by a panning noise whose threshold rides the Niagara dynamic parameter |
| `FlatAdd` | `/CkUsf/Looks/FlatAdd.ush` | `Script/CkUsf/CkUsf_FlatAddLooks_Assets.as` | `ParticleColor × Brightness` and nothing else. **Naming trap:** its one source instance is called `M_VFX_DisAdd_Flat02` but its parent is `M_VFX_FlatAdd`, NOT `M_VFX_DissolveAdd` |
| `ToonBand` | `/CkUsf/Looks/ToonBand.ush` | `Script/CkUsf/CkUsf_ToonLooks_Assets.as` | Stylized banded shading for the cookbook's prop stand-ins |

A family's parameter helper states the list in the ORDER the look declares it, because the validator enforces
that order positionally — so the helper is the single place it is stated, and a new parameter goes at the END
with a default equal to what every existing look already resolved. That is the only way to extend a family
without regenerating every look that predates the change into something different.

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
