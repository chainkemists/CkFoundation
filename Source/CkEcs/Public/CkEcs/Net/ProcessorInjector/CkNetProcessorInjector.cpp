#include "CkNetProcessorInjector.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Net_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
}

auto
    UCk_Net_ProcessorInjector_HandleRequests::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Net_ProcessorInjector_Replicate::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ReplicationDriver_FireOnDependentReplicationComplete>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
