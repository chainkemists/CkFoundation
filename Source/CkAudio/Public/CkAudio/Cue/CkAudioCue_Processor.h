#pragma once

#include "CkAudioCue_Fragment.h"
#include "CkAudioCue_EntityScript.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKAUDIO_API FProcessor_AudioCue_Setup : public ck_exp::TProcessor<
            FProcessor_AudioCue_Setup,
            FCk_Handle_AudioCue,
            FFragment_AudioCue_Current,
            FFragment_EntityScript_Current,
            FTag_AudioCue_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AudioCue_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioCue_HandleRequests : public ck_exp::TProcessor<
            FProcessor_AudioCue_HandleRequests,
            FCk_Handle_AudioCue,
            FFragment_AudioCue_Current,
            FFragment_EntityScript_Current,
            FFragment_AudioCue_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AudioCue_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            FFragment_AudioCue_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FCk_Request_AudioCue_Play& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FCk_Request_AudioCue_Stop& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FCk_Request_AudioCue_StopAll& InRequest) -> void;

    private:
        static auto
        DoSelectAndPlayTrack(
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent,
            const UCk_AudioCue_EntityScript* InAudioCueScript,
            TOptional<int32> InOverridePriority,
            FCk_Time InFadeInTime) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_AudioCue_Teardown : public ck_exp::TProcessor<
            FProcessor_AudioCue_Teardown,
            FCk_Handle_AudioCue,
            FFragment_AudioCue_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AudioCue_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
