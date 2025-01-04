#include "CkFlowGraph_ProcessorInjector.h"

#include "CkFlowGraph/CkFlowGraph_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_FlowGraph_ProcessorInjector_Requests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_FlowGraph_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------