#include "CkEcsWorld_Subsystem.h"

#include "CkWorld/Actor/CkWorldActor.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle UCk_EcsWorld_Subsystem_UE::_TransientEntity;

// --------------------------------------------------------------------------------------------------------------------

void UCk_EcsWorld_Subsystem_UE::
Deinitialize()
{
    OnDeInitialize();

    _WorldActor = nullptr;
}

void UCk_EcsWorld_Subsystem_UE::
Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    DoSpawnWorldActor();

    OnInitialize();
}

auto UCk_EcsWorld_Subsystem_UE::
DoSpawnWorldActor() -> void
{
    auto* WorldActor = GetWorld()->SpawnActorDeferred<ACk_World_Actor_UE>(
        ACk_World_Actor_UE::StaticClass(), FTransform::Identity);

    CK_ENSURE_IF_NOT(ck::IsValid(_WorldActor), TEXT("Failed to spawn ECS World Actor. ECS Pipeline will NOT work."))
    { return; }

    WorldActor->Initialize(_TickingGroup);

    auto& Registry = _WorldActor->_EcsWorld->Get_Registry();
    _TransientEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_WorldActor->_EcsWorld->Get_Registry());
}
