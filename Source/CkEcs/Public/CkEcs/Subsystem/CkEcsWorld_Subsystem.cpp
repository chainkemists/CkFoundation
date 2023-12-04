#include "CkEcsWorld_Subsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"

#include <Engine/World.h>

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_ECS_TAG(FTag_NAME_TransientEntity);

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

    _EcsWorld = EcsWorldType{};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsWorld_Subsystem_UE::
    OnWorldBeginPlay(
        UWorld& InWorld) -> void
{
    Super::OnWorldBeginPlay(InWorld);

    DoSpawnWorldActor();
}

auto
    UCk_EcsWorld_Subsystem_UE::
    DoSpawnWorldActor()
    -> void
{
    const auto& EcsWorldActorClass = UCk_Utils_Ecs_ProjectSettings_UE::Get_EcsWorldActorClass();
    const auto& EcsWorldTickingGroup = UCk_Utils_Ecs_ProjectSettings_UE::Get_EcsWorldTickingGroup();

    CK_ENSURE_IF_NOT(ck::IsValid(EcsWorldActorClass), TEXT("Invalid ECS World Actor class set in the Project Settings! ECS Framework won't work!"))
    { return; }

    _WorldActor = Cast<ACk_World_Actor_Base_UE>
    (
        UCk_Utils_Actor_UE::Request_SpawnActor
        (
            FCk_Utils_Actor_SpawnActor_Params{GetWorld(), EcsWorldActorClass}
            .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
            [&](AActor* InActor)
            {
                const auto WorldActor = Cast<ACk_World_Actor_Base_UE>(InActor);
                WorldActor->Initialize(EcsWorldTickingGroup);

                auto& Registry = WorldActor->_EcsWorld->Get_Registry();
                _TransientEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
                _TransientEntity.Add<FTag_NAME_TransientEntity>();
            }
        )
    );

    CK_ENSURE_IF_NOT(ck::IsValid(_WorldActor), TEXT("Failed to spawn ECS World Actor. ECS Pipeline will NOT work."))
    { return; }
}

// --------------------------------------------------------------------------------------------------------------------
