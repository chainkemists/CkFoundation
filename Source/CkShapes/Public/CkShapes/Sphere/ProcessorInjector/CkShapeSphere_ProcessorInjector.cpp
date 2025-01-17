#include "CkShapeSphere_ProcessorInjector.h"

#include "CkShapes/Sphere/CkShapeSphere_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ShapeSphere_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_ShapeSphere_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ShapeSphere_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ShapeSphere_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_ShapeSphere_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------