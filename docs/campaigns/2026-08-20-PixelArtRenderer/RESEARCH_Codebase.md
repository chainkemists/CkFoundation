# RESEARCH — codebase state: CkUsf, CkCamera, transform pipeline (2026-08-20)

> Two Explore sweeps over the plugin + engine fork, 2026-08-20. File:line references are that
> day's working tree (CkFoundation dev @ `96b5cad0e`). Siblings:
> [RESEARCH_Technique.md](RESEARCH_Technique.md), [RESEARCH_UeApis.md](RESEARCH_UeApis.md).

---

## 1. CkUsf — what exists, what the vehicle can and cannot do

### The house vehicle is a material post-process blendable

Every Stylize subsystem (CelShade, HandDrawn, CrossHatch, ScreenDither) and the OutlineSubsystem
share one shape — **not** a SceneViewExtension, **not** a global shader:

- `UWorldSubsystem`, dedicated-server-guarded, lazily creates a hidden transient actor
  (`CkUsf_CelShadeSubsystem.cpp:366-369`) with an unbound `UPostProcessComponent`
  (`:376-380`) carrying a MID of the generated look master
  (`/CkFoundation/CkUsf/GeneratedLooks/M_CkUsf_Look_<Name>`).
- Three-layer settings flow: project settings default preset → `Apply_Preset` / explicit
  `Request_SetSettings` (`_SettingsExplicitlySet` wins) → CVar overrides folded in on the way to
  the MID only (`DoGet_EffectiveSettings`, `.cpp:403-440`). Param transport = named MID
  scalar/vector writes, diffed against `TOptional<Params> _WrittenSettings`
  (`DoWrite_ChangedParams`, `.cpp:518-684`). Enums cross as integer index (reorder = silent
  remap). Fixed-width arrays fake material arrays (`BandEdge0..7` + count; `PaletteColor0..7`).
- Blendable location comes from the look asset (`_BlendableLocation`,
  `CkUsf_LookDefinition.h:71-87`, incl. `ReplacingTonemapper`); CelShade/HandDrawn/CrossHatch sit
  at `SceneColorAfterDOF` (pre-TAA), ScreenDither at `AfterTonemapping`.
- **Stacking limitation is design** (`Source/CkUsf/Claude.md:453`): two pre-TAA looks do not
  compose — the second repaints the whole frame. Disjoint pixels via the StylizeMask (stencil
  ranges) is the only supported dual-look mode. Stencil budget: outline 240-255 (refcounted
  allocator), cel span default 199-209, effect mask default 190 — precedence outline > cel
  pattern > effect mask.

### What already exists and is REUSABLE for pixel-art

- **Outline machinery** (three mechanisms):
  - `UCkUsf_OutlineSubsystem` — Custom Depth/Stencil selective outlines, params-LUT texture
    (16×2 FFloat16Color rows: color×brightness+type / fill+opacity), refcounted stencil
    allocation from 240-255 (`Get_OrAllocate_StencilFor` / `Release_StencilFor` are public for
    cross-module sharing, `CkUsf_OutlineSubsystem.h:83,90`), `SolidOutline.ush` 8-direction
    dilation, pre-TAA placement. Entity-level declarative API:
    `UCk_Utils_Usf_Outline_UE::Request_ApplyOutline(Handle, Preset, Scope)` +
    `ck::FFragment_Usf_OutlineTarget` + per-renderer sync processors (actor / Ism / Iskm).
  - Edge-detected outlines inside looks: `EdgeOutline.ush` (depth Laplacian + max normal angle,
    4 taps) and CelShade's ink line (`CkUsf_CelShade_EdgeMask` — thickness/quality/opacity/
    blend-mode/depth/normal/albedo thresholds/distance fade, fwidth-AA'd).
  - `StencilId.ush` — writes Custom Stencil as greyscale at `ReplacingTonemapper`: shipped
    precedent for a data-not-picture pass.
- **Quantization/palette library**: `StylizeCommon.ush` (389 lines — Bayer/noise dither
  dispatcher, 10 halftones, 4 stroke patterns, palette helpers); ScreenDither's
  `ECk_Usf_PaletteMode { ColorSteps, CustomPalette, LuminanceSteps }` + 8-color palette params +
  encode/decode around the quantizer; CelShade's band quantizer + `_QuantizeFinalColor`.
- **Pixelation prior art in-house**: ScreenDither step 1 is display-res block-snap
  (`ScreenDither.ush:146-176`: `BlockIndex = floor(GridPos/Scale)`, optional 4-tap box filter,
  dither threshold sampled at the block). This is a stylization at native res — NOT a pixel-art
  renderer (no low-res raster, no snap, no texel-stable lighting) — but proves the param plumbing.
- **Look pipeline** (LookDefinition → validator → generated master → MID → subsystem projection)
  and the AS authoring layer (`Script/CkUsf/*_Assets.as`) — any new post-process look for outline/
  toon rides this unchanged.
- Post-process looks get SceneColor/Depth/Normal by default; opt-in CustomDepth/CustomStencil/
  BaseColor/etc via `_SceneTextures`. Traps cookbook: neighbour taps must convert viewport→buffer
  UV once (`CkUsf_ViewportUVToBufferUV`, texel = `CkUsf_BufferTexelSize()`).

### What the vehicle CANNOT do — and the prior SVE ruling

A blendable cannot change the resolution the scene renders at, has no frame history, and cannot
occupy the upscale slot. The Stylize campaign explicitly ruled out SceneViewExtensions
(`Source/CkUsf/PROMPT.md:40` locked decision; `:78` "SceneViewExtension port — duplicates the look
pipeline; loses validator/MID/AS integration"; `PROGRESS.md:56` "NOT a SceneViewExtension …
Never (maintainer-approved)"). **That ruling's rationale is scoped to color-grading-shaped
effects the look pipeline already served. It does not cover a render-resolution change — the
pixel-art campaign needs a scoped re-open, surfaced explicitly to the maintainer.**

### Renderer-API usage across the plugin (census)

Zero uses of `ISpatialUpscaler`, `FScreenPassTexture`, `PrePostProcessPass`,
`SubscribeToPostProcessingPass`, `FRDGBuilder`, `FGlobalShader`, `IMPLEMENT_GLOBAL_SHADER`
(the only global shader artifact is CkIskmRendererVF's
`IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT`). **CkFoundation has never written an RDG pass.**
Shader dir mappings: CkUsf (`/CkUsf`), CkIskmRendererVF, CkVat, CkParticles, game-side BusterBlock.
CkUsf.Build.cs deps: RenderCore, RHI, Projects (+ Ck deps) — **no Renderer**. Loading phases: only
`CkIskmRendererVF` is PostConfigInit (uplugin:650-652); CkUsf & co. are Default (fine for
material-Custom-node .ush, which resolve at material translation, not global-shader-map init).

### The in-repo SVE reference implementation

`Plugins/ScreenSpaceFogScattering` (sibling plugin, UE 5.7, Dmitry Karpukhin) — complete working
example of everything the new module needs:
- `SSFSViewExtension.h:20-45` — `FSceneViewExtensionBase` subclass, `PrePostProcessPass_RenderThread`,
  `IsActiveThisFrame_Internal`, `TArray<FScreenPassTexture>` mip chains.
- `SSFSViewExtension.cpp:103-134` — `IMPLEMENT_UNIFORM_BUFFER_STRUCT` + 4×
  `IMPLEMENT_GLOBAL_SHADER(..., SF_Compute)`; scene color read via
  `(*Inputs.SceneTextures)->SceneColorTexture` + `ViewInfo.ViewRect`; `AddCopyTexturePass` back
  (comment at `:133`: `PrePostProcessPass_RenderThread` needs manual copy positions for stereo,
  unlike `SubscribeToPostProcessingPass`).
- `SSFSSubsystem.h:10-20` — **`UEngineSubsystem`** owns
  `TSharedPtr<FScreenSpaceFogScatteringViewExtension, ESPMode::ThreadSafe>`;
  `FSceneViewExtensions::NewExtension<>` in `Initialize`; `Deinitialize` pushes a false
  `IsActiveFunctor` before resetting — the correct teardown dance.
- `ScreenSpaceFogScattering.Build.cs:12-21` — `Renderer/Private` (+ 5.6+ `Renderer/Internal`)
  private include paths; deps RenderCore, Renderer, RHI, Projects. uplugin: Runtime/PostConfigInit.

### CkGraphics / CkRenderTarget

CkGraphics owns NO render-target or view utilities (its Claude.md's "render target management"
claim is stale — the API is material/intensity/render-status helpers only). CkRenderTarget is
entity-owned RTs + CPU readback (+ net relay) — not view-attached. The only "low-res chain" in
the plugin is `UCkUsf_MultiPassRenderer` — Shadertoy-style ping-pong `DrawMaterialToRenderTarget`
off-view, not a scene render.

---

## 2. CkCamera + transform pipeline — where snapping and ortho land

### The ECS view is the render authority

- Director entity (`FCk_Handle_Camera`) composes per-frame:
  `FProcessor_Camera_UpdatePOV::ForEachEntity` builds the POV (`CkCamera_Processor.cpp:266-280`),
  optional ViewTarget blend (`:287-291`), then assembles and stores `FMinimalViewInfo` at
  **`CkCamera_Processor.cpp:293-304`** — only `Location`, `Rotation`, `FOV`, `DesiredFOV`,
  conditionally `bConstrainAspectRatio`/`AspectRatio` are written; everything else stays at
  struct defaults. **`ProjectionMode` is never assigned → Perspective. No ortho support exists
  anywhere in CkCamera/Script (verified rg --no-ignore).**
- Handoff: `UCk_CameraComponent::GetCameraView` (`CkCamera_Component.cpp:9-21`) —
  `DesiredView = UCk_Utils_Camera_UE::Get_ViewInfo(_DirectorEntity);` at `:16` is a
  **whole-struct assignment** that discards everything the engine pre-filled and never calls
  Super on the valid path. The ECS `_ViewInfo` is the sole authority for
  ProjectionMode/OrthoWidth/OffCenterProjectionOffset. The component is caller-supplied and
  mandatory (`FCk_Fragment_Camera_ParamsData::_OutputComponent`).
- No custom `APlayerCameraManager` exists in the plugin (design note at
  `CkCamera_Component.h:12-13`).
- **But the ECS is NOT the last writer of Location/Rotation**: engine camera modifiers (shakes)
  run AFTER `GetCameraView` (`PlayerCameraManager.cpp:455-459` `ApplyCameraModifiers`;
  UpdateViewTarget resets POV defaults incl. OrthoWidth/ProjectionMode at `:369-372` first). A
  snap applied in ECS would be perturbed by any active shake. CkCamera routes shakes through
  `StartCameraShake` (`CkCameraShake_Processor.cpp:96` — engine modifiers), though
  anti-pattern #1 in `CkCamera/CLAUDE.md` already forbids processors calling it directly.
  **Render-side snapping (SVE `SetupViewProjectionMatrix`) runs after all of this and is
  immune** (see RESEARCH_UeApis §3).

### Per-camera attributes — the OrthoWidth pattern

The profile-leaves-as-attributes refactor is complete: every `FCk_CameraProfile` leaf is a
non-replicated tuner attribute on the director, grouped in per-section handle fragments
(`FFragment_Camera_Rig/Springs/Sensor/Noise/OrientationControl/AutoReorient/Collision/
DepthOfField`, `CkCamera_Fragment.h:178-262`). Bool + curve leaves live directly on
`FFragment_Camera_Current` (`:93-102`) because they don't blend. Layers acquire typed modifiers
(~60 `Acquire_CameraModifier_<Leaf>` entries, `CkCameraLayer_Utils.h:49-261`);
`FProcessor_CameraLayer_Blend` advances alpha and rewrites live deltas.

**Adding `_OrthoWidth` is a mechanical five-site change** (pattern:
`camera_materialize_detail` namespace, `CkCamera_Utils.cpp:370-494` + `DoMaterializeAttributes`
`:496-600` + `Get_Profile` `:602-709`):
1. Profile field in `FCk_CameraProfile_Sensor` (`CkCameraProfile.h:117-139`, beside `_FOV` :127).
2. Tag in `CkCamera_GameplayTags.h:25-27` + `.cpp` (beside `TAG_Camera_Sensor_FOV` :26).
3. Handle field in `FFragment_Camera_Sensor` (`CkCamera_Fragment.h:200-207`).
4. `AddFloat` line in the Sensor block (`CkCamera_Utils.cpp:527-533`).
5. `ReadFloat` in `Get_Profile`'s Sensor block (`:637-645`).
(+ optional `Acquire_CameraModifier_OrthoWidth` beside `CkCameraLayer_Utils.h:85-91` for layer
blending.) Non-blending companions (a projection-mode enum) follow the plain-field pattern on
`FFragment_Camera_Current` + `Request_Set_<Flag>` (template: `Request_Set_ConstrainAspectRatio`,
`CkCamera_Utils.cpp:283-293`). ViewInfo assembly at `CkCamera_Processor.cpp:293-302` is where
ProjectionMode/OrthoWidth/OffCenterProjectionOffset get written — nothing else can.

### Scheduler timing — everything is early enough

Group order (header authority `CkProcessorGroups.h:36-165`; the roster in `CkEcs/CLAUDE.md` is
STALE — predates `f7703bef9`):

```
DestructionPipeline → Gameplay_TimeDelta → Gameplay → Gameplay_AI → Gameplay_Audio
  → Gameplay_Rendering → Gameplay_Script → Gameplay_Chaos → Physics
  → Transform_SyncFrom → Transform → Transform_Derived → Transform_LateResolve
  → Transform_Finalize → Gameplay_Camera → PostTransform → DeferredApply
  → Replication → EntityLifecycle → EndPlay → Teardown        (all TG_PrePhysics)
FGroup_Overlap  (TG_PostPhysics)
```

Engine frame (`LevelTick.cpp`): TG_PrePhysics :1721 → physics → TG_PostPhysics :1749 → timers →
**"Update cameras" → `UpdateCameraManager` :1818** → TG_PostUpdateWork :1848. So the ENTIRE Ck
scheduler runs before the view is pulled; any group is "late enough", and `TG_PostUpdateWork`
would be too late. **`FGroup_Gameplay_Camera` is currently EMPTY** (no `using Group =` anywhere)
— a vacant, purpose-built slot for "readers that want final poses and the final composed view
without being transform writers" — the natural home for any ECS-side pixel-art camera bookkeeping
(e.g. publishing effective texel size, gameplay-side snapped-pose queries).

Recent camera/transform commits (context for integration):
- `f7703bef9` — `FGroup_Transform_Derived` + `FGroup_Transform_LateResolve` scheduler groups
  (registration in `CkProcessorGroups.cpp:17-18` is load-bearing).
- `9a005aa4e` — second transform resolution pass (`TProcessor_Transform_HandleRequests_InGroup<T>`),
  `Request_SetTransform_SameFrame` retired.
- `96b5cad0e` — camera composes in Transform_Derived; publishes the view to `_ViewAnchor`
  (`FFragment_Camera_Current`, `CkCamera_Fragment.h:80-87`) — a plain child transform driven by a
  deferred `Request_SetTransform` (`CkCamera_Processor.cpp:309-313`), consumable via
  `UCk_Utils_Camera_UE::Get_ViewAnchor`. Nothing consumes it yet — published extension point.

### Existing "final view modifier" shapes (candidate insertion points, evaluated)

- (a) CameraLayer attribute modifiers — wrong shape for a snap (blended scalar deltas; a snap is
  a nonlinear post-composition quantization that must never be eased).
- (b) In-processor post-composition override — the ViewTarget blend at
  `CkCamera_Processor.cpp:287-291` is exactly a final-transform rewrite between POV run and
  ViewInfo write. The natural ECS-side snap site IF snapping lived in ECS — but engine modifiers
  running later would break it (above).
- (c) `UCameraModifier` at highest priority — would be the repo's first, breaks the "no custom
  PCM" design note, and still precedes `SetupViewProjectionMatrix` anyway.

**Conclusion:** render-side snap in the SVE (`SetupViewProjectionMatrix`) is the only location
that is simultaneously last-word, shake-immune, camera-system-agnostic (works for non-CkCamera
consumers of the framework), and zero-coupling into CkCamera. CkCamera's campaign work reduces to
ortho projection support (+ explicit near/far planes) in `_ViewInfo`.

### Doc drift noticed in passing (fix during the campaign)

- `CkEcs/CLAUDE.md` § "Processor group pipeline order" — stale (missing the two new groups; lists
  "Camera" under `FGroup_Gameplay_Rendering`).
- `CkGraphics/Claude.md` — claims "render target management" that does not exist in the API.

### Sub-texel remainder transport (game→render)

No Material Parameter Collection plumbing exists in CkUsf; the MID `Set_Scalar/Vector` path is
per-look. For the upscaler, the per-frame `new ISpatialUpscaler(remainder, ...)` in
`BeginRenderViewFamily` is the correct transport (family-owned lifetime; see RESEARCH_UeApis §2).
If a LOOK ever needs the remainder (it shouldn't — looks run pre-upscale on the snapped raster),
an MPC or per-look MID write would be the fallback.
