#include "CkIsmSubsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcsExt/EntityScript/CkEntityScript_WithActor_Data.h"

#include "CkIsmRenderer/CkIsmRenderer_EntityScript.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_TransientFactory.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_Utils.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_IsmRenderer_Actor_UE::
    ACk_IsmRenderer_Actor_UE()
{
    _RootNode = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    RootComponent = _RootNode;
}

auto
    ACk_IsmRenderer_Actor_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());
    auto SpawnParams = FInstancedStruct::Make<FCk_EntityScript_WithActor_SpawnParams>(this);
    UCk_Utils_EntityScript_UE::Request_SpawnEntity(TransientEntity, UCk_EntityScript_IsmRenderer_UE::StaticClass(), SpawnParams);
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
