#include "CkWorldActor.h"

#include "NetworkTimeSyncComponent.h"

#include "CkActor/ActorModifier/CkActorModifier_Processor.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Processor.h"

#include "CkIntent/CkIntent_Processor.h"

#include "CkPhysics/Acceleration/CkAcceleration_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Processor.h"
#include "CkEcs/OwningActor/CkOwningActor_Processors.h"

#include "CkEcsBasics/Transform/CkTransform_Processor.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Processor.h"
#include "CkNet/TimeSync/CkNetTimeSync_Processor.h"
#include "CkNet/TimeSync/CkNetTimeSync_Utils.h"

#include "CkOverlapBody/Marker/CkMarker_Processor.h"
#include "CkOverlapBody/Sensor/CkSensor_Processor.h"

#include "CkPhysics/Acceleration/CkAcceleration_Processor.h"
#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"
#include "CkPhysics/Velocity/CkVelocity_Processor.h"
#include "CkProjectile/CkProjectile_Processor.h"

#include "CkRecord/Record/CkRecord_Processor.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Processor.h"
#include "CkTimer/CkTimer_Processor.h"

#include "CkUnreal/EntityBridge/CkEntityBridge_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_world_actor
{
    auto
        InjectAllEcsProcessorsIntoWorld(
            ACk_World_Actor_UE::EcsWorldType& InWorld)
        -> void
    {
        // Always first systems
        {
            InWorld.Add<ck::FProcessor_EntityLifetime_TriggerDestroyEntity>(InWorld.Get_Registry());
        }

        //InWorld.Add<ck::FProcessor_ReplicationDriver_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_FloatAttributeModifier_Additive_Teardown>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_FloatAttributeModifier_Multiplicative_Teardown>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_EntityBridge_HandleRequests>(InWorld.Get_Registry());

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

        InWorld.Add<ck::FProcessor_Timer_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Timer_Update>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_BulkVelocityModifier_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_BulkVelocityModifier_AddNewTargets>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_BulkVelocityModifier_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Velocity_Setup>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_VelocityModifier_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_VelocityModifier_Teardown>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_BulkAccelerationModifier_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_BulkAccelerationModifier_AddNewTargets>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_BulkAccelerationModifier_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Acceleration_Setup>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_AccelerationModifier_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_AccelerationModifier_Teardown>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_EulerIntegrator_Update>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_EulerIntegrator_DoOnePredictiveUpdate>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Projectile_Update>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Transform_InterpolateToGoal_Location>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_InterpolateToGoal_Rotation>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_Actor>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_FireSignals>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_FloatAttribute_RecomputeAll>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_FloatAttributeModifier_ComputeAll>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_FloatAttribute_FireSignals>(InWorld.Get_Registry());

        // TODO: Add FloatAttribute_FireSignals processor

        // Processors for Replication
        {
            InWorld.Add<ck::FProcessor_Timer_Replicate>(InWorld.Get_Registry());
            InWorld.Add<ck::FProcessor_Acceleration_Replicate>(InWorld.Get_Registry());
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

auto
    ACk_World_Actor_UE::
    Initialize(
        ETickingGroup InTickingGroup)
    -> void
{
    Super::Initialize(InTickingGroup);

    ck_world_actor::InjectAllEcsProcessorsIntoWorld(*_EcsWorld);
}

// --------------------------------------------------------------------------------------------------------------------
