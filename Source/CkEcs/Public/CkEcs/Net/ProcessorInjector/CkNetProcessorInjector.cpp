#include "CkNetProcessorInjector.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Processor.h"
#include "CkEcs/Net/TimeSync/CkNetTimeSync_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Net_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_NetTimeSync_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_NetTimeSync_OnNetworkClockSynchronized>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_NetTimeSync_FirstSync>(InWorld.Get_Registry());
}

auto
    UCk_Net_ProcessorInjector_HandleRequests::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_ReplicationDriver_ReplicateEntityScript>(InWorld.Get_Registry());
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
