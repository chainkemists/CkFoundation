#include "CkGraphics_ProcessorInjector.h"

#include "CkGraphics/RenderStatus/CkRenderStatus_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Graphics_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_RenderStatus_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
