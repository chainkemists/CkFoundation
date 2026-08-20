# RESEARCH — UE 5.7 engine integration, verified against the fork (2026-08-20)

> Merged output of a web-research sweep and a line-level verification pass against the local
> engine source. **Verification basis:** everything marked `[SRC]` was read directly from
> `D:\Repositories\UnrealEngine-Angelscript` (UE **5.7.4** per `Engine/Build/Build.version`;
> CompatibleChangelist 47537391). A git-log check found **no fork deviation** in any renderer /
> screen-percentage / scene-view path inspected — the fork's own commits are all
> AngelScript/editor-tooling. Web claims cited inline; **UNVERIFIED** marked.

---

## 1. Prior art: the Fab plugin ("Stable 3D Pixel Art", Lomilabs)

https://www.fab.com/listings/816f34f2-1a2e-4910-b3dd-009b5da0873b — published **2026-08-18**
(two days before this research), UE 5.8/Win64 only, $262–$1,243 by license, single-product seller,
no community breadcrumbs yet. Verbatim limitation set (the architecture fingerprint):

- "works with standard Unreal orthographic cameras and does not require a custom pawn, controller,
  or camera manager"
- "cannot run alongside an active TSR, DLSS, FSR, XeSS, or another competing **primary upscaler**"
- "Split screen, stereo rendering, VR, Scene Captures, and multiple simultaneous game views are
  not supported"
- "Camera **translation** is stabilized; actively **rotating** the camera changes the screen-space
  projection and re-rasterizes the scene"
- "The editor pixelation preview is an artistic preview rather than an exact replacement for Play
  In Editor or Standalone"

**Inferred architecture (high confidence):** custom primary spatial upscaler registered from a
scene view extension + screen percentage driven down for the internal res + per-frame ortho
view-origin snap with the sub-texel remainder consumed by a sharp-bilinear upscale shader. Every
limitation falls out of that design mechanically (see §2/§3 for the engine-source proof of each).

---

## 2. The custom-upscaler slot (the chosen integration)

### `ISpatialUpscaler` `[SRC]` — `Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessUpscale.h`

**Renderer-PRIVATE header** — a plugin must add `Runtime/Renderer/Private` to `PrivateIncludePaths`
(the FSR/NIS plugins do exactly this). `ITemporalUpscaler` and `ScreenPass.h`
(`FScreenPassTexture`/`FScreenPassRenderTarget`) are public by contrast.

- L8–20: `enum class EUpscaleMethod : uint8 { None, Nearest, Bilinear, Directional, CatmullRom, Lanczos, Gaussian, SmoothStep, Area, MAX }`
- L24–36: `enum class EUpscaleStage { PrimaryToSecondary, PrimaryToOutput, SecondaryToOutput, MAX }`
- L38 doc: *"Interface for custom spatial upscaling algorithm meant to be set on the
  FSceneViewFamily by ISceneViewExtension::BeginRenderViewFamily()."*
- L39: `class ISpatialUpscaler : public ISceneViewFamilyExtention`
- L42–52 `struct FInputs`: `FScreenPassRenderTarget OverrideOutput` ([Optional] render here if
  valid, else create+return); `FScreenPassTexture SceneColor` ([Required], carries the view rect);
  `EUpscaleStage Stage`.
- L54 `GetDebugName()`; L57 `ISpatialUpscaler* Fork_GameThread(const FSceneViewFamily&) const = 0;`
- L59–62: `virtual FScreenPassTexture AddPasses(FRDGBuilder&, const FViewInfo&, const FInputs&) const = 0;`
- L64–69: `static RENDERER_API FScreenPassTexture AddDefaultUpscalePass(...)` — callable fallback.

### Registration + lifetime `[SRC]`

- `FSceneViewFamily::SetPrimarySpatialUpscalerInterface(ISpatialUpscaler*)` — `SceneView.h:2573`;
  `check(ptr)` + `checkf(already-null, "View family already had a primary spatial upscaler
  assigned.")`. Secondary twin at `:2585`.
- **Only legal registration point:** `ISceneViewExtension::BeginRenderViewFamily` — enforced at
  `SceneRendering.cpp:5140-5146` ("Force the upscalers to be set no earlier than
  ISceneViewExtension::BeginRenderViewFamily()"; all three interfaces checked null before the SVE
  callbacks fire at `SceneRenderBuilder.cpp:508`).
- **Family owns the instance:** `~FSceneViewFamily` deletes it (`SceneView.cpp:3054-3079`, with the
  comment "ISpatialUpscaler* is only defined in renderer's private header because no backward
  compatibility is provided between major version") → allocate with `new` per frame, never delete.
  Split families clone via `Fork_GameThread` (`SceneRendering.cpp:2843/2861/2867`).
- **The per-frame `new` is the sub-texel-remainder transport:** bake this frame's snap error +
  internal res into the upscaler instance on the game thread; `AddPasses` reads it on the render
  thread. No globals, no race.

### Where the renderer calls it `[SRC]` — `PostProcessing.cpp`

EPass order (L441-483): MotionBlur → PPM(BeforeBloom) → Tonemap → FXAA → SMAA →
PPM(AfterTonemapping) → visualizers/editor prims → **PrimaryUpscale** → **SecondaryUpscale**.

- **L600 gate:** `PassSequence.SetEnabled(EPass::PrimaryUpscale, (bApplyLensDistortion &&
  !bApplyLensDistortionInTSR) || (View.PrimaryScreenPercentageMethod ==
  EPrimaryScreenPercentageMethod::SpatialUpscale && PrimaryViewRect.Size() !=
  View.GetSecondaryViewRectSize()));` — **at 100% screen percentage the pass never fires.**
- L1949–1983, the call site: strict `if (CustomUpscaler) { SceneColor =
  CustomUpscaler->AddPasses(...); } else { AddDefaultUpscalePass(...); }` — **a custom primary
  upscaler fully REPLACES the default upscale pass** (RDG event `"ThirdParty PrimaryUpscale …"`).
  Contract `check`s: if PrimaryUpscale is the last pass you MUST write `ViewFamilyOutput` (honor
  `OverrideOutput`); otherwise the returned rect must equal `View.GetSecondaryViewRectSize()`.
- Default methods: `r.Upscale.Quality` 0=Nearest 1=Bilinear 2=Directional 3=CatmullRom(default)
  4=Lanczos 5=Gaussian (`PostProcessUpscale.cpp:22-31`). **No sharp-bilinear in the stock set** —
  that is why a custom upscaler is needed at all.
- TSR conflict, proven: `SceneView.cpp:1012-1019` flips `PrimaryScreenPercentageMethod` to
  `TemporalUpscale` whenever AA is TSR (or TAA + `r.TemporalAA.Upsampling`), and the gate at L600
  then never enables PrimaryUpscale → the custom upscaler silently never runs. **AA must be
  None/FXAA/SMAA/MSAA for this renderer.**
- `ICustomStaticScreenPercentage` **does not exist in 5.7** (zero hits) — any tutorial referencing
  it is stale. Modern temporal upscalers use `UE::Renderer::Private::ITemporalUpscaler`
  (`SceneView.h:2561`) + their own screen-percentage drivers.

### Screen-percentage bounds + pixel-exact internal res `[SRC]`

- `ISceneViewFamilyScreenPercentage` (`SceneView.h:2163-2203`): `kMinResolutionFraction = 0.01f`,
  `kMaxResolutionFraction = 4.0f` (TSR min 0.25, TAAU min 0.5 — advisory, enforced by callers, and
  irrelevant on the spatial path). Interface: `GetResolutionFractionsUpperBound()`,
  `Fork_GameThread()`, protected `GetResolutionFractions_RenderThread()`.
- Clamps live only in the default driver (`LegacyScreenPercentageDriver.cpp:142,160`) and renderer
  checks (`SceneRendering.cpp:3315-3323`). `Scalability::MinResolutionScale` is **0.0f**
  (`Scalability.h:242`), and there is no `r.ScreenPercentage.MinResolution` cvar in 5.7.
- View rect derivation: `FSceneRenderer::ApplyResolutionFraction` — **`FMath::CeilToInt(UnscaledViewSize
  * ResolutionFraction)`** per axis (`SceneRendering.cpp:3116-3122`). **640×360 from 1920×1080
  (33.3%) or 320×180 (16.7%) are fully legal on the spatial path.** For pixel-exact integer sizes,
  compute `Fraction = (TargetW - 0.5) / ViewportW` per frame so CeilToInt lands exactly on TargetW
  for any viewport size.
- **Ownership conflict to solve in implementation:** `UGameViewportClient::Draw` installs its own
  `FLegacyScreenPercentageDriver` (`GameViewportClient.cpp:1626/1910`), and
  `SetScreenPercentageInterface` asserts on double-assign (`SceneView.h:2512-2517`). Options:
  (a) drive the global `r.ScreenPercentage` cvar (the legacy driver reads it;
  `GetCVarResolutionFraction` snaps to 1.0 only at ≤ `MinResolutionScale` = 0, so any fraction
  survives — `LegacyScreenPercentageDriver.cpp:176-187`); (b) replace the interface via
  `SetScreenPercentageInterface_Unchecked` (`SceneView.h:2519`); (c) a custom GameViewportClient.
  (a) is least invasive and per-frame updatable on viewport resize; decide in Phase 0/1.
- `EngineShowFlags.ScreenPercentage` must be on (renderer `checkf` at `SceneRendering.cpp:3315`).
- **Dynamic resolution must be disabled** (fights a fixed internal res by design).

### Secondary upscale (DPI) `[SRC]`

- `r.SecondaryScreenPercentage.GameViewport` (`GameViewportClient.cpp:115-121`): default 0 =
  "compute secondary screen percentage = 100 / DPIScalefactor automatically". Applied at
  `GameViewportClient.cpp:1566-1584`; `Min(...,1.0)` — never supersamples.
- **Trap:** in editor/PIE at >100% OS DPI, the engine silently inserts this DPI-driven secondary
  upscale (filter: `EUpscaleMethod::SmoothStep` under `LowerPixelDensitySimulation`,
  `PostProcessing.cpp:2007-2011`), resampling our pixels — this is almost certainly why the Fab
  plugin calls its editor preview "artistic". **Validate visuals in standalone at controlled
  resolution**, and consider forcing secondary = 100 in game.
- A separate **secondary spatial upscaler slot** exists (`SetSecondarySpatialUpscalerInterface`,
  consumed at `PostProcessing.cpp:1992` with `Stage = SecondaryToOutput`) — available if we ever
  need to own the DPI pass too.

---

## 3. SceneViewExtension hooks `[SRC]` — `Engine/Public/SceneViewExtension.h`

`ISceneViewExtension` (L112). Full virtual list, 5.7:

| Line | Hook | Thread |
|---|---|---|
| 140 | `SetupViewFamily(FSceneViewFamily&)` | game |
| 145 | `SetupView(FSceneViewFamily&, FSceneView&)` | game |
| 150 | `SetupViewPoint(APlayerController*, FMinimalViewInfo&)` | game (pre-projection) |
| 155 | `SetupViewProjectionMatrix(FSceneViewProjectionData&)` | game |
| 160 | `BeginRenderViewFamily(FSceneViewFamily&)` | game — **only legal upscaler-registration point** |
| 165 | `PostCreateSceneRenderer(const FSceneViewFamily&, ISceneRenderer*)` | game |
| 170–200 | `PreRenderViewFamily/PreRenderView/PreInitViews/PreRenderBasePass/PostRenderBasePassDeferred/PostRenderBasePassMobile/PostTLASBuild _RenderThread` | RT |
| 205 | `PrePostProcessPass_RenderThread(FRDGBuilder&, const FSceneView&, const FPostProcessingInputs&)` | RT |
| 217 | `SubscribeToPostProcessingPass(EPostProcessingPass, const FSceneView&, FPostProcessingPassDelegateArray&, bool)` — **signature changed in 5.5** (old no-FSceneView overload is `UE_DEPRECATED(5.5)` and no longer called, L248) | RT |
| 222/227 | `PostRenderViewFamily/PostRenderView _RenderThread` | RT |
| 232 | `GetPriority()` (higher runs first) | — |
| 237 | `IsActiveThisFrame(const FSceneViewExtensionContext&)` — `override final` on `FSceneViewExtensionBase`; functor list wins (first set `TOptional<bool>`), else `IsActiveThisFrame_Internal` (L256, default true). Queried once per frame per context; false suppresses ALL callbacks that frame. | — |

`EPostProcessingPass` (L117-133): `BeforeDOF, AfterDOF, TranslucencyAfterDOF, SSRInput,
ReplacingTonemapper, MotionBlur /*BL_SceneColorBeforeBloom*/, Tonemap
/*BL_SceneColorAfterTonemapping*/, FXAA, SMAA, VisualizeDepthOfField, MAX`. Callback delegate:
`FPostProcessingPassDelegate(FScreenPassTexture, FRDGBuilder&, const FSceneView&, const
FPostProcessMaterialInputs&)`; "The pass MUST write to the override output texture if it is
active". **Every hookable pass precedes PrimaryUpscale** → SVE-injected passes and all PP
materials run at the LOW internal resolution. A SVE cannot replace the upscale slot itself — only
`ISpatialUpscaler` occupies it.

`SetupViewProjectionMatrix` is called from `ULocalPlayer::GetProjectionData`
(`LocalPlayer.cpp:1264-1267`) *after* `FMinimalViewInfo::CalculateProjectionMatrixGivenView` —
i.e. the snap hook fires only for local-player game views (scene captures/stereo bypass it —
consistent with the Fab plugin's exclusions), and it runs AFTER the PlayerCameraManager's
modifiers (shakes), so a snap applied here is the last word on the view origin.

Construction: `FSceneViewExtensions::NewExtension<T>()` only (`FAutoRegister`, L265-269).
Convenience bases: `FWorldSceneViewExtension` (one UWorld), `FHMDSceneViewExtension`.
`FSceneViewExtensionContext` carries `FViewport*`, `FSceneInterface*`, `GetWorld()`.

Repo-local working reference: `Plugins/ScreenSpaceFogScattering` (UE 5.7) — `FSceneViewExtensionBase`
subclass with `PrePostProcessPass_RenderThread`, `IMPLEMENT_GLOBAL_SHADER` ×4, a `UEngineSubsystem`
owning the extension (`Deinitialize` pushes a false `IsActiveFunctor` before resetting — the
correct teardown dance), Build.cs adding `Renderer/Private` (+ `Renderer/Internal` on 5.6+), and a
`Runtime`/`PostConfigInit` uplugin module. See RESEARCH_Codebase.md §4.

---

## 4. Orthographic support state, 5.3 → 5.7 `[SRC where marked]`

Timeline: Experimental in 5.3; `bAutoCalculateOrthoPlanes` in 5.4; 5.5 "solved jittering and UI
issues". Official doc claims compat with Lumen/Nanite/VSM/TSR/reflections/volumetrics/path
tracing/water, warns "new engine features … might not be optimized for Orthographic cameras".
Never stamped production-ready in those words.

**API `[SRC]`** (`Camera/CameraTypes.h`): `ProjectionMode` (default Perspective), `OrthoWidth`
(L66, default 512), `bAutoCalculateOrthoPlanes` (L70, default **true**), `AutoPlaneShift`,
`bUpdateOrthoPlanes`, `bUseCameraHeightAsViewTarget`, `OrthoNearClipPlane` (default 0),
`OrthoFarClipPlane` (default `UE_OLD_WORLD_MAX`), and **`OffCenterProjectionOffset`** (L127-129 —
engine-shipping sub-pixel projection shift "as proportion of screen dimensions"; ortho matrices
take it in `M[3][0]/M[3][1]`, `CameraStackTypes.cpp:183-241`).

**Full 5.7 `r.Ortho.*` cvar census `[SRC]`:**
- `CameraStackTypes.cpp`: `r.Ortho.AutoPlanes` (:21), `.ClampToMaxFPBuffer` (:28, 16-bit depth
  scaling, max FP16 66504), `.ScaleIncrementingUnits` (:39), `.DepthScale` (:46),
  `.ShiftPlanes` (:54), `r.Ortho.Debug.ForceAllCamerasToOrtho/ForceOrthoWidth/ForceUseAutoPlanes/
  ForceCameraNearPlane/ForceCameraFarPlane` (:63-92, non-shipping).
- `SceneView.cpp`: `r.Ortho.CalculateDepthThicknessScaling` (:328),
  `r.Ortho.DepthThicknessScale` (:337, default 0.001 — "orthographic scene depth scales
  proportionally lower than perspective, typically 1/100"; THE knob when SSR/SSAO/screen traces
  misbehave in ortho), `r.Ortho.DefaultUpdateNearClipPlane` (:345),
  `r.Ortho.AllowNearPlaneCorrection` (:352), `r.Ortho.CameraHeightAsViewTarget` (:362).
- VSM: `r.Ortho.VSM.EstimateClipmapLevels/ClipmapLODBias/ProjectViewOrigin/RayCastViewOrigin`
  (`VirtualShadowMapClipmap.cpp:102-123`).
- `r.Ortho.UsePreviousMotionVelocityFlattenPass` (`PostProcessMotionBlur.cpp:87`),
  `r.Ortho.FogHeightAdjustment` (`FogRendering.cpp:47`), `r.Ortho.EditorDebugClipPlaneScale`.

**Auto-plane logic `[SRC]`** (`FMinimalViewInfo::AutoCalculateOrthoPlanes`,
`CameraStackTypes.cpp:386-473`): computes a unit-per-pixel ratio **from the view rect** — **the
auto far plane depends on the render resolution**, so a low-res ortho view gets a different depth
range than a full-res one. Combined with community reports of near-plane surprises (near plane
evaluating ~1.4× ortho height behind the camera; clipping as OrthoZoom changes), **prefer explicit
near/far planes for a fixed-pixel-scale renderer.**

**Shadows `[SRC]`:**
- `ShadowSetup.cpp:4094-4096`: "for now only support perspective projection as ortho camera
  shadows are broken anyway" — directional-light convex-hull culling skipped for ortho.
- `ShadowSetup.cpp:2952-2956`: `bool bIsPerspectiveProjection = true;` **hardcoded** in the CSM
  cascade frustum build — **CSM cascades are always built as if perspective. CSM + ortho is
  unsupported at the source level. VSM is the ortho shadow path** (four dedicated cvars above).
- Distance culling for shadow relevance skipped for ortho (`ShadowSetup.cpp:2012,2231`).

**Other ortho-conditional paths `[SRC]`:** lens distortion asserts perspective
(`LensDistortion.cpp:139`); volumetric render target disabled for ortho
(`VolumetricRenderTarget.cpp:93`); TSR special-cases ortho for precision (works, but must be off
for this renderer anyway); decals/Lumen/occlusion/translucent-lighting all have ortho branches.

**Community-reported pitfalls:** directional light functions don't render in ortho views as of
5.5.1 (forum, unresolved; **UNVERIFIED whether fixed in 5.7** — test on the fork before relying;
cloud shadows should be a material/global-uniform effect, not a light function); volumetric clouds
lack the perspective component (official doc).

---

## 5. UI compositing `[SRC]` — native-res UI is free

Proof chain, one frame: `FEngineLoop::Tick` → `UGameEngine::RedrawViewports`
(`GameEngine.cpp:2006`) → `FViewport::Draw` → `UGameViewportClient::Draw` →
`BeginRenderingViewFamily` (`GameViewportClient.cpp:1971`, enqueues the whole scene render incl.
both upscale passes) … then later `FSlateApplication::Tick` → `FSlateRHIRenderer::DrawWindows_Private`
(back-buffer pass). Same render-thread queue, game-thread order → **Slate paints the back buffer
after the scene's upscale, at native resolution.** In a shipping game the scene renders directly
into the window back buffer (`SViewport.cpp:158-160` "Only draw a quad if not rendering directly
to the backbuffer"; `GameEngine.cpp:214/241` `bRenderDirectlyToWindow = true`). **UI is never
upscaled by the primary or secondary spatial upscaler — crisp UMG for free.**

---

## 6. How upscaler plugins register (precedent) `[SRC]`

No FSR/DLSS/XeSS plugin ships in the fork (`MobileFSR` is an empty husk). The canonical path
remains: `FSceneViewExtensionBase` subclass → `BeginRenderViewFamily` →
`SetPrimarySpatialUpscalerInterface(new FMyUpscaler(...))`, with `SetupView` setting
`View.PrimaryScreenPercentageMethod = SpatialUpscale` where needed.

5.7 also ships `UE::VirtualProduction::IUpscalerModularFeature`
(`Runtime/VirtualProduction/.../IUpscalerModularFeature.h`) — a modular-feature wrapper consumed
by nDisplay (`DisplayClusterUpscaler.cpp:497-500` ends in `SetScreenPercentageInterface(new
FLegacyScreenPercentageDriver(...))`). Useful as a reference for driver installation, not needed
for a game-local renderer.

---

## 7. Global shaders from a plugin (PostConfigInit) `[SRC]`

Confirmed engine-plugin pattern — **`CompositeCore`** (`Runtime`/`PostConfigInit`):

```cpp
void FCompositeCoreModule::StartupModule()
{
    AddShaderSourceDirectoryMapping(TEXT("/Plugin/CompositeCore"),
        FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("CompositeCore"))->GetBaseDir(), TEXT("Shaders")));
    // Since we are so early in the loading phase, we first need to load the cvars
    // since they're not loaded at this point.
    UE::ConfigUtilities::ApplyCVarSettingsFromIni(
        TEXT("/Script/CompositeCore.CompositeCorePluginSettings"), *GEngineIni, ECVF_SetByProjectSetting);
}
```

plus `DECLARE_GLOBAL_SHADER` / `SHADER_USE_PARAMETER_STRUCT` / `BEGIN_SHADER_PARAMETER_STRUCT` /
permutations / `IMPLEMENT_GLOBAL_SHADER(F..., "/Plugin/CompositeCore/Private/....usf", "MainCS",
SF_Compute)` and RDG dispatch (`CompositeCorePassDilate.cpp:12-36`). Other PostConfigInit
precedents with the same idiom: `LensDistortion`, `OpenColorIO`, `Composite`, `LandscapePatch`.
The cvar note is the one PostConfigInit gotcha worth copying (config-driven cvars are not applied
yet at that phase). In-repo precedents: `CkIskmRendererVF` (the plugin's only PostConfigInit
module) and `Plugins/ScreenSpaceFogScattering`.

---

## 8. Ruled-out alternatives (for the record)

- **SceneCapture2D → RT → quad/UMG:** double view cost, feature divergence (velocity history,
  show flags, PP behavior), +1 frame latency via UMG, resize plumbing, capture Lumen/VSM cost.
  The Fab plugin explicitly excludes captures. Keep only for diegetic screens/icons.
- **PP-material pixelation only (UV snapping at native res):** no true texel grid (lighting/AA
  still native-res), no perf win, shimmer under motion, no stable 1-texel outlines, no
  snap+shift-back trick possible. Confirmed dead end by an Epic-forums attempt that tried it.
  (CkUsf ScreenDither's PixelScale block-snap is exactly this class — a stylization, not a
  pixel-art renderer.)
- **MSAA:** forward-only on desktop (`r.MSAACount` help `[SRC]`) — not available in deferred.
- **TAA/TSR at low res:** ghosting on outlines/flipbooks (community-verified), and TSR/TAAU
  structurally disable the PrimaryUpscale slot (§2).

## 9. Riskiest open items to validate on the fork (Phase 0)

1. The screen-percentage driver ownership conflict vs `UGameViewportClient` (§2 options a/b/c).
2. Directional light functions in ortho views (broken as of 5.5.1, unverified in 5.7).
3. Auto-ortho-planes vs explicit planes at low-res view rects (auto far plane is
   resolution-dependent, `CameraStackTypes.cpp:386-473`).
4. PIE DPI secondary-upscale distortion — visuals must be signed off in standalone.
5. Lumen/SSAO/SSR behavior at 640×360 ortho with `r.Ortho.DepthThicknessScale` — likely OFF for
   the aesthetic; measure and document the supported feature matrix.
6. HighResShot semantics with a fixed internal res (scale internal proportionally vs keep fixed —
   handle explicitly in the driver).
