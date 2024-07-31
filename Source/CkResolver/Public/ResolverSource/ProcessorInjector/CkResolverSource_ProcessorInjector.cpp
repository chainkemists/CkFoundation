#include "CkResolverSource_ProcessorInjector.h"

#include "ResolverSource/CkResolverSource_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ResolverSource_ProcessorInjector_HandleRequests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_ResolverSource_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
