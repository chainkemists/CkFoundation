#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment_Data.h"

#include <GameplayTags.h>
#include <NativeGameplayTags.h>

#include "CkVoiceChannel_Fragment_Data.generated.h"

class USoundAttenuation;
class USoundEffectSourcePresetChain;

// --------------------------------------------------------------------------------------------------------------------

CKVOICECHAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_VoiceChat_Channel_CategoryName);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VoiceChat_SpatializationPolicy : uint8
{
    // Spatialized at the speaker's body; routed only to listeners in the channel's audible range
    Positional3D,

    // Non-spatialized; routed to all channel members regardless of range
    Global2D,

    // One wire copy; played positionally when the speaker is within proximity range, as filtered 2D radio otherwise
    HybridRadio
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VoiceChat_SpatializationPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVOICECHAT_API FCk_Handle_VoiceChannel : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VoiceChannel); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VoiceChannel);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Fragment_VoiceChannel_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VoiceChannel_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "VoiceChat.Channel"))
    FGameplayTag _ChannelName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VoiceChat_SpatializationPolicy _SpatializationPolicy = ECk_VoiceChat_SpatializationPolicy::Positional3D;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0",
                  ToolTip = "Audible range in cm. Drives the routing probe for Positional3D; ignored by Global2D."))
    float _AudibleRange = 4000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<USoundAttenuation> _Attenuation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<USoundEffectSourcePresetChain> _SourceEffectChain;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  ToolTip = "When enabled, talkers whose AutoJoinChannels contains this channel's tag join on composition (the world-proximity channel sets this)."))
    ECk_EnableDisable _AutoJoinTalkers = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_ChannelName);
    CK_PROPERTY(_SpatializationPolicy);
    CK_PROPERTY(_AudibleRange);
    CK_PROPERTY(_Attenuation);
    CK_PROPERTY(_SourceEffectChain);
    CK_PROPERTY(_Priority);
    CK_PROPERTY(_AutoJoinTalkers);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VoiceChannel_ParamsData, _ChannelName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_VoiceChat_MemberFlags
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_VoiceChat_MemberFlags);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _CanTalk = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _CanHear = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY(_CanTalk);
    CK_PROPERTY(_CanHear);
};

// --------------------------------------------------------------------------------------------------------------------

// Membership mutations are authority-only: the server routes from ITS membership state and
// clients receive it via the control-plane replication - a client-side mutation would only
// diverge the local display copy. The Utils boundary rejects non-authority callers synchronously.
USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceChannel_Join : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceChannel_Join);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceChannel_Join);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VoiceTalker _Talker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_VoiceChat_MemberFlags _MemberFlags;

public:
    CK_PROPERTY_GET(_Talker);
    CK_PROPERTY(_MemberFlags);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceChannel_Join, _Talker);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceChannel_Leave : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceChannel_Leave);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceChannel_Leave);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VoiceTalker _Talker;

public:
    CK_PROPERTY_GET(_Talker);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceChannel_Leave, _Talker);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceChannel_SetMemberFlags : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceChannel_SetMemberFlags);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceChannel_SetMemberFlags);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VoiceTalker _Talker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_VoiceChat_MemberFlags _MemberFlags;

public:
    CK_PROPERTY_GET(_Talker);
    CK_PROPERTY_GET(_MemberFlags);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceChannel_SetMemberFlags, _Talker, _MemberFlags);
};

// --------------------------------------------------------------------------------------------------------------------

// Server mute is a ROUTING exclusion (the server stops forwarding this talker's frames on this
// channel), not a playback preference - the muted audio never reaches any client.
USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceChannel_ServerMute : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceChannel_ServerMute);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceChannel_ServerMute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VoiceTalker _Talker;

public:
    CK_PROPERTY_GET(_Talker);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceChannel_ServerMute, _Talker);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceChannel_ServerUnmute : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceChannel_ServerUnmute);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceChannel_ServerUnmute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VoiceTalker _Talker;

public:
    CK_PROPERTY_GET(_Talker);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceChannel_ServerUnmute, _Talker);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_VoiceChannel_Membership,
    FCk_Handle_VoiceChannel, InChannel,
    FCk_Handle_VoiceTalker, InTalker);

// --------------------------------------------------------------------------------------------------------------------
