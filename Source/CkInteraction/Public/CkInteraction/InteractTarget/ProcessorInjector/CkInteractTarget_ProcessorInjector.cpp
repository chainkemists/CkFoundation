#include "CkInteractTarget_ProcessorInjector.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InteractTarget_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_InteractTarget_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_InteractTarget_EndPlay>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_InteractTarget_HandleRequests>(InWorld.Get_Registry());

}

// --------------------------------------------------------------------------------------------------------------------