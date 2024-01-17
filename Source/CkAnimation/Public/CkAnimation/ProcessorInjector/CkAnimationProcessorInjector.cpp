#include "CkAnimationProcessorInjector.h"

#include "CkAnimation/AnimState/CkAnimState_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Animation_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_AnimState_HandleRequests>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_AnimState_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
