#include "CkIsmRenderer_ProcessorInjector.h"

#include "CkIsmRenderer/CkIsmRenderer_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_IsmRenderer_ClearInstances>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_IsmProxy_Dynamic>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_IsmRenderer_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_IsmProxy_Setup>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
