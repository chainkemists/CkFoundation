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

    // True when THIS machine may run the SM's transition-decision machinery (evaluate conditions +
    // walk transitions). Machines that follow replicated/relayed transitions instead must NOT run it,
    // or they fight the relay.
    //
    // Mirrors FProcessor_Sm_HandleRequests' request-authority — Standalone, OR Server+ServerAuth, OR
    // OwningClient+OwningClientAuth, OR the listen-server host owning its own pawn, OR the
    // DoesNotReplicate self-authoritative shortcut (a non-replicated local sub-SM self-derives on every
    // machine from its replicated inputs; this is the framework's original sub-SM design and keeps
    // non-owning clients deriving sub-SM state) — with ONE exception:
    //
    //   A RELAYED sub-SM (DoesNotReplicate but with an OwningClientAuthoritative NetIdentity inherited
    //   from its replicated root) returns FALSE here on every machine EXCEPT the owning client: the server
    //   the host does NOT own AND non-owning observer clients (P4). Both follow the relay — the server the
    //   owning client's relayed transitions, observers the server's leg-2 republish onto the root's
    //   WithHistory container — rather than self-evaluate (which would revert the relayed state on a
    //   locally-true release condition — the sprint self-revert). The START lifecycle in HandleRequests
    //   keeps the plain DoesNotReplicate shortcut, so the sub-SM still starts locally on these machines to
    //   exist for the relay; only transition DECISIONS are suppressed there.
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
