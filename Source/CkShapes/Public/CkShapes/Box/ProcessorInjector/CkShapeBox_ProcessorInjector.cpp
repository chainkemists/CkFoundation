#include "CkShapeBox_ProcessorInjector.h"

#include "CkShapes/Box/CkShapeBox_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ShapeBox_ProcessorInjector_Requests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ShapeBox_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------