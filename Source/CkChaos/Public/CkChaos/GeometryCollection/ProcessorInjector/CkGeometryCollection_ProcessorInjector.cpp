#include "CkGeometryCollection_ProcessorInjector.h"

#include "CkChaos/GeometryCollection/CkGeometryCollection_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GeometryCollection_ProcessorInjector_Requests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_GeometryCollection_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
