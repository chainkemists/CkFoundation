#include "CkGrid_ProcessorInjector.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Grid_ProcessorInjector_Debug::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_2dGridSystem_DebugDrawAll>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------