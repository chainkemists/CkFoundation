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
// Server-side handlers for owning-client → server RPCs. Handle resolution is transparent
// (FCk_Handle::NetSerialize round-trips through the ReplicationDriver), so InSMHandle arrives
// pointing at the server's local SM entity — but resolution alone is NOT authorization: the
// argument can name ANY replicated entity, hence DoGet_IsAuthorizedOwningClientPush on every path.
//
// --------------------------------------------------------------------------------------------------------------------

namespace ck_state_machine_relay_actor
{
    // The SM-handle argument is attacker-controlled, so a push is authorized only when all four
    // checks below hold. Dropping check 3 would let a client drive a ServerAuthoritative SM past
    // FProcessor_Sm_HandleRequests' gate; dropping check 4 would let one player's forged fingerprint
    // stamp FTag_Sm_DeterminismFault on a victim's SM and permanently quiesce it.
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

    // Identity routing: an empty _SubSmIdentity is a root-level event and enqueues on InSMHandle.
    // A parent-hierarchy path means the event was relayed through the root on behalf of a
    // non-replicated sub-SM, so it must land on the server's matching local sub-SM, not the root.
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
            // The hosting parent state isn't active yet, so the sub-SM doesn't exist here. Log and
            // drop rather than stash — a mis-timed event must not land on the wrong SM.
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

    // A single synthesized event (server's local current → incoming target) drives the transition
    // through the standard commit pipeline, which publishes a NoHistory delta to non-owning clients.
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

    // The request pipeline is bypassed because HandleRequests' single-authority gate would drop a
    // server-originated request on an OwningClientAuth SM. The defer-while-replaying wrapper keeps a
    // non-Running status from jumping ahead of relayed transitions still queued on this SM.
    ck::statemachine::MirrorRunStatus_OrDeferWhileReplaying(InSMHandle, InRunStatus);

    // Republish for non-owning clients. Best-effort: TryUpdateContainerFragment no-ops when the
    // entity has no rep driver yet, which happens during early startup races.
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
