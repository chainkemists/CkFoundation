#include "CkVoiceChannel_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"

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
    VoiceChannel.Add<ck::FTag_VoiceChannel_NeedsSetup>();

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
