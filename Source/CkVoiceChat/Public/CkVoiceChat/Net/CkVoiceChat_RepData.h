#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Fragment_Data.h"

#include <GameplayTags.h>

#include "CkVoiceChat_RepData.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Control plane of the voice system, replicated as ONE container fragment on the channel HOST
// entity: the channel registry (tag -> wire idx), memberships + flags, and the server-mute
// matrix. Late joiners receive it in full before any audio; audio needs no catch-up (streams
// resume). Never saved - voice is runtime net state only.

USTRUCT()
struct CKVOICECHAT_API FCk_RepData_VoiceChat_Member
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_VoiceChat_Member);

private:
    UPROPERTY()
    FCk_Handle _Talker;

    UPROPERTY()
    FCk_VoiceChat_MemberFlags _Flags;

public:
    CK_PROPERTY_GET(_Talker);
    CK_PROPERTY_GET(_Flags);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RepData_VoiceChat_Member, _Talker, _Flags);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKVOICECHAT_API FCk_RepData_VoiceChat_ChannelEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_VoiceChat_ChannelEntry);

private:
    UPROPERTY()
    FGameplayTag _ChannelName;

    UPROPERTY()
    uint8 _ChannelIdx = ck::VoiceChannel_UnassignedIdx;

    UPROPERTY()
    TArray<FCk_RepData_VoiceChat_Member> _Members;

    UPROPERTY()
    TArray<FCk_Handle> _ServerMuted;

public:
    CK_PROPERTY(_ChannelName);
    CK_PROPERTY(_ChannelIdx);
    CK_PROPERTY(_Members);
    CK_PROPERTY(_ServerMuted);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKVOICECHAT_API FCk_RepData_VoiceChat
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_VoiceChat);

private:
    UPROPERTY()
    TArray<FCk_RepData_VoiceChat_ChannelEntry> _Channels;

public:
    CK_PROPERTY(_Channels);

public:
    auto Find_Channel(const FGameplayTag& InChannelName) -> FCk_RepData_VoiceChat_ChannelEntry*
    {
        return _Channels.FindByPredicate([&](const FCk_RepData_VoiceChat_ChannelEntry& InEntry)
        { return InEntry.Get_ChannelName() == InChannelName; });
    }

    auto FindOrAdd_Channel(const FGameplayTag& InChannelName) -> FCk_RepData_VoiceChat_ChannelEntry&
    {
        if (auto* Existing = Find_Channel(InChannelName))
        { return *Existing; }

        auto& NewEntry = _Channels.Emplace_GetRef();
        NewEntry.Set_ChannelName(InChannelName);
        return NewEntry;
    }
};

// --------------------------------------------------------------------------------------------------------------------
