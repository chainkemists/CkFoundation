#include "CkLocator_ProcessorInjector.h"

#include "CkSpatialQuery/Locator/CkLocator_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Locator_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Locator_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Locator_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Locator_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Locator_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------