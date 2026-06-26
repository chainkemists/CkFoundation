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
        ComputeNetContext(
            const FCk_Handle_StateMachine& InSm)
        -> ECk_Sm_NetContext
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_ComputeNetContext);

        // Sub-SMs carry a resolved-once snapshot (they can't resolve their owning pawn live —
        // see FFragment_Sm_NetIdentity). Use it when present; top-level SMs fall through to live.
        if (InSm.Has<ck::FFragment_Sm_NetIdentity>())
        { return InSm.Get<ck::FFragment_Sm_NetIdentity>().Get_NetContext(); }

        // DoesNotReplicate SMs are self-authoritative on EVERY machine — the same contract
        // FProcessor_Sm_HandleRequests already honors (it exempts DoesNotReplicate from the
        // single-authority gate). They have no replicated history to replay, so the lifecycle
        // processors' NonOwningClient / OwningClient gates (FProcessor_SmTask_Tick,
        // FProcessor_SmCondition, FProcessor_SmTransition) must NOT suppress them. A local-only SM
        // hosted on a server-replicated, non-player-owned entity would otherwise resolve
        // NonOwningClient on a client and go inert there while running fine on the host. Resolving
        // to Standalone makes each machine run the full lifecycle locally, which IS the
        // DoesNotReplicate semantic. (Sub-SMs short-circuit above via FFragment_Sm_NetIdentity, so
        // their stamped identity is preserved either way.)
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
    }

    auto
        Get_IsTransitionAuthority(
            const FCk_Handle_StateMachine& InSm)
        -> bool
    {
        const auto NetContext    = ComputeNetContext(InSm);
        const auto EffectiveAuth = UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(InSm);
        const auto Replication   = UCk_Utils_StateMachine_UE::Get_Replication(InSm);

        const auto IsHostOwningClient =
            NetContext == ECk_Sm_NetContext::Server
            && EffectiveAuth == ECk_Sm_AuthorityModel::OwningClientAuthoritative
            && UCk_Utils_Net_UE::Get_IsEntityLocallyControlled_ByPlayer(InSm)
                == ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled;

        // Suppress self-evaluation for a RELAYED sub-SM (DoesNotReplicate, but carrying an
        // OwningClientAuthoritative NetIdentity inherited from its replicated root) on EVERY machine
        // except the owning client — the only machine that holds the client-local input and originates
        // the transition. Two such machines:
        //   - the server the host does NOT own (it follows the owning client's relayed transitions), and
        //   - non-owning observer clients (P4: they follow the server's leg-2 republish onto the root's
        //     WithHistory container — they have neither the non-replicated input nor authority).
        // Without this, such a machine self-evaluates the release condition (its local input copy is 0)
        // and reverts the relayed state right back (the sprint self-revert). This is the single exception
        // to the DoesNotReplicate-is-self-authoritative shortcut below; a ServerAuthoritative sub-SM still
        // self-derives on every machine from its replicated inputs (the "preserve non-owning self-derive"
        // intent is untouched — only OwningClientAuth relayed sub-SMs, now driven by the relay, are gated).
        if (Replication == ECk_Replication::DoesNotReplicate
            && EffectiveAuth == ECk_Sm_AuthorityModel::OwningClientAuthoritative
            && (NetContext == ECk_Sm_NetContext::Server || NetContext == ECk_Sm_NetContext::NonOwningClient)
            && NOT IsHostOwningClient)
        { return false; }

        // Otherwise mirror FProcessor_Sm_HandleRequests' request-authority, INCLUDING the
        // DoesNotReplicate-self-authoritative shortcut. A non-relayed local sub-SM (ServerAuth or
        // standalone NetIdentity) still self-derives on EVERY machine from its replicated inputs, so
        // non-owning clients keep deriving sub-SM state exactly as before this gate was introduced.
        // Replicates SMs do NOT get the shortcut: their non-authority machines (server of an
        // OwningClientAuth root, all non-owning clients) follow replication and never self-evaluate.
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
                    // First sync: this non-authority machine just learned the SM started, but it
                    // never ran DoStart (Start is authority-only) and the initial-state entry isn't
                    // a replayed transition. If it has no current state yet, schedule entry into the
                    // locally-known initial state so non-owning views show it instead of <none>.
                    // The actual entry runs in FProcessor_Sm_FirstSyncInitialState (registry-safe,
                    // outside this replication/RPC callback). Subsequent transitions replay on top.
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
}
