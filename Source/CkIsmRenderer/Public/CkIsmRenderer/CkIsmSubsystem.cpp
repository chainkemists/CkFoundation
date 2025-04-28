#include "CkIsmSubsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEntityBridge/Public/CkEntityBridge/CkEntityBridge_ConstructionScript.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_IsmRenderer_Actor_UE::
    ACk_IsmRenderer_Actor_UE()
{
    // Create a scene component to serve as the root
    _RootNode = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    RootComponent = _RootNode;

    _EntityBridge = CreateDefaultSubobject<UCk_EntityBridge_IsmRenderer_UE>(TEXT("EntityBridge"));
    _EntityBridge->_ConstructionScript = UCk_Entity_ConstructionScript_WithTransform_PDA::StaticClass();
    _EntityBridge->_Replication = ECk_Replication::DoesNotReplicate;
}

auto
    ACk_IsmRenderer_Actor_UE::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    ICk_Entity_ConstructionScript_Interface::DoConstruct_Implementation(InHandle);

    const auto DefaultObject = UCk_Utils_Object_UE::Get_ClassDefaultObject<ACk_IsmRenderer_Actor_UE>();
    UCk_Utils_IsmRenderer_UE::Add(InHandle, DefaultObject->_RenderData);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Subsystem_UE::
    GetOrCreate_IsmRenderer(
        const UCk_IsmRenderer_Data* InDataAsset)
    -> ACk_IsmRenderer_Actor_UE*
{
    if (auto Found = _IsmRenderers.Find(InDataAsset); ck::IsValid(Found, ck::IsValid_Policy_NullptrOnly{}))
    { return *Found; }

    const auto DefaultObject = UCk_Utils_Object_UE::Get_ClassDefaultObject<ACk_IsmRenderer_Actor_UE>();
    DefaultObject->_RenderData = InDataAsset;

    auto IsmRenderer = _IsmRenderers.Add(InDataAsset,
        GetWorld()->SpawnActor<ACk_IsmRenderer_Actor_UE>(ACk_IsmRenderer_Actor_UE::StaticClass()));

    return IsmRenderer;
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
