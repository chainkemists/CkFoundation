#include "CkEcsWorld_Subsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"

#include <Engine/World.h>

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_World_Actor_Base_UE::
    ACk_World_Actor_Base_UE()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = false;
}

auto
    ACk_World_Actor_Base_UE::
    Tick(float DeltaSeconds)
    -> void
{
    Super::Tick(DeltaSeconds);

    CK_ENSURE_IF_NOT(ck::IsValid(_EcsWorld), TEXT("ECS World is NOT set in the World Actor [{}]. Was Initialize() called?"), this)
    { return; }

    _EcsWorld->Tick(FCk_Time{DeltaSeconds});
}

auto
    ACk_World_Actor_Base_UE::
    Initialize(ETickingGroup InTickingGroup)
    -> void
{
    SetTickGroup(InTickingGroup);

    _EcsWorld = FEcsWorldType{};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsWorld_Subsystem_UE::
    Deinitialize()
    -> void
{
    OnDeInitialize();

    _WorldActor = nullptr;
}

auto
    UCk_EcsWorld_Subsystem_UE::
    Initialize(FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    if (NOT UCk_Utils_Game_UE::Get_IsInGame(this))
    { return; }

    DoSpawnWorldActor();

    OnInitialize();
}

auto
    UCk_EcsWorld_Subsystem_UE::
    DoSpawnWorldActor()
    -> void
{
    _WorldActor = Cast<ACk_World_Actor_Base_UE>
    (
        UCk_Utils_Actor_UE::Request_SpawnActor
        (
            FCk_Utils_Actor_SpawnActor_Params{GetWorld(), _WorldActorClass}
            .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
            [&](AActor* InActor)
            {
                const auto WorldActor = Cast<ACk_World_Actor_Base_UE>(InActor);
                WorldActor->Initialize(_TickingGroup);

                auto& Registry = WorldActor->_EcsWorld->Get_Registry();
                _TransientEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
            }
        )
    );

    CK_ENSURE_IF_NOT(ck::IsValid(_WorldActor), TEXT("Failed to spawn ECS World Actor. ECS Pipeline will NOT work."))
    { return; }
}
