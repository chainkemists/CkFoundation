CkPso research report: facts for manifest-driven PSO warm-up.

Path conventions: `RT/` = `D:\Repos\UnrealEngine-Angelscript\Engine\Source\Runtime\`, `NG/` = `...\Engine\Plugins\FX\Niagara\Source\`, `CK/` = `D:\Repos\CkPlugins2\Plugins\CkFoundation\Source\`, `BB/` = `D:\Repos\BusterBlock\`.
Tags: **[R]** = READ (citation given). **[I]** = INFERRED.

---

## 1. Recipe: precache one material for one vertex factory (VF)

### Signatures

**UMaterialInterface** — `RT/Engine/Public/Materials/MaterialInterface.h` [R]

648: `FGraphEventArray PrecachePSOs(const FVertexFactoryType* VertexFactoryType, const struct FPSOPrecacheParams& PreCacheParams)`
- Forwards to 652.

652: `FGraphEventArray PrecachePSOs(const TConstArrayView<const FVertexFactoryType*>& VertexFactoryTypes, const struct FPSOPrecacheParams& PreCacheParams)`
- Discards the request IDs.

658: `FGraphEventArray PrecachePSOs(const TConstArrayView<const FVertexFactoryType*>& VertexFactoryTypes, const struct FPSOPrecacheParams& PreCacheParams, TArray<FMaterialPSOPrecacheRequestID>& OutMaterialPSORequestIDs)`
- Priority is fixed to Medium.

663: `FGraphEventArray PrecachePSOs(const TConstArrayView<const FVertexFactoryType*>& VertexFactoryTypes, const struct FPSOPrecacheParams& PreCacheParams, EPSOPrecachePriority PSOPrecachePriority, TArray<FMaterialPSOPrecacheRequestID>& OutMaterialPSORequestIDs)`
- Builds `FPSOPrecacheVertexFactoryDataList` with `CustomDefaultVertexDeclaration = nullptr`.

673: `virtual FGraphEventArray PrecachePSOs(const FPSOPrecacheVertexFactoryDataList& VertexFactoryDataList, const struct FPSOPrecacheParams& PreCacheParams, EPSOPrecachePriority Priority, TArray<FMaterialPSOPrecacheRequestID>& OutMaterialPSORequestIDs) { return FGraphEventArray(); }`
- This is the only overload that lets you pass a custom vertex declaration.

Other properties of these overloads:
- They are inline and non-UFUNCTION, so they need no export macro.
- The overrides are exported:
  - `RT/Engine/Public/Materials/Material.h:1306`: `ENGINE_API virtual FGraphEventArray PrecachePSOs(const FPSOPrecacheVertexFactoryDataList&, const FPSOPrecacheParams&, EPSOPrecachePriority, TArray<FMaterialPSOPrecacheRequestID>&) override;`
  - `MaterialInstance.h:941`: `virtual ENGINE_API FGraphEventArray PrecachePSOs(...same...) override;`

Related types [R]:
- `FMaterialPSOPrecacheRequestID` is `uint32` (`PSOPrecacheFwd.h:30`).
- `EPSOPrecachePriority { Medium, High, Highest }` (`RT/RHI/Public/PipelineStateCache.h:32-37`).

### Internal gating (what makes a call return nothing)

**UMaterial** (`RT/Engine/Private/Materials/Material.cpp:2962-2981`) [R]. It only proceeds when all of these hold:
- `FApp::CanEverRender()`
- `MaterialResources.Num() > 0`
- `PipelineStateCache::IsPSOPrecachingEnabled() || IsPSOShaderPreloadingEnabled()`
- `UMaterialInterface::IsDefaultMaterialInitialized()`

It then loops `GetFeatureLevelsToCompileForRendering()` and calls `FindMaterialResource(..., GetCachedScalabilityCVars().MaterialQualityLevel, true)`. Only the **currently active material quality level** is precached. For each resource it calls `->CollectPSOs(...)`.

**UMaterialInstance** (`MaterialInstance.cpp:2871-2898`) [R]:
- Same render and precache-enabled gate.
- Calls `ConditionalPostLoad()`.
- If `bHasStaticPermutationResource`, it uses its own `StaticPermutationMaterialResources`; otherwise it calls `Parent->PrecachePSOs(...)`.
- So MIDs and MICs without static switches resolve to the parent material's FMaterial, and dedup happens there.

**`FMaterial::CollectPSOs`** (`RT/Engine/Private/Materials/MaterialShared.cpp:3153-3187`) [R]:
- Returns empty if `GameThreadShaderMap == nullptr` (3158-3161).
- Silently **skips any VF without `SupportsPSOPrecaching()`** (3164-3167).
- Per VF it builds an `FMaterialPSOPrecacheParams{FeatureLevel, this, VFData, PreCacheParams}` and calls `PrecacheMaterialPSOs(Params, Priority, GraphEvents)`.
- If the returned ID `!= INDEX_NONE`, it calls `AddUnique` into both `OutMaterialPSORequestIDs` and the FMaterial's own `PrecachedPSORequestIDs` (3176-3184).

**`FMaterialShaderMap::CollectPSOPrecacheData`** (`RT/Engine/Private/Materials/MaterialShader.cpp:3417-3462`) [R]:
- Returns empty if the shader map has no mesh shader map for that VF (3430-3434).
- In a cooked build this is exactly the case when the material lacks the usage flag (for example `bUsedWithSkeletalMesh`), because cooked usage flags cannot be added at runtime [I].
- It uses a **default `FSceneTexturesConfig` with only FeatureLevel set** ("multiview & alpha channel not taken into account", 3436-3442).
- It then iterates every registered `IPSOCollector` for the shading path.

### Threading

- [R] The engine comment at `RT/Engine/Private/Components/PrimitiveComponent.cpp:5106-5109` says: "only request PSOs from game thread because TStrongObjectPtr is used on the material".
- [R] The manager news a `TStrongObjectPtr<UMaterialInterface>` (`PSOPrecacheMaterial.cpp:282`), and `ClearMaterialPSORequests` does `check(IsInGameThread())` (452).
- [I] **Call from the game thread.** Collection then runs on `AnyBackgroundThreadNormalTask` (`PSOPrecacheMaterial.cpp:123`) when `GPSOUseBackgroundThreadForCollection && FApp::ShouldUseThreadingForPerformance() && !GIsEditor` (215). `r.PSOPrecache.UseBackgroundThreadForCollection` defaults to 1 and is ReadOnly (22-28).

### Shader map requirement

- [R] A loaded `GameThreadShaderMap` is required; otherwise the call returns empty and silently does nothing.
- [I] In cooked builds it is populated by serialization at load/PostLoad. Calling after the async load completes (post-PostLoad) is sufficient.

### When precaching is disabled

- [R] The UMaterial/UMaterialInstance overrides return an empty `FGraphEventArray` with no IDs.
- [R] In `r.PSOPrecache.Mode=1` (PreloadShader; `PSOPrecache.cpp:71-77`, ECVF_Default), the manager takes the `PreloadShaders` branch (`PSOPrecacheMaterial.cpp:196-199`). The returned ID stays `INDEX_NONE`, and the events are shader-preload events, not PSO compiles.
- [R] Precaching is also off when `UE_SERVER` (`RT/Core/Public/HAL/Platform.h:625-626`). D3D12 sets `GRHISupportsPSOPrecaching = PLATFORM_WINDOWS` (`RT/D3D12RHI/Private/D3D12RHI.cpp:240`).

### Deduplication (`FMaterialPSORequestManager::PrecachePSOs`, `RT/Engine/Private/PSOPrecacheMaterial.cpp:188-312`) [R]

- The key is the full `FMaterialPSOPrecacheParams` = {FeatureLevel, `FMaterial*`, VF type + custom declaration pointer, params `uint64`}. The hash is at `PSOPrecache.h:297-301`.
- **Already Completed:** the read-lock fast path returns `INDEX_NONE` and adds **no events** (202-210).
- **Existing and not complete:**
  - `CheckCompilingPSOs` prunes finished compiles, and boosts if the new priority is High and differs (227-228).
  - It returns the **existing request ID** plus either the `CollectionGraphEvent` (background mode) or each active `AsyncCompileEvent` (233-245).
  - If it became Completed during that check, the ID stays `INDEX_NONE`.
- **New:** `RequestID = MaterialPSORequests.Add(Params)` (an index into an ever-growing array) and the state is set to Collecting (252-275).
- A second, PSO-level dedup exists in the RHI. `TryAddNewState` (`RT/RHI/Private/PipelineStateCache.cpp:2366-2461`) and `HasPSOBeenRequested` (2671-2688) return an **invalid** result for an initializer already compiled.
  - `RequestPrecachePSOs` (`RT/Engine/Private/PSOPrecache.cpp:198-245`) only keeps results where `PSOPrecacheResult.IsValid() && PrecacheData.bRequired` (216, 230). So events cover **only still-pending, required** PSOs.
  - The RHI hash map is never shrunk: there is no `PrecachedPSOInitializerData.Remove/Empty` in `PipelineStateCache.cpp` [R by absence].
  - The hash is `RHIComputePrecachePSOHash` (`RT/RHI/Private/DynamicRHI.cpp:648+`): state hash plus RT formats, flags, samples, and so on.

### When does the returned FGraphEventArray complete?

**Answer: when the PSO compiles finish, not when collection finishes.** [R]

- **Background mode:**
  - `CollectionGraphEvent = FGraphEvent::CreateGraphEvent()` is what is returned (262, 289).
  - The collection task runs `CollectPSOPrecacheData`, then `RequestPrecachePSOs`, then `MarkCollectionComplete`.
  - It then calls `CollectionGraphEvent->AddPrerequisites(Result.AsyncCompileEvent)` for every compile, and finally `CollectionGraphEvent->Unlock()` (609-635).
- **Each AsyncCompileEvent** is created at `TryAddNewState` (2411-2412). It fires `DispatchSubsequents()` after `RHICreateGraphicsPipelineState` returns and `PrecacheFinished` runs (3376, 3389-3392, 3230-3235).
- **Synchronous (non-background) mode** returns the per-PSO `AsyncCompileEvent`s directly (304-308).
  - [I] If `FApp::ShouldUseThreadingForPerformance()` is false, `bDoAsyncCompile` is false and the event may be null. Null-check entries before calling `IsComplete()`.
- **Caveat:** PSOs with `bRequired=false` are compiled but **not** included in the events (`PSOPrecache.cpp:217-219`).
  - [R] Mesh-pass collectors pass `bRequired` through `MeshPassProcessor.inl:321`; `CustomDepthRendering.cpp:791` passes `true`.
  - [R] Light-function and deferred-light collectors use `false` (`LightRendering.cpp:3761`, `MobileDeferredShadingPass.cpp:1190/1315`).
- **Polling alternatives:**
  - `PipelineStateCache::IsPrecaching(const FPSOPrecacheRequestID&)` (`PipelineStateCache.h:209`, RHI_API).
  - Global counters: `NumActivePrecacheRequests()` (224) and `IsPrecaching()` (218).

---

## 2. `FPSOPrecacheParams` and how the engine fills it

### Fields and defaults

`RT/Engine/Public/PSOPrecache.h:29-141` [R]. All fields are bit-fields in a `uint64 Data` union (105-139); equality and hash use `Data` (55-68).

| Field (bits) | Default (ctor 31-53) |
|---|---|
| PrimitiveType:6 | `PT_TriangleList` |
| bDefaultMaterial:1 | false |
| bCanvasMaterial:1 | false |
| bSplineMesh:1 | false |
| bSkinnedMesh:1 | false |
| bRenderInMainPass:1 | true |
| bRenderInDepthPass:1 | true |
| bStaticLighting:1 | **true** |
| bCastShadow:1 | true |
| bRenderCustomDepth:1 | false |
| bUsesIndirectLightingCache:1 | **not set in ctor** (zero only via the union, [I]) |
| bAffectDynamicIndirectLighting:1 | true |
| bReverseCulling:1 | false |
| bDisableBackFaceCulling:1 | false |
| bCastShadowAsTwoSided:1 | false |
| bForceLODModel:1 | false |
| Mobility:4 | `EComponentMobility::Static` |
| bAnyMaterialHasWorldPositionOffset:1 | false |
| StencilWriteMask:4 | `EStencilMask::SM_Default` |
| BasePassPixelFormat:16 | `PF_Unknown` |
| Unused:18 | 0 |

Helpers: `SetMobility`, `GetMobility`, `IsMoveable` (Movable or Stationary), `SetStencilWriteMask`, `SetBassPixelFormat` (sic), `GetBassPixelFormat` (70-103).

Related types [R]:
- `FPSOPrecacheVertexFactoryData{VertexFactoryType, CustomDefaultVertexDeclaration}` (149-174). The `ENGINE_API` ctor taking an `FVertexDeclarationElementList` (153) is implemented in `PSOPrecache.cpp:147-152`.
- `FMaterialInterfacePSOPrecacheParams{Priority=Medium, MaterialInterface, PSOPrecacheParams, VertexFactoryDataList}` (186-192).
- `extern ENGINE_API void AddMaterialInterfacePSOPrecacheParamsToList(...)` (194). It merges entries with the same priority, material and params (`PSOPrecache.cpp:154-175`).

**Vertex declaration rule** (`RT/Renderer/Public/MeshPassProcessor.inl:278-288`) [R]:
- If `CustomDefaultVertexDeclaration` is null, the PSO uses `VFType->GetShaderPSOPrecacheVertexFetchElements(...)`, which is the manual-vertex-fetch (MVF) layout.
- So a null declaration is only correct when the VF supports MVF on the platform (`VertexFactory.h:426-429`), or for VFs whose declaration is identical either way (Niagara sprite and ribbon).

### (a) UStaticMesh resource precache

`RT/Engine/Private/StaticMesh.cpp:7656-7701`, inside `UStaticMesh::PostLoad` (7587) [R]:

```cpp
if (IsResourcePSOPrecachingEnabled() && GetRenderData() != nullptr) {
  ... bool bUseNanite = UseNanite(ShaderPlatform) && HasValidNaniteData();
  ... bAnySectionCastsShadows |= RenderSection.bCastShadow;  (all LODs)
  FPSOPrecacheParams PrecachePSOParams;
  PrecachePSOParams.bCastShadow = bAnySectionCastsShadows;
  PrecachePSOParams.SetMobility(EComponentMobility::Movable);
  CachingFactories.Add(bUseNanite ? &FNaniteVertexFactory::StaticType : &FLocalVertexFactory::StaticType);
  ... if (IsComponentPSOPrecachingEnabled()) MaterialInterface->PrecachePSOs(VFType, PrecachePSOParams);
```

- No custom vertex declaration is used.
- `r.PSOPrecache.Resources` defaults to **0** (`PSOPrecache.cpp:32-38`), so this path is off by default.

### (b) Static mesh, ISM and HISM components

**`UPrimitiveComponent::SetupPrecachePSOParams`** (`RT/Engine/Private/Components/PrimitiveComponent.cpp:5076-5101`) [R]:

```cpp
Params.bRenderInMainPass = bRenderInMainPass;
Params.bRenderInDepthPass = bRenderInDepthPass;
Params.bStaticLighting = HasStaticLighting();
Params.bUsesIndirectLightingCache = Params.bStaticLighting && IndirectLightingCacheQuality != ILCQ_Off && (!IsPrecomputedLightingValid() || GetLightmapType() == ELightmapType::ForceVolumetric);
Params.bAffectDynamicIndirectLighting = bAffectDynamicIndirectLighting;
Params.bCastShadow = CastShadow;
Params.bRenderCustomDepth = bRenderInDepthPass;   // "Custom depth can be toggled at runtime ... assume it might be needed"
Params.bCastShadowAsTwoSided = bCastShadowAsTwoSided;
Params.SetMobility(Mobility);
Params.SetStencilWriteMask(FRendererStencilMaskEvaluation::ToStencilMask(CustomDepthStencilWriteMask));
// bAnyMaterialHasWorldPositionOffset = any GetUsedMaterials() IsUsingWorldPositionOffset_Concurrent(GMaxRHIShaderPlatform)
```

**`UPrimitiveComponent::PrecachePSOs`** (5103-5127) [R]:
- Gate: `!FApp::CanEverRender() || !IsComponentPSOPrecachingEnabled()` returns early.
- Then: `SetupPrecachePSOParams`, `CollectPSOPrecacheData`, `PrecacheMaterialPSOs(list, MaterialPSOPrecacheRequestIDs, GraphEvents)`, `RequestRecreateRenderStateWhenPSOPrecacheFinished`.

Declarations (`PrimitiveComponent.h:974-988`, `public:` from 941) [R]:
- `ENGINE_API virtual void SetupPrecachePSOParams(FPSOPrecacheParams& Params);`
- `virtual void CollectPSOPrecacheData(const FPSOPrecacheParams& BasePrecachePSOParams, FMaterialInterfacePSOPrecacheParamsList& OutParams) {}`
- `ENGINE_API virtual void PrecachePSOs() override;`
- `ENGINE_API bool IsPSOPrecaching() const;`

**`FStaticMeshComponentHelper::CollectPSOPrecacheDataImpl`** (`RT/Engine/Public/StaticMeshComponentHelper.h:171-267`) [R]:
- Walks LODs from `GetMinLODIdx()` and builds per-material-index VF data.
- When the VF lacks MVF, it builds the vertex declaration from the LOD vertex buffers.
- It then sets:
  - `bCastShadow` = any section casts a shadow
  - `bReverseCulling |= IsReverseCulling() != (GetRenderMatrix().Determinant() < 0)`
  - `bForceLODModel = GetForcedLodModel() > 0`
- A null material is replaced with `UMaterial::GetDefaultMaterial(MD_Surface)`.
- Overlay materials are added with `bCastShadow=false` and the VF list of the first material.

VF choice:
- `UStaticMeshComponent` (`StaticMeshComponentHelper.h:269-298`): Nanite if `Nanite::FNaniteResourcesHelper::ShouldCreateNaniteProxy(Component, &NaniteMaterials)`, else `&FLocalVertexFactory::StaticType`.
- `UInstancedStaticMeshComponent` (`InstancedStaticMeshComponentHelper.h:50-84`; override at `InstancedStaticMesh.cpp:5392`, public at `InstancedStaticMeshComponent.h:616`): Nanite, else `&FInstancedStaticMeshVertexFactory::StaticType`.
  - Uses the dummy instance buffer when GPUScene is unavailable.
- **HISM has no override** (no hits in `HierarchicalInstancedStaticMesh.cpp` or `.h`), so it inherits ISMC behaviour.
- `USplineMeshComponent::CollectPSOPrecacheData` (`SplineMeshComponent.cpp:672-701`): Nanite or `FSplineMeshVertexFactory`.

Access [R]: `UStaticMeshComponent::CollectPSOPrecacheData` is `ENGINE_API ... override` but under `protected:` (`StaticMeshComponent.h:845-850`). It is still callable virtually through `UPrimitiveComponent*`.

Engine triggers for component precache [R]:
- `UStaticMeshComponent::OnRegister` (`StaticMeshComponent.cpp:871-887`)
- `SetStaticMesh` and others (2246, 2374, 3306)
- `UMeshComponent::SetMaterial` / `SetOverlayMaterial` (`MeshComponent.cpp:104, 267, 322`)
- `USkinnedMeshComponent` PostLoad, SetSkinnedAsset, SetRenderStatic (`SkinnedMeshComponent.cpp:672, 824, 2553, 5501`)
- `FComponentRecreateRenderStateContext` dtor (`ActorComponent.cpp:427-441`)

`ShouldCreateNaniteProxy` (`RT/Engine/Public/Rendering/NaniteResourcesHelper.h:111-150`) [R] uses `Component.GetScene()`, falling back to `GMaxRHIShaderPlatform`, and `Component.GetWorld()`. [I] It should therefore work on an unregistered transient component.

### (c) Skeletal and skinned meshes

**Resource path** (`RT/Engine/Private/SkinnedAsset.cpp:165-192`, in `USkinnedAsset::PostLoad` at 123) [R]:

```cpp
if (IsResourcePSOPrecachingEnabled() && GetResourceForRendering() != nullptr) {
  bool bCPUSkin = false; ... GetVertexFactoryTypesPerMaterialIndex(VFsPerMaterials, nullptr, MinLODIndex, bCPUSkin, FeatureLevel);
  FPSOPrecacheParams PrecachePSOParams; PrecachePSOParams.SetMobility(EComponentMobility::Movable);
  PrecachePSOParams.bCastShadow = bAnySectionCastsShadows;
  MaterialInterface->PrecachePSOs(VFsPerMaterial.VertexFactoryDataList, PrecachePSOParams, EPSOPrecachePriority::Medium, MaterialPSOPrecacheRequestIDs);
```

- Note that this resource path does **not** set `bSkinnedMesh`.

**Component path** (`USkinnedMeshComponent::CollectPSOPrecacheData`, `SkinnedMeshComponent.cpp:675-760`; public, `SkinnedMeshComponent.h:1316`) [R]:
- `bCPUSkin = bRenderStatic || ShouldCPUSkin()`
- `bCastShadow &= AnyRenderSectionCastsShadows(MinLOD)`
- `bSkinnedMesh = true`
- Adds `&FNaniteVertexFactory::StaticType` when Nanite skinning is valid.
- Adds overlays with `bCastShadow=false`.
- [R] `bSkinnedMesh` is only consumed by Nanite raster pipelines (`RT/Renderer/Private/Nanite/NaniteCullRaster.cpp:2977`).

**VF source** [R]:
- `ENGINE_API void USkinnedAsset::GetVertexFactoryTypesPerMaterialIndex(FPSOPrecacheVertexFactoryDataPerMaterialIndexList& OutList, USkinnedMeshComponent* SkinnedMeshComponent, int32 MinLODIndex, bool bCPUSkin, ERHIFeatureLevel::Type FeatureLevel);` (`SkinnedAsset.h:249`, public; impl `SkinnedAsset.cpp:195-253`).
- Uses `LODMaterialMap` remap.
- CPU skin: `FLocalVertexFactory`.
- GPU skin: `FSkeletalMeshObjectGPUSkin::GetUsedVertexFactoryData` (`SkeletalRenderGPUSkin.cpp:2305-2366`):
  - If not Inline, or the skin cache has ray tracing: `&FGPUSkinPassthroughVertexFactory::StaticType`.
  - If not MeshDeformer: cloth VF (`GetVertexFactoryDataCloth`, 2238) or `GetVertexFactoryData` (2120-2135).
    - `TGPUSkinVertexFactory<DefaultBoneInfluence>` or `<UnlimitedBoneInfluence>` depending on `SkinWeightVertexBuffer.GetBoneInfluenceType()`.
    - **Always with a custom vertex declaration** built from that LOD's buffers.
- [I] Consequence: GPU-skin PSOs **cannot** be reproduced via `GetVFByName` plus a null declaration. Use `GetVertexFactoryTypesPerMaterialIndex` or the component collector.

### (d) Niagara

**Asset path** (`NG/Niagara/Private/NiagaraSystem.cpp:1481-1505`) [R]:
- Collects with `EmitterInstance=nullptr` (no instance or user material overrides) over enabled emitters' enabled renderers.
- `UNiagaraRendererProperties::CollectPSOPrecacheData` (`NiagaraRendererProperties.cpp:1159-1186`; `NIAGARA_API`, public, `.h:345`):

```cpp
NewEntry.PSOPrecacheParams.SetMobility(EComponentMobility::Movable);
NewEntry.PSOPrecacheParams.bDisableBackFaceCulling = IsBackfaceCullingDisabled();
... bReverseCulling = false -> add; if (!bDisableBackFaceCulling && GNiagaraPSOPrecacheReverseCulling > 0) { bReverseCulling = true; add; }
```

- `fx.Niagara.PSOPrecache.ReverseCulling` defaults to 1 (36-41).
- All other fields stay at the defaults above, so `bStaticLighting=true` and `bCastShadow=true`.

**Component path** (`FNiagaraSystemInstanceController::CollectPSOPrecacheData`, `NiagaraSystemInstanceController.cpp:104-123`) [R]:
- **ignores `BasePrecachePSOParams`**. It calls the same per-renderer collector with the live emitter instance, so overrides apply.

Per-renderer [R]:

| Renderer | Material source | VF | Vertex declaration |
|---|---|---|---|
| Sprite (`NiagaraSpriteRendererProperties.cpp:134-151`) | `Material` only (not `MaterialUserParamBinding`) | `FNiagaraSpriteVertexFactory` | none ("same for MVF and non-MVF") |
| Ribbon (`NiagaraRibbonRendererProperties.cpp:211-228`) | `Material` | `FNiagaraRibbonVertexFactory` | none |
| Mesh (`NiagaraMeshRendererProperties.cpp:860-863, 908-953`) | Each static mesh's used materials + `ApplyMaterialOverrides(nullptr, ...)` | `FNiagaraMeshVertexFactory` | Custom declaration from MinLOD if no MVF |

### (e) Slate / UI materials

- **Auto-precache on load** [R]:
  - `UMaterial` PostLoad: `else if (IsDeferredDecal() || IsUIMaterial() || (MaterialDomain == MD_LightFunction) || IsPostProcessMaterial()) { FPSOPrecacheParams PSOPrecacheParams; UMaterialInterface::PrecachePSOs(&FLocalVertexFactory::StaticType, PSOPrecacheParams); }` (`Material.cpp:4793-4797`).
  - `MaterialInstance.cpp:3682-3686` does the same for decal, UI and post-process.
  - This is gated only by `IsPSOPrecachingEnabled` inside PrecachePSOs, **not** by `r.PSOPrecache.Resources`.
- **Collector** `FSlateMaterialPSOCollector` (`RT/SlateRHIRenderer/Private/SlateRHIRenderingPolicy.cpp:1838-1893`, registered for Deferred and Mobile at 1899-1900) [R]:
  - Acts only if `Material.IsUIMaterial()` and `r.PSOPrecache.SlateMaterials` (default 1, ReadOnly, 53-58).
  - Ignores the VF and params.
  - Covers `TriangleList`, instanced and non-instanced (Custom only), `MaskResource = nullptr` blend only.
  - RT = `GetDefaultBackBufferPixelFormat()`, 1 sample (`AddSlatePSOInitializer` 1731-1766).

### VF registered names (for `FVertexFactoryType::GetVFByName(FHashedName)`)

The macro stringizes the class token: `IMPLEMENT_VERTEX_FACTORY_TYPE` → `TEXT(#FactoryClass)` (`RT/RenderCore/Public/VertexFactory.h:577-584`); `IMPLEMENT_TEMPLATE_VERTEX_FACTORY_TYPE` likewise (588-595) [R].

| StaticType | Name string | Registration | Module | PSO-precache flag |
|---|---|---|---|---|
| `FLocalVertexFactory` | `"FLocalVertexFactory"` | `Engine/Private/LocalVertexFactory.cpp:577` | Engine | yes, plus MVF |
| `FGPUSkinPassthroughVertexFactory` | `"FLocalVertexFactory"`. Subclass of FLocalVertexFactory (`GPUSkinVertexFactory.h:784`) with no DECLARE/IMPLEMENT of its own; uses the parent StaticType | — | Engine | [I] from grep |
| `FInstancedStaticMeshVertexFactory` | `"FInstancedStaticMeshVertexFactory"` | `Engine/Private/InstancedStaticMesh.cpp:929` | Engine | yes, plus MVF |
| `FNaniteVertexFactory` | `"FNaniteVertexFactory"` | `Engine/Private/Rendering/NaniteResources.cpp:3535` | Engine | yes, plus MVF |
| `FSplineMeshVertexFactory` | `"FSplineMeshVertexFactory"` | `Engine/Private/Components/SplineMeshComponent.cpp:188` | Engine | — |
| `TGPUSkinVertexFactory<Default/Unlimited>` | `"TGPUSkinVertexFactoryDefault"` / `"TGPUSkinVertexFactoryUnlimited"` (macro at `GPUSkinVertexFactory.cpp:78-91`; instantiation at 1079-1085) | `GPUSkinVertexFactory.cpp:1079` | Engine | yes |
| `TGPUSkinAPEXClothVertexFactory<...>` | `"TGPUSkinAPEXClothVertexFactoryDefault"` / `"...Unlimited"` | `GPUSkinVertexFactory.cpp:1344` | Engine | yes |
| `FNiagaraSpriteVertexFactory` | `"FNiagaraSpriteVertexFactory"` | `NG/NiagaraVertexFactories/Private/NiagaraSpriteVertexFactory.cpp:177-183` | NiagaraVertexFactories | yes |
| `FNiagaraRibbonVertexFactory` | `"FNiagaraRibbonVertexFactory"` | `NiagaraRibbonVertexFactory.cpp:196-202` | NiagaraVertexFactories | yes |
| `FNiagaraMeshVertexFactory` | `"FNiagaraMeshVertexFactory"` | `NiagaraMeshVertexFactory.cpp:240-262` | NiagaraVertexFactories | yes, plus MVF |
| `TCk_Iskm_BatchedVertexFactory<4/8>` | `"CkIskmVF4"` / `"CkIskmVF8"` ([I]: token-pasted alias stringized) | `CK/CkIskmRendererVF/Public/CkIskmRendererVF/CkIskm_BatchedRenderResources.cpp:260-267` | CkIskmRendererVF (PostConfigInit) | **NO** |

---

## 3. Public, exported asset-level precache triggers

- **UStaticMesh: none.** The resource precache is inline in PostLoad (`StaticMesh.cpp:7656`). There are no `PrecachePSO` hits in `StaticMesh.h` [R].
- **USkinnedAsset / USkeletalMesh: none.** It is inline in PostLoad (`SkinnedAsset.cpp:165`) [R].
  - The only public building block is `ENGINE_API USkinnedAsset::GetVertexFactoryTypesPerMaterialIndex(...)` (`SkinnedAsset.h:249`).
- **UNiagaraSystem:** `NIAGARA_API void PrecachePSOs();` (`NG/Niagara/Classes/NiagaraSystem.h:390`, `public:` from 321) [R].
  - Gate (`NiagaraSystem.cpp:1483`): `if (HasLaunchedPSOPrecaching() || (!IsComponentPSOPrecachingEnabled() && !IsResourcePSOPrecachingEnabled())) return;`
  - It then calls `UFXSystemAsset::LaunchPSOPrecaching` (`ENGINE_API`, **protected**, `RT/Engine/Classes/Particles/ParticleSystem.h:146`; impl `ParticleSystem.cpp:37-76`).
  - That function only issues `PrecacheMaterialPSOs` if `IsComponentPSOPrecachingEnabled()` (40-43), but **always** sets `PSOPrecachingLaunched = true` (75). It is a one-shot per asset instance and cannot be re-run.
  - Public observers on `UFXSystemAsset` (`ParticleSystem.h:140-142`):
    - `HasLaunchedPSOPrecaching()`
    - `const FGraphEventRef& GetPrecachePSOsEvent() const` — nulled on the game thread when done (`ParticleSystem.cpp:47-72`)
    - `const TArray<FMaterialPSOPrecacheRequestID>& GetMaterialPSOPrecacheRequestIDs() const`
- **`r.PSOPrecache.NiagaraPrecachePSOAtAssetLoadingTime`** (default 1, ECVF_Default, `NiagaraSystem.cpp:196-200`) [R]:
  - At the end of `UNiagaraSystem::PostLoad` (1258), it does `if (GNiagaraPrecachePSOAtAssetLoadingTime) PrecachePSOs();` (1424-1427).
  - It is gated by the one-shot flag, `r.PSOPrecache.Components` (via `IsComponentPSOPrecachingEnabled`: CanEverRender, precaching enabled, `GPSOPrecacheComponents`, `!GIsEditor`; `PSOPrecache.cpp:99-102`), and `IsPSOPrecachingEnabled` (false under WITH_EDITOR).
  - **Separately**, GPU-sim emitters precache **compute** PSOs in `CacheFromCompiledData` (2408; call at 2474), run from `UpdateSystemAfterLoad` in cooked builds (1411-1415).
    - Gate: `fx.Niagara.Emitter.ComputePSOPrecacheMode` (default 1 = "if r.PSOPrecaching", `NiagaraEmitter.cpp:80-90`; impl 3484-3600).
    - The result is held in `PSOPrecacheCompletionEvent` (2524-2533), which `IsReadyToRun()` waits on (1678-1685).
- **Components:** `UPrimitiveComponent::PrecachePSOs()` (`ENGINE_API`, public) is gated by `IsComponentPSOPrecachingEnabled()`. `r.PSOPrecache.Resources` does not gate it.
- [I] The only way to precache "regardless of r.PSOPrecache.Resources" is to call `UMaterialInterface::PrecachePSOs` or `PrecacheMaterialPSOs` yourself. Their only gate is `IsPSOPrecachingEnabled` (material level). `UPrimitiveComponent::PrecachePSOs` and Niagara `LaunchPSOPrecaching` still require `r.PSOPrecache.Components` (default 1).

---

## 4. Tracking and control (`RT/Engine/Public/PSOPrecacheMaterial.h`)

### Signatures [R]

- 128: `extern ENGINE_API void PrecacheMaterialPSOs(const FMaterialInterfacePSOPrecacheParamsList& PSOPrecacheParamsList, TArray<FMaterialPSOPrecacheRequestID>& OutMaterialPSOPrecacheRequestIDs, FGraphEventArray& OutGraphEvents);`
  - Loops `MaterialInterface->PrecachePSOs(VFDataList, Params, Priority, OutIDs)` (`PSOPrecacheMaterial.cpp:705-714`).
- 133: `extern ENGINE_API FMaterialPSOPrecacheRequestID PrecacheMaterialPSOs(const FMaterialPSOPrecacheParams& MaterialPSOPrecacheParams, EPSOPrecachePriority Priority, FGraphEventArray& GraphEvents);`
  - Bypasses FMaterial bookkeeping. The ID is **not** added to `FMaterial::PrecachedPSORequestIDs`.
- 138: `extern ENGINE_API void PreloadMaterialShaderMap(const FMaterial* Material, FGraphEventArray& OutGraphEvents);`
- 143: `extern ENGINE_API void ReleasePSOPrecacheData(const TArray<FMaterialPSOPrecacheRequestID>& MaterialPSORequestIDs);`
- 148: `extern ENGINE_API void BoostPSOPriority(EPSOPrecachePriority NewPri, const TArray<FMaterialPSOPrecacheRequestID>& MaterialPSORequestIDs);`
- 153: `extern ENGINE_API void ClearMaterialPSORequests();` — game thread, check at 452.
- 158: `extern ENGINE_API FMaterialPSOPrecacheParams GetMaterialPSOPrecacheParams(FMaterialPSOPrecacheRequestID RequestID);`
- 163: `extern ENGINE_API FPSOPrecacheDataArray GetMaterialPSOPrecacheData(FMaterialPSOPrecacheRequestID RequestID);`
  - Returns real data only if `PSO_PRECACHING_TRACKING` = `!WITH_EDITOR && !UE_BUILD_SHIPPING && !UE_BUILD_TEST && UE_WITH_PSO_PRECACHING` (`PSOPrecacheValidation.h:15`). Otherwise it returns empty (`PSOPrecacheMaterial.cpp:763-773`).

RHI-level, all `RHI_API` in `PipelineStateCache.h:196-233` [R]:
- `IsPrecaching(const FPSOPrecacheRequestID&)`
- `BoostPrecachePriority`
- `NumActivePrecacheRequests()`
- `PrecachePSOsBoostToHighestPriority(bool bForceHighest)` — gated by `r.PSOPrecaching.PermitPriorityEscalation`
- `GetPSORuntimeCreationStats()` — returns `FPSORuntimeCreationStats{TotalPSOCreations, GraphicsPSOHitches, PreviouslyPrecachedPSOHitches, SuspectedUnhealthyDriverCachePSOHitches...}` (117-134). Usable as a post-warm-up hitch measurement.

### Threading

- [R] `BoostPSOPriority` and `ReleasePSOPrecacheData` use an internal RW lock and have no game-thread check (`PSOPrecacheMaterial.cpp:395-444`).
- [I] Call them from the game thread for ordering with PrecachePSOs.

### Release semantics [R]

- `ReleasePrecacheData` (395-409) does `check(ID != INDEX_NONE)`, `verify(MaterialPSORequestData.Remove(Params) == 1)`, then resets `MaterialPSORequests[ID]` to default params.
- There is **no refcount**. IDs are **shared** across all requesters of the same key (dedup returns the existing ID, 245).
- Ownership: `FMaterial` records every ID it issued (`MaterialShared.cpp:3182-3183`) and releases them all in `FMaterial::PrepareDestroy_GameThread` (2437-2448).
- No engine component ever calls `ReleasePSOPrecacheData`. Its only engine caller is that FMaterial path (grep).
- [I] **A warm-up must NOT call `ReleasePSOPrecacheData` on IDs obtained via `UMaterialInterface::PrecachePSOs`.**
  - The FMaterial would release the same ID again on destruction. `Remove` of the default params returns 0, so the `verify` fails: an assert in check-enabled builds.
  - Releasing also yanks the entry from under other requesters.
- Releasing never cancels compiles and never touches the RHI-level cache.
- Unloading the asset frees the request bookkeeping automatically. The compiled-PSO hash entries persist forever, as shown in §1.

### BoostPSOPriority [R]

- `BoostPriority` (411-444) returns if `ID >= Num`, if the entry is gone, if `NewPri <= current`, or if Completed.
- Otherwise it sets Priority and boosts still-compiling PSOs.
- An initial request made with `High` is auto-boosted once collection completes (`MarkCollectionComplete` 388-392).
- At the RHI level, `NumActivePrecacheRequests` depends on `r.PSOPrecaching.WaitForHighPriorityRequestsOnly` (0 = all; 1 = High+Highest; 2 = Highest; `PipelineStateCache.cpp:198-205, 2562-2574`).

### KeepInMemoryUntilUsed and the cap

`PipelineStateCache.cpp` [R]:
- `GPSOPrecacheKeepInMemoryUntilUsed = 2`, ECVF_ReadOnly (375-386). The help text says "0 = off (default)", but the code default is 2.
- It is effective only on NVIDIA or Qualcomm: `ShouldKeepPrecachedPSOsInMemory()` = `GPSOPrecacheKeepInMemoryUntilUsed && (IsRHIDeviceNVIDIA() || IsRHIDeviceQualcomm())` (404-407).
- `r.PSOPrecache.KeepInMemoryGraphicsMaxNum` = 2000 (388-394) and `...ComputeMaxNum` = 1000 (396-402). Both are **ECVF_RenderThreadSafe, not ReadOnly**, and are re-applied every frame by `PipelineStateCache::FlushResources` (3559-3560 calling `SetMaxInMemoryPSOs`, 2306-2326).
- Eviction is FIFO in `TryAddNewState` (2415-2432): at the cap, the oldest index is popped and queued in `PrecachedPSOsToCleanup`. `ProcessDelayedCleanup` (2617-2654) deletes the `FGraphicsPipelineState`.
- The hash entry and the Succeeded state remain, so a later identical request is a no-op and draw-time state reads `Complete`.
- On other vendors (AMD/Intel), every precached PSO object is queued for cleanup immediately on completion (`PrecacheFinished` 2592-2595).
- With mode 2, a PSO used for rendering is queued for cleanup (`MarkPSOAsUsed` 2657-2668; call 3392-3399).

Implication:
- [R] Help text (379-380): keeping in memory exists "to speed up the re-creation of precached PSOs ... and avoid small hitches".
- [I] A warm-up of more than 2000 graphics PSOs reverts the oldest to "driver cache only". Draw-time creation of those becomes a synchronous `RHICreateGraphicsPipelineState` that should hit the driver cache: a small cost, not a full compile.
- [I] That cost is visible as `PreviouslyPrecachedPSOHitches` in `GetPSORuntimeCreationStats()`.
- [I] Options: order the manifest so first-used content is precached last, or raise `KeepInMemoryGraphicsMaxNum` at runtime (it is mutable).

---

## 5. Runtime Asset Registry in cooked builds

- **Module:** `"AssetRegistry"` (`RT/AssetRegistry/`). `FARFilter` and `FAssetData` live in CoreUObject (`RT/CoreUObject/Public/AssetRegistry/ARFilter.h`) [R].
- **Access** [R]:
  - `IAssetRegistry::Get()` / `GetChecked()` (`RT/AssetRegistry/Public/AssetRegistry/IAssetRegistry.h:266-273`)
  - `FAssetRegistryModule::GetRegistry()` (`AssetRegistryModule.h:51`)
- **Signatures** [R]:
  - `virtual bool GetAssets(const FARFilter& Filter, TArray<FAssetData>& OutAssetData, bool bSkipARFilteredAssets=true) const = 0;` (361)
  - `GetAssetsByPath(FName PackagePath, TArray<FAssetData>&, bool bRecursive = false, ...)` (309)
  - `GetAssetsByClass(FTopLevelAssetPath ClassPathName, TArray<FAssetData>&, bool bSearchSubClasses = false)` (333)
  - `EnumerateAssets(const FARFilter&, TFunctionRef<bool(const FAssetData&)>, ...)` (392)
  - `WaitForPremadeAssetRegistry()` (830)
  - `IsLoadingAssets()` (1019)
- **FARFilter fields** (`ARFilter.h:34-91`) [R]: `PackageNames`, `PackagePaths` (FName, e.g. `"/Game/X"`, `"/CkFoundation/CkUsf/GeneratedLooks"`), `SoftObjectPaths`, `ClassPaths` (`TArray<FTopLevelAssetPath>`, e.g. `UMaterialInterface::StaticClass()->GetClassPathName()`), `TagsAndValues`, `RecursiveClassPathsExclusionSet`, `bRecursivePaths=false`, `bRecursiveClasses=false`, `bIncludeOnlyOnDiskAssets=false`.
- **Cooked behaviour:**
  - [R] `BaseEngine.ini` `[AssetRegistry]` (2927+) has `bSerializeAssetRegistry=true` (2959). Tags are filtered via `+CookedTagsBlacklist` (AssetImportData, Blueprint FiB) and `bFilterAssetDataWithNoTags=false` (2964).
  - [R] The premade registry is loaded in `LoadPremadeAssetRegistry` (`AssetRegistry.cpp:970-1006`; ini read 1597).
  - [R] Neither BusterBlock nor CkPlugins2 overrides `[AssetRegistry]` (grep).
  - [I] Caveats:
    - (1) It only contains cooked packages. Content not reached by the cook (for example CkUsf looks without `DirectoriesToAlwaysCook`, `BB/Config/DefaultGame.ini:127-130`) is absent.
    - (2) `bRecursiveClasses` works for native classes (UMaterial, UMaterialInstanceConstant).
    - (3) AngelScript `asset` objects (`/Script/AngelscriptAssets.*`) are not on-disk packages and will not appear.
    - (4) Call `WaitForPremadeAssetRegistry()` before an early-boot query.

---

## 6. Ck-side renderers

Grep of `PrecachePSOs|CollectPSOPrecacheData|IPSOCollector|PSOPrecache|SetupPrecachePSOParams|SupportsPSOPrecaching` across `CK/` and `Plugins/CkGameplayDebugger`: **zero code hits** [R]. Only comments and docs exist. `CkLoadingScreen_Subsystem.cpp:717-719` calls only `FShaderPipelineCache::SetBatchMode`.

| Ck path | What renders | VF | Precache status |
|---|---|---|---|
| CkIskmRenderer Plan-1 | Pooled `USkeletalMeshComponent` (BaseSKMC plus leader-pose submeshes) | TGPUSkin{Default/Unlimited}, cloth, passthrough(=Local), Nanite if Nanite-skinned | Engine-covered via `USkinnedMeshComponent` triggers (§2b/c). Ck adds nothing [R] |
| CkIskmRenderer Plan-2 | `UCk_Iskm_BatchedClusterComponent : UPrimitiveComponent` (`CK/CkIskmRenderer/.../CkIskm_BatchedClusterComponent.h:18`), proxy `FCk_Iskm_BatchedClusterProxy` with `bStaticRelevance=true` (`...ClusterProxy.cpp:182-192`) and `DrawStaticElements` with `MeshBatch.VertexFactory = VF` (313-349). Includes the highlight cluster (custom-depth only) | `CkIskmVF4` / `CkIskmVF8`. Flags = UsedWithMaterials, SupportsDynamicLighting, SupportsPrecisePrevWorldPos, SupportsPrimitiveIdStream (`CkIskm_BatchedRenderResources.cpp:251-258`). `MANUAL_VERTEX_FETCH 0` (~248). `ShouldCompilePermutation` needs `bIsUsedWithSkeletalMesh` (~230) | **GUARANTEED MISS.** No `CollectPSOPrecacheData` override, and the VF lacks `SupportsPSOPrecaching`, so even an explicit `UMaterialInterface::PrecachePSOs(CkIskmVF4, ...)` is skipped (`MaterialShared.cpp:3164-3167`). [I] Fixing it needs the flag, plus `GetPSOPrecacheVertexFetchElements` or a custom declaration (no MVF), plus a collector |
| CkIsmRenderer | `UInstancedStaticMeshComponent` / `UHierarchicalInstancedStaticMeshComponent` added via `Request_AddNewActorComponent` (`CkIsmRenderer_Processor.cpp:56-80`; init at `.h:126-161`: mobility, CastShadow, `SetStaticMesh`, `NumCustomDataFloats`, `SetMaterial`). Shadow ISMs (`CkIsmSubsystem.cpp:440-490`: `bRenderInMainPass=false`, `SetRenderCustomDepth(true)`, stencil value, `CastShadow=false`, `RegisterComponent`) | `FInstancedStaticMeshVertexFactory` or Nanite | Engine-covered (OnRegister/SetStaticMesh/SetMaterial precache). Ck adds nothing [R] |
| CkVat | Runs on CkIsmRenderer ISMs with a WPO material | ISM VF | Engine-covered. WPO is captured by `bAnyMaterialHasWorldPositionOffset` [R] |
| CkUsf looks `M_CkUsf_Look_*` | `UMaterial`s generated **in the editor** by `CkUsfEditor` (`CkUsf_Generator.cpp:438-505`) into `/CkFoundation/CkUsf/GeneratedLooks` (`CkUsf_LookDefinition_Naming.h:7-27`; 91 files on disk). MD_Surface always sets `bUsedWithInstancedStaticMeshes` and `bUsedWithSkeletalMesh` (502-505). Nanite and Niagara usage come from per-definition flags (475-481). Applied at runtime as MIDs (`Create_MID_ForLook`, `CkUsf_Utils.cpp:68`) | Whatever the consuming component uses: ISM, SKM, batched Plan-2 (CharacterMasterBatched → CkIskmVF, a miss), Niagara | Covered only transitively by the consumer's component (MID → parent FMaterial). Post-process looks get UMaterial PostLoad precache (`Material.cpp:4793-4797`) plus the engine `PostProcessMaterialPSOCollector` |
| CkUsfRenderer outline | Compute global shaders `FOutlineSeedCS`, `ReduceCS`, `CompositeCS` via `FComputeShaderUtils::AddPass` in an `FWorldSceneViewExtension` (`CkUsf_Outline_Renderer.cpp:32-169`) | n/a | No collector. Compute PSO is created on first dispatch. `r.PSOPrecache.GlobalShaders` = 0, ReadOnly (`PSOPrecache.cpp:16-22`) → **miss** [I] |
| CkPixelArtRenderer | Global PS `FCk_PixelArt_UpscalePS` (`CkPixelArtRenderer_UpscaleShader.cpp:7`) via `AddDrawScreenPass` (`..._Upscaler.cpp:178`) in an `FSceneViewExtensionBase` | n/a | No `FRegisterGlobalPSOCollectorFunction` → **guaranteed miss** on first frame used [R by grep]/[I] |
| CkPmg | `UProceduralMeshComponent` (`CkPmg_Fragment.h:48`; created and `SetMaterial` at `CkPmg_Processor_Donut.cpp:195-263`). The engine proxy uses `FLocalVertexFactory` and `GetDynamicMeshElements` (`Engine/Plugins/Runtime/ProceduralMeshComponent/.../ProceduralMeshComponent.cpp:50, 309`) | FLocalVertexFactory | **Miss.** `UProceduralMeshComponent` has no `CollectPSOPrecacheData`, so `UMeshComponent::SetMaterial` → `PrecachePSOs()` collects nothing [R]. [I] Coverable only by an explicit material × LocalVF precache with matching params |

---

## 7. CkResourceLoader for a non-entity consumer

- **Signature** (`CK/CkResourceLoader/Public/CkResourceLoader/CkResourceLoader_Utils.h:54-64`) [R], C++-only static on `UCk_Utils_ResourceLoader_UE`:
  ```cpp
  static auto RequestLoad_RootedBatch(FName InConsumerId, const TArray<FSoftObjectPath>& InSoftPaths) -> FCk_ResourceLoader_RootedAssetBatch;
  ```
- **Implementation** (`CkResourceLoader_Utils.cpp:58-107`) [R]:
  - Ensures the list is non-empty with no null paths. If that fails, it returns a requested batch with no handle (Ready, and HasFailed).
  - It resolves the policy, then calls `UAssetManager::GetStreamableManager().RequestSyncLoad(paths)` or `.RequestAsyncLoad(paths)`.
  - It uses default priority and no callback.
  - **No world, entity, or handle is involved.**
- **Struct** (`CkResourceLoader_Fragment_Data.h:154-182`) [R]: a plain non-USTRUCT `struct CKRESOURCELOADER_API FCk_ResourceLoader_RootedAssetBatch`:
  - `Get_IsRequested()`, `Get_IsReady()`, `Get_HasFailed()`, `Get_ResolvedObject(const FSoftObjectPath&)`, `Get_RequestedPaths()`.
  - Private members `_Requested`, `_RequestedPaths`, `TSharedPtr<FStreamableHandle> _StreamableHandle`.
  - The comment says the streamable handle is the GC root, and resetting the struct releases the assets.
- **Polling semantics** (`CkResourceLoader_Fragment_Data.cpp:114-169`) [R]:
  - Ready = `!handle.IsValid() || HasLoadCompleted() || WasCanceled()`.
  - Failed = handle invalid, or canceled, or completed with any path unresolved.
  - `Get_ResolvedObject` returns null until completed.
- **Progress without a tick:**
  - [R] Nothing in the batch ticks.
  - [I] `FStreamableManager` async loads progress on the async-loading thread and complete during engine-tick `ProcessAsyncLoading`. Completion does not need a world, but **someone must poll**.
  - A `UGameInstanceSubsystem` needs `FTickableGameObject` or `FTSTicker`. Precedent: `UCk_LoadingScreen_Subsystem_UE : public UGameInstanceSubsystem, public FTickableGameObject` (`CkLoadingScreen_Subsystem.h:42-46`).
- **Policy keying** [R]:
  - `UCk_ResourceLoader_ProjectSettings_UE` (base `UCLASS(Abstract, DefaultConfig, Config = CkFoundation)`, `CkSettings/.../CkProjectSettings.h:9-10`).
  - `_DefaultLoadingPolicy = Async` and `TMap<FName, ECk_ResourceLoader_LoadingPolicy> _PerConsumerLoadingPolicyOverrides` (`Settings/CkResourceLoader_Settings.h:22-35`).
  - Lookup is an exact FName match on the ConsumerId (`CkResourceLoader_Settings.cpp:24-35`).
  - ini section: `[/Script/CkResourceLoader.Ck_ResourceLoader_ProjectSettings_UE]` (`BB/Config/DefaultCkFoundation.ini:18`).
- **Existing call sites outside a processor:**
  - Utils boundary (still entity-owned): `CkVat/Public/CkVat/Proxy/CkVatProxy_Utils.cpp:45-46` (`Current._CollectionPinBatch = ...RequestLoad_RootedBatch(ck_vat_proxy_utils::PinConsumerId, {...})`), `CkIskmProxy_Utils.cpp:478/606/690/794`, `CkPmg_Utils_Donut.cpp:53/130/199`, `CkRenderTarget_Utils.cpp:238-392`, `CkMontagePlayer_Utils.cpp:181/372` [R].
  - **No holder outside an entity fragment exists.** Every `FCk_ResourceLoader_RootedAssetBatch` member is in a `*_Fragment*.h` [R grep].
- **Loading-screen hook for gating** [R]: `UCk_LoadingScreen_Subsystem_UE::Register_LoadingProcessor(TScriptInterface<ICk_LoadingProcess>)` / `Unregister_LoadingProcessor` (`CkLoadingScreen_Subsystem.h:105-112`). The interface is `ICk_LoadingProcess::Get_ShouldShowLoadingScreen(FString& OutReason) const` (`LoadingProcess/CkLoadingProcess_Interface.h:22-40`).

---

## 8. AngelScript `asset X of U...` for a C++ UDataAsset

- **Rules** (`CK/../Script/ARCHITECTURE.md:539-608`) [R]:
  - Creates an instance at script load with object path `/Script/AngelscriptAssets.<Name>`.
  - Initializer-block assignments of public UPROPERTY fields; "private BlueprintReadWrite fields work the same way".
  - No `default` keyword, no function definitions inside the block.
  - Editor-only types need `#if EDITOR`.
  - Files with heavyweight assets use the `_Assets.as` suffix.
- **C++ precedents** [R]:
  - `UCLASS(BlueprintType) class CKUSF_API UCkUsf_LookDefinition : public UDataAsset` with `public:` `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")` fields (`CK/CkUsf/Public/CkUsf/LookDefinition/CkUsf_LookDefinition.h:164-172`). Authored in `Script/CkUsf/CkUsf_Looks_Assets.as`.
  - `UCk_IsmRenderer_Data : UCk_DataAsset_PDA` with **private** `UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))` fields, including `TSoftObjectPtr<UStaticMesh> _MeshSoft` (`CkIsmRenderer_Fragment_Data.h:132-206`).
  - These are assigned from AS in `BB/Script/Common/BB_CommonRenderers_Assets.as:5-25` (`_MeshSoft = assets::CardboardBox_BB_SM(); _Mobility = ECk_Mobility::Movable; _LightingInfo._CastShadows = ...`).
  - [I] So BlueprintReadOnly + AllowPrivateAccess is writable in asset initializers in this fork.
- **Soft refs:** use `TSoftObjectPtr<UMaterialInterface>`, `TSoftObjectPtr<UStaticMesh>`, `TSoftObjectPtr<USkeletalMesh>`, `TSoftObjectPtr<UNiagaraSystem>` (or `TArray<...>`). The generated accessors return them [R]:
  - `BB/Script/Generated/BusterBlockAssets.as:6-9` (namespace `assets`, "Soft references"), for example 6701: `TSoftObjectPtr<UStaticMesh> CardboardBox_BB_SM()`.
  - `assets::load::X()` is the blocking variant and ensures if called before `IsEngineSafeForBlockingLoads()` (71287-71292).
  - `BB/Script/CONTINUATION_PROMPT_AssetsLoadRefactor.md:139`: asset blocks run before blocking loads are safe, so use soft refs.
- **BB example:** `UBb_MapDefinition` is an **AS-defined** class (`BB/Script/GameFlow/BB_GameFlow_MapDefinition.as:6-35`) with `TSoftObjectPtr<UWorld> Map`, `TArray<TSoftObjectPtr<UObject>> PreloadAssets`, and assets at 37-55. Its doc comment (24-26) already does a UI warm-up by constructing widgets "so its Slate tree builds and its UI PSOs compile" [R].
- **Referencing one asset from another** [R]:
  - Hard field: `_AnimCollection = bb::Asset_Npc_AnimCollection;` (`BB/Script/ECS/AmbientNpc/BB_AmbientNpc_Assets.as:594-598`, into `TObjectPtr`).
  - Soft field: `TSoftObjectPtr<UCk_IsmRenderer_Data>(FSoftObjectPath(bb::Asset_Shelf_OutlineRenderer))` (`BB/Script/Dev/Tests/SaveLoad/BB_ShelfRestoreProbe_EntityScript.as:59-60`; also `BB_AmbientChatter_Utils.as:76-77`).
- **Cook caveat:**
  - [R] AS is not cooked content (`ck-game-build-and-cook/SKILL.md:13`).
  - [I] Soft refs held only by AS assets are invisible to the cooker. The referenced packages must be cooked by another route (`+DirectoriesToAlwaysCook` or real references), exactly as BB does for GeneratedLooks (`DefaultGame.ini:127-130`).

---

## Open risks (things that make explicit warm-up inaccurate)

1. **Editor and PIE cannot validate any of this.** `IsPSOPrecachingEnabled()` is false under WITH_EDITOR, including `UnrealEditor -game`. Request-data introspection (`GetMaterialPSOPrecacheData`) exists only in Development cooked builds (`PSOPrecacheValidation.h:15`) [R].
2. **Params mismatch.** Component precache derives params from live state: Mobility, `HasStaticLighting`, CastShadow, `bRenderInMainPass/DepthPass`, stencil write mask, reverse culling from the transform determinant, `ForcedLodModel`, WPO, overlays (§2b) [R].
   - A manifest "declared usage" that differs produces different PSOs.
   - Defaults are `Static` + `bStaticLighting=true`, while asset-level engine paths use `Movable`.
   - [I] The most accurate path is to build a transient, unregistered component of the real class with the real mesh and materials, call `SetupPrecachePSOParams` + `CollectPSOPrecacheData` through `UPrimitiveComponent*`, then `PrecacheMaterialPSOs`. This is only partially safe: Nanite audit uses `GetWorld()`.
3. **Lightmap policy.** `r.PSOPrecache.LightMapPolicyMode` = 1 (ReadOnly) precaches only `LMP_NO_LIGHTMAP` (`RT/Renderer/Private/BasePassRendering.cpp:95-102`) [R]. Statically lit meshes with real lightmaps will miss regardless.
4. **Quality and feature level.** Only the material resource for the **current** `MaterialQualityLevel` is precached (`Material.cpp:2967-2972`, `MaterialInstance.cpp:2881-2886`). A scalability change requires a re-warm [R]. `ClearMaterialPSORequests` (cvar changes) invalidates the bookkeeping.
5. **Render-target and scene-texture config.** Collection uses a default `FSceneTexturesConfig` (FeatureLevel only; no multiview, no alpha-channel config) (`MaterialShader.cpp:3436-3442`) [R]. Slate uses the back-buffer format, 1 sample (`SlateRHIRenderingPolicy.cpp:1749-1751`). UI rendered into widget render targets or retainers with other formats will miss [I].
6. **Custom depth and stencil.** `bRenderCustomDepth` and the stencil mask are part of params. The Ck outline, cel and mask features toggle custom depth on SKMCs at runtime and use dedicated shadow ISMs with `bRenderInMainPass=false` [R §6]. The engine heuristic sets `bRenderCustomDepth = bRenderInDepthPass` (`PrimitiveComponent.cpp:5085`).
7. **Shadow passes.** Only covered if `bCastShadow` is true in params. `bCastShadowAsTwoSided` and `bReverseCulling` change the rasterizer state [R].
8. **Material usage flags.** If a VF's mesh shader map is absent (usage flag missing in the cooked material), collection returns nothing silently (`MaterialShader.cpp:3430-3434`) [R]. At draw time the engine falls back to the default material.
9. **Per-level and per-instance overrides.** Component `OverrideMaterials`, Niagara user-param material bindings (asset path passes `EmitterInstance=nullptr`), and MIDs with static switch overrides (static permutation resource) are separate FMaterials and are not covered by precaching the parent [R §1, §2d].
10. **Vertex declarations.** A null `CustomDefaultVertexDeclaration` is only correct for MVF-capable VFs (`MeshPassProcessor.inl:278-288`) [R]. GPU-skin VFs always need the per-LOD declaration (`SkeletalRenderGPUSkin.cpp:2120-2135`).
11. **Nanite vs fallback.** The VF choice depends on `UseNanite(platform)`, valid Nanite data, the material audit and masking allowance [R]. A wrong guess precaches the wrong VF.
12. **Non-required PSOs** are compiled but not represented in the returned events, so "complete" can be declared before they finish [R].
13. **The 2000-PSO in-memory cap** (NVIDIA/Qualcomm only) evicts the oldest FIFO. Other vendors keep none in memory; draw-time creation then relies on the driver cache [R §4, I for the cost].
14. **Ck guaranteed misses with no material-level fix:**
    - ISKM Plan-2 VF (no `SupportsPSOPrecaching`; MVF off)
    - CkPixelArt upscaler global PS
    - CkUsf outline compute shaders
    - CkPmg procedural meshes (no collector)

    [R §6]
15. **Engine auto-precache already runs** on component register and set-mesh/material, and on UI, post-process and Niagara asset load. With `r.PSOPrecache.ProxyCreationWhenPSOReady=1` (default, ReadOnly, `PSOPrecache.cpp:41-48`), cold PSOs on engine components cause **delayed proxy creation (pop-in)** rather than hitches [R]. Explicit warm-up mainly removes pop-in and covers asset-only paths.
16. **Dedup returns `INDEX_NONE`** for already-complete requests, and IDs are shared across requesters. Never use IDs for ownership or release (§4) [R].
17. **ResourceLoader `Get_IsReady()` returns true for an invalid handle** (empty or rejected batch), which `Get_HasFailed()` also reports. Check both [R].
