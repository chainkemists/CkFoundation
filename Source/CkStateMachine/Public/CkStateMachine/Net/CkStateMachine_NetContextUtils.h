#pragma once

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

namespace ck::statemachine
{
    // Resolved fresh per call from UCk_Utils_Net_UE queries — EXCEPT for sub-SMs, which carry a
    // resolved-once snapshot in FFragment_Sm_NetIdentity (they can't resolve their owning pawn
    // live); when that fragment is present its stored NetContext is returned directly.
    CKSTATEMACHINE_API auto
    ComputeNetContext(
        const FCk_Handle_StateMachine& InSm) -> ECk_Sm_NetContext;

    // True when THIS machine may run the SM's transition-decision machinery. Machines that follow
    // replicated/relayed transitions must NOT run it or they fight the relay. Full matrix (including
    // the relayed-sub-SM exception): CkStateMachine/CLAUDE.md § Transition authority.
    CKSTATEMACHINE_API auto
    Get_IsTransitionAuthority(
        const FCk_Handle_StateMachine& InSm) -> bool;

    // Client-side run-status mirror: writes _RunStatus, bookkeeps FTag_Sm_Running/FTag_Sm_Paused and
    // fires OnSmStarted/OnSmStopped. Applies IMMEDIATELY — receive paths that can race queued replayed
    // transitions must use MirrorRunStatus_OrDeferWhileReplaying instead of this raw form.
    CKSTATEMACHINE_API auto
    MirrorRunStatus(
        FCk_Handle& InEntity,
        ECk_SmRunStatus InNewStatus) -> void;

    // Ordering-safe mirror for receive paths (rep OnChange, stash drain, relay RPC). A non-Running
    // mirror landing while replayed transitions are queued is parked on
    // FFragment_Sm_DeferredRunStatusMirror and applied by the commit tail once the queue drains.
    CKSTATEMACHINE_API auto
    MirrorRunStatus_OrDeferWhileReplaying(
        FCk_Handle& InEntity,
        ECk_SmRunStatus InNewStatus) -> void;
}
