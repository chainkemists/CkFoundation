#include "CkPhysicsProcessorInjector.h"

#include "CkPhysics/Acceleration/CkAcceleration_Processor.h"
#include "CkPhysics/AutoReorient/CkAutoReorient_Processor.h"
#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"
#include "CkPhysics/PredictedVelocity/CkPredictedVelocity_Processor.h"
#include "CkPhysics/Velocity/CkVelocity_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Physics_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_BulkVelocityModifier_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_BulkVelocityModifier_AddNewTargets>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_BulkVelocityModifier_HandleRequests>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Velocity_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_VelocityModifier_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_BulkAccelerationModifier_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_BulkAccelerationModifier_AddNewTargets>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_BulkAccelerationModifier_HandleRequests>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Acceleration_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_AccelerationModifier_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_EulerIntegrator_Update>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EulerIntegrator_DoOnePredictiveUpdate>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_PredictedVelocity_Update>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Velocity_Clamp>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Physics_ProcessorInjector_Teardown::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_VelocityModifier_Teardown>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AccelerationModifier_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Physics_ProcessorInjector_Replicate::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Velocity_Replicate>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Acceleration_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Physics_ProcessorInjector_Orient::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_AutoReorient_OrientTowardsVelocity>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------