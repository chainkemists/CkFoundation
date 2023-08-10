#include "CkWorldActor.h"

#include "CkLifetime/EntityLifetime/CkEntityLifetime_Processor.h"

#include "CkUnreal/Entity/CkUnrealEntity_System.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_world_actor
{
    auto
    InjectAllEcsSystemsIntoWorld(ACk_World_Actor_UE::FEcsWorldType& InWorld) -> void
    {
        InWorld.Add<ck::FCk_Processor_EntityLifetime_TriggerDestroyEntity>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_UnrealEntity_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_EntityLifetime_EntityJustCreated>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_EntityLifetime_PendingDestroyEntity>(InWorld.Get_Registry());
    }
}

// --------------------------------------------------------------------------------------------------------------------

ACk_World_Actor_UE::
ACk_World_Actor_UE()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = false;
}

auto ACk_World_Actor_UE::Tick(float DeltaSeconds) -> void
{
    Super::Tick(DeltaSeconds);

    CK_ENSURE_IF_NOT(ck::IsValid(_EcsWorld), TEXT("ECS World is NOT set in the World Actor. Was Initialize() called?"))
    { return; }

    _EcsWorld->Tick(FCk_Time{DeltaSeconds});
}

auto ACk_World_Actor_UE::
Initialize(ETickingGroup InTickingGroup) -> void
{
    SetTickGroup(InTickingGroup);

    _EcsWorld = FEcsWorldType{};
    ck_world_actor::InjectAllEcsSystemsIntoWorld(*_EcsWorld);
}
