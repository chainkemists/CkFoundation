#include "CkEntityCollection_ProcessorInjector.h"

#include "CkEntityCollection/CkEntityCollection_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityCollection_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_EntityCollection_StorePrevious>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityCollection_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityCollection_FireSignals>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
