#include "CkStateTree_ProcessorInjector.h"

#include "CkStateTree/CkStateTree_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_StateTree_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_StateTree_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_StateTree_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_StateTree_EndPlay>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------