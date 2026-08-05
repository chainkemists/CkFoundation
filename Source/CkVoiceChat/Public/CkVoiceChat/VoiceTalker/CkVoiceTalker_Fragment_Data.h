#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>

#include "CkVoiceTalker_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKVOICECHAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_VoiceChat_Probe_Range);
CKVOICECHAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_VoiceChat_Probe_Presence);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VoiceChat_TransmitMode : uint8
{
    PushToTalk,
    OpenMic,
    Disabled
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VoiceChat_TransmitMode);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVOICECHAT_API FCk_Handle_VoiceTalker : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VoiceTalker); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VoiceTalker);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Fragment_VoiceTalker_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VoiceTalker_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VoiceChat_TransmitMode _TransmitMode = ECk_VoiceChat_TransmitMode::PushToTalk;

    // Gain is applied by this module's capture pipeline - the engine's voice.MicInputGain path is
    // broken upstream in some engine versions.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "10.0"))
    float _InputGain = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0"))
    float _VadThreshold = 0.07f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "VoiceChat.Channel"))
    FGameplayTagContainer _AutoJoinChannels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _PlaybackAttachSocketName;

    // Local self-monitor: the talker's own encoded frames run back through the full
    // jitter->decode playback path on this machine. Mic-test / test-pipeline feature.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Loopback = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY(_TransmitMode);
    CK_PROPERTY(_InputGain);
    CK_PROPERTY(_VadThreshold);
    CK_PROPERTY(_AutoJoinChannels);
    CK_PROPERTY(_PlaybackAttachSocketName);
    CK_PROPERTY(_Loopback);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceTalker_StartTransmit : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceTalker_StartTransmit);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceTalker_StartTransmit);

private:
    // Channels to speak into; routing consumes this once transport exists (P3). Empty = the
    // talker's AutoJoinChannels.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "VoiceChat.Channel"))
    FGameplayTagContainer _ChannelTags;

public:
    CK_PROPERTY(_ChannelTags);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceTalker_StopTransmit : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceTalker_StopTransmit);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceTalker_StopTransmit);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceTalker_SetTransmitMode : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceTalker_SetTransmitMode);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceTalker_SetTransmitMode);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VoiceChat_TransmitMode _TransmitMode = ECk_VoiceChat_TransmitMode::PushToTalk;

public:
    CK_PROPERTY_GET(_TransmitMode);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceTalker_SetTransmitMode, _TransmitMode);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceTalker_SetInputGain : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceTalker_SetInputGain);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceTalker_SetInputGain);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "10.0"))
    float _InputGain = 1.2f;

public:
    CK_PROPERTY_GET(_InputGain);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceTalker_SetInputGain, _InputGain);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceTalker_SetSelfMute : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceTalker_SetSelfMute);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceTalker_SetSelfMute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _SelfMute = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_SelfMute);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceTalker_SetSelfMute, _SelfMute);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_VoiceTalker,
    FCk_Handle_VoiceTalker, InHandle);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_VoiceTalker_SpeakingStateChanged,
    FCk_Handle_VoiceTalker, InHandle,
    bool, InIsSpeaking);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_VoiceTalker_FramesCaptured,
    FCk_Handle_VoiceTalker, InHandle,
    const TArray<uint8>&, InPcm16Mono,
    int32, InSampleRate);

// --------------------------------------------------------------------------------------------------------------------
