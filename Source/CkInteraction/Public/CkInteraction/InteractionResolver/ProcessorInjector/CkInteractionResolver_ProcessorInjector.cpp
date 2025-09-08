#include "CkInteractionResolver_ProcessorInjector.h"

#include "CkInteraction/InteractionResolver/CkInteractionResolver_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InteractionResolver_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_InteractionResolver_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_InteractionResolver_Persistent>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_InteractionResolver_Teardown>(InWorld.Get_Registry());

    // stubbed, but not yet used
    // InWorld.Add<ck::FProcessor_InteractionResolver_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------