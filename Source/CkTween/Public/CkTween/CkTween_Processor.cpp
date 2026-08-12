#include "CkTween_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/ValueRange/CkValueRange_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkTimer/CkTimer_Utils.h"
#include "CkTween/CkTween_Easing_Utils.h"
#include "CkTween/CkTween_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkSpline/CkSpline_Utils.h"

#include "CkTween/CkTween_Stats.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Tween::SplineFollowEval"), STAT_Tween_SplineFollowEval, STATGROUP_CkTween);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Tween_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_Tween_HandleYoyoDelays);
CK_REGISTER_PROCESSOR(ck::FProcessor_Tween_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Tween_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Tween_ApplyToTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_Tween_ApplySplineFollow);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_tween
{
    auto ApplyValueToTransform(
        const FCk_Handle& InTweenHandle,
        const FCk_TweenValue& InValue,
        ECk_TweenTarget InTarget)
        -> void
    {
        if (InTarget == ECk_TweenTarget::Custom)
        { return; }

        const auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InTweenHandle);
        if (ck::Is_NOT_Valid(TargetEntity))
        { return; }

        auto MaybeTransformHandle = UCk_Utils_Transform_UE::Cast(TargetEntity);
        if (ck::Is_NOT_Valid(MaybeTransformHandle))
        { return; }

        switch (InTarget)
        {
            case ECk_TweenTarget::Transform_Location:
            {
                if (InValue.IsVector())
                {
                    UCk_Utils_Transform_UE::Request_SetLocation(MaybeTransformHandle, FCk_Request_Transform_SetLocation{InValue.GetAsVector()}, {});
                }
                break;
            }
            case ECk_TweenTarget::Transform_Rotation:
            {
                if (InValue.IsRotator())
                {
                    UCk_Utils_Transform_UE::Request_SetRotation(MaybeTransformHandle, FCk_Request_Transform_SetRotation{InValue.GetAsRotator()}, {});
                }
                break;
            }
            case ECk_TweenTarget::Transform_Scale:
            {
                if (InValue.IsVector())
                {
                    UCk_Utils_Transform_UE::Request_SetScale(MaybeTransformHandle, FCk_Request_Transform_SetScale{InValue.GetAsVector()}, {});
                }
                break;
            }
            case ECk_TweenTarget::Custom:
            default:
            {
                break;
            }
        }
    }

    // Shared by FProcessor_Tween_ApplySplineFollow (per-tick) and the completion snap.
    auto ApplyProgressToSplineFollow(
        const FCk_Handle& InTweenHandle,
        const ck::FFragment_Tween_SplineFollow& InSplineFollow,
        float InProgress)
        -> void
    {
        const auto& SplineHandle = InSplineFollow.Get_Spline();
        if (ck::Is_NOT_Valid(SplineHandle))
        { return; }

        const auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InTweenHandle);
        if (ck::Is_NOT_Valid(TargetEntity))
        { return; }

        auto MaybeTransformHandle = UCk_Utils_Transform_UE::Cast(TargetEntity);
        if (ck::Is_NOT_Valid(MaybeTransformHandle))
        { return; }

        SCOPE_CYCLE_COUNTER(STAT_Tween_SplineFollowEval);

        const auto Distance = FMath::Clamp(InProgress, 0.0f, 1.0f) * UCk_Utils_Spline_UE::Get_Length(SplineHandle);

        const auto Location = UCk_Utils_Spline_UE::Get_LocationAtDistance(SplineHandle, Distance);
        UCk_Utils_Transform_UE::Request_SetLocation(MaybeTransformHandle, FCk_Request_Transform_SetLocation{Location}, {});

        if (InSplineFollow.Get_Orientation() != ECk_Tween_SplineOrientation::OrientToSpline)
        { return; }

        const auto Rotation = UCk_Utils_Spline_UE::Get_RotationAtDistance(SplineHandle, Distance);
        UCk_Utils_Transform_UE::Request_SetRotation(MaybeTransformHandle, FCk_Request_Transform_SetRotation{Rotation}, {});
    }

    // An unset channel contributes 0 rather than erroring: a single-axis shake authors one curve
    // and leaves the other two null.
    auto EvaluateCurveChannel(
        const UCurveFloat* InCurve,
        float InTime)
        -> float
    {
        if (ck::Is_NOT_Valid(InCurve, ck::IsValid_Policy_NullptrOnly{}))
        { return 0.0f; }

        return InCurve->GetFloatValue(InTime);
    }

    auto EvaluateCurveDrive(
        const ck::FFragment_Tween_CurveDrive& InCurveDrive,
        float InElapsedSeconds,
        FCk_FloatRange_0to1 InProgress)
        -> FCk_TweenValue
    {
        const auto Time = InCurveDrive.Get_TimeInput() == ECk_TweenCurveTimeInput::ElapsedSeconds
            ? InElapsedSeconds
            : InProgress.Get_Value();

        switch (InCurveDrive.Get_Output())
        {
            case ECk_TweenCurveOutput::VectorOffset:
            {
                const auto Offset = FVector
                {
                    EvaluateCurveChannel(InCurveDrive.Get_Curve_X().Get(), Time),
                    EvaluateCurveChannel(InCurveDrive.Get_Curve_Y().Get(), Time),
                    EvaluateCurveChannel(InCurveDrive.Get_Curve_Z().Get(), Time)
                };

                return FCk_TweenValue{InCurveDrive.Get_BaseValue().GetAsVector() + Offset};
            }
            case ECk_TweenCurveOutput::RotatorOffset:
            {
                const auto Offset = FRotator
                {
                    EvaluateCurveChannel(InCurveDrive.Get_Curve_X().Get(), Time),
                    EvaluateCurveChannel(InCurveDrive.Get_Curve_Y().Get(), Time),
                    EvaluateCurveChannel(InCurveDrive.Get_Curve_Z().Get(), Time)
                };

                // Quaternion compose, NOT a component-wise FRotator add. The two agree only while
                // the base is axis-aligned; once it carries a yaw (any placed prop) the additive
                // form shears the pose. The offset acts in the PARENT frame, hence Offset * Base.
                const auto Composed = (Offset.Quaternion() * InCurveDrive.Get_BaseValue().GetAsRotator().Quaternion()).Rotator();
                return FCk_TweenValue{Composed};
            }
            case ECk_TweenCurveOutput::Float:
            default:
            {
                return FCk_TweenValue{EvaluateCurveChannel(InCurveDrive.Get_Curve_X().Get(), Time)};
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Tween_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            FFragment_Tween_Current& InCurrent)
            -> void
    {
        const auto DeltaTime = InDeltaT.Get_Seconds() * InCurrent.Get_TimeMultiplier();
        InCurrent.Set_CurrentTime(InCurrent.Get_CurrentTime() + DeltaTime);

        const auto Progress = DoCalculateProgress(InParams, InCurrent);

        const auto InterpolatedValue = DoComputeValue(InHandle, InParams, InCurrent, Progress);
        InCurrent.Set_CurrentValue(InterpolatedValue);

        UUtils_Signal_OnTweenUpdate::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Tween_Payload_OnUpdate{InterpolatedValue, Progress}));

        if (InCurrent.Get_CurrentTime() >= InParams.Get_Duration())
        {
            DoCheckLoopCompletion(InHandle, InParams, InCurrent);
        }
    }

    auto
        FProcessor_Tween_Update::
        DoCalculateProgress(
            const FFragment_Tween_Params& InParams,
            const FFragment_Tween_Current& InCurrent)
        -> FCk_FloatRange_0to1
    {
        if (InParams.Get_Duration() <= 0.0f)
        { return UCk_Utils_FloatRange_UE::Make_FloatRange_0to1(1.0f); }

        const auto Progress = FMath::Clamp(InCurrent.Get_CurrentTime() / InParams.Get_Duration(), 0.0f, 1.0f);
        return UCk_Utils_FloatRange_UE::Make_FloatRange_0to1(Progress);
    }

    auto
        FProcessor_Tween_Update::
        DoResolveValue(
            const FCk_TweenValue& InValue,
            ECk_TweenTarget InTargetType)
        -> FCk_TweenValue
    {
        if (NOT InValue.IsTransformHandle())
        { return InValue; }

        const auto TransformHandle = InValue.GetAsTransformHandle();
        if (ck::Is_NOT_Valid(TransformHandle))
        { return InValue; }

        switch (InTargetType)
        {
            case ECk_TweenTarget::Transform_Location:
            {
                const auto Location = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);
                return FCk_TweenValue{Location};
            }
            case ECk_TweenTarget::Transform_Rotation:
            {
                const auto Rotation = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(TransformHandle);
                return FCk_TweenValue{Rotation};
            }
            case ECk_TweenTarget::Transform_Scale:
            {
                const auto Scale = UCk_Utils_Transform_UE::Get_EntityCurrentScale(TransformHandle);
                return FCk_TweenValue{Scale};
            }
            case ECk_TweenTarget::Custom:
            default:
            {
                return InValue;
            }
        }
    }

    auto
        FProcessor_Tween_Update::
        DoComputeValue(
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            const FFragment_Tween_Current& InCurrent,
            FCk_FloatRange_0to1 InProgress)
        -> FCk_TweenValue
    {
        // Curve-driven: the curve's OUTPUT is the value, so Start/End play no part at all. This is
        // the only shape that can express a motion returning to where it began -- with Start == End
        // the interpolation below is constant for every alpha, however the easing reshapes it.
        if (InHandle.Has<FFragment_Tween_CurveDrive>())
        {
            return ck_tween::EvaluateCurveDrive(
                InHandle.Get<FFragment_Tween_CurveDrive>(), InCurrent.Get_CurrentTime(), InProgress);
        }

        const auto& StartValueRef = InCurrent.Get_IsReversed() ? InParams.Get_EndValue() : InParams.Get_StartValue();
        const auto& EndValueRef = InCurrent.Get_IsReversed() ? InParams.Get_StartValue() : InParams.Get_EndValue();

        const auto StartValue = DoResolveValue(StartValueRef, InParams.Get_Target());
        const auto EndValue = DoResolveValue(EndValueRef, InParams.Get_Target());

        if (InHandle.Has<FFragment_Tween_EasingCurve>())
        {
            if (const auto& EasingCurve = InHandle.Get<FFragment_Tween_EasingCurve>();
                ck::IsValid(EasingCurve.Get_Curve().Get(), ck::IsValid_Policy_NullptrOnly{}))
            {
                // The curve REPLACES the easing table, so the table entry is bypassed (Linear).
                // Interpolate takes a 0to1 range, so a curve that overshoots (an easeOutBack shape)
                // is CLAMPED here -- authoring a real overshoot needs the curve-drive path above.
                const auto Shaped = ck_tween::EvaluateCurveChannel(EasingCurve.Get_Curve().Get(), InProgress.Get_Value());

                return UCk_Utils_TweenEasing_UE::Interpolate(StartValue, EndValue,
                    UCk_Utils_FloatRange_UE::Make_FloatRange_0to1(Shaped), ECk_TweenEasing::Linear);
            }
        }

        return UCk_Utils_TweenEasing_UE::Interpolate(StartValue, EndValue, InProgress, InParams.Get_Easing());
    }

    auto
        FProcessor_Tween_Update::
        DoCheckLoopCompletion(
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            FFragment_Tween_Current& InCurrent)
        -> void
    {
        const auto CurrentLoop = InCurrent.Get_CurrentLoop() + 1;

        if (const auto ShouldLoop = InParams.Get_LoopCount() == -1 || CurrentLoop < InParams.Get_LoopCount();
            NOT ShouldLoop)
        {
            InHandle.Remove<FTag_Tween_Playing>();
            InHandle.Add<FTag_Tween_Completed>();
            InCurrent.Set_State(ECk_TweenState::Completed);

            // A curve-driven tween's end is its curve sampled at the far end, NOT EndValue -- which
            // it never used. A shake curve whose last key is 0 therefore lands exactly on the
            // captured base, which is what restores the rest pose on completion.
            const auto FinalValue = [&]() -> FCk_TweenValue
            {
                if (InHandle.Has<FFragment_Tween_CurveDrive>())
                {
                    return ck_tween::EvaluateCurveDrive(
                        InHandle.Get<FFragment_Tween_CurveDrive>(),
                        InParams.Get_Duration(),
                        UCk_Utils_FloatRange_UE::Make_FloatRange_0to1(1.0f));
                }

                const auto& FinalValueRef = InCurrent.Get_IsReversed() ? InParams.Get_StartValue() : InParams.Get_EndValue();
                return DoResolveValue(FinalValueRef, InParams.Get_Target());
            }();

            InCurrent.Set_CurrentValue(FinalValue);

            // ApplyToTransform excludes FTag_Tween_Completed, so the final value must be applied here.
            ck_tween::ApplyValueToTransform(InHandle, FinalValue, InParams.Get_Target());

            // Snap to the path end so the follower lands exactly on it.
            if (InHandle.Has<FFragment_Tween_SplineFollow>())
            {
                ck_tween::ApplyProgressToSplineFollow(
                    InHandle, InHandle.Get<FFragment_Tween_SplineFollow>(), FinalValue.GetAsFloat());
            }

            UUtils_Signal_OnTweenComplete::Broadcast(InHandle,
                MakePayload(InHandle, FCk_Tween_Payload_OnComplete{FinalValue}));

            DoStartNextTweenInQueue(InHandle);

            if (InParams.Get_CompletionBehavior() == ECk_TweenCompletionBehavior::SelfDestruct)
            {
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            }
            return;
        }

        InCurrent.Set_CurrentLoop(CurrentLoop);
        InCurrent.Set_CurrentTime(0.0f);

        UUtils_Signal_OnTweenLoop::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Tween_Payload_OnLoop{CurrentLoop}));

        switch (InParams.Get_LoopType())
        {
            case ECk_TweenLoopType::Restart:
            {
                InCurrent.Set_IsReversed(false);
                break;
            }
            case ECk_TweenLoopType::Yoyo:
            {
                InCurrent.Set_IsReversed(NOT InCurrent.Get_IsReversed());

                if (InParams.Get_YoyoDelay() > 0.0f)
                {
                    InHandle.Add<FTag_Tween_InYoyoDelay>();
                    InCurrent.Set_YoyoDelayTimer(InParams.Get_YoyoDelay());
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    auto
        FProcessor_Tween_Update::
        DoStartNextTweenInQueue(
            HandleType InHandle)
        -> void
    {
        if (NOT InHandle.Has<FFragment_Tween_Chain>())
        { return; }

        const auto& Chain = InHandle.Get<FFragment_Tween_Chain>();
        if (NOT Chain.Get_NextTween().IsSet())
        { return; }

        auto NextTween = Chain.Get_NextTween().GetValue();
        if (ck::Is_NOT_Valid(NextTween))
        { return; }

        UCk_Utils_Timer_UE::ForEach_Timer(NextTween, [](FCk_Handle_Timer Timer) {
            UCk_Utils_Timer_UE::Request_Resume(Timer, {});
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Tween_HandleYoyoDelays::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            FFragment_Tween_Current& InCurrent)
            -> void
    {
        const auto DeltaTime = InDeltaT.Get_Seconds() * InCurrent.Get_TimeMultiplier();

        const auto NewYoyoDelayTimer = InCurrent.Get_YoyoDelayTimer() - DeltaTime;
        InCurrent.Set_YoyoDelayTimer(FMath::Max(0.0f, NewYoyoDelayTimer));

        if (NewYoyoDelayTimer <= 0.0f)
        {
            InHandle.Remove<FTag_Tween_InYoyoDelay>();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Tween_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FFragment_Tween_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_Tween_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests.Get_Requests(), Visitor([&](const auto& InRequest)
            {
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                Result = DoHandleRequest(InHandle, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_Tween_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Pause& InRequest)
        -> ECk_Request_OperationResult
    {
        if (InCurrent.Get_State() != ECk_TweenState::Playing)
        {
            // Already paused: the caller's intent holds afterwards, so the no-op Succeeded. A
            // terminal tween can never become paused, so only that path genuinely Failed.
            return InCurrent.Get_State() == ECk_TweenState::Paused
                ? ECk_Request_OperationResult::Succeeded
                : ECk_Request_OperationResult::Failed;
        }

        InHandle.Remove<FTag_Tween_Playing>();
        InHandle.Add<FTag_Tween_Paused>();
        InCurrent.Set_State(ECk_TweenState::Paused);

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_Tween_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Resume& InRequest)
        -> ECk_Request_OperationResult
    {
        if (InCurrent.Get_State() != ECk_TweenState::Paused)
        {
            // Already playing: the caller's intent holds afterwards, so the no-op Succeeded. A
            // terminal tween can never resume, so only that path genuinely Failed.
            return InCurrent.Get_State() == ECk_TweenState::Playing
                ? ECk_Request_OperationResult::Succeeded
                : ECk_Request_OperationResult::Failed;
        }

        InHandle.Remove<FTag_Tween_Paused>();
        InHandle.Add<FTag_Tween_Playing>();
        InCurrent.Set_State(ECk_TweenState::Playing);

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_Tween_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Stop& InRequest)
        -> ECk_Request_OperationResult
    {
        // A tween already in a terminal state must be left untouched: re-running this body would
        // bare-Add an already-present FTag_Tween_Completed (ensure) and re-broadcast OnTweenComplete.
        // Reachable without misuse — requests are deferred, so a tween can finish between the call
        // and this handler. The tween is stopped either way, so the caller's intent holds and this
        // no-op reports Succeeded.
        if (InCurrent.Get_State() != ECk_TweenState::Playing &&
            InCurrent.Get_State() != ECk_TweenState::Paused)
        { return ECk_Request_OperationResult::Succeeded; }

        InHandle.Try_Remove<FTag_Tween_Playing>();
        InHandle.Try_Remove<FTag_Tween_Paused>();
        InHandle.Try_Remove<FTag_Tween_InYoyoDelay>();

        InHandle.Add<FTag_Tween_Completed>();
        InCurrent.Set_State(ECk_TweenState::Cancelled);

        UUtils_Signal_OnTweenComplete::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Tween_Payload_OnComplete{InCurrent.Get_CurrentValue()}));

        if (InRequest.Get_Behavior() == ECk_TweenStopBehavior::SelfDestruct)
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
        }

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_Tween_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Restart& InRequest)
        -> ECk_Request_OperationResult
    {
        // Sampled BEFORE the reset below overwrites it: a Restart arriving on an already-playing
        // curve-offset tween must NOT re-read the base. That is the spammable re-trigger case (hold
        // to shake), and re-reading mid-flight would capture the half-shaken pose as the new rest,
        // so every re-trigger would walk the prop a little further from where it started.
        const auto WasPlaying = InCurrent.Get_State() == ECk_TweenState::Playing;

        InCurrent.Set_CurrentTime(0.0f);
        InCurrent.Set_YoyoDelayTimer(0.0f);
        InCurrent.Set_State(ECk_TweenState::Playing);
        InCurrent.Set_CurrentLoop(0);
        InCurrent.Set_IsReversed(false);

        // Restart is valid from ANY prior state, including Playing, so every state tag is cleared
        // first — otherwise the bare Add below ensures.
        InHandle.Try_Remove<FTag_Tween_Playing>();
        InHandle.Try_Remove<FTag_Tween_Paused>();
        InHandle.Try_Remove<FTag_Tween_Completed>();
        InHandle.Try_Remove<FTag_Tween_InYoyoDelay>();

        InHandle.Add<FTag_Tween_Playing>();

        if (NOT WasPlaying)
        { DoRecaptureCurveBase(InHandle); }

        return ECk_Request_OperationResult::Succeeded;
    }

    // A prop can be picked up and re-placed between shakes, so a tween restarted from rest re-reads
    // the pose it should wobble about. Guarded by the caller -- see the Restart handler.
    auto
        FProcessor_Tween_HandleRequests::
        DoRecaptureCurveBase(
            HandleType InHandle)
        -> void
    {
        if (NOT InHandle.Has<FFragment_Tween_CurveDrive>())
        { return; }

        auto& CurveDrive = InHandle.AddOrGet<FFragment_Tween_CurveDrive>();

        if (CurveDrive.Get_Output() == ECk_TweenCurveOutput::Float)
        { return; }

        const auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        if (ck::Is_NOT_Valid(TargetEntity))
        { return; }

        auto MaybeTransformHandle = UCk_Utils_Transform_UE::Cast(TargetEntity);
        if (ck::Is_NOT_Valid(MaybeTransformHandle))
        { return; }

        switch (CurveDrive.Get_Output())
        {
            case ECk_TweenCurveOutput::VectorOffset:
            {
                CurveDrive._BaseValue = FCk_TweenValue{UCk_Utils_Transform_UE::Get_EntityCurrentLocation(MaybeTransformHandle)};
                break;
            }
            case ECk_TweenCurveOutput::RotatorOffset:
            {
                CurveDrive._BaseValue = FCk_TweenValue{UCk_Utils_Transform_UE::Get_EntityCurrentRotation(MaybeTransformHandle)};
                break;
            }
            default:
            {
                break;
            }
        }
    }

    auto
        FProcessor_Tween_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_SetTimeMultiplier& InRequest)
        -> ECk_Request_OperationResult
    {
        InCurrent.Set_TimeMultiplier(FMath::Max(0.0f, InRequest.Get_Multiplier()));

        return ECk_Request_OperationResult::Succeeded;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Tween_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Tween_ApplyToTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Params& InParams,
            const FFragment_Tween_Current& InCurrent)
            -> void
    {
        ck_tween::ApplyValueToTransform(InHandle, InCurrent.Get_CurrentValue(), InParams.Get_Target());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Tween_ApplySplineFollow::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Tween_Current& InCurrent,
            const FFragment_Tween_SplineFollow& InSplineFollow)
            -> void
    {
        const auto Progress = InCurrent.Get_CurrentValue().GetAsFloat();
        ck_tween::ApplyProgressToSplineFollow(InHandle, InSplineFollow, Progress);
    }
}

// --------------------------------------------------------------------------------------------------------------------