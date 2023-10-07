#include "CkEntityReplicationDriver_Subsystem.h"

#include "CkCore/Game/CkGame_Utils.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

#include <GameFramework/PlayerController.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------
void
    UCk_EntityReplicationDriver_Subsystem_UE::Initialize(
        FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void
    UCk_EntityReplicationDriver_Subsystem_UE::OnWorldBeginPlay(
        UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
}

auto
    UCk_EntityReplicationDriver_Subsystem_UE::
    Get_NextAvailableReplicationDriver()
    -> UCk_Fragment_EntityReplicationDriver_Rep*
{
    return _ReplicationDrivers[_NextAvailableReplicationDriver++];
}

auto
    UCk_EntityReplicationDriver_Subsystem_UE::
    Get_ReplicationDriverWithName(
        FName InName)
    -> UCk_Fragment_EntityReplicationDriver_Rep*
{
    return *_ReplicationDriversMap.Find(InName);
}

auto
    UCk_EntityReplicationDriver_Subsystem_UE::
    CreateEntityReplicationDrivers(AActor* InOutermostActor)
    -> void
{
    if (NOT UCk_Utils_Game_UE::Get_IsInGame(GetWorld()))
    { return; }

    // TODO: we need a more scale-able way to create the ReplicatedObjects
    constexpr auto NumberOfRepObjects = 1000;

    for (auto Counter = 0; Counter < NumberOfRepObjects; ++Counter)
    {
        auto NewObject = Cast<UCk_Fragment_EntityReplicationDriver_Rep>(UCk_Ecs_ReplicatedObject_UE::Create
        (
            UCk_Fragment_EntityReplicationDriver_Rep::StaticClass(),
            InOutermostActor,
            NAME_None,
            FCk_Handle{}
        ));

        _ReplicationDrivers.Emplace(NewObject);
        _ReplicationDriversMap.Add(FName{NewObject->GetName()}, NewObject);
    }
}
