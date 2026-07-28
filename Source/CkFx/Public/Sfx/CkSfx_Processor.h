#pragma once

#include "Sfx/CkSfx_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKFX_API FProcessor_Sfx_Setup : public ck_exp::TProcessor<
            FProcessor_Sfx_Setup,
            FCk_Handle_Sfx,
            TReadOnly<FFragment_Sfx_Params>,
            TReadWrite<FFragment_Sfx_Current>,
            FTag_Sfx_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using MarkedDirtyBy = FTag_Sfx_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sfx_Params& InParams,
            FFragment_Sfx_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKFX_API FProcessor_Sfx_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Sfx_HandleRequests,
            FCk_Handle_Sfx,
            TReadOnly<FFragment_Sfx_Params>,
            TReadWrite<FFragment_Sfx_Current>,
            TReadWrite<FFragment_Sfx_Requests>,
            TExclude<FTag_Sfx_NeedsSetup>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_Sfx_Setup>;
        using MarkedDirtyBy = FFragment_Sfx_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sfx_Params& InParams,
            FFragment_Sfx_Current& InCurrent,
            FFragment_Sfx_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sfx_Params& InParams,
            FFragment_Sfx_Current& InCurrent,
            const FCk_Request_Sfx_PlayAttached& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sfx_Params& InParams,
            FFragment_Sfx_Current& InCurrent,
            const FCk_Request_Sfx_PlayAtLocation& InRequest) -> ECk_Request_OperationResult;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed sfx's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKFX_API FProcessor_Sfx_CancelPendingRequests : public ck_exp::TProcessor<
            FProcessor_Sfx_CancelPendingRequests,
            FCk_Handle_Sfx,
            TReadOnly<FFragment_Sfx_Requests>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sfx_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKFX_API FProcessor_Sfx_EndPlay : public ck_exp::TProcessor<
            FProcessor_Sfx_EndPlay,
            FCk_Handle_Sfx,
            TReadWrite<FFragment_Sfx_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sfx_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
