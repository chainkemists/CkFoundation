#include "CkInteraction_ProcessorInjector.h"

#include "CkInteraction/Interaction/CkInteraction_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Interaction_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Interaction_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Interaction_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Interaction_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------