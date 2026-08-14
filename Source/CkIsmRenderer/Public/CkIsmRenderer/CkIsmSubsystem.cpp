#include "CkIsmSubsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcsExt/EntityScript/CkEntityScript_WithActor_Utils.h"

#include "CkIsmRenderer/CkIsmRenderer_EntityScript.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_TransientFactory.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_Utils.h"

#include "CkUsf/Outline/CkUsf_OutlinePreset.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "MaterialShared.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_IsmRenderer_Actor_UE::
    ACk_IsmRenderer_Actor_UE()
{
    _RootNode = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    RootComponent = _RootNode;

#if WITH_EDITORONLY_DATA
    bListedInSceneOutliner = false;
#endif
}

auto
    ACk_IsmRenderer_Actor_UE::
    DoInitialize()
    -> void
{
    if (_Initialized)
    { return; }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    // The registry may not be ready yet — keep _Initialized false so DoInitialize retries later.
    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(World);
    if (ck::Is_NOT_Valid(TransientEntity))
    { return; }

    UCk_Utils_EntityScript_WithActor_UE::Request_SpawnEntityScript_OnActor(
        this, UCk_EntityScript_IsmRenderer_UE::StaticClass());

    _Initialized = true;
}

auto
    ACk_IsmRenderer_Actor_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();
    DoInitialize();
}

#if WITH_EDITOR
auto
    ACk_IsmRenderer_Actor_UE::
    IsSelectionChild() const
    -> bool
{
    return _EditorSelectionOwner.IsValid();
}

auto
    ACk_IsmRenderer_Actor_UE::
    GetSelectionParent() const
    -> AActor*
{
    return _EditorSelectionOwner.Get();
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);
    DoSweepLeakedRenderers();
}

auto
    UCk_IsmRenderer_Subsystem_UE::
    Deinitialize()
    -> void
{
    UCk_Utils_IsmRenderer_TransientFactory_UE::ClearCache(GetWorld());

    Super::Deinitialize();
}

auto
    UCk_IsmRenderer_Subsystem_UE::
    DoSweepLeakedRenderers()
    -> void
{
    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    auto Leaked = TArray<ACk_IsmRenderer_Actor_UE*>{};
    for (TActorIterator<ACk_IsmRenderer_Actor_UE> It(World); It; ++It)
    {
        Leaked.Add(*It);
    }

    for (auto* Actor : Leaked)
    {
        if (ck::IsValid(Actor))
        {
            Actor->Destroy();
        }
    }

    if (Leaked.Num() > 0 && World->WorldType == EWorldType::Editor)
    {
        if (auto* Level = World->PersistentLevel.Get())
        {
            Level->MarkPackageDirty();
        }
    }
}

auto
    UCk_IsmRenderer_Subsystem_UE::
    DoesSupportWorldType(
        const EWorldType::Type WorldType) const
    -> bool
{
    return Super::DoesSupportWorldType(WorldType) || WorldType == EWorldType::Editor;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    DoSpawn_IsmRendererActor(
        const UCk_IsmRenderer_Data* InDataAsset,
        const FString& InNameSuffix)
    -> ACk_IsmRenderer_Actor_UE*
{
    const auto& DescriptiveName = FName{*ck::Format_UE(TEXT("IsmRenderer [{}][{}][Shadows:{}][{}][Collision:{}]{}"),
        InDataAsset->Get_Mesh(),
        InDataAsset->Get_Mobility(),
        InDataAsset->Get_LightingInfo().Get_CastShadows(),
        InDataAsset->Get_RenderPolicy(),
        InDataAsset->Get_PhysicsInfo().Get_Collision(),
        InNameSuffix)};

    return Cast<ACk_IsmRenderer_Actor_UE>(UCk_Utils_Actor_UE::Request_SpawnActor
    (
        FCk_Utils_Actor_SpawnActor_Params{GetWorld(), ACk_IsmRenderer_Actor_UE::StaticClass()}
        .Set_NonUniqueName(DescriptiveName)
        .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
        [&](AActor* InActor)
        {
            const auto& NewIsmRendererActor = Cast<ACk_IsmRenderer_Actor_UE>(InActor);
            NewIsmRendererActor->_RenderData = InDataAsset;

            NewIsmRendererActor->SetFlags(RF_Transient);

            // Editor worlds never fire BeginPlay; DoInitialize is idempotent for runtime.
            NewIsmRendererActor->DoInitialize();
        }
    ));
}

auto
    UCk_IsmRenderer_Subsystem_UE::
    GetOrCreate_IsmRenderer(
        const UCk_IsmRenderer_Data* InDataAsset)
    -> ACk_IsmRenderer_Actor_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDataAsset),
        TEXT("Trying to GetOrCreate an IsmRenderer from an INVALID Data Asset"))
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InDataAsset->Get_Mesh()),
        TEXT("Trying to GetOrCreate an IsmRenderer with an INVALID Mesh on Data Asset [{}]"), InDataAsset)
    { return nullptr; }

    if (auto Found = _IsmRenderers.Find(InDataAsset);
        Found != nullptr && ck::IsValid(*Found))
    { return *Found; }

    const auto& SpawnedIsmRendererActor = DoSpawn_IsmRendererActor(InDataAsset, FString{});

    const auto IsmRenderer = _IsmRenderers.Add(InDataAsset, SpawnedIsmRendererActor);

    return IsmRenderer;
}

#if WITH_EDITOR
auto
    UCk_IsmRenderer_Subsystem_UE::
    GetOrCreate_IsmRenderer_ForEditorSelectionOwner(
        const UCk_IsmRenderer_Data* InDataAsset,
        AActor* InSelectionOwner)
    -> ACk_IsmRenderer_Actor_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDataAsset),
        TEXT("Trying to GetOrCreate a per-owner IsmRenderer from an INVALID Data Asset"))
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InDataAsset->Get_Mesh()),
        TEXT("Trying to GetOrCreate a per-owner IsmRenderer with an INVALID Mesh on Data Asset [{}]"), InDataAsset)
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InSelectionOwner),
        TEXT("Trying to GetOrCreate a per-owner IsmRenderer for an INVALID SelectionOwner (Data Asset [{}])"), InDataAsset)
    { return nullptr; }

    const auto Key = FPerOwnerRendererKey{InDataAsset, InSelectionOwner};

    if (const auto* MaybeFound = _PerOwnerIsmRenderers.Find(Key);
        MaybeFound != nullptr && MaybeFound->IsValid())
    { return MaybeFound->Get(); }

    const auto& NameSuffix = ck::Format_UE(TEXT("[Owner:{}]"), InSelectionOwner->GetActorNameOrLabel());
    const auto& SpawnedIsmRendererActor = DoSpawn_IsmRendererActor(InDataAsset, NameSuffix);

    if (ck::Is_NOT_Valid(SpawnedIsmRendererActor))
    { return nullptr; }

    SpawnedIsmRendererActor->_EditorSelectionOwner = InSelectionOwner;

    UCk_Utils_EditorSelectionOwner_UE::RegisterProxyActor(InSelectionOwner, SpawnedIsmRendererActor);

    _PerOwnerIsmRenderers.Add(Key, SpawnedIsmRendererActor);

    return SpawnedIsmRendererActor;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    DoFind_IsmComponentOnRenderer(
        const ACk_IsmRenderer_Actor_UE* InRenderer,
        const UCk_IsmRenderer_Data* InRendererData)
    -> UInstancedStaticMeshComponent*
{
    if (InRendererData->Get_RenderPolicy() == ECk_Ism_RenderPolicy::ISM)
    { return InRenderer->FindComponentByClass<UInstancedStaticMeshComponent>(); }

    return InRenderer->FindComponentByClass<UHierarchicalInstancedStaticMeshComponent>();
}

auto
    UCk_IsmRenderer_Subsystem_UE::
    FindOrCache_IsmComponent(
        const UCk_IsmRenderer_Data* InRendererData,
        const TWeakObjectPtr<AActor>& InEditorSelectionOwner)
    -> TWeakObjectPtr<UInstancedStaticMeshComponent>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRendererData),
        TEXT("InRendererData [{}] is NOT valid"), InRendererData)
    { return {}; }

#if WITH_EDITOR
    if (NOT InEditorSelectionOwner.IsExplicitlyNull())
    {
        const auto Key = FPerOwnerRendererKey{InRendererData, InEditorSelectionOwner};

        if (const auto& MaybeFound = _PerOwnerIsmComponentCache.Find(Key);
            ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*MaybeFound))
        { return *MaybeFound; }

        // Once the owner actor is gone the cached entry above is the only valid resolution — the
        // shared component never held this owner's instance indices.
        auto* SelectionOwner = InEditorSelectionOwner.Get();

        if (ck::Is_NOT_Valid(SelectionOwner))
        { return {}; }

        const auto NewRenderer = GetOrCreate_IsmRenderer_ForEditorSelectionOwner(InRendererData, SelectionOwner);

        CK_ENSURE_IF_NOT(ck::IsValid(NewRenderer),
            TEXT("Failed to GetOrCreate per-owner ISM Renderer Actor for [{}]"), InRendererData)
        { return {}; }

        auto* StaticMeshComponent = DoFind_IsmComponentOnRenderer(NewRenderer, InRendererData);

        if (ck::Is_NOT_Valid(StaticMeshComponent))
        { return {}; }

        return _PerOwnerIsmComponentCache.Add(Key, StaticMeshComponent);
    }
#endif

    if (const auto& MaybeFound = _IsmComponentCache.Find(InRendererData);
        ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*MaybeFound))
    { return *MaybeFound; }

    const auto NewRenderer = GetOrCreate_IsmRenderer(InRendererData);

    CK_ENSURE_IF_NOT(ck::IsValid(NewRenderer),
        TEXT("Failed to GetOrCreate ISM Renderer Actor for [{}]"), InRendererData)
    { return {}; }

    auto* StaticMeshComponent = DoFind_IsmComponentOnRenderer(NewRenderer, InRendererData);

    if (ck::Is_NOT_Valid(StaticMeshComponent))
    { return {}; }

    return _IsmComponentCache.Add(InRendererData, StaticMeshComponent);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    FindOrCreate_OutlineIsmComponent(
        const UCk_IsmRenderer_Data* InRendererData,
        const UCkUsf_OutlinePreset* InPreset,
        uint8 InStencilValue,
        const TWeakObjectPtr<AActor>& InEditorSelectionOwner)
    -> TWeakObjectPtr<UInstancedStaticMeshComponent>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRendererData) && ck::IsValid(InPreset),
        TEXT("FindOrCreate_OutlineIsmComponent: INVALID renderer data [{}] or preset"), InRendererData)
    { return {}; }

    const auto Key = FOutlineIsmKey{InRendererData, InPreset, InEditorSelectionOwner};

    if (const auto& MaybeFound = _OutlineIsmComponentCache.Find(Key);
        ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*MaybeFound))
    {
        // Re-assert: the preset's stencil can be freed + re-allocated to a different value.
        (*MaybeFound)->SetCustomDepthStencilValue(static_cast<int32>(InStencilValue));
        return *MaybeFound;
    }

    // Per-owner previews mirror into a shadow on the same renderer actor as their instances.
    const auto SourceIsm = FindOrCache_IsmComponent(InRendererData, InEditorSelectionOwner);

    CK_ENSURE_IF_NOT(ck::IsValid(SourceIsm),
        TEXT("FindOrCreate_OutlineIsmComponent: could NOT resolve the source ISM component for [{}]"), InRendererData)
    { return {}; }

    auto* ShadowIsm = DoCreate_CustomDepthShadowIsm(SourceIsm.Get(), InStencilValue, TEXT("IsmOutlineShadow"));

    if (ck::Is_NOT_Valid(ShadowIsm))
    { return {}; }

    return _OutlineIsmComponentCache.Add(Key, ShadowIsm);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    FindOrCreate_CelPatternIsmComponent(
        const UCk_IsmRenderer_Data* InRendererData,
        uint8 InStencilValue,
        const TWeakObjectPtr<AActor>& InEditorSelectionOwner)
    -> TWeakObjectPtr<UInstancedStaticMeshComponent>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRendererData),
        TEXT("FindOrCreate_CelPatternIsmComponent: INVALID renderer data"))
    { return {}; }

    const auto Key = FCelPatternIsmKey{InRendererData, InStencilValue, InEditorSelectionOwner};

    if (const auto& MaybeFound = _CelPatternIsmComponentCache.Find(Key);
        ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*MaybeFound))
    { return *MaybeFound; }

    // Per-owner previews mirror into a shadow on the same renderer actor as their instances.
    const auto SourceIsm = FindOrCache_IsmComponent(InRendererData, InEditorSelectionOwner);

    CK_ENSURE_IF_NOT(ck::IsValid(SourceIsm),
        TEXT("FindOrCreate_CelPatternIsmComponent: could NOT resolve the source ISM component for [{}]"), InRendererData)
    { return {}; }

    auto* ShadowIsm = DoCreate_CustomDepthShadowIsm(SourceIsm.Get(), InStencilValue, TEXT("IsmCelPatternShadow"));

    if (ck::Is_NOT_Valid(ShadowIsm))
    { return {}; }

    return _CelPatternIsmComponentCache.Add(Key, ShadowIsm);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    FindOrCreate_StylizeMaskIsmComponent(
        const UCk_IsmRenderer_Data* InRendererData,
        uint8 InStencilValue,
        const TWeakObjectPtr<AActor>& InEditorSelectionOwner)
    -> TWeakObjectPtr<UInstancedStaticMeshComponent>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRendererData),
        TEXT("FindOrCreate_StylizeMaskIsmComponent: INVALID renderer data"))
    { return {}; }

    const auto Key = FStylizeMaskIsmKey{InRendererData, InStencilValue, InEditorSelectionOwner};

    if (const auto& MaybeFound = _StylizeMaskIsmComponentCache.Find(Key);
        ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*MaybeFound))
    { return *MaybeFound; }

    // Per-owner previews mirror into a shadow on the same renderer actor as their instances.
    const auto SourceIsm = FindOrCache_IsmComponent(InRendererData, InEditorSelectionOwner);

    CK_ENSURE_IF_NOT(ck::IsValid(SourceIsm),
        TEXT("FindOrCreate_StylizeMaskIsmComponent: could NOT resolve the source ISM component for [{}]"), InRendererData)
    { return {}; }

    auto* ShadowIsm = DoCreate_CustomDepthShadowIsm(SourceIsm.Get(), InStencilValue, TEXT("IsmStylizeMaskShadow"));

    if (ck::Is_NOT_Valid(ShadowIsm))
    { return {}; }

    return _StylizeMaskIsmComponentCache.Add(Key, ShadowIsm);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    DoCreate_CustomDepthShadowIsm(
        UInstancedStaticMeshComponent* InSourceIsm,
        uint8 InStencilValue,
        FName InNameBase)
    -> UInstancedStaticMeshComponent*
{
    auto* Owner = InSourceIsm->GetOwner();

    CK_ENSURE_IF_NOT(ck::IsValid(Owner),
        TEXT("DoCreate_CustomDepthShadowIsm: source ISM component [{}] has no owning actor"), InSourceIsm)
    { return nullptr; }

    auto* ShadowIsm = NewObject<UInstancedStaticMeshComponent>(Owner,
        MakeUniqueObjectName(Owner, UInstancedStaticMeshComponent::StaticClass(), InNameBase));

    ShadowIsm->SetupAttachment(Owner->GetRootComponent());
    ShadowIsm->SetMobility(InSourceIsm->Mobility);
    ShadowIsm->SetStaticMesh(InSourceIsm->GetStaticMesh());

    // Inheriting the source's materials is what makes a WPO-animated silhouette track its mesh, and
    // translucent-family slots must NOT be inherited. Rationale: CkIsmRenderer/CLAUDE.md.
    for (auto MaterialIndex = 0; MaterialIndex < InSourceIsm->GetNumMaterials(); ++MaterialIndex)
    {
        const auto& SourceMaterial = InSourceIsm->GetMaterial(MaterialIndex);

        if (ck::Is_NOT_Valid(SourceMaterial))
        { continue; }

        if (IsTranslucentBlendMode(*SourceMaterial) && NOT SourceMaterial->IsTranslucencyWritingCustomDepth())
        { continue; }

        ShadowIsm->SetMaterial(MaterialIndex, SourceMaterial);
    }

    ShadowIsm->NumCustomDataFloats = InSourceIsm->NumCustomDataFloats;
    ShadowIsm->bEvaluateWorldPositionOffset = InSourceIsm->bEvaluateWorldPositionOffset;
    ShadowIsm->WorldPositionOffsetDisableDistance = InSourceIsm->WorldPositionOffsetDisableDistance;

    ShadowIsm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShadowIsm->CastShadow = false;
    ShadowIsm->InstanceStartCullDistance = InSourceIsm->InstanceStartCullDistance;
    ShadowIsm->InstanceEndCullDistance = InSourceIsm->InstanceEndCullDistance;

    ShadowIsm->bRenderInMainPass = false;
    ShadowIsm->SetRenderCustomDepth(true);
    ShadowIsm->SetCustomDepthStencilValue(static_cast<int32>(InStencilValue));

    ShadowIsm->RegisterComponent();

    return ShadowIsm;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_Subsystem_UE::
    GetOrCreate_IsmRenderer(
        const UWorld* InWorld,
        const UCk_IsmRenderer_Data* InDataAsset)
    -> ACk_IsmRenderer_Actor_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
        TEXT("Trying to get Ism Renderer from an INVALID World"))
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InDataAsset),
        TEXT("Trying to get Ism Renderer from an INVALID Data Asset"))
    { return nullptr; }

    const auto& IsmRendererSubsystem = InWorld->GetSubsystem<UCk_IsmRenderer_Subsystem_UE>(InWorld);

    CK_ENSURE_IF_NOT(ck::IsValid(IsmRendererSubsystem),
        TEXT("Could NOT find the IsmRender_Subsystem for the World [{}]"), InWorld)
    { return nullptr; }

    return IsmRendererSubsystem->GetOrCreate_IsmRenderer(InDataAsset);
}

// --------------------------------------------------------------------------------------------------------------------
