#include "CkBallistics_ProcessorInjector.h"

#include "CkBallistics/CkBallistics_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ballistics_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Ballistics_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Ballistics_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Ballistics_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Ballistics_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------