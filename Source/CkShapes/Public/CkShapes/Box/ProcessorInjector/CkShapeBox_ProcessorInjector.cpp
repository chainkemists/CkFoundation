#include "CkShapeBox_ProcessorInjector.h"

#include "CkShapes/Box/CkShapeBox_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ShapeBox_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_ShapeBox_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ShapeBox_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ShapeBox_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_ShapeBox_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------