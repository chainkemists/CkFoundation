#include "CkProbe_ProcessorInjector.h"

#include "CkSpatialQuery/Probe/CkProbe_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Probe_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Probe_Setup>(FCk_Registry{}, InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Probe_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Probe_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Probe_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
