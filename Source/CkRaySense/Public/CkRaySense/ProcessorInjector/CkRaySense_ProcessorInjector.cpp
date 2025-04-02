#include "CkRaySense_ProcessorInjector.h"

#include "CkRaySense/CkRaySense_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RaySense_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_RaySense_LineTrace_Update>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_RaySense_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_RaySense_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_RaySense_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------