#include "CkResolverTarget_ProcessorInjector.h"

#include "ResolverTarget/CkResolverTarget_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ResolverTarget_ProcessorInjector_HandlRequests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ResolverTarget_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
