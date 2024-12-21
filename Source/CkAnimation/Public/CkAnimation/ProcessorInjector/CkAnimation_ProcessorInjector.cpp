#include "CkAnimation_ProcessorInjector.h"

#include "CkAnimation/AnimPlan/CkAnimPlan_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Animation_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_AnimPlan_HandleRequests>(InWorld.Get_Registry());
}

auto
    UCk_Animation_ProcessorInjector_Replicate::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_AnimPlan_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
