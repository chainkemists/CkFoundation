#include "CkIsmSubsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

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
    UCk_Utils_IsmRenderer_TransientFactory_UE::ClearCache();

    Super::Deinitialize();
}

auto
    UCk_IsmRenderer_Subsystem_UE::
    DoSweepLeakedRenderers()
    -> void
{
    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}))
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
        // Bake the cleanup into the level package on next save. The entity-spawner rebuild
        // path will repopulate fresh transient renderers on demand.
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

    // Identify the renderer via SpawnInfo.Name (passed through .Set_NonUniqueName) rather than
    // .Set_Label(). SetActorLabel calls AActor::Modify() → MarkPackageDirty() on the actor's
    // package; in editor worlds the only package is the persistent level, so every spawn would
    // dirty the level. RF_Transient (applied in the post-spawn callback) prevents serialization
    // on save but does NOT undo the dirty flag — by the time it's set, the dirty has already
    // fired. SpawnInfo.Name is the construction-time identifier, doesn't call Modify(), and
    // surfaces as the actor's FName (visible in outliner when bListedInSceneOutliner is true,
    // and in `obj list class=Ck_IsmRenderer_Actor_UE` regardless).
    const auto& DescriptiveName = FName{*ck::Format_UE(TEXT("IsmRenderer [{}][{}][Shadows:{}][{}][Collision:{}]"),
        InDataAsset->Get_Mesh(),
        InDataAsset->Get_Mobility(),
        InDataAsset->Get_LightingInfo().Get_CastShadows(),
        InDataAsset->Get_RenderPolicy(),
        InDataAsset->Get_PhysicsInfo().Get_Collision())};

    const auto& SpawnedIsmRendererActor = Cast<ACk_IsmRenderer_Actor_UE>(UCk_Utils_Actor_UE::Request_SpawnActor
    (
        FCk_Utils_Actor_SpawnActor_Params{GetWorld(), ACk_IsmRenderer_Actor_UE::StaticClass()}
        .Set_NonUniqueName(DescriptiveName)
        .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
        [&](AActor* InActor)
        {
            const auto& NewIsmRendererActor = Cast<ACk_IsmRenderer_Actor_UE>(InActor);
            NewIsmRendererActor->_RenderData = InDataAsset;

            // ISM renderer actors are a runtime cache derived from UCk_IsmRenderer_Data; they
            // must never be saved into the level package. Without RF_Transient, every editor
            // open re-spawns one and dirties the level, then a save bakes the duplicate in.
            NewIsmRendererActor->SetFlags(RF_Transient);

            // Editor worlds never fire BeginPlay; explicit init also covers runtime without
            // double-spawning (DoInitialize is idempotent).
            NewIsmRendererActor->DoInitialize();
        }
    ));

    const auto IsmRenderer = _IsmRenderers.Add(InDataAsset, SpawnedIsmRendererActor);

    return IsmRenderer;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    FindOrCache_IsmComponent(
        const UCk_IsmRenderer_Data* InRendererData)
    -> TWeakObjectPtr<UInstancedStaticMeshComponent>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRendererData),
        TEXT("InRendererData [{}] is NOT valid"), InRendererData)
    { return {}; }

    if (const auto& MaybeFound = _IsmComponentCache.Find(InRendererData);
        ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*MaybeFound))
    { return *MaybeFound; }

    const auto NewRenderer = GetOrCreate_IsmRenderer(InRendererData);

    CK_ENSURE_IF_NOT(ck::IsValid(NewRenderer),
        TEXT("Failed to GetOrCreate ISM Renderer Actor for [{}]"), InRendererData)
    { return {}; }

    auto StaticMeshComponent = [&]() -> UInstancedStaticMeshComponent*
    {
        if (InRendererData->Get_RenderPolicy() == ECk_Ism_RenderPolicy::ISM)
        { return NewRenderer->FindComponentByClass<UInstancedStaticMeshComponent>(); }

        return NewRenderer->FindComponentByClass<UHierarchicalInstancedStaticMeshComponent>();
    }();

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
        uint8 InStencilValue)
    -> TWeakObjectPtr<UInstancedStaticMeshComponent>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRendererData) && ck::IsValid(InPreset, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("FindOrCreate_OutlineIsmComponent: INVALID renderer data [{}] or preset"), InRendererData)
    { return {}; }

    const auto Key = FOutlineIsmKey{InRendererData, InPreset};

    if (const auto& MaybeFound = _OutlineIsmComponentCache.Find(Key);
        ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*MaybeFound))
    {
        // The preset keeps its stencil while any refcount is live, but re-assert in case it was
        // freed + re-allocated to a different value between outlines.
        (*MaybeFound)->SetCustomDepthStencilValue(static_cast<int32>(InStencilValue));
        return *MaybeFound;
    }

    const auto SourceIsm = FindOrCache_IsmComponent(InRendererData);

    CK_ENSURE_IF_NOT(ck::IsValid(SourceIsm),
        TEXT("FindOrCreate_OutlineIsmComponent: could NOT resolve the source ISM component for [{}]"), InRendererData)
    { return {}; }

    auto* Owner = SourceIsm->GetOwner();

    CK_ENSURE_IF_NOT(ck::IsValid(Owner),
        TEXT("FindOrCreate_OutlineIsmComponent: source ISM component for [{}] has no owning actor"), InRendererData)
    { return {}; }

    auto* ShadowIsm = NewObject<UInstancedStaticMeshComponent>(Owner,
        MakeUniqueObjectName(Owner, UInstancedStaticMeshComponent::StaticClass(), TEXT("IsmOutlineShadow")));

    ShadowIsm->SetupAttachment(Owner->GetRootComponent());
    ShadowIsm->SetMobility(SourceIsm->Mobility);
    ShadowIsm->SetStaticMesh(SourceIsm->GetStaticMesh());

    // The silhouette has to come out of the SAME vertex shader as the visible mesh. A material that
    // animates through World Position Offset (CkVat samples its baked pose texture in WPO, keyed off
    // the per-instance custom data) would otherwise silhouette its bind pose while the mesh animates.
    // The custom depth pass runs the full material VS whenever the material modifies mesh position,
    // so inheriting the source's materials + custom-data width is what makes an animated outline track
    // its mesh. Free for ordinary ISMs: that pass swaps the position-only default material back in
    // when the material does NOT modify mesh position.
    //
    // Translucent-family materials are the one slot we must NOT inherit: that same pass DROPS them
    // outright unless they opt into translucent custom-depth writes (UseDefaultMaterial's final else ->
    // bIgnoreThisMaterial -> no draw), which would silently delete the outline of anything rendering a
    // translucent look. Those slots keep the static mesh's own material — the pre-change behaviour.
    for (auto MaterialIndex = 0; MaterialIndex < SourceIsm->GetNumMaterials(); ++MaterialIndex)
    {
        const auto& SourceMaterial = SourceIsm->GetMaterial(MaterialIndex);

        if (ck::Is_NOT_Valid(SourceMaterial, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        if (IsTranslucentBlendMode(*SourceMaterial) && NOT SourceMaterial->IsTranslucencyWritingCustomDepth())
        { continue; }

        ShadowIsm->SetMaterial(MaterialIndex, SourceMaterial);
    }

    ShadowIsm->NumCustomDataFloats = SourceIsm->NumCustomDataFloats;
    ShadowIsm->bEvaluateWorldPositionOffset = SourceIsm->bEvaluateWorldPositionOffset;
    ShadowIsm->WorldPositionOffsetDisableDistance = SourceIsm->WorldPositionOffsetDisableDistance;

    ShadowIsm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShadowIsm->CastShadow = false;
    ShadowIsm->InstanceStartCullDistance = SourceIsm->InstanceStartCullDistance;
    ShadowIsm->InstanceEndCullDistance = SourceIsm->InstanceEndCullDistance;

    // Custom-depth-only: the silhouette source for the SolidOutline post-process, invisible everywhere else.
    ShadowIsm->bRenderInMainPass = false;
    ShadowIsm->SetRenderCustomDepth(true);
    ShadowIsm->SetCustomDepthStencilValue(static_cast<int32>(InStencilValue));

    ShadowIsm->RegisterComponent();

    return _OutlineIsmComponentCache.Add(Key, ShadowIsm);
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
