#include "CkWorldActor.h"

#include "CkActor/ActorModifier/CkActorModifier_Processor.h"

#include "CkIntent/CkIntent_Processor.h"

#include "CkPhysics/Velocity/CkVelocity_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Processor.h"

#include "CkEcsBasics/Transform/CkTransform_Processor.h"

#include "CkPhysics/Acceleration/CkAcceleration_Processor.h"
#include "CkPhysics/Integrator/CkIntegrator_Processor.h"
#include "CkProjectile/CkProjectile_Processor.h"

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
        InWorld.Add<ck::FCk_Processor_ActorModifier_AddActorComponent_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Intent_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Intent_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Velocity_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Acceleration_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Integrator_Update>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Projectile_Update>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Transform_InterpolateToGoal>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Transform_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Transform_Actor>(InWorld.Get_Registry());

        // Processors for Replication
        {
            InWorld.Add<ck::FCk_Processor_Velocity_Replicate>(InWorld.Get_Registry());
            InWorld.Add<ck::FCk_Processor_Transform_Replicate>(InWorld.Get_Registry());
        }

        InWorld.Add<ck::FCk_Processor_EntityLifetime_EntityJustCreated>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_EntityLifetime_PendingDestroyEntity>(InWorld.Get_Registry());
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto ACk_World_Actor_UE::
Initialize(ETickingGroup InTickingGroup) -> void
{
    Super::Initialize(InTickingGroup);
    ck_world_actor::InjectAllEcsSystemsIntoWorld(*_EcsWorld);
}

// --------------------------------------------------------------------------------------------------------------------
