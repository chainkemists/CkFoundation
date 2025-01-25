#include "CkShapeCylinder_ProcessorInjector.h"

#include "CkShapes/Cylinder/CkShapeCylinder_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ShapeCylinder_ProcessorInjector_Requests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ShapeCylinder_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------