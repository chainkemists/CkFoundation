#include "CkNetProcessorInjector.h"

#include "CkNet/TimeSync/CkNetTimeSync_Processor.h"

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

// --------------------------------------------------------------------------------------------------------------------
