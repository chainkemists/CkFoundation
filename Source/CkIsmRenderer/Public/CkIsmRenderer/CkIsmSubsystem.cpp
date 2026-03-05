#include "CkIsmSubsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEntityBridge/Public/CkEntityBridge/CkEntityBridge_ConstructionScript.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_TransientFactory.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_Utils.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_IsmRenderer_Actor_UE::
    ACk_IsmRenderer_Actor_UE()
{
    // Create a scene component to serve as the root
    _RootNode = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    RootComponent = _RootNode;

    _EntityBridge = CreateDefaultSubobject<UCk_EntityBridge_ActorComponent_UE>(TEXT("EntityBridge"));
    _EntityBridge->_ConstructionScript = UCk_Entity_ConstructionScript_WithTransform_PDA::StaticClass();
    _EntityBridge->_Replication = ECk_Replication::DoesNotReplicate;
}

auto
    ACk_IsmRenderer_Actor_UE::
    DoConstruct_Implementation(
        FCk_Handle& InHandle)
    -> void
{
    ICk_Entity_ConstructionScript_Interface::DoConstruct_Implementation(InHandle);

    UCk_Utils_IsmRenderer_UE::Add(InHandle, this->_RenderData);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    Deinitialize()
    -> void
{
    UCk_Utils_IsmRenderer_TransientFactory_UE::ClearCache();

    Super::Deinitialize();
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
        ck::IsValid(Found, ck::IsValid_Policy_NullptrOnly{}))
    { return *Found; }

    const auto& SpawnedIsmRendererActor = Cast<ACk_IsmRenderer_Actor_UE>(UCk_Utils_Actor_UE::Request_SpawnActor
    (
        FCk_Utils_Actor_SpawnActor_Params{GetWorld(), ACk_IsmRenderer_Actor_UE::StaticClass()}
        .Set_Label(ck::Format_UE(TEXT("IsmRenderer [{}][{}][Shadows:{}][{}][Collision:{}]"), InDataAsset->Get_Mesh(),
            InDataAsset->Get_Mobility(), InDataAsset->Get_LightingInfo().Get_CastShadows(),
            InDataAsset->Get_RenderPolicy(), InDataAsset->Get_PhysicsInfo().Get_Collision()))
        .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
        [&](AActor* InActor)
        {
            const auto& NewIsmRendererActor = Cast<ACk_IsmRenderer_Actor_UE>(InActor);
            NewIsmRendererActor->_RenderData = InDataAsset;;
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
