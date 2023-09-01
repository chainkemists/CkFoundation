#include "CkWorldActor.h"

#include "NetworkTimeSyncComponent.h"

#include "CkActor/ActorModifier/CkActorModifier_Processor.h"

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
        InWorld.Add<ck::FCk_Processor_EntityLifetime_TriggerDestroyEntity>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_UnrealEntity_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_ActorModifier_SpawnActor_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_ActorModifier_AddActorComponent_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_TimeSync_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_TimeSync_OnNetworkClockSynchronized>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_TimeSync_FirstSync>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Intent_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Intent_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Marker_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Sensor_Setup>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Marker_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Sensor_HandleRequests>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Velocity_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_VelocityModifier_SingleTarget_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_VelocityModifier_SingleTarget_Teardown>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Acceleration_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_AccelerationModifier_SingleTarget_Setup>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_AccelerationModifier_SingleTarget_Teardown>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_EulerIntegrator_Update>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_EulerIntegrator_DoOnePredictiveUpdate>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Projectile_Update>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Transform_InterpolateToGoal>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Transform_HandleRequests>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Transform_Actor>(InWorld.Get_Registry());

        // Processors for Replication
        {
            InWorld.Add<ck::FCk_Processor_Velocity_Replicate>(InWorld.Get_Registry());
            InWorld.Add<ck::FCk_Processor_Acceleration_Replicate>(InWorld.Get_Registry());
            InWorld.Add<ck::FCk_Processor_Transform_Replicate>(InWorld.Get_Registry());
        }

        InWorld.Add<ck::FCk_Processor_RecordEntry_Destructor>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Record_Destructor>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Marker_UpdateTransform>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Sensor_UpdateTransform>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_Marker_DebugPreviewAll>(InWorld.Get_Registry());
        InWorld.Add<ck::FCk_Processor_Sensor_DebugPreviewAll>(InWorld.Get_Registry());

        InWorld.Add<ck::FCk_Processor_OwningActor_Destroy>(InWorld.Get_Registry());
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
