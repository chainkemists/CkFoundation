#include "CkEcsExtProcessorInjector.h"

#include "CkEcsExt/Transform/CkTransform_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsExt_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Transform_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_InterpolateToGoal_Location>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_InterpolateToGoal_Rotation>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_Actor>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Transform_FireSignals>(InWorld.Get_Registry());


    InWorld.Add<ck::FProcessor_Transform_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
