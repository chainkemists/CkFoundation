#include "CkStateMachineRelay_Actor.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

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
//                          go through HandleRequests because the single-authority gate would
//                          drop the request (server isn't the originator for OwningClientAuth SMs)
//
// Every handler is gated by DoGet_IsAuthorizedOwningClientPush below — the handle argument can
// point at ANY replicated entity, so resolution alone is not authorization.
//
// --------------------------------------------------------------------------------------------------------------------

namespace ck_state_machine_relay_actor
{
    // Server-side authorization for owning-client relay pushes. UE only routes Server RPCs from
    // the relay actor's OWNING connection, so the caller's identity is this relay's connection —
    // but the SM-handle argument is attacker-controlled and can resolve to any replicated entity.
    // A push is authorized only when ALL hold:
    //   1. the handle is actually a StateMachine entity,
    //   2. its root SM Replicates (the relay is the leg-1 transport of a replicated root),
    //   3. its root's effective authority model is OwningClientAuthoritative (a client must never
    //      drive a ServerAuthoritative SM — that bypasses FProcessor_Sm_HandleRequests' gate),
    //   4. the root's owning actor belongs to this relay's connection (a client must not drive
    //      another player's SM; a forged fingerprint would otherwise let one RPC stamp
    //      FTag_Sm_DeterminismFault on a victim's SM and permanently quiesce it).
    auto
    DoGet_IsAuthorizedOwningClientPush(
        const AActor* InRelay,
        const FCk_Handle& InSMHandle,
        const TCHAR* InContext) -> bool
    {
        if (NOT UCk_Utils_StateMachine_UE::Has(InSMHandle))
        {
            ck::sm::Warning(TEXT("{}: pushed handle [{}] is not a StateMachine entity. Dropping."),
                InContext, InSMHandle);
            return false;
        }

        const auto SmHandle = UCk_Utils_StateMachine_UE::CastChecked(InSMHandle);
        const auto RootSm   = UCk_Utils_StateMachine_UE::Get_RootStateMachine(SmHandle);

        if (UCk_Utils_StateMachine_UE::Get_Replication(RootSm) != ECk_Replication::Replicates)
        {
            ck::sm::Warning(TEXT("{}: SM [{}] root does not Replicate — relay pushes are only valid for replicated roots. Dropping."),
                InContext, InSMHandle);
            return false;
        }

        if (UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(RootSm) != ECk_Sm_AuthorityModel::OwningClientAuthoritative)
        {
            ck::sm::Warning(TEXT("{}: SM [{}] root is not OwningClientAuthoritative — a client may not drive it. Dropping."),
                InContext, InSMHandle);
            return false;
        }

        const auto* OwningActor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor_Recursive(FCk_Handle{RootSm});
        if (OwningActor == nullptr)
        {
            ck::sm::Warning(TEXT("{}: SM [{}] root has no owning actor — cannot verify connection ownership. Dropping."),
                InContext, InSMHandle);
            return false;
        }

        const auto* SmConnection    = OwningActor->GetNetConnection();
        const auto* RelayConnection = InRelay->GetNetConnection();
        if (SmConnection == nullptr || SmConnection != RelayConnection)
        {
            ck::sm::Warning(TEXT("{}: SM [{}] is not owned by the pushing connection. Dropping."),
                InContext, InSMHandle);
            return false;
        }

        return true;
    }
}

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

    if (NOT ck_state_machine_relay_actor::DoGet_IsAuthorizedOwningClientPush(this, InSMHandle, TEXT("Server_PushTransitionBatch")))
    { return; }

    ck::sm::Verbose(TEXT("Server_PushTransitionBatch: enqueuing [{}] events on SM [{}]"),
        InBatch.Num(), InSMHandle);

    // Push events onto the appropriate replay queue. ApplyReplicatedHistory drains one per tick
    // into PendingTransition; CommitPendingTransition then lands the transition and republishes
    // via the IsRepPublisher branch in CkStateMachine_Processor.cpp. NextSeq on the server
    // assigns its own monotonic sequence — non-owning clients see server-side seqs (consistent
    // within the server's history), and the owning client's echo suppression prevents it from
    // ever interpreting the server's seqs (its own _ClientLastAppliedSeq is its own bookkeeping).
    //
    // Identity routing: a root-level event (empty _SubSmIdentity) enqueues on InSMHandle (the root).
    // An event tagged with a sub-SM parent-hierarchy path was relayed through the root on behalf of a
    // non-replicated sub-SM — resolve the server's matching local sub-SM and enqueue onto ITS replay
    // queue so the transition lands on the right SM, not the root.
    const auto RootHandle = UCk_Utils_StateMachine_UE::CastChecked(InSMHandle);

    for (const auto& Event : InBatch)
    {
        const auto& SubSmIdentity = Event.Get_SubSmIdentity();

        if (SubSmIdentity.IsEmpty())
        {
            InSMHandle.AddOrGet<ck::FFragment_Sm_ReplayQueue>().Get_Queue().Add(Event);
            continue;
        }

        auto TargetSubSm = UCk_Utils_StateMachine_UE::TryFind_ActiveSubSm_ByParentHierarchy(RootHandle, SubSmIdentity);
        if (ck::Is_NOT_Valid(TargetSubSm))
        {
            // The hosting parent state isn't active yet (the sub-SM doesn't exist on this machine).
            // P4 will stash-and-defer for the parent-then-child ordering case; for now log + drop so a
            // mis-timed event can't land on the wrong SM.
            ck::sm::Warning(TEXT("Server_PushTransitionBatch: sub-SM for identity-path [{}] not active under root [{}]; dropping event (seq [{}])"),
                SubSmIdentity.Num(), InSMHandle, Event.Get_Seq());
            continue;
        }

        TargetSubSm.AddOrGet<ck::FFragment_Sm_ReplayQueue>().Get_Queue().Add(Event);
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

    if (NOT ck_state_machine_relay_actor::DoGet_IsAuthorizedOwningClientPush(this, InSMHandle, TEXT("Server_PushCurrentState")))
    { return; }

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

    if (NOT ck_state_machine_relay_actor::DoGet_IsAuthorizedOwningClientPush(this, InSMHandle, TEXT("Server_PushRunStatus")))
    { return; }

    // Apply the run-status to the server's local SM, mirroring the same lifecycle bookkeeping
    // and signal broadcast that authority's Start/Stop/Pause/Resume handlers do. We skip the
    // request pipeline because the HandleRequests single-authority gate would drop a
    // server-originated request on an OwningClientAuth SM (owning client is the request
    // authority, not the server). Routed through the defer-while-replaying wrapper so a
    // non-Running status cannot jump ahead of relayed transitions still in this SM's queue.
    ck::statemachine::MirrorRunStatus_OrDeferWhileReplaying(InSMHandle, InRunStatus);

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
