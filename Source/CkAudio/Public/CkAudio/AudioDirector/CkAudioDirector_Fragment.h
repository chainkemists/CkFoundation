#pragma once

#include "CkAudioDirector_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkTimer/CkTimer_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AudioDirector_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AudioDirector_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_AudioDirector_Params = FCk_Fragment_AudioDirector_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAudioTracks, FCk_Handle_AudioTrack);
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAudioTimers, FCk_Handle_Timer);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_AudioDirector_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AudioDirector_Current);

    public:
        friend class FProcessor_AudioDirector_Setup;
        friend class FProcessor_AudioDirector_HandleRequests;
        friend class FProcessor_AudioDirector_Teardown;
        friend class UCk_Utils_AudioDirector_UE;

    private:
        int32 _CurrentHighestPriority = -1;
        TMap<FGameplayTag, FCk_Handle_AudioTrack> _TracksByName;
        TMap<FGameplayTag, TStrongObjectPtr<UCk_MusicLibrary_Base>> _MusicLibrariesByName;
        TMap<FGameplayTag, TStrongObjectPtr<UCk_StingerLibrary_Base>> _StingerLibrariesByName;
        TMap<FGameplayTag, FCk_Handle_AudioTrack> _ActiveMusicTracks;

        TMap<FGameplayTag, TArray<FCk_Handle_AudioTrack>> _ActiveStingers;
        TMap<FGameplayTag, float> _StingerCooldowns;

    public:
        CK_PROPERTY_GET(_CurrentHighestPriority);
        CK_PROPERTY_GET(_TracksByName);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_AudioDirector_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_AudioDirector_Requests);

    public:
        friend class FProcessor_AudioDirector_HandleRequests;
        friend class UCk_Utils_AudioDirector_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_AudioDirector_StartTrack,
            FCk_Request_AudioDirector_StopTrack,
            FCk_Request_AudioDirector_StopAllTracks,
            FCk_Request_AudioDirector_AddTrack,
            FCk_Request_AudioDirector_StartMusicLibrary,
            FCk_Request_AudioDirector_PlayStinger,
            FCk_Request_AudioDirector_AddMusicLibrary
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
        OnAudioDirector_TrackStarted,
        FCk_Delegate_AudioDirector_Track_MC,
        FCk_Handle_AudioDirector,
        FGameplayTag,
        FCk_Handle_AudioTrack);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioDirector_TrackStopped,
        FCk_Delegate_AudioDirector_Track_MC,
        FCk_Handle_AudioDirector,
        FGameplayTag,
        FCk_Handle_AudioTrack);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKAUDIO_API,
        OnAudioDirector_TrackAdded,
        FCk_Delegate_AudioDirector_Track_MC,
        FCk_Handle_AudioDirector,
        FGameplayTag,
        FCk_Handle_AudioTrack);
}

// --------------------------------------------------------------------------------------------------------------------
