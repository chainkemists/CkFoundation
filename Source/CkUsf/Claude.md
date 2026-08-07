# CkUsf

**Purpose:** Author materials as plain HLSL functions instead of material-editor node graphs. A
*look* = one `.ush` entry point + one `UCkUsf_LookDefinition` data asset; the editor-side generator
(CkUsfEditor) assembles a master `UMaterial` around a Custom node, validates the asset↔HLSL
contract, force-compiles the shaders, and saves it under `/CkFoundation/CkUsf/GeneratedLooks/`.
Runtime code applies looks via `UCk_Utils_Usf_UE` (master lookup, MID creation, post-process
attach). Also home to the Shadertoy-style multi-pass renderer, the Custom-Stencil outline subsystem,
and the four-effect **Stylize** suite (HandDrawn / CelShade / ScreenDither / CrossHatch) built on top
of all of it.

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
- `UCkUsf_ScreenDitherSubsystem` (`Stylize/`) — per-world screen dithering / palette reduction:
  `Request_SetEnabled` / `Get_IsEnabled`, `Apply_Preset`, `Request_SetSettings` / `Get_Settings`,
  `Request_ResetToDefaults`. Settings are `FCk_Usf_ScreenDither_Params`; presets are
  `UCkUsf_ScreenDitherPreset` data assets (AS: `Script/CkUsf/CkUsf_ScreenDitherPresets_Assets.as`).
- `UCkUsf_CelShadeSubsystem` (`Stylize/`) — per-world cel shading: same surface as ScreenDither
  (`Request_SetEnabled` / `Get_IsEnabled`, `Apply_Preset`, `Request_SetSettings` / `Get_Settings`,
  `Request_ResetToDefaults`) plus the Custom-Stencil contract accessors `Get_StencilValueFor`,
  `Get_StencilSuppressValue`, `Get_StencilRangeIsFree`. Settings are `FCk_Usf_CelShade_Params`;
  presets are `UCkUsf_CelShadePreset` data assets (AS: `Script/CkUsf/CkUsf_CelShadePresets_Assets.as`).
- `UCkUsf_HandDrawnSubsystem` (`Stylize/`) — per-world hand-drawn illustration: the same surface as
  ScreenDither (`Request_SetEnabled` / `Get_IsEnabled`, `Apply_Preset`, `Request_SetSettings` /
  `Get_Settings`, `Request_ResetToDefaults`) and nothing else — the feature is strictly full-screen, so
  there is no stencil contract and no entity API. Settings are `FCk_Usf_HandDrawn_Params`; presets are
  `UCkUsf_HandDrawnPreset` data assets (AS: `Script/CkUsf/CkUsf_HandDrawnPresets_Assets.as`).
- `UCkUsf_CrossHatchSubsystem` (`Stylize/`) — per-world normal-aligned hatching: the same surface as
  ScreenDither (`Request_SetEnabled` / `Get_IsEnabled`, `Apply_Preset`, `Request_SetSettings` /
  `Get_Settings`, `Request_ResetToDefaults`). Settings are `FCk_Usf_CrossHatch_Params`; presets are
  `UCkUsf_CrossHatchPreset` data assets (AS: `Script/CkUsf/CkUsf_CrossHatchPresets_Assets.as`).
- `UCk_Utils_Usf_StylizeMask_UE` (`Stylize/CkUsf_StylizeMask_Utils.h`) — ENTITY-level membership of the
  effect mask (`Request_AddToStylizeMask(Handle, Scope)` / `Request_RemoveFromStylizeMask` /
  `Has_StylizeMask`), plus the range validators every effect's `Request_SetSettings` delegates to
  (`Get_MaskRangeIsAddressable`, `Get_MaskRangeAvoidsOutline`, `Get_MaskRangeAvoidsCelSpan` /
  `Get_MaskRangeAvoidsCelPatterns`, `Get_MaskRangeIsFree`). Reuses `ECk_Usf_OutlineScope`.
- `UCk_Utils_Usf_CelPattern_UE` (`Stylize/CkUsf_CelPattern_Utils.h`) — ENTITY-level cel patterns:
  `Request_SetCelPattern(Handle, Pattern, Scope)` / `Request_ClearCelPattern`. Reuses
  `ECk_Usf_OutlineScope`. Per-renderer sync processors apply it, the same distribution entity outlines use
  (actor path here; shadow ISM in CkIsmRenderer; SKMC custom depth in CkIskmRenderer Plan-1; batched Plan-2
  members are not entities — use `UCk_Utils_IskmBatched_UE::Set_CrowdMemberCelPattern`).
- `UCk_Utils_Usf_Outline_UE` (`Outline/CkUsf_Outline_Utils.h`) — ENTITY-level outlines:
  `Request_ApplyOutline(Handle, Preset, Scope)` / `Request_RemoveOutline`. Per-renderer sync processors
  apply it (actor path here; shadow ISM in CkIsmRenderer; SKMC custom depth in CkIskmRenderer Plan-1;
  batched Plan-2 members are not entities — use `UCk_Utils_IskmBatched_UE::Set_CrowdMemberOutline`).
  Design + mechanisms: the *Entity outlines* section below. (A `DESIGN_EntityOutlines.md` that never existed
  was cited by eleven files; all now point here instead.)
- `UCk_Usf_Stylize_ProjectSettings_UE` (`Stylize/CkUsf_Stylize_ProjectSettings.h`) — one optional
  default-preset soft ref per effect, plus `_MaskStencilValue` (the single Custom-Stencil value the
  entity-level effect mask stamps, default 190). Unset = the effect stays off; set = that world subsystem applies it
  at `OnWorldBeginPlay` with no game code involved. It is a `UCk_Plugin_ProjectSettings_UE`, so it inherits
  the family's `Config = CkFoundation` and the "CkFoundation" settings category — the row lives in
  `DefaultCkFoundation.ini`, NOT `DefaultGame.ini`. Read it through `UCk_Utils_Usf_Stylize_Settings_UE`.
- `ck.Usf.{HandDrawn,CelShade,ScreenDither,CrossHatch}.{Enabled,Debug}` (`Stylize/CkUsf_Stylize_CVars.h`) — the
  developer overlay. `-1` on both is "settings decide"; `Enabled` `0`/`1` forces off/on; `Debug` takes the
  effect's DebugMode index.
- `/CkUsf/Common.ush` — the input/output structs + the shader stdlib (sampling, normals, parallax,
  triplanar, flow maps, SDFs, color ops, dithering).
- `/CkUsf/StylizeCommon.ush` — procedural pattern library for the stylize looks (Bayer/noise dither
  thresholds, the 10 cel halftone patterns, the 4 hand-drawn stroke patterns, palette quantization).
  The dispatcher index orders are STENCIL CONTRACTS shared with the asset-side enums — never reorder.
- Generation (editor): `ck::usf_editor::Generate_AllLookMaterials()` / `Generate_LookMaterial(Def)`
  (`CkUsfEditor/Generator/CkUsf_Generator.h`); validation in `CkUsf_LookValidator.h`. Both take an
  optional package-root override — TESTS ONLY, so parallel automation lanes generate into lane-unique
  paths instead of colliding on the shipped `GeneratedLooks/` packages; omitted, the editor-facing
  output is unchanged.

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
| `_UsedWithNiagaraRibbons` | bakes `bUsedWithNiagaraRibbons` (engine `Material.h:724`) — the same contract for a RIBBON renderer |
| `_ParticleColor` | wires `UMaterialExpressionParticleColor` → `In.ParticleColor` (float4) |
| `_ParticleDynamicParameter` | wires `UMaterialExpressionDynamicParameter` (index 0) → `In.DynamicParameter` (float4) |
| `_ParticleDynamicParameterNames` | up to 4 channel names, for readability in the generated master |

Both inputs are **Surface-domain only** — the generator wires them nowhere else, and the validator
errors rather than leaving them silently inert. Reading them on a non-particle mesh is safe:
`In.ParticleColor` defaults to opaque white and `In.DynamicParameter` to zero.

The three usages are INDEPENDENT engine bits: the flag decides which renderer accepts the master, not what
the shader may read, so a look reading `ParticleColor` needs whichever one matches its renderer. A material
the source draws on two renderer classes across systems is therefore TWO generated masters, not one master
carrying both flags (`LightStripDisAdd` / `LightStripDisAddSprite` is the shipped instance of that).

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

**Four Vefects material FAMILIES live here, and each is ONE shader with N parameterizations** — the source
ships one parent graph and one material instance per emitter, so a look is a set of parameter defaults, never
a second copy of the math:

| Family | Shader | Look assets | Notes |
|---|---|---|---|
| `DissolveAdd` | `/CkUsf/Looks/DissolveAdd.ush` | `Script/CkUsf/CkUsf_SlashLooks_Assets.as` (the family helper `Usf_DissolveAddParams` + the NS_BasicAttack/projectile looks), `CkUsf_HitLooks_Assets.as` (the hit/impact looks), `CkUsf_CastLooks_Assets.as` (the cast/spawn looks), `CkUsf_LoopLooks_Assets.as` (the idle-loop looks), `CkUsf_TrailLooks_Assets.as` (the ribbon trails), `CkUsf_ExplosionLooks_Assets.as` (the scorch decal + the two bomb-shell paints) and `CkUsf_DashLooks_Assets.as` (the dash's speed cone) | 36 looks. Additive-reading unlit translucency: a shape mask tinted by Particle Color, eroded by a panning noise whose threshold rides the Niagara dynamic parameter |
| `FlatAdd` | `/CkUsf/Looks/FlatAdd.ush` | `Script/CkUsf/CkUsf_FlatAddLooks_Assets.as` | `ParticleColor × Brightness` and nothing else. **Naming trap:** its one source instance is called `M_VFX_DisAdd_Flat02` but its parent is `M_VFX_FlatAdd`, NOT `M_VFX_DissolveAdd` |
| `ToonBand` | `/CkUsf/Looks/ToonBand.ush` | `Script/CkUsf/CkUsf_ToonLooks_Assets.as` | Stylized banded shading for the cookbook's prop stand-ins |
| `FresnelBomb` | `/CkUsf/Looks/FresnelBomb.ush` | `Script/CkUsf/CkUsf_FresnelLooks_Assets.as` (helper `Usf_FresnelBombParams`) | 3 looks. The explosion shell: a Fresnel lerp between an interior and a grazing-angle colour, eroded by a panning noise. Reads the same dynamic-parameter channel names as DissolveAdd but samples ONE texture and has no shape/distortion/gradient chain |

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
- `_SceneTextures` also accepts the GBuffer reads `BaseColor` / `Metallic` / `Roughness` /
  `Specular` → `In.SceneBaseColor` / `In.SceneMetallic` / `In.SceneRoughness` / `In.SceneSpecular`
  (deferred only; forward/mobile reject them at translation). BaseColor/Specular are the
  shading-model-MODIFIED variants; the raw stored values (`PPI_StoredBaseColor`/`PPI_StoredSpecular`)
  are not wired. The GBuffer uniform buffer is bound at EVERY blendable location — pre-TAA placement
  is for temporal stability, not availability. NEVER add a `PPI_SceneColor` wiring row: the
  translator rejects it in the PostProcess domain (SceneColor stays `PPI_PostProcessInput0`).
- `_PostProcessWorldPosition` (opt-in, PostProcess-only) wires the engine WorldPosition expression,
  which in a PP material is the depth-reconstructed SCENE SURFACE position. At after-tonemap/SSR
  locations that reconstruction is dynamic-resolution scaled — intended for the pre-TAA locations.
  The regeneration negative (`StylizeSceneTextureNegative`) holds both extensions to the
  byte-identical default-path invariant.
- **Shader-compile gate:** `Validate_LookShaderCompile` (`CkUsf_Generator.h`) reads the material
  resource's REAL compile-error list; the bare shader-map null check was mutation-tested toothless
  (2026-08-06). Its `InForceSynchronousCompile` is what gives a verdict teeth and is DESTRUCTIVE
  (the forced master renders black) — generation passes `false` for the roster; only throwaway test
  masters force. Under `-nullrhi` every generation test skips as environmental — a green there says
  nothing about HLSL; real verdicts need a `--no-nullrhi` run (parallel-safe since the generation tests
  took the package-root override — they no longer write the shipped `GeneratedLooks/`).
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

### Stylize — the four-effect suite

Four PostProcess-domain looks, each with a per-world subsystem and data-asset presets:
**HandDrawn** (paint → ink → strokes → paper), **CelShade** (quantized bands, halftone transitions,
sky/metallic/specular/rim groups, its own outline, per-object pattern via Custom Stencil),
**ScreenDither** (post-tonemap palette reduction, ordered/noise dithering, pixelation) and
**CrossHatch** (normal-aligned engraving strokes, density stepped by darkness). They are ordinary
looks — the generator, validator, MID runtime and blendable placement are the shipped machinery, not
anything new. Every effect is view-wide except for its EFFECT MASK (below); CelShade additionally has a
per-object pattern surface.

**One value flows through four hands, and each hand can only narrow the previous one:**

```
UCk_Usf_Stylize_ProjectSettings_UE  (unset = off; applied once, at OnWorldBeginPlay; yields to game code)
        │  Apply_Preset
UCkUsf_<Effect>Preset             (an authored FCk_Usf_<Effect>_Params, AS- or editor-authored)
        │  Request_SetSettings / Request_SetEnabled  ← game code lives here
_Settings                         (the SOURCE OF TRUTH; survives a missing master)
        │  + ck.Usf.<Effect>.{Enabled,Debug}         ← developer overlay, never persisted
effective settings ──► MID        (changed fields only)
```

Consequences worth knowing before debugging one of these:

- **The stored settings are never overwritten by a CVar.** An override is folded in on the way to the MID,
  so `Get_Settings()` keeps reporting what the game asked for while the screen shows what the console asked
  for. Set the CVar back to `-1` and the settings value is what renders again, with no re-apply.
- **`Request_ResetToDefaults` re-reads the project settings**, not the params-struct defaults — it means
  "back to how this project ships", and only falls through to `FCk_Usf_<Effect>_Params{}` when no default
  preset is configured.
- **The effect is created lazily, on first use.** The params default to `Enabled`, so a subsystem that
  nothing has touched reports Enabled while rendering nothing at all. That is why the CVar re-sync refuses
  to instantiate an effect it did not already find alive unless `Enabled` is explicitly forced to `1` — and
  why a gym toggle must track its own state rather than read `Get_IsEnabled()`.
- **The project default lands at `OnWorldBeginPlay`, not `Initialize`.** Subsystem `Initialize` runs inside
  `UWorld::InitializeSubsystems`, too early to spawn the view actor; and a packaged game runs its startup
  map's BeginPlay from inside `FEngineLoop::Init`, *before* `OnFEngineLoopInitComplete` flips
  `GIsEngineSafeForBlockingLoads`. So the soft-ref resolve is gated on
  `UCk_Utils_IO_UE::Get_IsEngineSafeForBlockingLoads_Peek()` and otherwise defers onto that delegate — the
  CkCore deferred-config discipline, applied to a settings row instead of a data asset. The deferred retry
  deliberately does not re-test the flag: it is already at the safe point, and CkCore's own flag-setting
  registrar may run after us in the delegate's add-order.
- **A project default is a DEFAULT: game code that has already spoken wins, on both paths.** Any explicit
  `Request_SetSettings` / `Request_SetEnabled` latches the subsystem, and the apply checks that latch. This
  is not theoretical — on the deferred path a packaged game's BeginPlay runs before the delegate fires, so
  without the latch the project row would land *after* gameplay had configured the effect and silently undo
  it. `Request_ResetToDefaults` is the deliberate way back to the project row.
- **None of the three subsystems is created on a dedicated server** (`ShouldCreateSubsystem` →
  `IsRunningDedicatedServer()`), so a configured default preset cannot make a headless server load a
  master, build a MID and spawn a post-process actor per world. `UCkUsf_OutlineSubsystem` carries the same
  guard (2026-08-07) for the same reason — nothing should build its params LUT and view machinery headless.
  Callers must already tolerate a null subsystem — `Get_*Subsystem` returns null for an unresolvable world
  context anyway. Known granularity gap, shared with `UCk_LoadingScreen_Subsystem_UE`: a PIE
  dedicated-server world lives in the editor process and still gets one.

**The Custom-Stencil byte is shared, and the THREE claims on it are disjoint by construction.**
`UCkUsf_OutlineSubsystem` *allocates* refcounted values from the top of the range (240–255);
`UCkUsf_CelShadeSubsystem` uses *direct* values, `[StencilBase-1, StencilBase+9]`, default 199–209; the
**effect mask** claims a per-effect *range*, which the entity API stamps with the single project value
`UCk_Usf_Stylize_ProjectSettings_UE::_MaskStencilValue` (default 190, below both). Every effect's
`Request_SetSettings` rejects a mask range that intersects the outline range, intersects the cel span, is
inverted, or reaches stencil 0 — each with its own diagnostic. Every feature's undo *disables* custom
depth rather than restoring prior state — a mesh hand-authored to render custom depth does not get that
back.

**Precedence is outline > cel pattern > effect mask, and the asymmetry is deliberate.** An entity carries
at most one claim. A claim REFUSES loudly when a higher-precedence one is already present (upward:
`Request_SetCelPattern` refuses an outlined entity, `Request_AddToStylizeMask` refuses an outlined OR
patterned one), and is silently taken over when a higher-precedence one arrives afterwards (downward: the
sync processors exclude their betters, and the drop processors retire the now-false applied-state so the
lower claim reappears once the higher one is removed). Refusing upward keeps a caller from destroying a
silhouette it asked for earlier; yielding downward lets the newer, more specific request win without a
two-step dance. A *cascade* SKIPS a dependent a higher claim owns with a Verbose log rather than failing
whole, since the caller asked about the root.

**One saturation policy for all three effects, and the shader half is the one that protects anything.**
`CkUsf_Stylize_ApplySaturation` (`StylizeCommon.ush`) floors its output at zero, and that floor is what
makes the parameter safe: values above 1 drive any channel sitting below the pixel's luminance NEGATIVE,
and CelShade and HandDrawn emit into scene-referred linear where nothing clips it before the tonemapper.
Shipped presets do reach 1.15. The floor is inert at `Saturation <= 1`.
The `ClampMax = 2` on the reflected `_Saturation` fields (matching the `UIMax` they already advertised) is
a details-panel affordance ONLY — it bounds what a designer can type, it does not make the value safe, and
it constrains nothing reached from C++, Blueprint or AngelScript, all of which write the struct directly.
Never treat it as the guard.

**Limitations that are design, not defects** — each of these is a documented consequence of a material
blendable being the vehicle:

| Limitation | Why | The alternative |
|---|---|---|
| No temporal stabilization: world-space cel patterns and world-attached strokes SLIDE on translating and skinned meshes | Correcting it needs a frame history a blendable does not have; the source feature's own docs warn of trail artifacts | `PatternSpace = Screen` / `StrokeSpace = ScreenStable` |
| Cel is DEFERRED-ONLY | The illumination reconstruction reads GBuffer BaseColor/Metallic/Roughness, which the translator rejects under forward and mobile | none — the feature is off on those paths |
| Cel illumination is an APPROXIMATION (`SceneColor / max(BaseColor, eps)`) | A blendable cannot see deferred lighting. The quotient is incoming light for a diffuse dielectric and nothing useful elsewhere — hence the Metallic / `AffectUnlit` / Sky exception groups, which are not tuning knobs | `_QuantizeFinalColor` bands SceneColor luminance directly |
| World-space patterns and strokes quantize, then stop moving, past ~1e6 uu from the origin | `In.WorldPosition` is float32 | screen space on far-from-origin levels |
| Paper grain is measured in OUTPUT pixels, so its density changes with output resolution | It is a property of the sheet, not of the scene | tune `_GrainScale` at target resolution |
| Stacking: any pre-TAA look composes with dither; two pre-TAA looks do not compose with each other | Dither sits post-tonemap and reads disjoint inputs. Hand-drawn, cel and cross-hatch all restyle the whole frame at the same location, so the second paints over the first | A/B them from the gyms' `Toggle*Stack` Execs; or give two of them disjoint pixels with the effect MASK |
| An effect-mask edge is ONE pixel wide and never fwidth-smoothed | Stencil is a nominal id — its derivative measures nothing about how far a pixel sits from the range | pre-TAA looks get a temporally resolved edge for free; ScreenDither softens over its 4 axis neighbours |

### Effect mask (Stylize) — one block, four effects

`FCk_Usf_StylizeMask_Params` (`Stylize/CkUsf_StylizeMask_Params.h`) is carried identically by all four
params structs: a mode (`Off` / `IncludeStencilRange` / `ExcludeStencilRange`) and an inclusive
Custom-Stencil range. One shared block rather than four near-identical field groups because the stencil
byte is ONE resource and the rule that keeps its claims disjoint has to be stated once (NN#9).

**Every look applies it as its LAST composite step**, one lerp against the untouched frame
(`CkUsf_Stylize_MaskWeight`, `StylizeCommon.ush`). A masked-out pixel is therefore the original scene
colour EXACTLY, not an approximation of it, and masked-out costs the same as masked-in. CelShade puts the
lerp after its own ink outline so the whole look — bands, pattern, specular, rim, outline — sits inside
what the mask gates.

**The mask is deliberately NOT fwidth-anti-aliased, unlike every other threshold in these looks.** Stencil
is a NOMINAL id: its derivative spikes at every id boundary regardless of how far the pixel sits from the
range, so a footprint built from it would widen the mask by an amount unrelated to the test. Mask edges
are one pixel wide by construction, and their quality is a property of WHERE the look sits in the chain:
- **CelShade / HandDrawn / CrossHatch** are pre-TAA, so the edge is temporally resolved like any other
  geometry edge. HandDrawn's placement was previously a preference (the WorldPosition reconstruction) and
  is now a REQUIREMENT for the same reason CelShade's always was.
- **ScreenDither** sits at `AfterTonemapping` while custom depth is rendered with the TAA-jittered
  projection, so its mask edge is never resolved and a hard test visibly crawls on a stationary camera. It
  averages the centre + 4 axis neighbours into a 5-level ramp instead (`CkUsf_ScreenDither_MaskWeight`,
  short-circuited to a single compare when the mask is Off). That is a genuine soft edge, not a fix —
  sub-pixel crawl inside the ramp remains. A mask that must be pixel-crisp belongs on one of the pre-TAA
  looks; moving ScreenDither pre-TAA is NOT the alternative, since it would defeat the reason it is placed
  where it is. Membership is what is averaged, never the stencil VALUE — averaging ids would invent ids
  nothing wrote.

**Entity API and its per-renderer distribution** mirror the cel pattern's exactly, with one simplification:
there is no per-entity payload (one project-wide value, so membership IS the payload), hence
`ck::FFragment_Usf_StylizeMaskTarget` carries only the cascade flag and the sync processors read
`UCk_Utils_Usf_Stylize_Settings_UE::Get_MaskStencilValue()` instead of consulting a subsystem. Sync
processors: `FProcessor_Usf_StylizeMaskActor_*` here, `FProcessor_IsmProxy_StylizeMask_*`
(`CkIsmRenderer/Proxy/CkIsmProxy_StylizeMaskProcessor.h`) and `FProcessor_IskmProxy_StylizeMask_*`
(`CkIskmRenderer/Proxy/CkIskmProxy_StylizeMaskProcessor.h`); batched Plan-2 members are not entities —
use `UCk_Utils_IskmBatched_UE::Set_CrowdMemberStylizeMask` / `Clear_CrowdMemberStylizeMask`. The
drop-on-higher-claim processor is TWO processors per renderer (one keyed on the outline target, one on the
cel-pattern target), because a processor view is a conjunction and "a higher claim arrived" is a
disjunction; both bodies are the same idempotent `Try_Remove`.

**Changing `_MaskStencilValue` does NOT retune the effects.** Each effect's own mask RANGE decides whether
the stamped value is inside the mask, and the shipped params default that range to `[190, 190]`. Move the
project value and every effect's range has to move with it, or the mask silently stops biting. The gym
and the `StylizeMaskRangeValidation` test both read the project value rather than restating 190, so a
project that relocates the reservation keeps both honest.

### Screen dither (Stylize)

`/CkUsf/Looks/ScreenDither.ush` + `UCkUsf_ScreenDitherSubsystem` (`Source/CkUsf/Public/CkUsf/Stylize/`).
Placed at `AfterTonemapping` on purpose: palette reduction has to see the final display-referred frame,
or the authored step count means nothing (quantizing scene-referred HDR bands wherever the tonemapper
is steep). Nothing here reads Custom Depth/Stencil, so the pre-TAA rule above does not apply.

**Pipeline order is the contract.** pixelate → shape (saturation/contrast/monochrome) → encode
(colour space + PreGamma) → dither → quantize → decode → weight-lerp against the original. The
threshold must offset the value *before* quantization; applied after, the result is plain banding with
a noise overlay, which is exactly the symptom the gym's FourColorHandheld station is there to catch.

**Three index contracts** — the subsystem writes each enum's integer value straight into a scalar
parameter, so reordering an enum silently re-maps the look:

| Enum | Consumer |
|---|---|
| `ECk_Usf_DitherPattern` | `CkUsf_Stylize_DitherThreshold`'s dispatcher in `StylizeCommon.ush` |
| `ECk_Usf_PaletteMode`, `ECk_Usf_DitherColorSpace` | branches in `ScreenDither.ush` |
| `ECk_Usf_ScreenDither_DebugMode` | the `DebugMode` dispatcher at the end of `ScreenDither.ush` |

**The custom palette is a FIXED 8 vector parameters + a count**, not an array — a material has no
array parameters. Entries past 8 are dropped and unused slots are written black, so a shrunk palette
cannot leave a stale colour behind for the nearest-entry search to find. The shader ENCODES each entry
before the nearest search, because entries are authored in output space while the search runs in encode
space — matching raw entries would pick the wrong one and hand Decode a value it never encoded, shifting
the author's colour by PreGamma. An empty palette in CustomPalette mode is REJECTED (ensure + no
mutation): every pixel would otherwise snap to the black in the unused slots and the view would go black
with nothing naming the cause.

**Settings are the source of truth; the MID is a projection.** `Request_SetSettings` stores and only
then syncs, so Get/Set round-trips whether or not the generated master exists (a fresh checkout warns
ONCE per world and renders nothing). Only fields that changed are written to the MID — of the EFFECTIVE
value (settings + any `ck.Usf.ScreenDither.*` override), so an override that changes nothing writes nothing.
`Request_SetEnabled` toggles the post-process component's `bEnabled`, which is why the "Off" preset must
restore the frame losslessly. `Get_ScreenDitherSubsystem` returns null for an unresolvable world context
rather than ensuring — the `UCkUsf_OutlineSubsystem` precedent, so a call during world teardown is not a
diagnostic.

**Everything lives in PostProcessInput0's OWN viewport space, not the scene buffer's** — the taps via
`ViewportUVToSceneTextureUV(uv, PPI_PostProcessInput0)` + `ClampSceneTextureUV`, and the block grid via
`GetSceneTextureViewSize(PPI_PostProcessInput0)`. The house `CkUsf_ViewportUVToBufferUV` /
`View.ViewSizeAndInvSize` pair maps to the SCENE buffer, which is right for a depth/normal tap
(EdgeOutline) and wrong for PostProcessInput0 at AfterTonemapping, where the tonemapped input carries
its own rect and size. Mixing the two is not a cosmetic slip: `BlockIndex` doubles as the dither
pattern's pixel position, so a grid in one space and samples in the other desyncs the pattern from the
blocks it is meant to threshold. `PixelScale` is therefore measured in input-viewport pixels — display
resolution at this placement. `_StabilizeGrid` rounds the block to a whole number of them; a fractional
block size gives every block a different sub-pixel phase and crawls as the viewport resizes.

Presets ship in `Script/CkUsf/CkUsf_ScreenDitherPresets_Assets.as` (Balanced, SubtleColor, RetroPixel,
FourColorHandheld, AnimatedGrain, Off). Gym: "Stylize: Screen Dither" (CkTests) — stations are preset
SELECTORS over one shared judge scene, because the effect is view-wide.

### Cel shade (Stylize)

`/CkUsf/Looks/CelShade.ush` + `UCkUsf_CelShadeSubsystem` (`Source/CkUsf/Public/CkUsf/Stylize/`). Placed at
`SceneColorAfterDOF` because it reads Custom Stencil (pre-TAA rule above); consequence — its input and
output are scene-referred LINEAR, so every tint and threshold is authored in that space, not in display
values like ScreenDither's.

**Illumination is RECONSTRUCTED, not read.** A material blendable cannot see deferred lighting, so the lit
term is `SceneColor / max(BaseColor, MinimumAlbedo)`. That quotient is the incoming light for a diffuse
dielectric and nothing useful anywhere else, which is exactly why the three exception groups exist rather
than being tuning knobs: metals get their light from reflections (Metallic group), unlit/emissive pixels
have no diffuse response (`AffectUnlit`), and the sky has no GBuffer albedo at all (Sky group, which bands
SceneColor luminance directly). `QuantizeFinalColor` switches the WHOLE look to banding SceneColor
luminance — the documented fallback if the reconstruction ever reads as albedo-driven rather than
light-driven. The gradient wall in the gym's judge scene is the test: band boundaries must run straight
across it, because the light there is uniform and only the albedo varies.

**Midpoint is the exposure anchor, not a taste knob.** Pre-tonemap illumination is unbounded, so a fixed
0..1 band ramp would mean nothing. Midpoint is the illumination that lands mid-ramp — and `2*Midpoint` is
what the TOP band resolves to, so it sets band positions AND the brightness of the extreme bands. It is
NOT a neutral reparameterization: raising it darkens the lit side while spreading the bands. (Distribution
IS neutral — it is inverted exactly on the way out.) Its units are the scene's own pre-exposure linear
values at this chain location, so changing project exposure or world unit scale means retuning Midpoint,
not the band count. Tune it first; every other band control is relative to it.

**Band placement has two modes, and `Distribution` cannot express what the second one is for.**
`ECk_Usf_CelDistribution::Exponent` is the historical path and stays the default: N bands of EQUAL ramp
width, with `_Distribution` biasing where that spacing sits. One exponent moves every boundary together,
so it cannot produce "two narrow bands in the shadows and one wide one above" — pulling the first two down
drags the third with them. `CustomEdges` hands the boundaries over directly: `_BandEdges` lists them, the
bands are the intervals BETWEEN them (so N edges make N+1 bands and `_Bands` is ignored), and
`_Distribution` is forced to 1 in the shader because the edges already ARE the placement. The list is a
FIXED 8 shader scalars plus a live count — the ScreenDither palette precedent, since a material has no
array parameters — and entries past 8 are dropped, so `Get_EffectiveBandEdges()` is what both the
validation and the MID write read. Unused slots are written **1.0**, the only filler that cannot fabricate
a band if the count were ever wrong (the shader derives the band ordinal by COUNTING edges at or below the
ramp). `Request_SetSettings` rejects an empty, non-ascending, or out-of-`(0,1)` list with zero mutation:
a non-ascending list does not fail, it silently re-orders the bands, and an edge sitting on 0 or 1 fences
off a zero-width band no pixel can reach. Shipped demonstration: `DA_Cel_DramaticBands`.

**CustomEdges overrides `_SkyBands` and `_MetallicBands` too** — the edge list is the placement for EVERY
group, so those two counts stop having any effect in that mode. Deliberate, and stated because silence
here reads as a bug: a per-group edge list would be three more 8-slot parameter blocks on an already
84-input node, to express something no shipped preset asks for. Authored placement is a property of the
LOOK, not of the group; the groups keep their own strengths and pattern strengths, which is what actually
distinguishes them.

**Strength governs bands and pattern only.** The outline, stepped specular and rim are separate features
with their own opacity/intensity controls (this look inherits per-group strengths rather than one global
style weight), so `Strength = 0` still draws ink lines. The whole-look passthrough is the subsystem's
`Enabled`, not `Strength`.

**The halftone IS the quantizer.** The within-band fraction is compared against the pattern threshold, so
a band transition renders as a growing dot/line field rather than a hard edge. `BandSoftness` is the
pattern-free alternative for the same boundary, which is why the two are BLENDED by pattern strength
instead of added — a look cannot be both hard-stepped and dithered at one boundary.

**That comparison is anti-aliased against the pattern's own screen footprint, and it has to be.** A bare
threshold makes every dot edge a binary one-pixel boundary, which is what produced the two symptoms
reported from PIE on 2026-08-07: moire/crawling dots on a STATIC wall, because the world lattice is built
from `In.WorldPosition` — depth-reconstructed under the TAA-JITTERED projection, so the "world-locked"
pattern re-anchors by a sub-pixel of world offset every frame and a hard step flips the boundary pixels
with it; and speckle on distant geometry, because past `PatternOctaveMax` the cell keeps its world size and
shrinks on screen without bound. The width comes from the pattern COORDINATE — smooth, being world position
over cell size — never from the threshold VALUE, which is periodic and would blow the width up at every
cell seam. `CKUSF_CEL_HALFTONE_SLOPE` converts a footprint in cells into one in threshold units and
`PatternContrast` scales it, because contrast is exactly what steepens the primitive. Past a footprint of
~1 the comparison resolves to its average, the correct limit for a pattern below pixel size.
Distance scaling was NOT the cause and needed no change: `_PatternDistanceScaling` already defaults to 1.0
and no shipped preset overrides it, so the octave mechanism holds a cell between roughly 15 and 31 screen
pixels over 500–8000 uu, and the authored `PatternWorldSize` values (8–24 uu) are not too small for the
judge-scene distances either.

**Every outline edge test is anti-aliased the same way** (`CkUsf_CelShade_EdgeMask`), for the same reason
one level down: a MARGINAL edge — an armour crease whose normal difference sits right at
`OutlineNormalThreshold` is the canonical one — flips per pixel under a hard `step` and draws as a DOTTED
line instead of as a line or as nothing. Smoothing across the measure's own footprint keeps each preset's
authored threshold meaning exactly (it is still the 50% crossing), which is why no threshold needed
retuning. Two things deliberately NOT done: replacing the normal detector's max-of-gradients with the depth
channel's Laplacian (more selective on curved surfaces, recorded as a follow-up — but the two measures are
on different scales, so swapping it silently re-tunes every preset's `OutlineNormalThreshold`); and adding
a slope-scaled bias from `fwidth(Normal)`, which is wrong on the math — that derivative spikes AT the
crease, so it would raise the threshold exactly where the line is wanted.

**Per-object stencil is a DIRECT-VALUE contract, not an allocation** (unlike `UCkUsf_OutlineSubsystem`'s
refcounted slots): `StencilBase - 1` suppresses transitions on that mesh, `StencilBase + N` forces
`ECk_Usf_CelPattern` N, anything else takes the global pattern. The span is therefore
`[base-1, base+9]` (default 199–209) and requires `r.CustomDepth 3` plus `RenderCustomDepth` on the mesh.
`Request_SetSettings` REJECTS a span on two grounds, each with its own diagnostic: it must not intersect
the outline subsystem's range (both features write the same byte, so an overlap restyles the other's
meshes with nothing naming the cause), and it must not reach Custom Stencil **0** — 0 is what the renderer
leaves for every mesh that wrote nothing, so base 1 would make suppression the view-wide default and base 0
would force pattern 0 on every pixel. Hence `ClampMin = 2` on `_StencilBase`.

**Per-renderer sync mirrors the outline distribution, with two deliberate deltas.** Each renderer module owns
a sync processor over (`ck::FFragment_Usf_CelPatternTarget` + its own proxy fragment) and records what it
applied in a module-local `...CelPatternApplied` fragment: `FProcessor_Usf_CelPatternActor_*` here,
`FProcessor_IsmProxy_CelPattern_*` (`CkIsmRenderer/Proxy/CkIsmProxy_CelPatternProcessor.h`) and
`FProcessor_IskmProxy_CelPattern_*` (`CkIskmRenderer/Proxy/CkIskmProxy_CelPatternProcessor.h`). The deltas
against the outline twins:
- **Nothing is allocated or released.** The cel contract is a direct stencil value, so there is no
  `Get_OrAllocate_StencilFor` / `Release_StencilFor` half and no strong preset ref keeping a refcount key
  alive. The ISM shadow component is therefore keyed on the stencil VALUE, not a preset — two patterns on one
  renderer are two shadow ISMs (`UCk_IsmRenderer_Subsystem_UE::FindOrCreate_CelPatternIsmComponent`).
- **The drop-on-outline processor means different things per renderer, and both are forced by the mechanism.**
  Where the two features share ONE primitive's stencil byte (actor path, ISKM SKMCs) the drop must NOT clear
  the flags — the outline's own Sync overwrites the byte in the same group, and clearing could blank its
  silhouette for a frame. Where the cel pattern owns its OWN component (ISM shadow, ISKM Plan-2 batched
  cluster) the drop must fully tear it down, or two custom-depth writers land on the same pixels with the
  winner undefined.

**The batched (ISKM Plan-2) path is member-indexed, not a processor.** Batched crowd members are
(crowd, index) pairs rather than entities, so there is no Target fragment and no sync processor: the
imperative `UCk_Utils_IskmBatched_UE::Set_CrowdMemberCelPattern` / `Clear_CrowdMemberCelPattern` reuse the
outline's highlight-cluster machinery verbatim — one custom-depth-only `UCk_Iskm_BatchedClusterComponent`
per (crowd, stencil value), holding the patterned members' mirrored `FInstance` data and pushed every
manager tick so the silhouette tracks the live skinned pose. Same two deltas as everywhere else, in their
member-indexed form: the group key is the resolved stencil VALUE rather than a preset and nothing is
allocated or released, and because the pattern owns its own cluster, an outline arriving on a patterned
member TEARS THAT CLUSTER MEMBERSHIP DOWN (`Set_MemberOutline` clears the member's pattern first). The
refusal runs the other way, matching `Request_SetCelPattern`: a pattern applied to an already-outlined
member is rejected loudly with zero mutation. Machinery and the world-space-bounds gotcha it inherits:
*Plan-2 production guide → Outline (highlight cluster)* in `CkIskmRenderer/Claude.md`.

**One entity, one Custom-Stencil value.** `Request_SetCelPattern` refuses an entity that already carries a
`ck::FFragment_Usf_OutlineTarget` (loud, zero mutation); a *cascade* skips such a dependent with a Verbose
log rather than failing whole, since the caller asked about the root. `FProcessor_Usf_CelPatternActor_Sync`
excludes outline targets so an outline applied afterwards wins, and
`FProcessor_Usf_CelPatternActor_DropAppliedOnOutline` drops the now-false applied-state — without it the
cache survives the outline's removal and the sync processor early-outs forever, silently losing the
pattern. The undo path only touches primitives still holding the value it wrote, so taking the byte over
never erases someone else's silhouette.

**Undo DISABLES custom depth; it does not restore prior state** — `UCkUsf_OutlineSubsystem::
Remove_Outline_From_Component`'s precedent verbatim, and the two must agree. Consequence shared by both
features: a mesh hand-authored to render custom depth does not get that back after a pattern (or an
outline) is applied and then cleared.

**Four index contracts** — the subsystem writes each enum's integer value straight into a scalar
parameter, so reordering an enum silently re-maps the look (and, for `ECk_Usf_CelPattern`, silently
restyles every stencil-tagged mesh in every level):

| Enum | Consumer |
|---|---|
| `ECk_Usf_CelPattern` | `CkUsf_Stylize_HalftonePattern`'s dispatcher in `StylizeCommon.ush` **and** the stencil mapping |
| `ECk_Usf_CelPatternSpace`, `ECk_Usf_CelOutlineQuality`, `ECk_Usf_CelOutlineBlend` | branches in `CelShade.ush` |
| `ECk_Usf_CelShade_DebugMode` | the `DebugMode` dispatcher at the end of `CelShade.ush` |

**Scene textures wired:** SceneColor, SceneDepth, SceneNormal, CustomStencil, BaseColor, Metallic,
Roughness. `PPI_Specular` is deliberately NOT wired — nothing reads it, and every input costs a pin plus a
GBuffer fetch on an already-wide (70-input) Custom node. Re-add it only when a tuning pass actually reads
it, and note `PPI_StoredSpecular` is the un-shading-model-modified variant if it ever matters.

**Documented limitation: world-space patterns SLIDE on translating and skinned meshes.** Correcting that
needs a frame history a blendable does not have; the source feature's own docs warn of trail artifacts.
A second world-space caveat: `In.WorldPosition` is float32, so pattern cells lose sub-cell precision past
roughly 1e6 uu from the origin and eventually stop moving — use `Screen` space on far-from-origin levels.
`ECk_Usf_CelShade_DebugMode::MotionOffset` renders BLACK to say so out loud instead of being quietly
missing, and `PatternSpace = Screen` is the stable alternative. The gym's translating mover exists to make
the limitation visible.

**Neighbour taps go through each scene texture's OWN viewport**
(`ViewportUVToSceneTextureUV(uv, PPI_X)` + `ClampSceneTextureUV`), not the house
`CkUsf_ViewportUVToBufferUV` — the same lesson ScreenDither carries. The rim light's view vector comes
from the engine's `ScreenVectorFromScreenRect` rather than differencing `In.WorldPosition` against a
camera origin: it is orthographic-safe and needs no large-world-coordinate arithmetic.

Presets ship in `Script/CkUsf/CkUsf_CelShadePresets_Assets.as` (Balanced, CleanAnime, ComicHalftone,
InkCrosshatch, SoftToon, Off). Gym: "Stylize: Cel Shade" (CkTests) — preset-selector stations over one
judge scene, plus two per-object rows (hand-tagged stencil cubes and entity-API subjects) that must look
identical to each other.

### Hand drawn (Stylize)

`/CkUsf/Looks/HandDrawn.ush` + `UCkUsf_HandDrawnSubsystem` (`Source/CkUsf/Public/CkUsf/Stylize/`). Placed
at `SceneColorAfterDOF`. It reads no Custom Depth/Stencil, so the pre-TAA *requirement* does not apply —
the placement is chosen for the OTHER pre-TAA property: at after-tonemap locations the WorldPosition
reconstruction is dynamic-resolution scaled, and the world-attached stroke lattice rides on it.
Consequence shared with CelShade: the input and output are scene-referred LINEAR.

**Pipeline order is the contract: paint → ink → strokes → paper.** Paint simplifies the regions the ink is
then drawn between; strokes go on top of painted regions because hatching is drawn over paint; paper is
LAST because it is the surface everything was drawn ON — applied earlier the posterizer would quantize the
grain into bands and the ink detectors would read the fibre as an edge.

**`StyleStrength` governs EVERYTHING** — one lerp over the whole composite, unlike CelShade's `Strength`
(which bands only and leaves the ink drawing at 0). A drawing is one medium: there is no meaningful state
where the ink is at full weight and the paper is absent. The whole-look passthrough is still the
subsystem's `Enabled`; `StyleStrength = 0` is the same picture by a slower route.

**Tone is normalized through Reinhard, not an exposure anchor.** Pre-tonemap light is unbounded, so a 0..1
posterizer applied to it would put every colour-region boundary in the darks. `L/(1+L)` supplies a monotone
0..1 domain with no knob to mistune — CelShade needs its `Midpoint` because it places band POSITIONS
against a scene's exposure; the paint here only needs somewhere to quantize and threshold. Quantization is
CELL-CENTRED (`(floor(F)+0.5)/N`) because the top level of an edge-aligned quantizer would be exactly 1.0,
whose Reinhard inverse is infinite.

**The highlight ceiling is the LEVEL COUNT, not the constant.** With `_SimplifyColor` on, the brightest
region resolves to `(N-0.5)/N`, so restored luminance can never exceed `2N-1` — **9.0** at StorybookInk's
5 levels, **3.0** at DarkGothic's 2. That is what actually keeps emissive props from blooming under the
look, and it falls out of the artist's level count: fewer colour regions means a harder highlight clamp.
`CKUSF_HANDDRAWN_MAX_TONE` (64) only bites on the OTHER path — with `_SimplifyColor` off there is no level
count to cap anything, and that constant is all that stands between `Contrast > 1` and an unbounded value.

**`AffectSky` gates the paint AND the strokes — but never the ink.** A silhouette against the sky is the
drawing's most important contour, so the sky keeps its horizon line while keeping its own colours. Paper
still covers it, because paper is the sheet the whole picture sits on, not a property of a subject in it.
Caveat: the ink DISTANCE FADE still applies out there — with a fade configured the sky sits past the end
distance and the horizon contour goes with it, so that line survives only while `_InkFadeEndDistance` is 0.

**`Enable in Editor Viewports` (the source feature's Master group) is deliberately dropped.** A per-world
subsystem plus `Request_SetEnabled` already expresses it: the editor-preview world is its own world with
its own subsystem instance, so a separate global toggle would be a second, weaker way to say the same
thing — and one that could disagree with the per-world state.

**Strokes are drawn in `InkColor`.** Hatching and contours are the same medium in a drawing, and a second
colour would let the two disagree about what the pencil is. (The source feature publishes no stroke colour
either — this is the clean-room reading of that absence, not an omission.)

**Three index contracts** — the subsystem writes each enum's integer value straight into a scalar
parameter, so reordering an enum silently re-draws the look:

| Enum | Consumer |
|---|---|
| `ECk_Usf_HandDrawnStrokePattern` | `CkUsf_Stylize_StrokePattern`'s dispatcher in `StylizeCommon.ush` |
| `ECk_Usf_HandDrawnStrokeSpace` | the stroke-space branch in `HandDrawn.ush` |
| `ECk_Usf_HandDrawn_DebugMode` | the `DebugMode` dispatcher at the end of `HandDrawn.ush` |

**Scene textures wired: the default trio only** (SceneColor / SceneDepth / SceneNormal), plus the
`_PostProcessWorldPosition` opt-in. Nothing here reconstructs illumination, so no GBuffer read is opted
into. The WorldPosition opt-in is load-bearing and silently degrades if dropped: `In.WorldPosition` would
read zero, every pixel would land in the same pattern cell, and the world-attached strokes would look
screen-locked with nothing failing — hence its own assertion in `HandDrawnGeneration`.

**Neighbour taps go through each scene texture's OWN viewport** (`ViewportUVToSceneTextureUV(uv, PPI_X)` +
`ClampSceneTextureUV`), and every pixel measurement — ink thickness, stroke pixel size, grain scale, the
line-variation wavelength — is in `GetSceneTextureViewSize(PPI_PostProcessInput0)`, not
`CkUsf_ViewportUVToBufferUV`. Same lesson ScreenDither and CelShade carry.

**Rejected loudly by `Request_SetSettings`: an inverted ink fade range.** With `_InkFadeEndDistance` at 0
the fade is off; any other value must be strictly greater than the start, or the fade's denominator changes
sign and the contour grows HEAVIER with distance — the opposite of what the setting names, with nothing in
the frame saying why. Zero-width counts as inverted.

**Documented limitations.** (a) World-attached strokes SLIDE on translating and skinned meshes — correcting
that needs a frame history a blendable does not have; `ScreenStable` is the alternative, and the gym's
mover exists to make the difference visible. (b) Paper grain is measured in OUTPUT pixels, so the same
scene at a different output resolution gets a different number of grains across it — the source feature's
docs carry the same warning; tune `_GrainScale` at target resolution. (c) `In.WorldPosition` arrives as
float32, so world-attached stroke cells lose sub-cell precision past roughly 1e6 uu from the origin —
strokes on far-from-origin geometry quantize and then stop moving. Same limit CelShade's world-space
patterns carry.

Presets ship in `Script/CkUsf/CkUsf_HandDrawnPresets_Assets.as` (StorybookInk, SoftPainted, BoldAnimation,
DarkGothic, PencilWash, Off) — the params-struct defaults ARE StorybookInk, so each preset reads as its
delta from it. Gym: "Stylize: Hand-Drawn" (CkTests) — a preset row over one judge scene, plus a debug row
that forces the ink / stroke / paper masks on.

### Cross hatch (Stylize)

`/CkUsf/Looks/CrossHatch.ush` + `UCkUsf_CrossHatchSubsystem` (`Source/CkUsf/Public/CkUsf/Stylize/`).
Placed at `SceneColorAfterDOF` because it reads Custom Stencil for the effect mask (pre-TAA rule above);
consequence shared with CelShade and HandDrawn — the input and output are scene-referred LINEAR, which is
the space `_PaperColor` is authored in. A paper value of 1.0 tonemaps to a mid grey, not to paper white,
which is why every shipped preset's sheet sits above 1.

**The DIRECTION is the whole point, and it is what a screen-space hatch cannot fake.** Classic NPR
hatching runs ACROSS the form — an engraver's lines curve around a cylinder because they follow its
surface. Projecting the scene normal to screen and rotating the stroke coordinate by its angle
reproduces that from a post-process pass, because the projected normal already carries the form's turning.
`_NormalAlignment` blends between the raw `_AngleOffset` (0 = a fixed screen hatch, the control that
proves the alignment does anything) and full form-following.

**`_UseWorldSpaceNormals` is an ARTISTIC choice, not a correctness one** — the maintainer asked for the
tickbox for exactly that reason. Off (default) transforms the normal into VIEW space first, so the hatch
direction is a property of how the surface faces the CAMERA and the lines keep wrapping a form under
orbit — the engraving behaviour. On uses `In.SceneNormal` raw, so the direction encodes WORLD orientation
and re-orients as the camera moves around a static object, which reads as the sheet turning rather than
the object: wrong for an engraving, right for a top-lit map or a technical drawing whose north is fixed.
The view-space transform goes through the engine's own `TransformWorldVectorToView` helper (engine
`Shaders/Private/Common.ush:1999`, verified 2026-08-07) rather than a hand-written multiply, because
`ResolvedView.TranslatedWorldToView` is the member that moves across engine versions and the helper is
what absorbs that.

**The direction blend happens on the VECTOR, not on the angle.** Blending angles walks into `atan2`'s
branch cut: the projected angle wraps from +pi to -pi across a silhouette, so at any intermediate
`_NormalAlignment` the blended angle sweeps the whole circle backwards along that seam and draws a band of
wrong-direction strokes. Interpolating the unit vectors and taking `atan2` once at the end has no seam.
Only 0 and 1 are safe under the naive form, which is exactly why the bug survives a preset walk.

**A normal facing exactly at or away from the camera projects to a zero-length 2D vector** and has no
direction to contribute — `atan2(0,0)` is undefined, and letting it through would make the centre of every
sphere and every head-on wall hatch at an angle that flickers with the normal's wobble. Those pixels fall
back to `_AngleOffset` alone. `ECk_Usf_CrossHatch_DebugMode::HatchDirection` puts the projected length in
blue precisely so the fallback region is visible rather than mysterious.

**Density is STEPPED, not continuous.** `_LayerCount` slices the darkness range — layer 0 covers all of
it, layer 1 starts halfway down, and so on, each rotated a further `_LayerAngleStep`. Darker pixels
accumulate CROSSING stroke fields rather than thicker strokes, which is how the medium works: an engraver
adds a second pass at an angle, not a fatter burin. The layers are unioned with `max`, never summed —
summing would double-darken every crossing point into a lattice of dots. `_StrokeThickness` below 1 is
what keeps the darkest regions showing paper between the strokes instead of filling to black.

**Darkness is normalized through Reinhard**, HandDrawn's approach and for its reason: pre-tonemap light is
unbounded, so a 0..1 ramp applied to it directly would put every layer boundary in the darks. There are no
band POSITIONS to place here (CelShade needs its `Midpoint` for that), only a monotone domain to threshold
in, and Reinhard supplies one with no knob to mistune.

**The stroke coverage test is fwidth-anti-aliased against the stroke COORDINATE**, never the threshold
VALUE — the coordinate's derivative is smooth while the threshold is periodic and would blow the width up
at every cell seam. Past a footprint of ~1 the comparison resolves to its average, the correct limit for a
stroke field below pixel size, which is what keeps distant geometry from moire-ing. The layer loop runs
its full compile-time bound with an ACTIVE weight rather than an early `continue`, so no derivative ever
sits inside control flow. `_LayerCount` selects how many of the four contribute.

**`_AffectSky` is a switch, not a threshold.** The sky has no form for the direction to follow, so its
strokes take whatever angle its normal happens to give — ungated it is the single most obviously wrong
thing this look can do.

**`ECk_Usf_HandDrawnStrokePattern` is REUSED, not duplicated.** It is the index contract of
`CkUsf_Stylize_StrokePattern`'s dispatcher, and a second enum over the same four primitives would be one
more thing to keep in sync. The name says HandDrawn because that feature declared it first.

**Three index contracts** — the subsystem writes each enum's integer value straight into a scalar
parameter, so reordering an enum silently re-draws the look:

| Enum | Consumer |
|---|---|
| `ECk_Usf_HandDrawnStrokePattern` | `CkUsf_Stylize_StrokePattern`'s dispatcher in `StylizeCommon.ush` |
| `ECk_Usf_CrossHatchBackground` | the background branch in `CrossHatch.ush` |
| `ECk_Usf_CrossHatch_DebugMode` | the `DebugMode` dispatcher at the end of `CrossHatch.ush` |

**Scene textures wired: the default trio + CustomStencil.** `SceneNormal` is the load-bearing one — the
hatch direction IS the projected normal, so dropping it would silently turn the look into a screen-space
texture, which is why `CrossHatchGeneration` asserts it by name. `_PostProcessWorldPosition` is
deliberately NOT opted into: the stroke lattice is screen-space by construction.

Presets ship in `Script/CkUsf/CkUsf_CrossHatchPresets_Assets.as` (Sketch, Engraving, Blueprint, Off) —
the params-struct defaults ARE Sketch, so each preset reads as its delta from it. Blueprint doubles as the
`_NormalAlignment = 0` control. Gym: "Stylize: Cross Hatch" (CkTests) — a preset row and a debug row over
one judge scene of CURVED forms (a box cannot show a wrapping hatch: one normal per face makes any
direction look correct), plus a mask row where hand-tagged cubes and entity-API subjects must be
indistinguishable.

### Stylize follow-ups

- **The effect mask has no AngelScript AutoTest coverage for its ISM / ISKM / batched paths.** The cel
  pattern has three (`CkAutoTest_UsfCelPattern_{IsmShadowInstances, IskmApplyRemove, BatchedMembers}`);
  the mask's renderer distribution is currently covered only by the C++ actor-path test
  (`StylizeMaskEntityStencilSync`) plus mechanical mimicry of the cel processors. Deliberately deferred
  rather than half-done: authoring AS AutoTests carries the deletion-churn hazard documented in
  `CkTests`, and three near-duplicate tests are worth one deliberate pass, not three rushed ones.

- **A cel pattern applied while `EnableStencilPatterns` is on survives the setting being turned off.** Every
  sync processor early-outs on `Get_StencilValueFor() == 0` rather than undoing, so the already-written
  stencil (or shadow instance) stays. Deliberate parity across all three renderer paths — the actor path has
  behaved this way since the feature shipped — but it means the switch is not a kill switch for meshes
  already patterned. `Request_ClearCelPattern` is. The batched member API is the one deliberate exception:
  `Set_CrowdMemberCelPattern` is a one-shot imperative call, not a per-frame processor, so a 0 there is
  refused LOUDLY at the call site rather than silently skipped — ensuring once per call is diagnosis, not spam.
- **Per-preset outline fill textures** (recorded earlier, still open): `_UseFillTexture` samples the
  subsystem's single shared texture; per-preset would need an atlas or array.

## See also

- `CkGraphics/Claude.md` (MID utilities), `CkIsmRenderer/Claude.md` (per-instance data writers).
- Tests: `CkTests/Source/CkTests/Private/UnitTests/CkUsf/` (generation, multi-pass, outline);
  gyms: `CkTests/Script/CkUsf/`.
