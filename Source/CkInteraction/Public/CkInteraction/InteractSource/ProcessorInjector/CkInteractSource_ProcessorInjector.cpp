#include "CkInteractSource_ProcessorInjector.h"

#include "CkInteraction/InteractSource/CkInteractSource_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InteractSource_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_InteractSource_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_InteractSource_Teardown>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_InteractSource_HandleRequests>(InWorld.Get_Registry());

	// Currently unused
    // InWorld.Add<ck::FProcessor_InteractSource_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------