#include "CkWorldActor.h"

#include "NetworkTimeSyncComponent.h"

#include "CkActor/ActorModifier/CkActorModifier_Processor.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Processor.h"

#include "CkIntent/CkIntent_Processor.h"

#include "CkPhysics/Velocity/CkVelocity_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Processor.h"
#include "CkEcs/OwningActor/CkOwningActor_Processors.h"

#include "CkEcsBasics/Transform/CkTransform_Processor.h"

#include "CkNet/TimeSync/CkNetTimeSync_Processor.h"
#include "CkNet/TimeSync/CkNetTimeSync_Utils.h"

#include "CkOverlapBody/Marker/CkMarker_Processor.h"
#include "CkOverlapBody/Sensor/CkSensor_Processor.h"

#include "CkPhysics/Acceleration/CkAcceleration_Processor.h"
#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"
#include "CkProjectile/CkProjectile_Processor.h"

#include "CkRecord/Record/CkRecord_Processor.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Processor.h"

#include "CkUnreal/Entity/CkUnrealEntity_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_world_actor
{
    auto
        InjectAllEcsSystemsIntoWorld(
            ACk_World_Actor_UE::FEcsWorldType& InWorld)
        -> void
    {
        // Always first systems
        {
            InWorld.Add<ck::FProcessor_EntityLifetime_TriggerDestroyEntity>(InWorld.Get_Registry());
        }

        InWorld.Add<ck::FProcessor_FloatAttributeModifier_Additive_Teardown>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_FloatAttributeModifier_Multiplicative_Teardown>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_UnrealEntity_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_ActorModifier_SpawnActor_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_ActorModifier_AddActorComponent_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_NetTimeSync_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_NetTimeSync_OnNetworkClockSynchronized>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_NetTimeSync_FirstSync>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Intent_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Intent_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Marker_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Sensor_Setup>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Marker_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Sensor_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Velocity_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_VelocityModifier_SingleTarget_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_VelocityModifier_SingleTarget_Teardown>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Acceleration_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_AccelerationModifier_SingleTarget_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_AccelerationModifier_SingleTarget_Teardown>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_EulerIntegrator_Update>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_EulerIntegrator_DoOnePredictiveUpdate>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Projectile_Update>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Transform_InterpolateToGoal>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_Actor>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_FireSignals>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_FloatAttribute_RecomputeAll>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_FloatAttributeModifier_Additive_Compute>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_FloatAttributeModifier_Multiplicative_Compute>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_FloatAttribute_FireSignals>(InWorld.Get_Registry());

        // TODO: Add FloatAttribute_FireSignals processor

        // Processors for Replication
        {
            InWorld.Add<ck::FProcessor_Velocity_Replicate>(InWorld.Get_Registry());
            InWorld.Add<ck::FProcessor_Acceleration_Replicate>(InWorld.Get_Registry());
            InWorld.Add<ck::FProcessor_Transform_Replicate>(InWorld.Get_Registry());
        }

        InWorld.Add<ck::FProcessor_RecordEntry_Destructor>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_RecordOfEntities_Destructor>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Marker_UpdateTransform>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Sensor_UpdateTransform>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Marker_DebugPreviewAll>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Sensor_DebugPreviewAll>(InWorld.Get_Registry());

        // Always last systems
        {
            InWorld.Add<ck::FProcessor_OwningActor_Destroy>(InWorld.Get_Registry());
            InWorld.Add<ck::FProcessor_EntityLifetime_EntityJustCreated>(InWorld.Get_Registry());
            InWorld.Add<ck::FProcessor_EntityLifetime_PendingDestroyEntity>(InWorld.Get_Registry());
        }
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
