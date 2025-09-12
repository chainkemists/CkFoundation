#pragma once

#include "CkAudio/AudioDirector/CkAudioDirector_Fragment_Data.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkAudio/AudioTrack/CkAudioTrack_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkAudioCue_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKAUDIO_API FCk_Handle_AudioCue : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AudioCue); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AudioCue);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AudioCue_SourcePriority : uint8
{
    PreferSingleTrack,
    PreferLibrary,
    SingleTrackOnly,
    LibraryOnly
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AudioCue_SourcePriority);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AudioCue_SelectionMode : uint8
{
    Random,
    WeightedRandom,
    Sequential,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AudioCue_SelectionMode);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioCue_Play : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioCue_Play);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioCue_Play);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TOptional<FCk_Time> _FadeInTime;

public:
    CK_PROPERTY(_FadeInTime);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioCue_Stop : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioCue_Stop);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioCue_Stop);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioCue_StopAll : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioCue_StopAll);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioCue_StopAll);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AudioCue_Event,
    FCk_Handle_AudioCue, InAudioCue,
    FGameplayTag, InTrackName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AudioCue_Event_MC,
    FCk_Handle_AudioCue, InAudioCue,
    FGameplayTag, InTrackName);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_AudioCue_AllTracksFinished,
    FCk_Handle_AudioCue, InAudioCue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_AudioCue_AllTracksFinished_MC,
    FCk_Handle_AudioCue, InAudioCue);

// --------------------------------------------------------------------------------------------------------------------
