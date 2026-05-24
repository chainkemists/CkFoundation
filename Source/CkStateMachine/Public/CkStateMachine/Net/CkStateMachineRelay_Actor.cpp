#include "CkStateMachineRelay_Actor.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
//
// Server-side handlers for owning-client → server RPCs. Cross-machine handle resolution is
// transparent: FCk_Handle::NetSerialize (CkHandle.cpp) serializes the entity's ReplicationDriver
// UObject pointer; on the receive side it dereferences the driver and reads back the local
// AssociatedEntity, so InSMHandle arrives already pointing at the server's local SM entity.
//
// The three handlers route incoming intent into the same pipeline the server's own commits use:
//
//   PushTransitionBatch  → ReplayQueue → ApplyReplicatedHistory → CommitPendingTransition →
//                          publication path (1) above broadcasts to non-owning clients
//
//   PushCurrentState     → synthesize a single event from server's local current → incoming
//                          target class, same pipeline
//
//   PushRunStatus        → MirrorRunStatus locally + republish RunStatus into RepData. Doesn't
//                          go through HandleRequests because Phase 6.4's authority gate would
//                          drop the request (server isn't the originator for OwningClientAuth SMs)
//
// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_StateMachineRelay_UE::
    Server_PushTransitionBatch_Implementation(
        FCk_Handle InSMHandle,
        const TArray<FCk_Sm_TransitionEvent>& InBatch)
    -> void
{
    if (ck::Is_NOT_Valid(InSMHandle))
    {
        ck::sm::Warning(TEXT("Server_PushTransitionBatch: SM handle did not resolve on server (driver missing or entity destroyed). Dropping [{}] events."),
            InBatch.Num());
        return;
    }

    if (InBatch.IsEmpty())
    { return; }

    ck::sm::Verbose(TEXT("Server_PushTransitionBatch: enqueuing [{}] events on SM [{}]"),
        InBatch.Num(), InSMHandle);

    // Push events onto the server's replay queue. ApplyReplicatedHistory drains one per tick
    // into PendingTransition; CommitPendingTransition then lands the transition and republishes
    // via the IsRepPublisher branch in CkStateMachine_Processor.cpp. NextSeq on the server
    // assigns its own monotonic sequence — non-owning clients see server-side seqs (consistent
    // within the server's history), and the owning client's echo suppression prevents it from
    // ever interpreting the server's seqs (its own _ClientLastAppliedSeq is its own bookkeeping).
    auto& Queue = InSMHandle.AddOrGet<ck::FFragment_Sm_ReplayQueue>().Get_Queue();
    for (const auto& Event : InBatch)
    {
        Queue.Add(Event);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_StateMachineRelay_UE::
    Server_PushCurrentState_Implementation(
        FCk_Handle InSMHandle,
        TSubclassOf<UCk_SmState_EntityScript> InCurrentStateClass,
        int32 InSeq,
        int32 InCurrentStateFingerprint)
    -> void
{
    if (ck::Is_NOT_Valid(InSMHandle))
    {
        ck::sm::Warning(TEXT("Server_PushCurrentState: SM handle did not resolve on server. Dropping push to state [{}]."),
            InCurrentStateClass);
        return;
    }

    if (ck::Is_NOT_Valid(InCurrentStateClass))
    {
        ck::sm::Warning(TEXT("Server_PushCurrentState: invalid target state class on SM [{}]."), InSMHandle);
        return;
    }

    // Synthesize a single transition event from the server's local current state → the
    // incoming target. The replay queue path then drives the transition through the standard
    // commit pipeline (which publishes a NoHistory rep delta to non-owning clients).
    const auto LocalCurrentClass = InSMHandle.Has<ck::FFragment_Sm_Current>()
        ? InSMHandle.Get<ck::FFragment_Sm_Current>().Get_CurrentStateClass()
        : TSubclassOf<UCk_SmState_EntityScript>{};

    const auto Event = FCk_Sm_TransitionEvent
    {
        LocalCurrentClass,
        InCurrentStateClass,
        InSeq,
        InCurrentStateFingerprint
    };

    ck::sm::Verbose(TEXT("Server_PushCurrentState: enqueuing snap on SM [{}] -> state [{}]"),
        InSMHandle, InCurrentStateClass);

    auto& Queue = InSMHandle.AddOrGet<ck::FFragment_Sm_ReplayQueue>().Get_Queue();
    Queue.Add(Event);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_StateMachineRelay_UE::
    Server_PushRunStatus_Implementation(
        FCk_Handle InSMHandle,
        ECk_SmRunStatus InRunStatus)
    -> void
{
    if (ck::Is_NOT_Valid(InSMHandle))
    {
        ck::sm::Warning(TEXT("Server_PushRunStatus: SM handle did not resolve on server. Dropping push of status [{}]."),
            InRunStatus);
        return;
    }

    // Apply the run-status to the server's local SM, mirroring the same lifecycle bookkeeping
    // and signal broadcast that authority's Start/Stop/Pause/Resume handlers do. We skip the
    // request pipeline because Phase 6.4's HandleRequests gate would drop a server-originated
    // request on an OwningClientAuth SM (owning client is the request authority, not the server).
    ck::statemachine::MirrorRunStatus(InSMHandle, InRunStatus);

    // Republish into RepData so non-owning clients pick up the run-status change. Best-effort:
    // TryUpdateContainerFragment silently no-ops if the entity doesn't have a rep driver yet,
    // which can happen during early startup races.
    if (NOT InSMHandle.Has<ck::FFragment_Sm_Params>())
    { return; }

    const auto& Params = InSMHandle.Get<ck::FFragment_Sm_Params>();
    if (Params.Get_Replication() != ECk_Replication::Replicates)
    { return; }

    switch (Params.Get_ReplicationModel())
    {
        case ECk_Sm_ReplicationModel::WithHistory:
        {
            UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_StateMachine_WithHistory>(
                InSMHandle,
                [&](FCk_RepData_StateMachine_WithHistory& RepData) -> void
                {
                    RepData.Set_RunStatus(InRunStatus);
                });
            break;
        }
        case ECk_Sm_ReplicationModel::WithoutHistory:
        {
            UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_StateMachine_NoHistory>(
                InSMHandle,
                [&](FCk_RepData_StateMachine_NoHistory& RepData) -> void
                {
                    RepData.Set_RunStatus(InRunStatus);
                });
            break;
        }
    }

    ck::sm::Verbose(TEXT("Server_PushRunStatus: applied [{}] on SM [{}]"),
        InRunStatus, InSMHandle);
}

// --------------------------------------------------------------------------------------------------------------------
