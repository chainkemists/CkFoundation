#include "CkProcessorInjector.h"

#include "CkAbility/Ability/CkAbility_Processor.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Processor.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Processor.h"

#include "CkActor/ActorModifier/CkActorModifier_Processor.h"

#include "CkAnimation/AnimState/CkAnimState_Processor.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Processor.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Processor.h"

#include "CkCamera/CameraShake/CkCameraShake_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Processor.h"
#include "CkEcs/OwningActor/CkOwningActor_Processors.h"

#include "CkEcsBasics/Transform/CkTransform_Processor.h"

#include "CkMemory/CkMemory_Processor.h"

#include "CkNet/TimeSync/CkNetTimeSync_Processor.h"

#include "CkOverlapBody/Marker/CkMarker_Processor.h"
#include "CkOverlapBody/Sensor/CkSensor_Processor.h"

#include "CkPhysics/Acceleration/CkAcceleration_Processor.h"
#include "CkPhysics/AutoReorient/CkAutoReorient_Processor.h"
#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"
#include "CkPhysics/Velocity/CkVelocity_Processor.h"

#include "CkProjectile/CkProjectile_Processor.h"

#include "CkRecord/RecordEntry/CkRecordEntry_Processor.h"

#include "CkTimer/CkTimer_Processor.h"

#include "CkEntityBridge/CkEntityBridge_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_world_processor_injector
{
    auto
        InjectAllEcsProcessorsIntoWorld(
            UCk_EcsWorld_ProcessorInjector_Base::EcsWorldType& InWorld)
        -> void
    {
        // Always first systems
        {
            InWorld.Add<ck::FProcessor_EntityLifetime_TriggerDestroyEntity>(InWorld.Get_Registry());
        }

        //InWorld.Add<ck::FProcessor_ReplicationDriver_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_AbilityOwner_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_AbilityOwner_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_AbilityOwner_HandleEvents>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_FloatAttributeModifier_Additive_Teardown>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_FloatAttributeModifier_Multiplicative_Teardown>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_EntityBridge_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_ActorModifier_SpawnActor_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_ActorModifier_AddActorComponent_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_NetTimeSync_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_NetTimeSync_OnNetworkClockSynchronized>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_NetTimeSync_FirstSync>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Marker_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Sensor_Setup>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Marker_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Sensor_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_AnimState_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Timer_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Timer_Update>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Timer_Update_Countdown>(InWorld.Get_Registry());

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

        InWorld.Add<ck::FProcessor_Velocity_Clamp>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Projectile_Update>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_AutoReorient_OrientTowardsVelocity>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Transform_InterpolateToGoal_Location>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_InterpolateToGoal_Rotation>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_Actor>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Transform_FireSignals>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_FloatAttribute_RecomputeAll>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_FloatAttributeModifier_ComputeAll>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Meter_Clamp>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_FloatAttribute_FireSignals>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_CameraShake_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_RecordEntry_Destructor>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Marker_UpdateTransform>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Sensor_UpdateTransform>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_Marker_DebugPreviewAll>(InWorld.Get_Registry());
        InWorld.Add<ck::FProcessor_Sensor_DebugPreviewAll>(InWorld.Get_Registry());

        InWorld.Add<ck::FProcessor_AbilityCue_Spawn>(InWorld.Get_Registry());

        // Processors for Replication
        {
            InWorld.Add<ck::FProcessor_Acceleration_Replicate>(InWorld.Get_Registry());
            InWorld.Add<ck::FProcessor_Acceleration_Replicate>(InWorld.Get_Registry());
            InWorld.Add<ck::FProcessor_Transform_Replicate>(InWorld.Get_Registry());
            InWorld.Add<ck::FProcessor_AnimState_Replicate>(InWorld.Get_Registry());
        }

        // Always last systems
        {
            InWorld.Add<ck::FProcessor_OwningActor_Destroy>(InWorld.Get_Registry());
            InWorld.Add<ck::FProcessor_EntityLifetime_EntityJustCreated>(InWorld.Get_Registry());
            InWorld.Add<ck::FProcessor_EntityLifetime_PendingDestroyEntity>(InWorld.Get_Registry());

#if CK_MEMORY_TRACKING
            InWorld.Add<ck::FProcessor_Memory_Stats>();
#endif
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_World_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    ck_world_processor_injector::InjectAllEcsProcessorsIntoWorld(InWorld);
}

// --------------------------------------------------------------------------------------------------------------------
