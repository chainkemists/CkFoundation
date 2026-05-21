#pragma once

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

namespace ck::statemachine
{
    // Computes the NetContext for a given SM entity based on its current authority/role.
    // Resolved fresh per call from UCk_Utils_Net_UE queries — no caching, no fragment storage.
    CKSTATEMACHINE_API auto
    ComputeNetContext(
        const FCk_Handle_StateMachine& InSm) -> ECk_Sm_NetContext;
}
