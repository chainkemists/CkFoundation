#pragma once

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

namespace ck::statemachine
{
    // Computes the NetContext for a given SM entity based on its current authority/role.
    // Resolved fresh per call from UCk_Utils_Net_UE queries — EXCEPT for sub-SMs, which carry a
    // resolved-once snapshot in FFragment_Sm_NetIdentity (they can't resolve their owning pawn
    // live); when that fragment is present its stored NetContext is returned directly.
    CKSTATEMACHINE_API auto
    ComputeNetContext(
        const FCk_Handle_StateMachine& InSm) -> ECk_Sm_NetContext;

    // True when THIS machine is the SM's transition authority — the one allowed to evaluate
    // conditions and decide transitions. Everyone else FOLLOWS the replicated/relayed transitions
    // (the server of an OwningClientAuth SM, all non-owning clients) and must not run the decision
    // machinery (condition enter/tick + state-evaluate walk), so they don't fight the relay.
    //
    // Authority = Standalone, OR Server+ServerAuth, OR OwningClient+OwningClientAuth, OR the
    // listen-server host owning its own pawn (Server context + OwningClientAuth + locally controlled).
    //
    // NOTE: unlike the request-authority gate in FProcessor_Sm_HandleRequests, this does NOT treat
    // DoesNotReplicate as self-authoritative-everywhere. That shortcut is needed for the START
    // lifecycle (a non-replicated sub-SM must start locally on every machine so it exists to receive
    // relayed transitions), but it is WRONG for transition decisions: a relayed sub-SM is
    // DoesNotReplicate yet carries an OwningClientAuth NetIdentity, so the server must follow the relay,
    // not self-evaluate. ComputeNetContext already maps a genuinely-local (no NetIdentity, root)
    // DoesNotReplicate SM to Standalone, so it resolves as authority here without the shortcut.
    CKSTATEMACHINE_API auto
    Get_IsTransitionAuthority(
        const FCk_Handle_StateMachine& InSm) -> bool;

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
