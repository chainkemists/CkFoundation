#include "CkShapeCapsule_ProcessorInjector.h"

#include "CkShapes/Capsule/CkShapeCapsule_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ShapeCapsule_ProcessorInjector_Requests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ShapeCapsule_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------