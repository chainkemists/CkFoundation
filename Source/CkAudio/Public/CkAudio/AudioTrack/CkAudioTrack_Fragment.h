#pragma once

#include "CkAudio/AudioTrack/CkAudioTrack_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <Components/AudioComponent.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AudioTrack_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AudioTrack_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_AudioTrack_IsPlaying);
    CK_DEFINE_ECS_TAG(FTag_AudioTrack_IsFading);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_AudioTrack_Params = FCk_Fragment_AudioTrack_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_AudioTrack_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AudioTrack_Current);

    public:
        friend class FProcessor_AudioTrack_Setup;
        friend class FProcessor_AudioTrack_HandleRequests;
        friend class FProcessor_AudioTrack_Playback;
        friend class FProcessor_AudioTrack_Teardown;
        friend class FProcessor_AudioTrack_SpatialUpdate;
        friend class UCk_Utils_AudioTrack_UE;

    private:
        TStrongObjectPtr<UAudioComponent> _AudioComponent;
        ECk_AudioTrack_State _State = ECk_AudioTrack_State::Stopped;
        float _CurrentVolume = 0.0f;
        float _TargetVolume = 0.0f;
        float _FadeSpeed = 0.0f; // Volume units per second

    public:
        CK_PROPERTY_GET(_AudioComponent);
        CK_PROPERTY_GET(_State);
        CK_PROPERTY_GET(_CurrentVolume);
        CK_PROPERTY_GET(_TargetVolume);
        CK_PROPERTY_GET(_FadeSpeed);
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
        FCk_Delegate_AudioTrack_Event_MC,
        FCk_Handle_AudioTrack,
        ECk_AudioTrack_State);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_PlaybackFinished,
        FCk_Delegate_AudioTrack_Event_MC,
        FCk_Handle_AudioTrack,
        ECk_AudioTrack_State);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioTrack_FadeCompleted,
        FCk_Delegate_AudioTrack_Fade_MC,
        FCk_Handle_AudioTrack,
        float,
        ECk_AudioTrack_State);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AudioTrack_DebugDraw);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_AudioTrack_Debug
    {
    public:
        friend class FProcessor_AudioTrack_DebugDraw_Individual_NonSpatial;
        friend class FProcessor_AudioTrack_DebugDraw_All_NonSpatial;
        friend auto AudioTrack_UpdateDebugInfo(FCk_Time, const FFragment_AudioTrack_Current&, FFragment_AudioTrack_Debug&) -> void;

        CK_GENERATED_BODY(FFragment_AudioTrack_Debug);

    private:
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
