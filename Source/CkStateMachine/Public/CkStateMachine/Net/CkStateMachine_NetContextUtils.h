#pragma once

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

namespace ck::statemachine
{
    // Computes the NetContext for a given SM entity based on its current authority/role.
    // Resolved fresh per call from UCk_Utils_Net_UE queries — no caching, no fragment storage.
    CKSTATEMACHINE_API auto
    ComputeNetContext(
        const FCk_Handle_StateMachine& InSm) -> ECk_Sm_NetContext;

    // Client-side run-status mirror. Updates FFragment_Sm_Current._RunStatus to InNewStatus,
    // bookkeeps FTag_Sm_Running/FTag_Sm_Paused, and fires OnSmStarted/OnSmStopped signals so
    // non-authority machines see the same lifecycle pulses as authority. No-op when the
    // entity lacks FFragment_Sm_Current or the status is already current. Used by Phase 11's
    // OnChange/OnAdd handlers (direct path) and FlushPendingReplication_Drain (stashed path).
    CKSTATEMACHINE_API auto
    MirrorRunStatus(
        FCk_Handle& InEntity,
        ECk_SmRunStatus InNewStatus) -> void;
}
