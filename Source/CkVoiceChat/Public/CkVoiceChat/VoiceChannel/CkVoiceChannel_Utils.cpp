#include "CkVoiceChannel_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voice_channel_utils
{
    // Shared boundary for every membership request: server-authoritative, so a non-authority
    // caller is rejected synchronously and its delegate completes Failed_NotEnqueued.
    auto
    Get_HasAuthorityForMembershipRequest(
        const FCk_Handle_VoiceChannel& InVoiceChannel,
        const TCHAR* InRequestName,
        const FCk_Delegate_Request_OnCompleted& InDelegate) -> bool
    {
        const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InVoiceChannel);
        CK_ENSURE_IF_NOT(HasAuthority,
            TEXT("[{}] on VoiceChannel [{}] called without authority - membership is server-authoritative "
                 "and replicates to clients via the control plane"),
            FString{InRequestName}, InVoiceChannel)
        {}
        if (NOT HasAuthority)
        {
            InDelegate.ExecuteIfBound(InVoiceChannel, ECk_Request_OperationResult::Failed_NotEnqueued);
            return false;
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceChannel_UE::
    Add(
        FCk_Handle& InChannelHost,
        const FCk_Fragment_VoiceChannel_ParamsData& InParams)
    -> FCk_Handle_VoiceChannel
{
    const auto ChannelNameIsValid = ck::IsValid(InParams.Get_ChannelName());
    CK_ENSURE_IF_NOT(ChannelNameIsValid,
        TEXT("Cannot add a VoiceChannel to Host [{}] - the ChannelName tag is invalid"), InChannelHost)
    {}
    if (NOT ChannelNameIsValid)
    { return {}; }

    ck::voice_chat::VeryVerbose(TEXT("Creating VoiceChannel [{}] under Host [{}]"),
        InParams.Get_ChannelName(), InChannelHost);

    auto VoiceChannel = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_VoiceChannel>(InChannelHost);

    UCk_Utils_GameplayLabel_UE::Add(VoiceChannel, InParams.Get_ChannelName());

    VoiceChannel.Add<ck::FFragment_VoiceChannel_Params>(InParams);
    VoiceChannel.Add<ck::FFragment_VoiceChannel_Current>();
    VoiceChannel.Add<ck::FTag_VoiceChannel_NeedsSetup>();
    VoiceChannel.Add<ck::FTag_VoiceChannel_NeedsIdx>();

    RecordOfVoiceChannels_Utils::AddIfMissing(InChannelHost, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfVoiceChannels_Utils::Request_Connect(InChannelHost, VoiceChannel, ECk_Record_LabelRequirementPolicy::Required);

    return VoiceChannel;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VoiceChannel_UE, FCk_Handle_VoiceChannel,
    ck::FFragment_VoiceChannel_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceChannel_UE::
    Has_Any(
        const FCk_Handle& InChannelHost)
    -> bool
{
    return RecordOfVoiceChannels_Utils::Has(InChannelHost);
}

auto
    UCk_Utils_VoiceChannel_UE::
    TryGet_VoiceChannel(
        const FCk_Handle& InChannelHost,
        FGameplayTag InChannelName)
    -> FCk_Handle_VoiceChannel
{
    return RecordOfVoiceChannels_Utils::Get_ValidEntry_ByTag(InChannelHost, InChannelName);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceChannel_UE::
    Get_ChannelName(
        const FCk_Handle_VoiceChannel& InVoiceChannel)
    -> FGameplayTag
{
    return InVoiceChannel.Get<ck::FFragment_VoiceChannel_Params>().Get_ChannelName();
}

auto
    UCk_Utils_VoiceChannel_UE::
    Get_SpatializationPolicy(
        const FCk_Handle_VoiceChannel& InVoiceChannel)
    -> ECk_VoiceChat_SpatializationPolicy
{
    return InVoiceChannel.Get<ck::FFragment_VoiceChannel_Params>().Get_SpatializationPolicy();
}

auto
    UCk_Utils_VoiceChannel_UE::
    Get_AudibleRange(
        const FCk_Handle_VoiceChannel& InVoiceChannel)
    -> float
{
    return InVoiceChannel.Get<ck::FFragment_VoiceChannel_Params>().Get_AudibleRange();
}

auto
    UCk_Utils_VoiceChannel_UE::
    Get_Priority(
        const FCk_Handle_VoiceChannel& InVoiceChannel)
    -> int32
{
    return InVoiceChannel.Get<ck::FFragment_VoiceChannel_Params>().Get_Priority();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceChannel_UE::
    Request_Join(
        FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Request_VoiceChannel_Join& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceChannel
{
    if (NOT ck_voice_channel_utils::Get_HasAuthorityForMembershipRequest(InVoiceChannel, TEXT("Request_Join"), InDelegate))
    { return InVoiceChannel; }

    const auto Request = InRequest;

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceChannel.AddOrGet<ck::FFragment_VoiceChannel_Requests>()._Requests.Emplace(Request);

    return InVoiceChannel;
}

auto
    UCk_Utils_VoiceChannel_UE::
    Request_Leave(
        FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Request_VoiceChannel_Leave& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceChannel
{
    if (NOT ck_voice_channel_utils::Get_HasAuthorityForMembershipRequest(InVoiceChannel, TEXT("Request_Leave"), InDelegate))
    { return InVoiceChannel; }

    const auto Request = InRequest;

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceChannel.AddOrGet<ck::FFragment_VoiceChannel_Requests>()._Requests.Emplace(Request);

    return InVoiceChannel;
}

auto
    UCk_Utils_VoiceChannel_UE::
    Request_SetMemberFlags(
        FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Request_VoiceChannel_SetMemberFlags& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceChannel
{
    if (NOT ck_voice_channel_utils::Get_HasAuthorityForMembershipRequest(InVoiceChannel, TEXT("Request_SetMemberFlags"), InDelegate))
    { return InVoiceChannel; }

    const auto Request = InRequest;

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceChannel.AddOrGet<ck::FFragment_VoiceChannel_Requests>()._Requests.Emplace(Request);

    return InVoiceChannel;
}

auto
    UCk_Utils_VoiceChannel_UE::
    Request_ServerMute(
        FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Request_VoiceChannel_ServerMute& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceChannel
{
    if (NOT ck_voice_channel_utils::Get_HasAuthorityForMembershipRequest(InVoiceChannel, TEXT("Request_ServerMute"), InDelegate))
    { return InVoiceChannel; }

    const auto Request = InRequest;

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceChannel.AddOrGet<ck::FFragment_VoiceChannel_Requests>()._Requests.Emplace(Request);

    return InVoiceChannel;
}

auto
    UCk_Utils_VoiceChannel_UE::
    Request_ServerUnmute(
        FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Request_VoiceChannel_ServerUnmute& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VoiceChannel
{
    if (NOT ck_voice_channel_utils::Get_HasAuthorityForMembershipRequest(InVoiceChannel, TEXT("Request_ServerUnmute"), InDelegate))
    { return InVoiceChannel; }

    const auto Request = InRequest;

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InVoiceChannel.AddOrGet<ck::FFragment_VoiceChannel_Requests>()._Requests.Emplace(Request);

    return InVoiceChannel;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceChannel_UE::
    Get_ChannelIdx(
        const FCk_Handle_VoiceChannel& InVoiceChannel)
    -> uint8
{
    return InVoiceChannel.Get<ck::FFragment_VoiceChannel_Current>().Get_ChannelIdx();
}

auto
    UCk_Utils_VoiceChannel_UE::
    Get_IsMember(
        const FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Handle_VoiceTalker& InTalker)
    -> bool
{
    return InVoiceChannel.Get<ck::FFragment_VoiceChannel_Current>().Get_Members().Contains(InTalker);
}

auto
    UCk_Utils_VoiceChannel_UE::
    Get_MemberFlags(
        const FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Handle_VoiceTalker& InTalker)
    -> FCk_VoiceChat_MemberFlags
{
    const auto& Members = InVoiceChannel.Get<ck::FFragment_VoiceChannel_Current>().Get_Members();

    const auto* Found = Members.Find(InTalker);

    const auto TalkerIsMember = Found != nullptr;
    CK_ENSURE_IF_NOT(TalkerIsMember,
        TEXT("Get_MemberFlags on VoiceChannel [{}] for Talker [{}] who is not a member"),
        InVoiceChannel, InTalker)
    {}
    if (NOT TalkerIsMember)
    { return {}; }

    return *Found;
}

auto
    UCk_Utils_VoiceChannel_UE::
    Get_Members(
        const FCk_Handle_VoiceChannel& InVoiceChannel)
    -> TArray<FCk_Handle_VoiceTalker>
{
    const auto& Members = InVoiceChannel.Get<ck::FFragment_VoiceChannel_Current>().Get_Members();

    auto Result = TArray<FCk_Handle_VoiceTalker>{};
    Result.Reserve(Members.Num());

    for (const auto& [MemberHandle, MemberFlags] : Members)
    {
        auto MemberCopy = MemberHandle;
        Result.Emplace(UCk_Utils_VoiceTalker_UE::CastChecked(MemberCopy));
    }

    return Result;
}

auto
    UCk_Utils_VoiceChannel_UE::
    Get_IsServerMuted(
        const FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Handle_VoiceTalker& InTalker)
    -> bool
{
    return InVoiceChannel.Get<ck::FFragment_VoiceChannel_Current>().Get_ServerMuted().Contains(InTalker);
}

auto
    UCk_Utils_VoiceChannel_UE::
    TryGet_ChannelByIdx(
        const FCk_Handle& InAnyHandle,
        uint8 InChannelIdx)
    -> FCk_Handle_VoiceChannel
{
    if (ck::Is_NOT_Valid(InAnyHandle))
    { return {}; }

    auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InAnyHandle);

    if (NOT TransientEntity.Has<ck::FFragment_VoiceChat_ChannelRegistry>())
    { return {}; }

    const auto& Entries = TransientEntity.Get<ck::FFragment_VoiceChat_ChannelRegistry>().Get_Entries();

    for (const auto& Entry : Entries)
    {
        if (Entry.Get_ChannelIdx() != InChannelIdx)
        { continue; }

        if (ck::Is_NOT_Valid(Entry.Get_Channel()))
        { return {}; }

        return Entry.Get_Channel();
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceChannel_UE::
    Apply_ReplicatedControlPlane(
        FCk_Handle& InChannelHost,
        const FCk_RepData_VoiceChat& InRepData)
    -> ECk_Persistence_ApplyResult
{
    // NotReady before ANY mutation: every named channel must already be composed here.
    for (const auto& Entry : InRepData.Get_Channels())
    {
        if (ck::Is_NOT_Valid(TryGet_VoiceChannel(InChannelHost, Entry.Get_ChannelName())))
        { return ECk_Persistence_ApplyResult::NotReady; }
    }

    for (const auto& Entry : InRepData.Get_Channels())
    {
        auto Channel = TryGet_VoiceChannel(InChannelHost, Entry.Get_ChannelName());
        auto& Current = Channel.Get<ck::FFragment_VoiceChannel_Current>();

        Current._ChannelIdx = Entry.Get_ChannelIdx();

        if (Channel.Has<ck::FTag_VoiceChannel_NeedsIdx>())
        { Channel.Remove<ck::FTag_VoiceChannel_NeedsIdx>(); }

        auto NewMembers = TMap<FCk_Handle, FCk_VoiceChat_MemberFlags>{};
        NewMembers.Reserve(Entry.Get_Members().Num());
        for (const auto& Member : Entry.Get_Members())
        {
            NewMembers.Add(Member.Get_Talker(), Member.Get_Flags());
        }

        // Joined/Left fire from the local state diff. A member whose talker feature has not
        // composed on this world yet still lands in the map (keys are base handles); only the
        // typed signal payload is skipped for it.
        for (const auto& [NewMemberHandle, NewMemberFlags] : NewMembers)
        {
            if (Current._Members.Contains(NewMemberHandle))
            { continue; }

            auto MemberCopy = NewMemberHandle;
            if (auto Talker = UCk_Utils_VoiceTalker_UE::Cast(MemberCopy);
                ck::IsValid(Talker))
            { ck::UUtils_Signal_OnVoiceChannel_MemberJoined::Broadcast(Channel, ck::MakePayload(Channel, Talker)); }
        }

        for (const auto& [OldMemberHandle, OldMemberFlags] : Current._Members)
        {
            if (NewMembers.Contains(OldMemberHandle))
            { continue; }

            auto MemberCopy = OldMemberHandle;
            if (auto Talker = UCk_Utils_VoiceTalker_UE::Cast(MemberCopy);
                ck::IsValid(Talker))
            { ck::UUtils_Signal_OnVoiceChannel_MemberLeft::Broadcast(Channel, ck::MakePayload(Channel, Talker)); }
        }

        Current._Members = MoveTemp(NewMembers);

        Current._ServerMuted.Reset();
        for (const auto& Muted : Entry.Get_ServerMuted())
        {
            Current._ServerMuted.Add(Muted);
        }
    }

    return ECk_Persistence_ApplyResult::Applied;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoiceChannel_UE::
    BindTo_OnMemberJoined(
        FCk_Handle_VoiceChannel& InVoiceChannel,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceChannel_Membership& InDelegate)
    -> FCk_Handle_VoiceChannel
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVoiceChannel_MemberJoined, InVoiceChannel, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InVoiceChannel;
}

auto
    UCk_Utils_VoiceChannel_UE::
    UnbindFrom_OnMemberJoined(
        FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Delegate_VoiceChannel_Membership& InDelegate)
    -> FCk_Handle_VoiceChannel
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVoiceChannel_MemberJoined, InVoiceChannel, InDelegate);
    return InVoiceChannel;
}

auto
    UCk_Utils_VoiceChannel_UE::
    BindTo_OnMemberLeft(
        FCk_Handle_VoiceChannel& InVoiceChannel,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceChannel_Membership& InDelegate)
    -> FCk_Handle_VoiceChannel
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVoiceChannel_MemberLeft, InVoiceChannel, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InVoiceChannel;
}

auto
    UCk_Utils_VoiceChannel_UE::
    UnbindFrom_OnMemberLeft(
        FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Delegate_VoiceChannel_Membership& InDelegate)
    -> FCk_Handle_VoiceChannel
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVoiceChannel_MemberLeft, InVoiceChannel, InDelegate);
    return InVoiceChannel;
}

// --------------------------------------------------------------------------------------------------------------------
