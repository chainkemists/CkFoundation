#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include <GameplayTagContainer.h>

#include "CkVoiceTalker_Fragment_Data.generated.h"

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

public:
    CK_PROPERTY(_TransmitMode);
    CK_PROPERTY(_InputGain);
    CK_PROPERTY(_VadThreshold);
    CK_PROPERTY(_AutoJoinChannels);
    CK_PROPERTY(_PlaybackAttachSocketName);
};

// --------------------------------------------------------------------------------------------------------------------
