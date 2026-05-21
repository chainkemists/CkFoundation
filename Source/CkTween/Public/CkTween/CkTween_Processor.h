#pragma once

#include "CkTween_Fragment.h"

#include "CkCore/Math/ValueRange/CkValueRange.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Tween_HandleRequests;

    class CKTWEEN_API FProcessor_Tween_Update : public ck_exp::TProcessor<
        FProcessor_Tween_Update,
        FCk_Handle_Tween,
        ck::TReadOnly<FFragment_Tween_Params>,
        ck::TReadWrite<FFragment_Tween_Current>,
        FTag_Tween_Playing,
        TExclude<FTag_Tween_Paused>,
        TExclude<FTag_Tween_Completed>,
        TExclude<FTag_Tween_InYoyoDelay>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_Tween_HandleRequests>;
        using TProcessor::TProcessor;

    public:
        static auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            FFragment_Tween_Current& InCurrent) -> void;

    private:
        static auto DoCalculateProgress(const FFragment_Tween_Params& InParams, const FFragment_Tween_Current& InCurrent) -> FCk_FloatRange_0to1;
        static auto DoCheckLoopCompletion(HandleType InHandle, const FFragment_Tween_Params& InParams, FFragment_Tween_Current& InCurrent) -> void;
        static auto DoStartNextTweenInQueue(HandleType InHandle) -> void;
        static auto DoResolveValue(const FCk_TweenValue& InValue, ECk_TweenTarget InTargetType) -> FCk_TweenValue;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTWEEN_API FProcessor_Tween_HandleYoyoDelays : public ck_exp::TProcessor<
        FProcessor_Tween_HandleYoyoDelays,
        FCk_Handle_Tween,
        ck::TReadOnly<FFragment_Tween_Params>,
        ck::TReadWrite<FFragment_Tween_Current>,
        FTag_Tween_InYoyoDelay,
        TExclude<FTag_Tween_Paused>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_Tween_Update>;
        using TProcessor::TProcessor;

    public:
        static auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            FFragment_Tween_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTWEEN_API FProcessor_Tween_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Tween_HandleRequests,
        FCk_Handle_Tween,
        ck::TReadWrite<FFragment_Tween_Current>,
        ck::TReadOnly<FFragment_Tween_Requests>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FFragment_Tween_Requests;
        using TProcessor::TProcessor;

    public:
        auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FFragment_Tween_Requests& InRequestsComp) const -> void;

    private:
        static auto
    	DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Pause& InRequest) -> void;

        static auto
    	DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Resume& InRequest) -> void;

        static auto
    	DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Stop& InRequest) -> void;

        static auto
    	DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Restart& InRequest) -> void;

        static auto
    	DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_SetTimeMultiplier& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTWEEN_API FProcessor_Tween_ApplyToTransform : public ck_exp::TProcessor<
        FProcessor_Tween_ApplyToTransform,
        FCk_Handle_Tween,
        ck::TReadOnly<FFragment_Tween_Params>,
        ck::TReadOnly<FFragment_Tween_Current>,
        FTag_Tween_Playing,
        TExclude<FTag_Tween_Paused>,
        TExclude<FTag_Tween_Completed>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        // Must run before Transform_HandleRequests so the SetLocation/Rotation/Scale
        // requests this processor enqueues are applied the same frame. Without this
        // ordering, the request sits until the next frame's HandleRequests sweep —
        // but by then the FTag_Transform_Updated from HandleRequests has already
        // been cleared, so scene-node descendants never observe the parent update
        // and the tween appears to move only the root.
        using RunBefore = TDepList<FProcessor_Transform_HandleRequests>;
        using TProcessor::TProcessor;

    public:
        static auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            const FFragment_Tween_Current& InCurrent) -> void;

    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTWEEN_API FProcessor_Tween_ApplySplineFollow : public ck_exp::TProcessor<
        FProcessor_Tween_ApplySplineFollow,
        FCk_Handle_Tween,
        ck::TReadOnly<FFragment_Tween_Current>,
        ck::TReadOnly<FFragment_Tween_SplineFollow>,
        FTag_Tween_Playing,
        TExclude<FTag_Tween_Paused>,
        TExclude<FTag_Tween_Completed>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        // Same ordering rationale as FProcessor_Tween_ApplyToTransform: must run before
        // Transform_HandleRequests so the SetLocation/SetRotation requests are applied
        // the same frame and scene-node descendants observe the parent update.
        using RunBefore = TDepList<FProcessor_Transform_HandleRequests>;
        using TProcessor::TProcessor;

    public:
        static auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Current& InCurrent,
            const FFragment_Tween_SplineFollow& InSplineFollow) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
