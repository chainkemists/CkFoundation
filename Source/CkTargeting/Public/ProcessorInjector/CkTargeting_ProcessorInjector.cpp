#include "CkTargeting_ProcessorInjector.h"

#include "Targetable/CkTargetable_Processor.h"
#include "Targeter/CkTargeter_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Targeting_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Targetable_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Targeter_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Targeter_Update>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Targeter_Cleanup>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Targeting_ProcessorInjector_Requests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Targetable_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Targeter_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
