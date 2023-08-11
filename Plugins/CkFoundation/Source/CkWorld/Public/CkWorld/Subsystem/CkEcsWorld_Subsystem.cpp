#include "CkEcsWorld_Subsystem.h"

#include "Actor/CkActor_Utils.h"

#include "CkWorld/Actor/CkWorldActor.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_EcsWorld_Subsystem_UE::
Deinitialize() -> void
{
    OnDeInitialize();

    _WorldActor = nullptr;
}

auto UCk_EcsWorld_Subsystem_UE::
Initialize(FSubsystemCollectionBase& Collection) -> void
{
    Super::Initialize(Collection);

    DoSpawnWorldActor();

    OnInitialize();
}

auto UCk_EcsWorld_Subsystem_UE::
DoSpawnWorldActor() -> void
{
    // TODO: replace with our own spawn actor function
    _WorldActor = GetWorld()->SpawnActorDeferred<ACk_World_Actor_UE>(
        ACk_World_Actor_UE::StaticClass(), FTransform::Identity);

    CK_ENSURE_IF_NOT(ck::IsValid(_WorldActor), TEXT("Failed to spawn ECS World Actor. ECS Pipeline will NOT work."))
    { return; }

    _WorldActor->Initialize(_TickingGroup);

    auto& Registry = _WorldActor->_EcsWorld->Get_Registry();
    _TransientEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_WorldActor->_EcsWorld->Get_Registry());
}
