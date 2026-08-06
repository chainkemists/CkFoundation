#pragma once

#include "CkAudio/AudioTrack/CkAudioTrack_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkResourceLoader/CkResourceLoader_Fragment_Data.h"

#include <Components/AudioComponent.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AudioTrack_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AudioTrack_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_AudioTrack_PendingAssetLoad);
    CK_DEFINE_ECS_TAG(FTag_AudioTrack_IsPlaying);
    CK_DEFINE_ECS_TAG(FTag_AudioTrack_IsFading);

    // --------------------------------------------------------------------------------------------------------------------

    // The retained immutable residue of FCk_AudioTrack_Spec: the fields read after construction
    // (director arbitration, Play/Stop defaults, lookups, debug). _TrackName is stored RESOLVED —
    // the spec's derive-from-sound-path logic runs once at Create.
    struct CKAUDIO_API FFragment_AudioTrack_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AudioTrack_Params);

    private:
        FName _TrackName = NAME_None;
        TSoftObjectPtr<USoundBase> _Sound;
        int32 _Priority = 50;
        ECk_AudioTrack_OverrideBehavior _OverrideBehavior = ECk_AudioTrack_OverrideBehavior::Crossfade;
        ECk_LoopBehavior _LoopBehavior = ECk_LoopBehavior::Loop;
        float _Volume = 1.0f;
        FCk_Time _DefaultFadeInTime = FCk_Time{1.0f};
        FCk_Time _DefaultFadeOutTime = FCk_Time{1.0f};

    public:
        CK_PROPERTY_GET(_TrackName);
        CK_PROPERTY_GET(_Sound);
        CK_PROPERTY_GET(_Priority);
        CK_PROPERTY_GET(_OverrideBehavior);
        CK_PROPERTY_GET(_LoopBehavior);
        CK_PROPERTY_GET(_Volume);
        CK_PROPERTY_GET(_DefaultFadeInTime);
        CK_PROPERTY_GET(_DefaultFadeOutTime);

    public:
        FFragment_AudioTrack_Params() = default;
        explicit FFragment_AudioTrack_Params(const FCk_AudioTrack_Spec& InSpec)
            : _TrackName(InSpec.Get_TrackName())
            , _Sound(InSpec.Get_Sound())
            , _Priority(InSpec.Get_Priority())
            , _OverrideBehavior(InSpec.Get_OverrideBehavior())
            , _LoopBehavior(InSpec.Get_LoopBehavior())
            , _Volume(InSpec.Get_Volume())
            , _DefaultFadeInTime(InSpec.Get_DefaultFadeInTime())
            , _DefaultFadeOutTime(InSpec.Get_DefaultFadeOutTime())
        {}
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Construction-only Spec fields that must survive to the DEFERRED Setup processor (async asset
    // resolve): Setup consumes them and REMOVES this fragment on every completion path — success,
    // unset-sound bail, and failed-load bail alike.
    struct CKAUDIO_API FFragment_AudioTrack_PendingSetup
    {
    public:
        CK_GENERATED_BODY(FFragment_AudioTrack_PendingSetup);

    private:
        TSubclassOf<UCk_EntityScript_UE> _ScriptAsset;
        TSoftObjectPtr<USoundAttenuation> _LibraryAttenuationSettings;
        TSoftObjectPtr<USoundConcurrency> _LibraryConcurrencySettings;
        TSoftObjectPtr<USoundClass> _LibrarySoundClassSettings;

    public:
        CK_PROPERTY_GET(_ScriptAsset);
        CK_PROPERTY_GET(_LibraryAttenuationSettings);
        CK_PROPERTY_GET(_LibraryConcurrencySettings);
        CK_PROPERTY_GET(_LibrarySoundClassSettings);

    public:
        FFragment_AudioTrack_PendingSetup() = default;
        explicit FFragment_AudioTrack_PendingSetup(const FCk_AudioTrack_Spec& InSpec)
            : _ScriptAsset(InSpec.Get_ScriptAsset())
            , _LibraryAttenuationSettings(InSpec.Get_LibraryAttenuationSettings())
            , _LibraryConcurrencySettings(InSpec.Get_LibraryConcurrencySettings())
            , _LibrarySoundClassSettings(InSpec.Get_LibrarySoundClassSettings())
        {}
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_AudioTrack_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AudioTrack_Current);

    public:
        friend class FProcessor_AudioTrack_Setup;
        friend class FProcessor_AudioTrack_HandleRequests;
        friend class FProcessor_AudioTrack_Playback;
        friend class FProcessor_AudioTrack_EndPlay;
        friend class FProcessor_AudioTrack_SpatialUpdate;
        friend class UCk_Utils_AudioTrack_UE;

    private:
        // WEAK — lifetime owned by the CkCore ObjectPooling subsystem (DestroyOnRelease)
        TWeakObjectPtr<UAudioComponent> _AudioComponent;

        // The GC root for the track's resolved sound + library settings: the batch's streamable
        // handle keeps them loaded for exactly as long as this fragment holds it (reset at EndPlay).
        FCk_ResourceLoader_RootedAssetBatch _LoadedAssets;

        ECk_AudioTrack_State _State = ECk_AudioTrack_State::Stopped;
        float _CurrentVolume = 0.0f;
        float _TargetVolume = 0.0f;
        float _FadeSpeed = 0.0f; // Volume units per second

        // Cached data from AudioComponent for debug/query purposes
        float _PlaybackPercent = 0.0f;
        bool _IsVirtualized = false;

        // Delegate handles for AudioComponent bindings
        FDelegateHandle _PlayStateChangedHandle;
        FDelegateHandle _VirtualizationChangedHandle;
        FDelegateHandle _PlaybackPercentHandle;
        FDelegateHandle _SingleEnvelopeHandle;
        FDelegateHandle _MultiEnvelopeHandle;
        FDelegateHandle _AudioFinishedHandle;

    public:
        CK_PROPERTY_GET(_AudioComponent);
        CK_PROPERTY_GET(_State);
        CK_PROPERTY_GET(_CurrentVolume);
        CK_PROPERTY_GET(_TargetVolume);
        CK_PROPERTY_GET(_FadeSpeed);
        CK_PROPERTY_GET(_PlaybackPercent);
        CK_PROPERTY_GET(_IsVirtualized);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_AudioTrack_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_AudioTrack_Requests);

    public:
        friend class FProcessor_AudioTrack_HandleRequests;
        friend class UCk_Utils_AudioTrack_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_AudioTrack_Play,
            FCk_Request_AudioTrack_Stop,
            FCk_Request_AudioTrack_SetVolume
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_PlaybackStarted,
        FCk_Delegate_AudioTrack_Event,
        FCk_Handle_AudioTrack,
        ECk_AudioTrack_State);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_PlaybackFinished,
        FCk_Delegate_AudioTrack_Event,
        FCk_Handle_AudioTrack,
        ECk_AudioTrack_State);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_FadeCompleted,
        FCk_Delegate_AudioTrack_Fade,
        FCk_Handle_AudioTrack,
        float,
        ECk_AudioTrack_State);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_PlayStateChanged,
        FCk_Delegate_AudioTrack_PlayStateChanged,
        FCk_Handle_AudioTrack,
        EAudioComponentPlayState);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_VirtualizationChanged,
        FCk_Delegate_AudioTrack_VirtualizationChanged,
        FCk_Handle_AudioTrack,
        bool);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_PlaybackPercent,
        FCk_Delegate_AudioTrack_PlaybackPercent,
        FCk_Handle_AudioTrack,
        float);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_SingleEnvelope,
        FCk_Delegate_AudioTrack_SingleEnvelope,
        FCk_Handle_AudioTrack,
        float);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_MultiEnvelope,
        FCk_Delegate_AudioTrack_MultiEnvelope,
        FCk_Handle_AudioTrack,
        float,
        float,
        int32);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_AudioFinished,
        FCk_Delegate_AudioTrack_AudioFinished,
        FCk_Handle_AudioTrack);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AudioTrack_DebugDraw);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_AudioTrack_Debug
    {
    public:
        CK_GENERATED_BODY(FFragment_AudioTrack_Debug);

    public:
        FCk_Time _LastPulseTime = FCk_Time::ZeroSecond();
        float _CurrentPulseScale = 1.0f;
        FLinearColor _StateColor = FLinearColor::White;

        // For non-spatial tracks, we track screen position for HUD elements
        FVector2D _NonSpatialHUDPosition = FVector2D::ZeroVector;
        int32 _HUDSlotIndex = 0; // Which slot this track occupies in the HUD

    public:
        CK_PROPERTY_GET(_LastPulseTime);
        CK_PROPERTY_GET(_CurrentPulseScale);
        CK_PROPERTY_GET(_StateColor);
        CK_PROPERTY_GET(_NonSpatialHUDPosition);
        CK_PROPERTY_GET(_HUDSlotIndex);

    private:
        friend class FProcessor_AudioTrack_Debug;
    };
}

// --------------------------------------------------------------------------------------------------------------------
