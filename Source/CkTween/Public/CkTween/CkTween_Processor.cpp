#include "CkTween_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/ValueRange/CkValueRange_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkTimer/CkTimer_Utils.h"
#include "CkTween/CkTween_Easing_Utils.h"
#include "CkTween/CkTween_Utils.h"

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

        const auto& StartValueRef = InCurrent.Get_IsReversed() ? InParams.Get_EndValue() : InParams.Get_StartValue();
        const auto& EndValueRef = InCurrent.Get_IsReversed() ? InParams.Get_StartValue() : InParams.Get_EndValue();

        const auto StartValue = DoResolveValue(StartValueRef, InParams.Get_Target());
        const auto EndValue = DoResolveValue(EndValueRef, InParams.Get_Target());

        const auto InterpolatedValue = UCk_Utils_TweenEasing_UE::Interpolate(StartValue, EndValue, Progress, InParams.Get_Easing());
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

            const auto& FinalValueRef = InCurrent.Get_IsReversed() ? InParams.Get_StartValue() : InParams.Get_EndValue();
            const auto FinalValue = DoResolveValue(FinalValueRef, InParams.Get_Target());

            InCurrent.Set_CurrentValue(FinalValue);

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
            UCk_Utils_Timer_UE::Request_Resume(Timer);
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
                DoHandleRequest(InHandle, InCurrent, InRequest);

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
        -> void
    {
        if (InCurrent.Get_State() != ECk_TweenState::Playing)
        { return; }

        InHandle.Remove<FTag_Tween_Playing>();
        InHandle.Add<FTag_Tween_Paused>();
        InCurrent.Set_State(ECk_TweenState::Paused);
    }

    auto
        FProcessor_Tween_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Resume& InRequest)
        -> void
    {
        if (InCurrent.Get_State() != ECk_TweenState::Paused)
        { return; }

        InHandle.Remove<FTag_Tween_Paused>();
        InHandle.Add<FTag_Tween_Playing>();
        InCurrent.Set_State(ECk_TweenState::Playing);
    }

    auto
        FProcessor_Tween_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Stop& InRequest)
        -> void
    {
        if (InCurrent.Get_State() == ECk_TweenState::Cancelled)
        { return; }

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
    }

    auto
        FProcessor_Tween_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_Restart& InRequest)
        -> void
    {
        InCurrent.Set_CurrentTime(0.0f);
        InCurrent.Set_YoyoDelayTimer(0.0f);
        InCurrent.Set_State(ECk_TweenState::Playing);
        InCurrent.Set_CurrentLoop(0);
        InCurrent.Set_IsReversed(false);

        InHandle.Try_Remove<FTag_Tween_Paused>();
        InHandle.Try_Remove<FTag_Tween_Completed>();
        InHandle.Try_Remove<FTag_Tween_InYoyoDelay>();

        InHandle.Add<FTag_Tween_Playing>();
    }

    auto
        FProcessor_Tween_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Tween_Current& InCurrent,
            const FCk_Request_Tween_SetTimeMultiplier& InRequest)
        -> void
    {
        InCurrent.Set_TimeMultiplier(FMath::Max(0.0f, InRequest.Get_Multiplier()));
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
        const auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        if (ck::Is_NOT_Valid(TargetEntity))
        { return; }

        DoApplyValueToTransform(TargetEntity, InCurrent.Get_CurrentValue(), InParams.Get_Target());
    }

    auto
        FProcessor_Tween_ApplyToTransform::
        DoApplyValueToTransform(
            const FCk_Handle& InTargetEntity,
            const FCk_TweenValue& InValue,
            ECk_TweenTarget InTarget)
        -> void
    {
        auto MaybeTransformHandle = UCk_Utils_Transform_UE::Cast(InTargetEntity);

        if (ck::Is_NOT_Valid(MaybeTransformHandle))
        { return; }

        switch (InTarget)
        {
            case ECk_TweenTarget::Transform_Location:
            {
                if (InValue.IsVector())
                {
                    UCk_Utils_Transform_UE::Request_SetLocation(MaybeTransformHandle, FCk_Request_Transform_SetLocation{InValue.GetAsVector()});
                }
                break;
            }
            case ECk_TweenTarget::Transform_Rotation:
            {
                if (InValue.IsRotator())
                {
                    UCk_Utils_Transform_UE::Request_SetRotation(MaybeTransformHandle, FCk_Request_Transform_SetRotation{InValue.GetAsRotator()});
                }
                break;
            }
            case ECk_TweenTarget::Transform_Scale:
            {
                if (InValue.IsVector())
                {
                    UCk_Utils_Transform_UE::Request_SetScale(MaybeTransformHandle, FCk_Request_Transform_SetScale{InValue.GetAsVector()});
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
}

// --------------------------------------------------------------------------------------------------------------------