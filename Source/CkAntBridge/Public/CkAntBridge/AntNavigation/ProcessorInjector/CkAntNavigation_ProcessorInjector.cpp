#include "CkAntNavigation_ProcessorInjector.h"

#include "CkAntBridge/AntNavigation/CkAntNavigation_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AntNavigation_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_AntNavigation_HandleRequests>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_AntNavigation_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
