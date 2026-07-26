#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/CkStateMachine_Stats.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Sm::ComputeNetContext"), STAT_Sm_ComputeNetContext, STATGROUP_CkStateMachine);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::statemachine
{
    auto
    DoGet_IsTransitionAuthority_Live(
        const FCk_Handle_StateMachine& InSm) -> bool;

    auto
        ComputeNetContext(
            const FCk_Handle_StateMachine& InSm)
        -> ECk_Sm_NetContext
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_ComputeNetContext);

        // Sub-SMs carry a resolved-once snapshot (they can't resolve their owning pawn live —
        // see FFragment_Sm_NetIdentity). Use it when present; top-level SMs fall through to live.
        if (InSm.Has<ck::FFragment_Sm_NetIdentity>())
        { return InSm.Get<ck::FFragment_Sm_NetIdentity>().Get_NetContext(); }

        // Per-frame memo: the live resolve below walks owning-actor/PlayerState chains and the
        // lifecycle processors call it several times per SM element per frame.
        auto Sm = InSm;
        auto& Memo = Sm.AddOrGet<ck::FFragment_Sm_NetContextMemo>();
        if (Memo.Get_NetContextFrame() == GFrameCounter)
        { return Memo.Get_NetContext(); }

        const auto LiveNetContext = [&]() -> ECk_Sm_NetContext
        {
            // DoesNotReplicate SMs are self-authoritative on EVERY machine, so Standalone (not the
            // machine's real role) is the correct answer: a local-only SM hosted on a replicated,
            // non-player-owned entity would otherwise resolve NonOwningClient and go inert there.
            if (UCk_Utils_StateMachine_UE::Get_Replication(InSm) == ECk_Replication::DoesNotReplicate)
            { return ECk_Sm_NetContext::Standalone; }

            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InSm);

            if (ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}) && World->IsNetMode(NM_Standalone))
            { return ECk_Sm_NetContext::Standalone; }

            if (UCk_Utils_Net_UE::Get_HasAuthority(InSm))
            { return ECk_Sm_NetContext::Server; }

            const auto LocallyControlled = UCk_Utils_Net_UE::Get_IsEntityLocallyControlled_ByPlayer(InSm);

            if (LocallyControlled == ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled)
            { return ECk_Sm_NetContext::OwningClient; }

            return ECk_Sm_NetContext::NonOwningClient;
        }();

        Memo.Set_NetContextFrame(GFrameCounter);
        Memo.Set_NetContext(LiveNetContext);
        return LiveNetContext;
    }

    auto
        Get_IsTransitionAuthority(
            const FCk_Handle_StateMachine& InSm)
        -> bool
    {
        // Per-frame memo — same contract as ComputeNetContext's above; this predicate gates the
        // state, transition, condition and task processors, once per element per frame.
        auto Sm = InSm;
        auto& Memo = Sm.AddOrGet<ck::FFragment_Sm_NetContextMemo>();
        if (Memo.Get_TransitionAuthorityFrame() == GFrameCounter)
        { return Memo.Get_IsTransitionAuthority(); }

        const auto IsTransitionAuthority = DoGet_IsTransitionAuthority_Live(InSm);
        Memo.Set_TransitionAuthorityFrame(GFrameCounter);
        Memo.Set_IsTransitionAuthority(IsTransitionAuthority);
        return IsTransitionAuthority;
    }

    auto
        DoGet_IsTransitionAuthority_Live(
            const FCk_Handle_StateMachine& InSm)
        -> bool
    {
        const auto NetContext    = ComputeNetContext(InSm);
        const auto EffectiveAuth = UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(InSm);
        const auto Replication   = UCk_Utils_StateMachine_UE::Get_Replication(InSm);

        // The locally-controlled probe MUST target the ROOT SM (the one bridged to the pawn): a sub-SM
        // is a driverless child entity with no owning actor, so probing it directly always returns
        // IsNotValidPawn and a listen host would be misread as not-the-owning-client, freezing its own
        // sub-SM. Get_RootStateMachine returns InSm unchanged for a top-level SM.
        const auto RootSm = UCk_Utils_StateMachine_UE::Get_RootStateMachine(InSm);
        const auto IsHostOwningClient =
            NetContext == ECk_Sm_NetContext::Server
            && EffectiveAuth == ECk_Sm_AuthorityModel::OwningClientAuthoritative
            && UCk_Utils_Net_UE::Get_IsEntityLocallyControlled_ByPlayer(RootSm)
                == ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled;

        // The single exception to the DoesNotReplicate-is-self-authoritative shortcut below: a RELAYED
        // sub-SM must not self-evaluate anywhere but the owning client, the only machine holding the
        // client-local input. Elsewhere it would evaluate the release condition against its own zeroed
        // input copy and revert the relayed state immediately. ServerAuthoritative sub-SMs are
        // untouched and still self-derive everywhere from their replicated inputs.
        if (Replication == ECk_Replication::DoesNotReplicate
            && EffectiveAuth == ECk_Sm_AuthorityModel::OwningClientAuthoritative
            && (NetContext == ECk_Sm_NetContext::Server || NetContext == ECk_Sm_NetContext::NonOwningClient)
            && NOT IsHostOwningClient)
        { return false; }

        // Otherwise mirror FProcessor_Sm_HandleRequests' request-authority, INCLUDING the
        // DoesNotReplicate-self-authoritative shortcut. Replicates SMs do NOT get that shortcut:
        // their non-authority machines follow replication and never self-evaluate.
        return Replication == ECk_Replication::DoesNotReplicate
            || NetContext == ECk_Sm_NetContext::Standalone
            || (NetContext == ECk_Sm_NetContext::Server
                && EffectiveAuth == ECk_Sm_AuthorityModel::ServerAuthoritative)
            || (NetContext == ECk_Sm_NetContext::OwningClient
                && EffectiveAuth == ECk_Sm_AuthorityModel::OwningClientAuthoritative)
            || IsHostOwningClient;
    }

    auto
        MirrorRunStatus(
            FCk_Handle& InEntity,
            ECk_SmRunStatus InNewStatus)
        -> void
    {
        if (NOT InEntity.Has<ck::FFragment_Sm_Current>())
        { return; }

        auto& Current = InEntity.Get<ck::FFragment_Sm_Current>();
        const auto OldStatus = Current.Get_RunStatus();
        if (OldStatus == InNewStatus)
        { return; }

        Current.Set_RunStatus(InNewStatus);

        auto SmHandle = UCk_Utils_StateMachine_UE::CastChecked(InEntity);

        switch (InNewStatus)
        {
            case ECk_SmRunStatus::Running:
            {
                InEntity.AddOrGet<ck::FTag_Sm_Running>();
                InEntity.Try_Remove<ck::FTag_Sm_Paused>();

                if (OldStatus == ECk_SmRunStatus::Stopped)
                {
                    // First sync: this non-authority machine never ran DoStart and the initial-state
                    // entry is not a replayed transition, so schedule it here or the view shows <none>.
                    // FProcessor_Sm_FirstSyncInitialState does the entry, outside this RPC callback.
                    if (ck::Is_NOT_Valid(Current.Get_CurrentStateHandle()))
                    {
                        InEntity.AddOrGet<ck::FTag_Sm_NeedsInitialStateEntry>();
                    }

                    ck::UUtils_Signal_OnSmStarted::Broadcast(SmHandle,
                        ck::MakePayload(SmHandle, FCk_Sm_Payload_OnStarted{}));
                }
                break;
            }
            case ECk_SmRunStatus::Paused:
            {
                InEntity.AddOrGet<ck::FTag_Sm_Paused>();
                break;
            }
            case ECk_SmRunStatus::Stopped:
            {
                InEntity.Try_Remove<ck::FTag_Sm_Running>();
                InEntity.Try_Remove<ck::FTag_Sm_Paused>();

                ck::UUtils_Signal_OnSmStopped::Broadcast(SmHandle,
                    ck::MakePayload(SmHandle, FCk_Sm_Payload_OnStopped{}));
                break;
            }
        }

        ck::sm::VeryVerbose(TEXT("Mirrored RunStatus on [{}]: [{}] -> [{}]"),
            InEntity, OldStatus, InNewStatus);
    }

    auto
        MirrorRunStatus_OrDeferWhileReplaying(
            FCk_Handle& InEntity,
            ECk_SmRunStatus InNewStatus)
        -> void
    {
        if (InNewStatus != ECk_SmRunStatus::Running
            && UCk_Utils_StateMachine_UE::Get_HasReplayTransitionsInFlight(InEntity))
        {
            InEntity.AddOrGet<ck::FFragment_Sm_DeferredRunStatusMirror>().Set_Status(InNewStatus);
            return;
        }

        // A Running mirror supersedes any parked non-Running status: the authority's LAST status wins.
        InEntity.Try_Remove<ck::FFragment_Sm_DeferredRunStatusMirror>();
        MirrorRunStatus(InEntity, InNewStatus);
    }
}
