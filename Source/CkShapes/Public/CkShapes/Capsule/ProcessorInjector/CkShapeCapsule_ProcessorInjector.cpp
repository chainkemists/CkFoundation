#include "CkShapeCapsule_ProcessorInjector.h"

#include "CkShapes/Capsule/CkShapeCapsule_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ShapeCapsule_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_ShapeCapsule_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ShapeCapsule_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ShapeCapsule_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_ShapeCapsule_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------