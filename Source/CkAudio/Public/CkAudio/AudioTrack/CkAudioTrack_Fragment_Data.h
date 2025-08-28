#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>
#include <Sound/SoundBase.h>
#include <Sound/SoundConcurrency.h>
#include <Sound/SoundAttenuation.h>

#include "CkAudioTrack_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKAUDIO_API FCk_Handle_AudioTrack : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AudioTrack); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AudioTrack);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AudioTrack_OverrideBehavior : uint8
{
    Interrupt,   // Stop lower priority immediately, start this track
    Crossfade,   // Fade out lower while fading in this track
    Queue        // Wait for current track to finish naturally
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AudioTrack_OverrideBehavior);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AudioTrack_State : uint8
{
    Stopped,
    Playing,
    FadingIn,
    FadingOut,
    Paused
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AudioTrack_State);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_LoopBehavior : uint8
{
    PlayOnce,    // Play the track once and stop
    Loop         // Loop the track continuously
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_LoopBehavior);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Fragment_AudioTrack_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AudioTrack_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Audio.Track"))
    FGameplayTag _TrackName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<USoundBase> _Sound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_AudioTrack_OverrideBehavior _OverrideBehavior = ECk_AudioTrack_OverrideBehavior::Crossfade;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LoopBehavior _LoopBehavior = ECk_LoopBehavior::Loop;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Volume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _DefaultFadeInTime = FCk_Time{1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _DefaultFadeOutTime = FCk_Time{1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
          meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_UE> _ScriptAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<USoundAttenuation> _LibraryAttenuationSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<USoundConcurrency> _LibraryConcurrencySettings;

public:
    CK_PROPERTY_GET(_TrackName);
    CK_PROPERTY_GET(_Sound);
    CK_PROPERTY(_Priority);
    CK_PROPERTY(_OverrideBehavior);
    CK_PROPERTY(_LoopBehavior);
    CK_PROPERTY(_Volume);
    CK_PROPERTY(_DefaultFadeInTime);
    CK_PROPERTY(_DefaultFadeOutTime);
    CK_PROPERTY(_ScriptAsset);
    CK_PROPERTY(_LibraryAttenuationSettings);
    CK_PROPERTY(_LibraryConcurrencySettings);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_AudioTrack_ParamsData, _TrackName, _Sound);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioTrack_Play : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioTrack_Play);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioTrack_Play);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeInTime = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY(_FadeInTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AudioTrack_Play, _FadeInTime);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioTrack_Stop : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioTrack_Stop);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioTrack_Stop);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeOutTime = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY(_FadeOutTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AudioTrack_Stop, _FadeOutTime);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_AudioTrack_SetVolume : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AudioTrack_SetVolume);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AudioTrack_SetVolume);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _TargetVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeTime = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY(_TargetVolume);
    CK_PROPERTY(_FadeTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AudioTrack_SetVolume, _TargetVolume, _FadeTime);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AudioTrack_Event,
    FCk_Handle_AudioTrack, InTrack,
    ECk_AudioTrack_State, InState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AudioTrack_Event_MC,
    FCk_Handle_AudioTrack, InTrack,
    ECk_AudioTrack_State, InState);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_AudioTrack_Fade,
    FCk_Handle_AudioTrack, InTrack,
    float, InVolume,
    ECk_AudioTrack_State, InState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_AudioTrack_Fade_MC,
    FCk_Handle_AudioTrack, InTrack,
    float, InVolume,
    ECk_AudioTrack_State, InState);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AudioTrack_PlayStateChanged,
    FCk_Handle_AudioTrack, InTrack,
    EAudioComponentPlayState, InPlayState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AudioTrack_PlayStateChanged_MC,
    FCk_Handle_AudioTrack, InTrack,
    EAudioComponentPlayState, InPlayState);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AudioTrack_VirtualizationChanged,
    FCk_Handle_AudioTrack, InTrack,
    bool, InIsVirtualized);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AudioTrack_VirtualizationChanged_MC,
    FCk_Handle_AudioTrack, InTrack,
    bool, InIsVirtualized);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AudioTrack_PlaybackPercent,
    FCk_Handle_AudioTrack, InTrack,
    float, InPlaybackPercent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AudioTrack_PlaybackPercent_MC,
    FCk_Handle_AudioTrack, InTrack,
    float, InPlaybackPercent);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AudioTrack_SingleEnvelope,
    FCk_Handle_AudioTrack, InTrack,
    float, InEnvelopeValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AudioTrack_SingleEnvelope_MC,
    FCk_Handle_AudioTrack, InTrack,
    float, InEnvelopeValue);

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_AudioTrack_MultiEnvelope,
    FCk_Handle_AudioTrack, InTrack,
    float, InAverageEnvelopeValue,
    float, InMaxEnvelope,
    int32, InNumWaveInstances);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FCk_Delegate_AudioTrack_MultiEnvelope_MC,
    FCk_Handle_AudioTrack, InTrack,
    float, InAverageEnvelopeValue,
    float, InMaxEnvelope,
    int32, InNumWaveInstances);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_AudioTrack_AudioFinished,
    FCk_Handle_AudioTrack, InTrack);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_AudioTrack_AudioFinished_MC,
    FCk_Handle_AudioTrack, InTrack);

// --------------------------------------------------------------------------------------------------------------------
