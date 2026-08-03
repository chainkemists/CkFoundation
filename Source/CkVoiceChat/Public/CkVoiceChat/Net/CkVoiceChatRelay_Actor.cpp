#include "CkVoiceChatRelay_Actor.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"
#include "CkVoiceChat/CkVoiceChat_Stats.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Utils.h"

#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>

DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Bundles Inbound (Server)"), STAT_CkVoiceChat_BundlesInboundServer, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Bundles Inbound (Client)"), STAT_CkVoiceChat_BundlesInboundClient, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Bundles Dropped (Unresolvable)"), STAT_CkVoiceChat_BundlesDroppedUnresolvable, STATGROUP_CkVoiceChat);
DECLARE_DWORD_COUNTER_STAT(TEXT("VoiceChat Bundles Dropped (Inbox Full)"), STAT_CkVoiceChat_BundlesDroppedInboxFull_Relay, STATGROUP_CkVoiceChat);

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_VoiceChatRelay_UE::
    Server_SendVoiceBundle_Implementation(
        FCk_Handle InTalkerEntity,
        const TArray<uint8>& InPackedBundle)
    -> void
{
    // Drops here log VeryVerbose, not Warning - an unresolvable talker during composition races
    // fires per-bundle (~25/s); per-talker throttled attribution is the Route processor's job.
    auto Talker = UCk_Utils_VoiceTalker_UE::Cast(InTalkerEntity);

    if (ck::Is_NOT_Valid(Talker))
    {
        INC_DWORD_STAT(STAT_CkVoiceChat_BundlesDroppedUnresolvable);
        ck::voice_chat::VeryVerbose(TEXT("Server_SendVoiceBundle: talker [{}] did not resolve - dropping [{}] byte(s)"),
            InTalkerEntity, InPackedBundle.Num());
        return;
    }

    auto* Sender = DoResolve_OwningPlayerState();

    if (ck::Is_NOT_Valid(Sender))
    {
        INC_DWORD_STAT(STAT_CkVoiceChat_BundlesDroppedUnresolvable);
        ck::voice_chat::Warning(TEXT("Server_SendVoiceBundle: could not resolve the sending player from the channel owner chain - dropping"));
        return;
    }

    auto& Inbox = Talker.AddOrGet<ck::FFragment_VoiceTalker_ServerInbox>().Get_Bundles();

    if (Inbox.Num() >= ck::VoiceChat_MaxInboxBundles)
    {
        INC_DWORD_STAT(STAT_CkVoiceChat_BundlesDroppedInboxFull_Relay);
        return;
    }

    INC_DWORD_STAT(STAT_CkVoiceChat_BundlesInboundServer);

    Inbox.Emplace(ck::FCk_VoiceChat_InboundBundle{InPackedBundle, MakeWeakObjectPtr(Sender)});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_VoiceChatRelay_UE::
    Client_ReceiveVoiceBundle_Implementation(
        FCk_Handle InTalkerEntity,
        const TArray<uint8>& InPackedBundle)
    -> void
{
    auto Talker = UCk_Utils_VoiceTalker_UE::Cast(InTalkerEntity);

    if (ck::Is_NOT_Valid(Talker))
    {
        INC_DWORD_STAT(STAT_CkVoiceChat_BundlesDroppedUnresolvable);
        ck::voice_chat::VeryVerbose(TEXT("Client_ReceiveVoiceBundle: talker [{}] not composed here - dropping [{}] byte(s)"),
            InTalkerEntity, InPackedBundle.Num());
        return;
    }

    auto& InboxFragment = Talker.AddOrGet<ck::FFragment_VoiceTalker_ReceiveInbox>();
    InboxFragment.Set_TotalArrivedBundles(InboxFragment.Get_TotalArrivedBundles() + 1);
    InboxFragment.Set_TotalArrivedBytes(InboxFragment.Get_TotalArrivedBytes() + InPackedBundle.Num());

    auto& Inbox = InboxFragment.Get_PackedBundles();

    if (Inbox.Num() >= ck::VoiceChat_MaxInboxBundles)
    {
        INC_DWORD_STAT(STAT_CkVoiceChat_BundlesDroppedInboxFull_Relay);
        return;
    }

    INC_DWORD_STAT(STAT_CkVoiceChat_BundlesInboundClient);

    Inbox.Emplace(InPackedBundle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_VoiceChatRelay_UE::
    DoResolve_OwningPlayerState() const
    -> APlayerState*
{
    for (auto* Outer = GetOwner(); Outer != nullptr; Outer = Outer->GetOwner())
    {
        if (const auto* OwningController = ::Cast<APlayerController>(Outer))
        { return OwningController->PlayerState; }

        if (auto* OwningPlayerState = ::Cast<APlayerState>(Outer))
        { return OwningPlayerState; }
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
