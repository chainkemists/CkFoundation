#include "CkWorldActor.h"

#include "CkActor/ActorModifier/CkActorModifier_Processor.h"
#include "CkActor/ActorInfo/CkActorInfo_Processors.h"

#include "CkIntent/CkIntent_Processor.h"

#include "CkLifetime/EntityLifetime/CkEntityLifetime_Processor.h"

#include "CkUnreal/Entity/CkUnrealEntity_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_world_actor
{
    auto
    InjectAllEcsSystemsIntoWorld(ACk_World_Actor_UE::FEcsWorldType& InWorld) -> void
    {
        InWorld.Add<ck::FCk_Processor_EntityLifetime_TriggerDestroyEntity>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_UnrealEntity_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_ActorModifier_SpawnActor_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_ActorInfo_LinkUp>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_ActorModifier_AddActorComponent_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Intent_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Intent_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_ActorModifier_Location_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_ActorModifier_Scale_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_ActorModifier_Rotation_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_ActorModifier_Transform_HandleRequests>(InWorld.Get_Registry());

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

    CK_ENSURE_IF_NOT(ck::IsValid(_EcsWorld), TEXT("ECS World is NOT set in the World Actor [{}]. Was Initialize() called?"), this)
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
