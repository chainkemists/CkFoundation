#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkAudio/AudioTrack/CkAudioTrack_Fragment_Data.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

#include <GameplayTagContainer.h>

#include "CkAudioDirector_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKAUDIO_API FCk_Handle_AudioDirector : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AudioDirector); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AudioDirector);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Fragment_AudioDirector_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AudioDirector_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _DefaultCrossfadeDuration = FCk_Time{2.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _MaxConcurrentTracks = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _AllowSamePriorityTracks = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _Script;

public:
    CK_PROPERTY(_DefaultCrossfadeDuration);
    CK_PROPERTY(_MaxConcurrentTracks);
    CK_PROPERTY(_AllowSamePriorityTracks);
    CK_PROPERTY(_Script);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_AudioDirector_ParamsData, _Script);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioDirector_StartTrack : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioDirector_StartTrack);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioDirector_StartTrack);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Audio.Track"))
    FGameplayTag _TrackName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TOptional<int32> _OverridePriority;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeInTime = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY(_TrackName);
    CK_PROPERTY(_OverridePriority);
    CK_PROPERTY(_FadeInTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AudioDirector_StartTrack, _TrackName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioDirector_StopTrack : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioDirector_StopTrack);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioDirector_StopTrack);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Audio.Track"))
    FGameplayTag _TrackName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeOutTime = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY_GET(_TrackName);
    CK_PROPERTY(_FadeOutTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AudioDirector_StopTrack, _TrackName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioDirector_StopAllTracks : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioDirector_StopAllTracks);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioDirector_StopAllTracks);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeOutTime = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY(_FadeOutTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AudioDirector_StopAllTracks, _FadeOutTime);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioDirector_AddTrack : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioDirector_AddTrack);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioDirector_AddTrack);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_AudioTrack_ParamsData _TrackParams;

public:
    CK_PROPERTY(_TrackParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AudioDirector_AddTrack, _TrackParams);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_AudioDirector_Track,
    FCk_Handle_AudioDirector, InDirector,
    FGameplayTag, InTrackName,
    FCk_Handle_AudioTrack, InTrack);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_AudioDirector_Track_MC,
    FCk_Handle_AudioDirector, InDirector,
    FGameplayTag, InTrackName,
    FCk_Handle_AudioTrack, InTrack);

// --------------------------------------------------------------------------------------------------------------------
