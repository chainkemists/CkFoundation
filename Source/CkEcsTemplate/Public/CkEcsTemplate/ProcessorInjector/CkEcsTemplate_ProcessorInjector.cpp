#include "CkEcsTemplate_ProcessorInjector.h"

#include "CkEcsTemplate/CkEcsTemplate_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsTemplate_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_EcsTemplate_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EcsTemplate_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EcsTemplate_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_EcsTemplate_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------