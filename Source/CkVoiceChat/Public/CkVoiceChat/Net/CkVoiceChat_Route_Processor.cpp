#include "CkVoiceChat_Route_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"
#include "CkVoiceChat/CkVoiceChat_Stats.h"
#include "CkVoiceChat/Codec/CkVoiceChat_Codec.h"
#include "CkVoiceChat/Net/CkVoiceChatRelay_Actor.h"
#include "CkVoiceChat/Net/CkVoiceChatRelay_Subsystem.h"
#include "CkVoiceChat/Settings/CkVoiceChat_Settings.h"
#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Utils.h"
#include "CkVoiceChat/VoiceListener/CkVoiceListener_Fragment.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Utils.h"

#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChat_Route);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChat_FlushForwards);

DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Forwarded"), STAT_CkVoiceChat_RouteForwarded, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Dropped (Unresolvable ChannelIdx)"), STAT_CkVoiceChat_RouteDroppedUnresolvableIdx, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Dropped (Malformed)"), STAT_CkVoiceChat_RouteDroppedMalformed, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Dropped (Sender Mismatch)"), STAT_CkVoiceChat_RouteDroppedSenderMismatch, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Dropped (Not Authorized)"), STAT_CkVoiceChat_RouteDroppedNotAuthorized, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Dropped (Server Muted)"), STAT_CkVoiceChat_RouteDroppedServerMuted, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Dropped (Budget)"), STAT_CkVoiceChat_RouteDroppedBudget, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Dropped (Listener Muted)"), STAT_CkVoiceChat_RouteDroppedListenerMuted, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Dropped (Speaker Cap)"), STAT_CkVoiceChat_RouteDroppedSpeakerCap, STATGROUP_CkVoiceChat);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voice_chat_route_processor
{
    // Rise-limited so a spoofed instant-max amplitude claim takes ~half a second to reach the
    // top loudness bucket; falls faster than it rises so a talker gone quiet yields its
    // audible-speaker slot promptly.
    constexpr auto EnvelopeRisePerSecond = 2.0f;
    constexpr auto EnvelopeFallPerSecond = 4.0f;

    // A talker's connection identity: the player behind its owning actor, or nullptr for
    // player-less talkers (NPCs, test subjects with no owner chain).
    auto
    ResolveOwningPlayer(
        const FCk_Handle& InEntity) -> APlayerState*
    {
        auto* OwningActor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor_Recursive(InEntity);

        if (ck::Is_NOT_Valid(OwningActor))
        { return nullptr; }

        if (const auto* Pawn = ::Cast<APawn>(OwningActor))
        { return Pawn->GetPlayerState(); }

        for (auto* Outer = OwningActor; Outer != nullptr; Outer = Outer->GetOwner())
        {
            if (const auto* Controller = ::Cast<APlayerController>(Outer))
            { return Controller->PlayerState; }

            if (auto* PlayerState = ::Cast<APlayerState>(Outer))
            { return PlayerState; }
        }

        return nullptr;
    }

    auto
    ResolveRelayChannel_ForPlayer(
        UWorld* InWorld,
        APlayerState* InPlayerState) -> ACk_VoiceChatRelay_UE*
    {
        if (ck::Is_NOT_Valid(InWorld, ck::IsValid_Policy_NullptrOnly{}) || ck::Is_NOT_Valid(InPlayerState))
        { return nullptr; }

        auto* Subsystem = InWorld->GetSubsystem<UCk_VoiceChatRelay_Subsystem_UE>();

        if (ck::Is_NOT_Valid(Subsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return nullptr; }

        auto Pending = Subsystem->Request_AcquireChannel_ForPlayer(InPlayerState);
        const auto Result = Subsystem->Try_ResolvePending(Pending);

        return ::Cast<ACk_VoiceChatRelay_UE>(Result.Get_ChannelActor().Get());
    }

    // The listener-mute privacy gate: true when this recipient asked to never receive this
    // talker (the exclusion is server-side by design - muted audio is never sent at all).
    auto
    Get_IsMutedByRecipient(
        const FCk_Handle& InAnyEntity,
        APlayerState* InRecipient,
        const FCk_Handle& InTalker) -> bool
    {
        auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InAnyEntity);

        if (NOT TransientEntity.Has<ck::FFragment_VoiceChat_ListenerMuteMatrix>())
        { return false; }

        const auto* MutedSet = TransientEntity.Get<ck::FFragment_VoiceChat_ListenerMuteMatrix>()
            .Get_MutedByPlayer().Find(MakeWeakObjectPtr(InRecipient));

        return MutedSet != nullptr && MutedSet->Contains(InTalker);
    }

    // Per-connection bytes forwarded this tick, shared across every talker's inbox drain.
    auto
    Get_RouteBudgets(
        const FCk_Handle& InAnyEntity) -> ck::FFragment_VoiceChat_RouteBudgets&
    {
        auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InAnyEntity);
        auto& Budgets = TransientEntity.AddOrGet<ck::FFragment_VoiceChat_RouteBudgets>();

        if (Budgets.Get_LastResetFrame() != GFrameCounter)
        {
            Budgets.Get_SpentBytes().Reset();
            Budgets.Set_LastResetFrame(GFrameCounter);
        }

        return Budgets;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoiceChat_Route::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_Current& InCurrent,
            FFragment_VoiceTalker_ServerInbox& InInbox)
        -> void
    {
        using namespace ck_voice_chat_route_processor;

        const auto BundlesCopy = InInbox.Get_Bundles();
        InInbox.Get_Bundles().Reset();

        if (BundlesCopy.IsEmpty())
        { return; }

        auto* TalkerOwnerPlayer = ResolveOwningPlayer(InVoiceTalkerEntity);

        const auto SlewSeconds = InDeltaT.Get_Seconds() / BundlesCopy.Num();

        for (const auto& Bundle : BundlesCopy)
        {
            const auto& Packed = Bundle.Get_PackedBundle();

            const auto Unpacked = voice_chat::codec::Unpack_Bundle(Packed);

            const auto BundleIsWellFormed = Unpacked.IsSet();
            CK_ENSURE_IF_NOT(BundleIsWellFormed,
                TEXT("Malformed voice bundle ([{}] byte(s)) reached the routing inbox of Talker [{}] - "
                     "the pack side and RPC boundary should make this unreachable"),
                Packed.Num(), InVoiceTalkerEntity)
            {}
            if (NOT BundleIsWellFormed)
            {
                INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedMalformed);
                continue;
            }

            // N1: an unresolvable ChannelIdx is EXPECTED traffic (bundles race the registry) -
            // drop and count, never stash, no ensure. The stream self-heals when the registry
            // lands; anything dropped here was stale by then anyway.
            const auto Channel = UCk_Utils_VoiceChannel_UE::TryGet_ChannelByIdx(
                InVoiceTalkerEntity, Unpacked->Get_Header().Get_ChannelIdx());

            if (ck::Is_NOT_Valid(Channel))
            {
                INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedUnresolvableIdx);
                voice_chat::VeryVerbose(TEXT("Route: dropping bundle for Talker [{}] - ChannelIdx [{}] does not resolve"),
                    InVoiceTalkerEntity, Unpacked->Get_Header().Get_ChannelIdx());
                continue;
            }

            // Clause (c) enforcement: the RPC boundary stamped the sending connection; a bundle
            // whose stamped sender and talker-owning player BOTH resolve but differ is a spoof
            // attempt. Player-less talkers (NPCs) skip the check by construction.
            auto* Sender = Bundle.Get_Sender().Get();

            const auto SenderMatchesTalker =
                Sender == nullptr || TalkerOwnerPlayer == nullptr || Sender == TalkerOwnerPlayer;
            CK_ENSURE_IF_NOT(SenderMatchesTalker,
                TEXT("Voice bundle for Talker [{}] arrived from player [{}] who does not own it - dropping (spoof shape)"),
                InVoiceTalkerEntity, Sender->GetPlayerName())
            {}
            if (NOT SenderMatchesTalker)
            {
                INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedSenderMismatch);
                continue;
            }

            const auto TalkerIsAuthorized =
                UCk_Utils_VoiceChannel_UE::Get_IsMember(Channel, InVoiceTalkerEntity)
                && UCk_Utils_VoiceChannel_UE::Get_MemberFlags(Channel, InVoiceTalkerEntity).Get_CanTalk() == ECk_EnableDisable::Enable;

            if (NOT TalkerIsAuthorized)
            {
                // Expected during join/leave races - count only.
                INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedNotAuthorized);
                voice_chat::VeryVerbose(TEXT("Route: dropping bundle - Talker [{}] is not an authorized speaker on Channel [{}]"),
                    InVoiceTalkerEntity, Channel);
                continue;
            }

            if (UCk_Utils_VoiceChannel_UE::Get_IsServerMuted(Channel, InVoiceTalkerEntity))
            {
                // The privacy property: muted audio is never forwarded anywhere.
                INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedServerMuted);
                continue;
            }

            InCurrent._FairnessEnvelope += FMath::Clamp(
                voice_chat::codec::Dequantize_Amplitude(Unpacked->Get_Header().Get_AmplitudeQ8()) - InCurrent._FairnessEnvelope,
                -EnvelopeFallPerSecond * SlewSeconds,
                EnvelopeRisePerSecond * SlewSeconds);

            // Recipients: CanHear members, minus the talker entity and the sending connection.
            // Positional3D range filtering arrives with the routing probe (gate item 5); until
            // then both policies route by membership. Authorized forwards STAGE here; the flush
            // processor applies the audible-speaker cap + byte budget and does the actual send.
            for (const auto& Member : UCk_Utils_VoiceChannel_UE::Get_Members(Channel))
            {
                if (Member == InVoiceTalkerEntity)
                { continue; }

                if (UCk_Utils_VoiceChannel_UE::Get_MemberFlags(Channel, Member).Get_CanHear() != ECk_EnableDisable::Enable)
                { continue; }

                auto* RecipientPlayer = ResolveOwningPlayer(Member);

                if (ck::Is_NOT_Valid(RecipientPlayer))
                { continue; }

                if (RecipientPlayer == Sender || RecipientPlayer == TalkerOwnerPlayer)
                { continue; }

                if (Get_IsMutedByRecipient(InVoiceTalkerEntity, RecipientPlayer, InVoiceTalkerEntity))
                {
                    INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedListenerMuted);
                    continue;
                }

                auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InVoiceTalkerEntity);
                TransientEntity.AddOrGet<FFragment_VoiceChat_PendingForwards>().Get_Forwards().Emplace(
                    FCk_VoiceChat_PendingForward{MakeWeakObjectPtr(RecipientPlayer), InVoiceTalkerEntity,
                        Packed, InCurrent._FairnessEnvelope});
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceChat_FlushForwards::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTransientEntity,
            FFragment_VoiceChat_PendingForwards& InPending)
        -> void
    {
        using namespace ck_voice_chat_route_processor;

        const auto ForwardsCopy = InPending.Get_Forwards();
        InPending.Get_Forwards().Reset();

        if (ForwardsCopy.IsEmpty())
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InTransientEntity);
        auto& Budgets = Get_RouteBudgets(InTransientEntity);
        auto& ServeHistory = InTransientEntity.AddOrGet<FFragment_VoiceChat_ServeHistory>();

        const auto ByteBudget = UCk_Utils_VoiceChat_Settings_UE::Get_MaxVoiceBytesPerConnectionPerTick();
        const auto MaxSpeakers = UCk_Utils_VoiceChat_Settings_UE::Get_MaxAudibleSpeakers();

        auto ForwardsByRecipient = TMap<TWeakObjectPtr<APlayerState>, TArray<int32>>{};
        for (auto Idx = 0; Idx < ForwardsCopy.Num(); ++Idx)
        {
            if (NOT ForwardsCopy[Idx].Get_Recipient().IsValid())
            { continue; }

            ForwardsByRecipient.FindOrAdd(ForwardsCopy[Idx].Get_Recipient()).Add(Idx);
        }

        for (const auto& [Recipient, ForwardIdxs] : ForwardsByRecipient)
        {
            auto& RecipientHistory = ServeHistory.Get_LastServedFrame().FindOrAdd(Recipient);

            // Distinct competing talkers this tick, keyed into a candidate list for the pure
            // selection policy (candidate id = index into Talkers).
            auto Talkers = TArray<FCk_Handle>{};
            auto Candidates = TArray<FCk_VoiceChat_TopNCandidate>{};
            for (const auto ForwardIdx : ForwardIdxs)
            {
                const auto& Talker = ForwardsCopy[ForwardIdx].Get_Talker();

                if (Talkers.Contains(Talker))
                { continue; }

                const auto* LastServed = RecipientHistory.Find(Talker);
                Candidates.Emplace(FCk_VoiceChat_TopNCandidate{
                    Talkers.Num(), ForwardsCopy[ForwardIdx].Get_Envelope(),
                    LastServed != nullptr ? *LastServed : uint64{0}});
                Talkers.Emplace(Talker);
            }

            auto SelectedTalkers = TSet<FCk_Handle>{};
            for (const auto SelectedId : voice_chat::codec::Select_TopNTalkers(Candidates, MaxSpeakers))
            {
                SelectedTalkers.Add(Talkers[SelectedId]);
            }

            if (SelectedTalkers.Num() < Talkers.Num())
            {
                INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedSpeakerCap);
            }

            auto* Relay = ResolveRelayChannel_ForPlayer(World, Recipient.Get());

            if (ck::Is_NOT_Valid(Relay))
            { continue; }

            auto& SpentBytes = Budgets.Get_SpentBytes().FindOrAdd(Recipient);

            for (const auto ForwardIdx : ForwardIdxs)
            {
                const auto& Forward = ForwardsCopy[ForwardIdx];

                if (NOT SelectedTalkers.Contains(Forward.Get_Talker()))
                { continue; }

                if (SpentBytes + Forward.Get_PackedBundle().Num() > ByteBudget)
                {
                    INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedBudget);
                    continue;
                }

                Relay->Client_ReceiveVoiceBundle(Forward.Get_Talker(), Forward.Get_PackedBundle());

                SpentBytes += Forward.Get_PackedBundle().Num();
                RecipientHistory.Add(Forward.Get_Talker(), GFrameCounter);
                INC_DWORD_STAT(STAT_CkVoiceChat_RouteForwarded);
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
