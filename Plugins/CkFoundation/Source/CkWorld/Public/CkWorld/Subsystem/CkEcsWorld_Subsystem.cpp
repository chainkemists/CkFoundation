#include "CkEcsWorld_Subsystem.h"

#include "Actor/CkActor_Utils.h"

#include "CkWorld/Actor/CkWorldActor.h"

#include <Engine/LevelScriptActor.h>
#include <Engine/World.h>

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

    if (NOT UCk_Game_Utils_UE::Get_IsInGame(this))
    { return; }

    DoSpawnWorldActor();

    OnInitialize();
}

auto UCk_EcsWorld_Subsystem_UE::
DoSpawnWorldActor() -> void
{
    _WorldActor = UCk_Utils_Actor_UE::Request_SpawnActor<ACk_World_Actor_UE>
    (
        FCk_Utils_Actor_SpawnActor_Params{GetWorld(), ACk_World_Actor_UE::StaticClass()}
        .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
        [&](ACk_World_Actor_UE* InActor)
        {
            InActor->Initialize(_TickingGroup);

            auto& Registry = InActor->_EcsWorld->Get_Registry();
            _TransientEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
        }
    );

    CK_ENSURE_IF_NOT(ck::IsValid(_WorldActor), TEXT("Failed to spawn ECS World Actor. ECS Pipeline will NOT work."))
    { return; }
}
