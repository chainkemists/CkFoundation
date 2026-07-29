#pragma once

#include "CkVfx_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKFX_API FProcessor_Vfx_Setup : public ck_exp::TProcessor<
            FProcessor_Vfx_Setup,
            FCk_Handle_Vfx,
            TReadOnly<FFragment_Vfx_Params>,
            TReadWrite<FFragment_Vfx_Current>,
            FTag_Vfx_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using MarkedDirtyBy = FTag_Vfx_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Vfx_Params& InParams,
            FFragment_Vfx_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKFX_API FProcessor_Vfx_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Vfx_HandleRequests,
            FCk_Handle_Vfx,
            TReadOnly<FFragment_Vfx_Params>,
            TReadWrite<FFragment_Vfx_Current>,
            TReadWrite<FFragment_Vfx_Requests>,
            TExclude<FTag_Vfx_NeedsSetup>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_Vfx_Setup>;
        using MarkedDirtyBy = FFragment_Vfx_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Vfx_Params& InParams,
            FFragment_Vfx_Current& InCurrent,
            FFragment_Vfx_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Vfx_Params& InParams,
            FFragment_Vfx_Current& InCurrent,
            const FCk_Request_Vfx_PlayAttached& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Vfx_Params& InParams,
            FFragment_Vfx_Current& InCurrent,
            const FCk_Request_Vfx_PlayAtLocation& InRequest) -> ECk_Request_OperationResult;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed vfx's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKFX_API FProcessor_Vfx_CancelPendingRequests : public ck_exp::TProcessor<
            FProcessor_Vfx_CancelPendingRequests,
            FCk_Handle_Vfx,
            TReadOnly<FFragment_Vfx_Requests>,
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
            const FFragment_Vfx_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKFX_API FProcessor_Vfx_EndPlay : public ck_exp::TProcessor<
            FProcessor_Vfx_EndPlay,
            FCk_Handle_Vfx,
            TReadWrite<FFragment_Vfx_Current>,
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
            FFragment_Vfx_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
