#include "CkVoiceChat_Route_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkShapes/Sphere/CkShapeSphere_Utils.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Probe/CkProbe_Utils.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"
#include "CkVoiceChat/CkVoiceChat_Stats.h"
#include "CkVoiceChat/Codec/CkVoiceChat_Codec.h"
#include "CkVoiceChat/Net/CkVoiceChatRelay_Actor.h"
#include "CkVoiceChat/Net/CkVoiceChatRelay_Subsystem.h"
#include "CkVoiceChat/Settings/CkVoiceChat_Settings.h"
#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Fragment.h"
#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Utils.h"
#include "CkVoiceChat/VoiceListener/CkVoiceListener_Fragment.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Utils.h"

#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChat_ProximityProbes);
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
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Dropped (No Proximity Set)"), STAT_CkVoiceChat_RouteDroppedNoProximity, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Route Dropped (Out Of Range)"), STAT_CkVoiceChat_RouteDroppedOutOfRange, STATGROUP_CkVoiceChat);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voice_chat_route_processor
{
    // Rise-limited so a spoofed instant-max amplitude claim takes ~half a second to reach the
    // top loudness bucket; falls faster than it rises so a talker gone quiet yields its
    // audible-speaker slot promptly.
    constexpr auto EnvelopeRisePerSecond = 2.0f;
    constexpr auto EnvelopeFallPerSecond = 4.0f;

    // N2 attribution throttle: at most one drop-summary Warning per talker per this window.
    constexpr auto RouteDropWarnCooldownSeconds = 5.0f;

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

    // The widest audible range this talker's Positional3D memberships demand - 0 means no
    // Positional3D membership at all (and therefore no probes).
    auto
    Get_MaxPositional3DRangeCm(
        const FCk_Handle_VoiceTalker& InTalker) -> float
    {
        auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InTalker);

        if (NOT TransientEntity.Has<ck::FFragment_VoiceChat_ChannelRegistry>())
        { return 0.0f; }

        auto MaxRangeCm = 0.0f;

        for (const auto& Entry : TransientEntity.Get<ck::FFragment_VoiceChat_ChannelRegistry>().Get_Entries())
        {
            const auto& Channel = Entry.Get_Channel();

            if (ck::Is_NOT_Valid(Channel))
            { continue; }

            if (UCk_Utils_VoiceChannel_UE::Get_SpatializationPolicy(Channel) != ECk_VoiceChat_SpatializationPolicy::Positional3D)
            { continue; }

            if (NOT UCk_Utils_VoiceChannel_UE::Get_IsMember(Channel, InTalker))
            { continue; }

            MaxRangeCm = FMath::Max(MaxRangeCm, UCk_Utils_VoiceChannel_UE::Get_AudibleRange(Channel));
        }

        return MaxRangeCm;
    }

    struct FCk_VoiceProbeChild
    {
        FCk_Handle _Child;
        FCk_Handle_Probe _Probe;
        FCk_Handle_ShapeSphere _Shape;
    };

    // The CkCrowd probe-composition ritual (CkCrowdAgent_Processor.cpp): child entity under the
    // talker (cascade-destroys with it), Transform seeded from the talker for the first frame,
    // sphere shape, Kinematic probe, SceneNode so it follows the talker every tick after.
    auto
    Create_VoiceProbeChild(
        FCk_Handle& InTalker,
        const FGameplayTag& InProbeName,
        const FGameplayTagContainer& InFilter,
        float InRadiusCm,
        ECk_Probe_PersistContacts InPersistContacts) -> FCk_VoiceProbeChild
    {
        auto Child = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InTalker);

        auto TalkerTransformHandle = UCk_Utils_Transform_UE::CastChecked(InTalker);
        auto ChildTransform = UCk_Utils_Transform_UE::Add(
            Child, UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TalkerTransformHandle), ECk_Replication::DoesNotReplicate);

        const auto Shape = UCk_Utils_ShapeSphere_UE::Add(
            Child, FCk_Fragment_ShapeSphere_ParamsData{FCk_ShapeSphere_Dimensions{InRadiusCm}});

        auto ProbeParams = FCk_Fragment_Probe_ParamsData{InProbeName};
        ProbeParams.Set_Filter(InFilter);
        ProbeParams.Set_ContextOverlapPolicy(ECk_Probe_ContextOverlapPolicy::Any);
        ProbeParams.Set_MotionType(ECk_MotionType::Kinematic);
        ProbeParams.Set_PersistContacts(InPersistContacts);

        const auto Probe = UCk_Utils_Probe_UE::Add(ChildTransform, ProbeParams, FCk_Probe_DebugInfo{});

        UCk_Utils_SceneNode_UE::Add(ChildTransform, TalkerTransformHandle, FTransform::Identity);

        return {Child, Probe, Shape};
    }

    // The hysteresis gate, candidates only (the probe already bounded the search): a recipient
    // becomes audible on a channel when it comes within that channel's range and STAYS audible
    // until it leaves range + margin, so a listener hovering at the boundary doesn't flicker
    // mid-word - and one drifting through the margin band from outside never pops in.
    auto
    Get_PassesProximityGate(
        TMap<FCk_Handle, TSet<uint8>>& InAudibleOn,
        const FCk_Handle& InRecipientTalker,
        uint8 InChannelIdx,
        float InRangeCm,
        float InMarginCm,
        const FVector& InSenderLocation,
        const TSet<FCk_Handle>& InCandidates) -> bool
    {
        if (NOT InCandidates.Contains(InRecipientTalker))
        {
            if (auto* Held = InAudibleOn.Find(InRecipientTalker))
            { Held->Remove(InChannelIdx); }

            return false;
        }

        if (NOT UCk_Utils_Transform_UE::Has(InRecipientTalker))
        { return false; }

        const auto DistSq = FVector::DistSquared(
            InSenderLocation,
            UCk_Utils_Transform_UE::Get_EntityCurrentLocation(UCk_Utils_Transform_UE::CastChecked(InRecipientTalker)));

        auto& Held = InAudibleOn.FindOrAdd(InRecipientTalker);

        if (Held.Contains(InChannelIdx))
        {
            const auto OuterCm = InRangeCm + InMarginCm;

            if (DistSq <= OuterCm * OuterCm)
            { return true; }

            Held.Remove(InChannelIdx);
            return false;
        }

        if (DistSq <= InRangeCm * InRangeCm)
        {
            Held.Add(InChannelIdx);
            return true;
        }

        return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoiceChat_ProximityProbes::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity)
        -> void
    {
        using namespace ck_voice_chat_route_processor;

        InVoiceTalkerEntity.Try_Remove<FTag_VoiceTalker_ProximityDirty>();

        const auto MaxRangeCm = Get_MaxPositional3DRangeCm(InVoiceTalkerEntity);
        const auto TalkerHasTransform = UCk_Utils_Transform_UE::Has(InVoiceTalkerEntity);
        const auto WantsProbes = MaxRangeCm > 0.0f && TalkerHasTransform;
        const auto HasProximity = InVoiceTalkerEntity.Has<FFragment_VoiceTalker_Proximity>();

        if (NOT WantsProbes)
        {
            if (MaxRangeCm > 0.0f && NOT TalkerHasTransform)
            {
                voice_chat::VeryVerbose(TEXT("Talker [{}] is a Positional3D channel member but has no Transform - "
                    "no probes composed, proximity routing fails closed for it"), InVoiceTalkerEntity);
            }

            if (NOT HasProximity)
            { return; }

            auto& Proximity = InVoiceTalkerEntity.Get<FFragment_VoiceTalker_Proximity>();

            if (ck::IsValid(Proximity._RangeProbeChild))
            { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Proximity._RangeProbeChild); }

            if (ck::IsValid(Proximity._PresenceProbeChild))
            { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Proximity._PresenceProbeChild); }

            InVoiceTalkerEntity.Remove<FFragment_VoiceTalker_Proximity>();
            return;
        }

        const auto DesiredRadiusCm = MaxRangeCm + UCk_Utils_VoiceChat_Settings_UE::Get_ProximityHysteresisMarginCm();

        if (HasProximity)
        {
            auto& Proximity = InVoiceTalkerEntity.Get<FFragment_VoiceTalker_Proximity>();

            if (ck::IsValid(Proximity._RangeShape) && Proximity._RangeProbeRadius != DesiredRadiusCm)
            {
                UCk_Utils_ShapeSphere_UE::Request_UpdateDimensions(Proximity._RangeShape,
                    FCk_Request_ShapeSphere_UpdateDimensions{FCk_ShapeSphere_Dimensions{DesiredRadiusCm}}, {});
                Proximity._RangeProbeRadius = DesiredRadiusCm;
            }

            return;
        }

        const auto RangeChild = Create_VoiceProbeChild(InVoiceTalkerEntity,
            Tag_VoiceChat_Probe_Range, FGameplayTagContainer{Tag_VoiceChat_Probe_Presence},
            DesiredRadiusCm, ECk_Probe_PersistContacts::Enabled);

        // The presence dot reports nothing itself (empty filter) - it exists so OTHER talkers'
        // range probes have something to detect.
        const auto PresenceChild = Create_VoiceProbeChild(InVoiceTalkerEntity,
            Tag_VoiceChat_Probe_Presence, FGameplayTagContainer{},
            VoiceChat_PresenceProbeRadiusCm, ECk_Probe_PersistContacts::Disabled);

        auto& Proximity = InVoiceTalkerEntity.Add<FFragment_VoiceTalker_Proximity>();
        Proximity._RangeProbe = RangeChild._Probe;
        Proximity._RangeShape = RangeChild._Shape;
        Proximity._RangeProbeChild = RangeChild._Child;
        Proximity._PresenceProbeChild = PresenceChild._Child;
        Proximity._RangeProbeRadius = DesiredRadiusCm;
    }

    // --------------------------------------------------------------------------------------------------------------------

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

        auto DroppedMalformed = 0;
        auto DroppedUnresolvable = 0;
        auto DroppedSenderMismatch = 0;
        auto DroppedNotAuthorized = 0;
        auto DroppedServerMuted = 0;
        auto DroppedNoProximity = 0;

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
                ++DroppedMalformed;
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
                ++DroppedUnresolvable;
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
                ++DroppedSenderMismatch;
                continue;
            }

            const auto TalkerIsAuthorized =
                UCk_Utils_VoiceChannel_UE::Get_IsMember(Channel, InVoiceTalkerEntity)
                && UCk_Utils_VoiceChannel_UE::Get_MemberFlags(Channel, InVoiceTalkerEntity).Get_CanTalk() == ECk_EnableDisable::Enable;

            if (NOT TalkerIsAuthorized)
            {
                // Expected during join/leave races - count only.
                INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedNotAuthorized);
                ++DroppedNotAuthorized;
                voice_chat::VeryVerbose(TEXT("Route: dropping bundle - Talker [{}] is not an authorized speaker on Channel [{}]"),
                    InVoiceTalkerEntity, Channel);
                continue;
            }

            if (UCk_Utils_VoiceChannel_UE::Get_IsServerMuted(Channel, InVoiceTalkerEntity))
            {
                // The privacy property: muted audio is never forwarded anywhere.
                INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedServerMuted);
                ++DroppedServerMuted;
                continue;
            }

            InCurrent._FairnessEnvelope += FMath::Clamp(
                voice_chat::codec::Dequantize_Amplitude(Unpacked->Get_Header().Get_AmplitudeQ8()) - InCurrent._FairnessEnvelope,
                -EnvelopeFallPerSecond * SlewSeconds,
                EnvelopeRisePerSecond * SlewSeconds);

            // ADR-6: Positional3D forwards only within the probe-maintained candidate set, with
            // per-(recipient, channel) hysteresis applied below. A sender with no proximity set
            // (no Transform, probes not composed yet) forwards to NOBODY on this channel - no
            // position means no proximity; the policy never falls back to membership-wide sends.
            const auto Policy = UCk_Utils_VoiceChannel_UE::Get_SpatializationPolicy(Channel);

            auto CandidateTalkers = TSet<FCk_Handle>{};
            auto SenderLocation = FVector::ZeroVector;
            auto Proximity = static_cast<FFragment_VoiceTalker_Proximity*>(nullptr);

            if (Policy == ECk_VoiceChat_SpatializationPolicy::Positional3D)
            {
                const auto ProximityIsReady =
                    InVoiceTalkerEntity.Has<FFragment_VoiceTalker_Proximity>()
                    && UCk_Utils_Transform_UE::Has(InVoiceTalkerEntity)
                    && ck::IsValid(InVoiceTalkerEntity.Get<FFragment_VoiceTalker_Proximity>().Get_RangeProbe());

                if (NOT ProximityIsReady)
                {
                    INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedNoProximity);
                    ++DroppedNoProximity;
                    voice_chat::VeryVerbose(TEXT("Route: dropping bundle - Talker [{}] has no proximity set for Positional3D Channel [{}]"),
                        InVoiceTalkerEntity, Channel);
                    continue;
                }

                Proximity = &InVoiceTalkerEntity.Get<FFragment_VoiceTalker_Proximity>();
                SenderLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(
                    UCk_Utils_Transform_UE::CastChecked(InVoiceTalkerEntity));

                for (const auto& Overlap : Proximity->Get_RangeProbe().Get<FFragment_Probe_Current>().Get_CurrentOverlaps())
                {
                    const auto& PresenceChild = Overlap.Get_OtherEntity();

                    if (ck::Is_NOT_Valid(PresenceChild))
                    { continue; }

                    const auto CandidateTalker = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(PresenceChild);

                    if (ck::Is_NOT_Valid(CandidateTalker) || CandidateTalker == InVoiceTalkerEntity)
                    { continue; }

                    CandidateTalkers.Add(CandidateTalker);
                }

                for (auto AudibleIt = Proximity->_AudibleOn.CreateIterator(); AudibleIt; ++AudibleIt)
                {
                    if (NOT CandidateTalkers.Contains(AudibleIt.Key()))
                    { AudibleIt.RemoveCurrent(); }
                }
            }

            const auto ChannelRangeCm = UCk_Utils_VoiceChannel_UE::Get_AudibleRange(Channel);
            const auto HysteresisMarginCm = UCk_Utils_VoiceChat_Settings_UE::Get_ProximityHysteresisMarginCm();

            // Recipients: CanHear members - range-gated for Positional3D - minus the talker
            // entity and the sending connection. Authorized forwards STAGE here; the flush
            // processor applies the audible-speaker cap + byte budget and does the actual send.
            for (const auto& Member : UCk_Utils_VoiceChannel_UE::Get_Members(Channel))
            {
                if (Member == InVoiceTalkerEntity)
                { continue; }

                if (UCk_Utils_VoiceChannel_UE::Get_MemberFlags(Channel, Member).Get_CanHear() != ECk_EnableDisable::Enable)
                { continue; }

                if (Proximity != nullptr &&
                    NOT Get_PassesProximityGate(Proximity->_AudibleOn, Member,
                        Unpacked->Get_Header().Get_ChannelIdx(), ChannelRangeCm, HysteresisMarginCm,
                        SenderLocation, CandidateTalkers))
                {
                    INC_DWORD_STAT(STAT_CkVoiceChat_RouteDroppedOutOfRange);
                    continue;
                }

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

        InInbox._DropWarnCooldown = FCk_Time{
            FMath::Max(0.0f, InInbox._DropWarnCooldown.Get_Seconds() - InDeltaT.Get_Seconds())};

        const auto DroppedThisDrain = DroppedMalformed + DroppedUnresolvable + DroppedSenderMismatch
            + DroppedNotAuthorized + DroppedServerMuted + DroppedNoProximity;

        if (DroppedThisDrain > 0 && InInbox._DropWarnCooldown.Get_Seconds() <= 0.0f)
        {
            voice_chat::Warning(TEXT("Route dropped [{}] bundle(s) from Talker [{}] this drain "
                "(malformed [{}], unresolvable-idx [{}], sender-mismatch [{}], unauthorized [{}], "
                "server-muted [{}], no-proximity [{}]) - throttled to one warning per [{}]s per talker; "
                "stats carry exact totals"),
                DroppedThisDrain, InVoiceTalkerEntity, DroppedMalformed, DroppedUnresolvable,
                DroppedSenderMismatch, DroppedNotAuthorized, DroppedServerMuted, DroppedNoProximity,
                RouteDropWarnCooldownSeconds);

            InInbox._DropWarnCooldown = FCk_Time{RouteDropWarnCooldownSeconds};
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
