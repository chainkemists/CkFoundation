#pragma once

#include "CkAudioDirector_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKAUDIO_API FProcessor_AudioDirector_Setup : public ck_exp::TProcessor<
            FProcessor_AudioDirector_Setup,
            FCk_Handle_AudioDirector,
            FFragment_AudioDirector_Params,
            FFragment_AudioDirector_Current,
            FTag_AudioDirector_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AudioDirector_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioDirector_HandleRequests : public ck_exp::TProcessor<
            FProcessor_AudioDirector_HandleRequests,
            FCk_Handle_AudioDirector,
            FFragment_AudioDirector_Params,
            FFragment_AudioDirector_Current,
            FFragment_AudioDirector_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AudioDirector_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            FFragment_AudioDirector_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_AddTrack& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_StartTrack& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_StopTrack& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_StopAllTracks& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_AddMusicLibrary& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_AddStingerLibrary& InRequest) -> void;

        static auto
        DoHandleRequest(
            FCk_Handle_AudioDirector InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_StartMusicLibrary& InRequest) -> void;

        static auto
        DoHandleRequest(
            FCk_Handle_AudioDirector InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            const FCk_Request_AudioDirector_PlayStinger& InRequest) -> void;

    private:
        static auto
        DoHandlePriorityOverride(
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            FCk_Handle_AudioTrack InNewTrack,
            int32 InNewTrackPriority,
            ECk_AudioTrack_OverrideBehavior InOverrideBehavior) -> void;

        static auto
        DoStopLowerPriorityTracks(
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent,
            int32 InNewTrackPriority) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioDirector_Teardown : public ck_exp::TProcessor<
            FProcessor_AudioDirector_Teardown,
            FCk_Handle_AudioDirector,
            FFragment_AudioDirector_Params,
            FFragment_AudioDirector_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AudioDirector_Params& InParams,
            FFragment_AudioDirector_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
