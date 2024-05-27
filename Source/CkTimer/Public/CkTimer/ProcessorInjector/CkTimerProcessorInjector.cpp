#include "CkTimerProcessorInjector.h"

#include "CkTimer/CkTimer_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Timer_ProcessorInjector_Requests::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Timer_HandleRequests>(InWorld.Get_Registry());
}

auto
    UCk_Timer_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Timer_Update>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Timer_Update_Countdown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
