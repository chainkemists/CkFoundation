#pragma once

#include "CkAudioCue_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AudioCue_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AudioCue_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_AudioCue_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AudioCue_Current);

    public:
        friend class FProcessor_AudioCue_Setup;
        friend class FProcessor_AudioCue_HandleRequests;
        friend class FProcessor_AudioCue_Teardown;
        friend class UCk_Utils_AudioCue_UE;

    private:
        TArray<FGameplayTag> _RecentTracks; // For mood/selection tracking
        int32 _LastSelectedIndex = INDEX_NONE;

    public:
        CK_PROPERTY_GET(_RecentTracks);
        CK_PROPERTY_GET(_LastSelectedIndex);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_AudioCue_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_AudioCue_Requests);

    public:
        friend class FProcessor_AudioCue_HandleRequests;
        friend class UCk_Utils_AudioCue_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_AudioCue_Play,
            FCk_Request_AudioCue_Stop,
            FCk_Request_AudioCue_StopAll
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
        OnAudioCue_TrackStarted,
        FCk_Delegate_AudioCue_Event_MC,
        FCk_Handle_AudioCue,
        FGameplayTag);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioCue_TrackStopped,
        FCk_Delegate_AudioCue_Event_MC,
        FCk_Handle_AudioCue,
        FGameplayTag);
}

// --------------------------------------------------------------------------------------------------------------------
