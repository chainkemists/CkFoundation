#include "CkEntityBridge_ProcessorInjector.h"

#include "CkEntityBridge/CkEntityBridge_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityBridge_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_EntityBridge_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
