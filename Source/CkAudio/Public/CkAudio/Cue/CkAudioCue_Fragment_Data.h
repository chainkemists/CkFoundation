#pragma once

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
    PreferSingleTrack,    // Use _SingleTrack if valid, fallback to library
    PreferLibrary,        // Use library if not empty, fallback to single
    SingleTrackOnly,      // Error if library populated
    LibraryOnly          // Error if single track populated
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AudioCue_SourcePriority);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AudioCue_SelectionMode : uint8
{
    Random,
    WeightedRandom,
    Sequential,
    MoodBased
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
    TOptional<int32> _OverridePriority;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeInTime = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY(_OverridePriority);
    CK_PROPERTY(_FadeInTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AudioCue_Play, _OverridePriority, _FadeInTime);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioCue_Stop : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioCue_Stop);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioCue_Stop);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeOutTime = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY(_FadeOutTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AudioCue_Stop, _FadeOutTime);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioCue_StopAll : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioCue_StopAll);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioCue_StopAll);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeOutTime = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY(_FadeOutTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AudioCue_StopAll, _FadeOutTime);
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

// --------------------------------------------------------------------------------------------------------------------