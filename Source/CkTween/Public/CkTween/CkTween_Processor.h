#pragma once

#include "CkTween_Fragment.h"

#include "CkCore/Math/ValueRange/CkValueRange.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKTWEEN_API FProcessor_Tween_Setup : public ck_exp::TProcessor<
        FProcessor_Tween_Setup,
        FCk_Handle_Tween,
        FFragment_Tween_Params,
        FFragment_Tween_Current,
        FTag_Tween_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Tween_NeedsSetup;
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            FFragment_Tween_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTWEEN_API FProcessor_Tween_Update : public ck_exp::TProcessor<
        FProcessor_Tween_Update,
        FCk_Handle_Tween,
        FFragment_Tween_Params,
        FFragment_Tween_Current,
        FTag_Tween_Playing,
        TExclude<FTag_Tween_Paused>,
        TExclude<FTag_Tween_Completed>,
        TExclude<FTag_Tween_InYoyoDelay>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            FFragment_Tween_Current& InCurrent) const -> void;

    private:
        static auto DoCalculateProgress(const FFragment_Tween_Params& InParams, const FFragment_Tween_Current& InCurrent) -> FCk_FloatRange_0to1;
        static auto DoCheckLoopCompletion(HandleType InHandle, const FFragment_Tween_Params& InParams, FFragment_Tween_Current& InCurrent) -> void;
        static auto DoStartNextTweenInQueue(HandleType InHandle, const FFragment_Tween_Params& InParams) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTWEEN_API FProcessor_Tween_HandleYoyoDelays : public ck_exp::TProcessor<
        FProcessor_Tween_HandleYoyoDelays,
        FCk_Handle_Tween,
        FFragment_Tween_Params,
        FFragment_Tween_Current,
        FTag_Tween_InYoyoDelay,
        TExclude<FTag_Tween_Paused>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            FFragment_Tween_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTWEEN_API FProcessor_Tween_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Tween_HandleRequests,
        FCk_Handle_Tween,
        FFragment_Tween_Current,
        FFragment_Tween_Requests,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Tween_Requests;
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FFragment_Tween_Requests& InRequestsComp) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Pause& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Resume& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Stop& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Restart& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_SetTimeMultiplier& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTWEEN_API FProcessor_Tween_Teardown : public ck_exp::TProcessor<
        FProcessor_Tween_Teardown,
        FCk_Handle_Tween,
        FFragment_Tween_Params,
        FFragment_Tween_Current,
        CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            const FFragment_Tween_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTWEEN_API FProcessor_Tween_ApplyToTransform : public ck_exp::TProcessor<
        FProcessor_Tween_ApplyToTransform,
        FCk_Handle_Tween,
        FFragment_Tween_Params,
        FFragment_Tween_Current,
        FTag_Tween_Playing,
        TExclude<FTag_Tween_Paused>,
        TExclude<FTag_Tween_Completed>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            const FFragment_Tween_Current& InCurrent) const -> void;

    private:
        static auto DoApplyValueToTransform(
            const FCk_Handle& InTargetEntity,
            const FCk_TweenValue& InValue,
            FGameplayTag InTweenName) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
