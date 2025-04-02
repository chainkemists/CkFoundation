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
    InWorld.Add<ck::FProcessor_RaySense_BoxSweep_Update>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_RaySense_SphereSweep_Update>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_RaySense_CapsuleSweep_Update>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_RaySense_CylinderSweep_Update>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_RaySense_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_RaySense_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------