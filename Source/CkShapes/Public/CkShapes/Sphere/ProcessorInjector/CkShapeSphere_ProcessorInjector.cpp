#include "CkShapeSphere_ProcessorInjector.h"

#include "CkShapes/Sphere/CkShapeSphere_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ShapeSphere_ProcessorInjector_Requests::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ShapeSphere_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------