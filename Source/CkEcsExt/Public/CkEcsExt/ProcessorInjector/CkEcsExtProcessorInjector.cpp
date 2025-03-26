#include "CkEcsExtProcessorInjector.h"

#include "CkEcs/EntityScript/CkEntityScript_Processor.h"

#include "CkEcsExt/Transform/CkTransform_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsExt_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Transform_SyncFromActor>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_SyncFromMeshSocket>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_InterpolateToGoal_Location>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_InterpolateToGoal_Rotation>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Transform_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_SyncToActor>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_FireSignals>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsExt_ProcessorInjector_Replicate::
    DoInjectProcessors(
        EcsWorldType& InWorld) -> void
{
    InWorld.Add<ck::FProcessor_Transform_Replicate>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_Cleanup>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsExt_ProcessorInjector_Requests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_EntityScript_SpawnEntity_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityScript_BeginPlay>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsExt_ProcessorInjector_Teardown_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_EntityScript_EndPlay>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
