#include "CkTween_Utils.h"

#include "CkCore/Math/ValueRange/CkValueRange_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkTimer/CkTimer_Utils.h"
#include "CkTween/CkTween_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Tween_UE::
    Create_TweenFloat(
        FCk_Handle& InOwner,
        float InStartValue,
        float InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing,
        ECk_TweenLoopType InLoopType,
        int32 InLoopCount,
        float InYoyoDelay)
    -> FCk_Handle_Tween
{
    return DoCreateTween(InOwner, FCk_TweenValue{InStartValue}, FCk_TweenValue{InEndValue},
        InDuration, InEasing, InLoopType, InLoopCount, InYoyoDelay, ECk_TweenTarget::Custom);
}

auto
    UCk_Utils_Tween_UE::
    Create_TweenVector(
        FCk_Handle& InOwner,
        FVector InStartValue,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing,
        ECk_TweenLoopType InLoopType,
        int32 InLoopCount,
        float InYoyoDelay)
    -> FCk_Handle_Tween
{
    return DoCreateTween(InOwner, FCk_TweenValue{InStartValue}, FCk_TweenValue{InEndValue},
        InDuration, InEasing, InLoopType, InLoopCount, InYoyoDelay, ECk_TweenTarget::Custom);
}

auto
    UCk_Utils_Tween_UE::
    Create_TweenRotator(
        FCk_Handle& InOwner,
        FRotator InStartValue,
        FRotator InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing,
        ECk_TweenLoopType InLoopType,
        int32 InLoopCount,
        float InYoyoDelay)
    -> FCk_Handle_Tween
{
    return DoCreateTween(InOwner, FCk_TweenValue{InStartValue}, FCk_TweenValue{InEndValue},
        InDuration, InEasing, InLoopType, InLoopCount, InYoyoDelay, ECk_TweenTarget::Custom);
}

auto
    UCk_Utils_Tween_UE::
    Create_TweenLinearColor(
        FCk_Handle& InOwner,
        FLinearColor InStartValue,
        FLinearColor InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing,
        ECk_TweenLoopType InLoopType,
        int32 InLoopCount,
        float InYoyoDelay)
    -> FCk_Handle_Tween
{
    return DoCreateTween(InOwner, FCk_TweenValue{InStartValue}, FCk_TweenValue{InEndValue},
        InDuration, InEasing, InLoopType, InLoopCount, InYoyoDelay, ECk_TweenTarget::Custom);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Tween_UE::
    Create_TweenEntityLocation(
        FCk_Handle_Transform& InEntity,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing,
        ECk_TweenLoopType InLoopType,
        int32 InLoopCount,
        float InYoyoDelay)
    -> FCk_Handle_Tween
{
    const auto StartValue = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InEntity).GetLocation();
    return DoCreateTween(InEntity, FCk_TweenValue{StartValue}, FCk_TweenValue{InEndValue},
        InDuration, InEasing, InLoopType, InLoopCount, InYoyoDelay, ECk_TweenTarget::Transform_Location);
}

auto
    UCk_Utils_Tween_UE::
    Create_TweenEntityRotation(
        FCk_Handle_Transform& InEntity,
        FRotator InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing,
        ECk_TweenLoopType InLoopType,
        int32 InLoopCount,
        float InYoyoDelay)
    -> FCk_Handle_Tween
{
    const auto StartValue = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InEntity).GetRotation().Rotator();
    return DoCreateTween(InEntity, FCk_TweenValue{StartValue}, FCk_TweenValue{InEndValue},
        InDuration, InEasing, InLoopType, InLoopCount, InYoyoDelay, ECk_TweenTarget::Transform_Rotation);
}

auto
    UCk_Utils_Tween_UE::
    Create_TweenEntityScale(
        FCk_Handle_Transform& InEntity,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing,
        ECk_TweenLoopType InLoopType,
        int32 InLoopCount,
        float InYoyoDelay)
    -> FCk_Handle_Tween
{
    const auto StartValue = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InEntity).GetScale3D();
    return DoCreateTween(InEntity, FCk_TweenValue{StartValue}, FCk_TweenValue{InEndValue},
        InDuration, InEasing, InLoopType, InLoopCount, InYoyoDelay, ECk_TweenTarget::Transform_Scale);
}

auto
    UCk_Utils_Tween_UE::
    Create_TweenEntityTransform(
        FCk_Handle_Transform& InEntity,
        FTransform InEndTransform,
        float InDuration,
        ECk_TweenEasing InEasing,
        ECk_TweenLoopType InLoopType,
        int32 InLoopCount,
        float InYoyoDelay)
    -> FCk_TweenTransformResult
{
    const auto LocationTween = Create_TweenEntityLocation(InEntity, InEndTransform.GetLocation(), InDuration, InEasing, InLoopType, InLoopCount, InYoyoDelay);
    const auto RotationTween = Create_TweenEntityRotation(InEntity, InEndTransform.GetRotation().Rotator(), InDuration, InEasing, InLoopType, InLoopCount, InYoyoDelay);
    const auto ScaleTween = Create_TweenEntityScale(InEntity, InEndTransform.GetScale3D(), InDuration, InEasing, InLoopType, InLoopCount, InYoyoDelay);

    return FCk_TweenTransformResult{LocationTween, RotationTween, ScaleTween};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Tween_UE::
    ChainTween(
        FCk_Handle_Tween& InNextTween,
        FCk_Handle_Tween& InFirstTween,
        float InDelay)
    -> FCk_Handle_Tween
{
    if (ck::Is_NOT_Valid(InFirstTween) || ck::Is_NOT_Valid(InNextTween))
    { return InFirstTween; }

    // Find the last tween in the chain starting from InFirstTween
    auto CurrentTween = InFirstTween;
    auto LastTween = InFirstTween;

    while (ck::IsValid(CurrentTween))
    {
        if (NOT CurrentTween.Has<ck::FFragment_Tween_Chain>())
        { break; }

        const auto& Chain = CurrentTween.Get<ck::FFragment_Tween_Chain>();
        if (NOT Chain.Get_NextTween().IsSet())
        { break; }

        LastTween = CurrentTween;
        CurrentTween = Chain.Get_NextTween().GetValue();
    }

    // Add InNextTween to the end of the chain with optional delay
    DoChainWithDelay(LastTween, InNextTween, InDelay);

    return InFirstTween;
}

auto
    UCk_Utils_Tween_UE::
    ChainTween_Transform(
        FCk_TweenTransformResult& InFirstTransformTween,
        FCk_TweenTransformResult& InNextTransformTween,
        float InDelay)
    -> FCk_TweenTransformResult
{
    // Chain each component
    auto LocationTween = InFirstTransformTween.Get_LocationTween();
    auto RotationTween = InFirstTransformTween.Get_RotationTween();
    auto ScaleTween = InFirstTransformTween.Get_ScaleTween();

    auto NextLocationTween = InNextTransformTween.Get_LocationTween();
    auto NextRotationTween = InNextTransformTween.Get_RotationTween();
    auto NextScaleTween = InNextTransformTween.Get_ScaleTween();

    ChainTween(LocationTween, NextLocationTween, InDelay);
    ChainTween(RotationTween, NextRotationTween, InDelay);
    ChainTween(ScaleTween, NextScaleTween, InDelay);

    return InFirstTransformTween;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Tween_UE::
    Pause(
        FCk_Handle_Tween& InTween)
    -> FCk_Handle_Tween
{
    return DoAddRequestToTween(InTween, FCk_Request_Tween_Pause{});
}

auto
    UCk_Utils_Tween_UE::
    Resume(
        FCk_Handle_Tween& InTween)
    -> FCk_Handle_Tween
{
    return DoAddRequestToTween(InTween, FCk_Request_Tween_Resume{});
}

auto
    UCk_Utils_Tween_UE::
    Stop(
        FCk_Handle_Tween& InTween)
    -> FCk_Handle_Tween
{
    return DoAddRequestToTween(InTween, FCk_Request_Tween_Stop{});
}

auto
    UCk_Utils_Tween_UE::
    Restart(
        FCk_Handle_Tween& InTween)
    -> FCk_Handle_Tween
{
    return DoAddRequestToTween(InTween, FCk_Request_Tween_Restart{});
}

auto
    UCk_Utils_Tween_UE::
    SetTimeMultiplier(
        FCk_Handle_Tween& InTween,
        float InMultiplier)
    -> FCk_Handle_Tween
{
    return DoAddRequestToTween(InTween, FCk_Request_Tween_SetTimeMultiplier{InMultiplier});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Tween_UE::
    Pause_TransformTween(
        FCk_TweenTransformResult& InTransformTween)
    -> FCk_TweenTransformResult
{
    auto LocationTween = InTransformTween.Get_LocationTween();
    auto RotationTween = InTransformTween.Get_RotationTween();
    auto ScaleTween = InTransformTween.Get_ScaleTween();

    Pause(LocationTween);
    Pause(RotationTween);
    Pause(ScaleTween);

    return InTransformTween;
}

auto
    UCk_Utils_Tween_UE::
    Resume_TransformTween(
        FCk_TweenTransformResult& InTransformTween)
    -> FCk_TweenTransformResult
{
    auto LocationTween = InTransformTween.Get_LocationTween();
    auto RotationTween = InTransformTween.Get_RotationTween();
    auto ScaleTween = InTransformTween.Get_ScaleTween();

    Resume(LocationTween);
    Resume(RotationTween);
    Resume(ScaleTween);

    return InTransformTween;
}

auto
    UCk_Utils_Tween_UE::
    Stop_TransformTween(
        FCk_TweenTransformResult& InTransformTween)
    -> FCk_TweenTransformResult
{
    auto LocationTween = InTransformTween.Get_LocationTween();
    auto RotationTween = InTransformTween.Get_RotationTween();
    auto ScaleTween = InTransformTween.Get_ScaleTween();

    Stop(LocationTween);
    Stop(RotationTween);
    Stop(ScaleTween);

    return InTransformTween;
}

auto
    UCk_Utils_Tween_UE::
    Restart_TransformTween(
        FCk_TweenTransformResult& InTransformTween)
    -> FCk_TweenTransformResult
{
    auto LocationTween = InTransformTween.Get_LocationTween();
    auto RotationTween = InTransformTween.Get_RotationTween();
    auto ScaleTween = InTransformTween.Get_ScaleTween();

    Restart(LocationTween);
    Restart(RotationTween);
    Restart(ScaleTween);

    return InTransformTween;
}

auto
    UCk_Utils_Tween_UE::
    SetTimeMultiplier_TransformTween(
        FCk_TweenTransformResult& InTransformTween,
        float InMultiplier)
    -> FCk_TweenTransformResult
{
    auto LocationTween = InTransformTween.Get_LocationTween();
    auto RotationTween = InTransformTween.Get_RotationTween();
    auto ScaleTween = InTransformTween.Get_ScaleTween();

    SetTimeMultiplier(LocationTween, InMultiplier);
    SetTimeMultiplier(RotationTween, InMultiplier);
    SetTimeMultiplier(ScaleTween, InMultiplier);

    return InTransformTween;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Tween_UE, FCk_Handle_Tween,
    ck::FFragment_Tween_Params, ck::FFragment_Tween_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Tween_UE::
    Get_State(
        const FCk_Handle_Tween& InTween)
    -> ECk_TweenState
{
    return InTween.Get<ck::FFragment_Tween_Current>().Get_State();
}

auto
    UCk_Utils_Tween_UE::
    Get_Progress(
        const FCk_Handle_Tween& InTween)
    -> FCk_FloatRange_0to1
{
    const auto& Params = InTween.Get<ck::FFragment_Tween_Params>();
    const auto& Current = InTween.Get<ck::FFragment_Tween_Current>();

    if (Params.Get_Duration() <= 0.0f)
    { return UCk_Utils_FloatRange_0to1_UE::Make_FloatRange_0to1(1.0f); }

    const auto Progress = FMath::Clamp(Current.Get_CurrentTime() / Params.Get_Duration(), 0.0f, 1.0f);
    return UCk_Utils_FloatRange_0to1_UE::Make_FloatRange_0to1(Progress);
}

auto
    UCk_Utils_Tween_UE::
    Get_CurrentValue(
        const FCk_Handle_Tween& InTween)
    -> FCk_TweenValue
{
    return InTween.Get<ck::FFragment_Tween_Current>().Get_CurrentValue();
}

auto
    UCk_Utils_Tween_UE::
    Get_CurrentLoop(
        const FCk_Handle_Tween& InTween)
    -> int32
{
    return InTween.Get<ck::FFragment_Tween_Current>().Get_CurrentLoop();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Tween_UE::
    TweenValue_IsFloat(
        const FCk_TweenValue& InValue)
    -> bool
{
    return InValue.IsFloat();
}

auto
    UCk_Utils_Tween_UE::
    TweenValue_IsVector(
        const FCk_TweenValue& InValue)
    -> bool
{
    return InValue.IsVector();
}

auto
    UCk_Utils_Tween_UE::
    TweenValue_IsRotator(
        const FCk_TweenValue& InValue)
    -> bool
{
    return InValue.IsRotator();
}

auto
    UCk_Utils_Tween_UE::
    TweenValue_IsLinearColor(
        const FCk_TweenValue& InValue)
    -> bool
{
    return InValue.IsLinearColor();
}

auto
    UCk_Utils_Tween_UE::
    TweenValue_GetAsFloat(
        const FCk_TweenValue& InValue)
    -> float
{
    return InValue.GetAsFloat();
}

auto
    UCk_Utils_Tween_UE::
    TweenValue_GetAsVector(
        const FCk_TweenValue& InValue)
    -> FVector
{
    return InValue.GetAsVector();
}

auto
    UCk_Utils_Tween_UE::
    TweenValue_GetAsRotator(
        const FCk_TweenValue& InValue)
    -> FRotator
{
    return InValue.GetAsRotator();
}

auto
    UCk_Utils_Tween_UE::
    TweenValue_GetAsLinearColor(
        const FCk_TweenValue& InValue)
    -> FLinearColor
{
    return InValue.GetAsLinearColor();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Tween_UE::
    BindTo_OnUpdate(
        FCk_Handle_Tween& InTween,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Tween_OnUpdate& InDelegate)
    -> FCk_Handle_Tween
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnTweenUpdate, InTween, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTween;
}

auto
    UCk_Utils_Tween_UE::
    UnbindFrom_OnUpdate(
        FCk_Handle_Tween& InTween,
        const FCk_Delegate_Tween_OnUpdate& InDelegate)
    -> FCk_Handle_Tween
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnTweenUpdate, InTween, InDelegate);
    return InTween;
}

auto
    UCk_Utils_Tween_UE::
    BindTo_OnComplete(
        FCk_Handle_Tween& InTween,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Tween_OnComplete& InDelegate)
    -> FCk_Handle_Tween
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnTweenComplete, InTween, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTween;
}

auto
    UCk_Utils_Tween_UE::
    UnbindFrom_OnComplete(
        FCk_Handle_Tween& InTween,
        const FCk_Delegate_Tween_OnComplete& InDelegate)
    -> FCk_Handle_Tween
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnTweenComplete, InTween, InDelegate);
    return InTween;
}

auto
    UCk_Utils_Tween_UE::
    BindTo_OnLoop(
        FCk_Handle_Tween& InTween,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Tween_OnLoop& InDelegate)
    -> FCk_Handle_Tween
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnTweenLoop, InTween, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTween;
}

auto UCk_Utils_Tween_UE::UnbindFrom_OnLoop(
    FCk_Handle_Tween& InTween,
    const FCk_Delegate_Tween_OnLoop& InDelegate) -> FCk_Handle_Tween
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnTweenLoop, InTween, InDelegate);
    return InTween;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Tween_UE::
    OnTimerDone(
        FCk_Handle_Timer InTimer,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT)
    -> void
{
    auto TweenEntity = Cast(InTimer);
    Resume(TweenEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Tween_UE::
    DoCreateTween(
        FCk_Handle& InOwner,
        const FCk_TweenValue& InStartValue,
        const FCk_TweenValue& InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing,
        ECk_TweenLoopType InLoopType,
        int32 InLoopCount,
        float InYoyoDelay,
        ECk_TweenTarget InTarget)
    -> FCk_Handle_Tween
{
    auto TweenEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);

    auto Params = FCk_Fragment_Tween_ParamsData{InStartValue, InEndValue, InDuration, InEasing}
        .Set_LoopType(InLoopType)
        .Set_LoopCount(InLoopCount)
        .Set_YoyoDelay(InYoyoDelay)
        .Set_Target(InTarget);

    TweenEntity.Add<ck::FTag_Tween_Playing>();
    TweenEntity.Add<ck::FFragment_Tween_Params>(Params);
    TweenEntity.Add<ck::FFragment_Tween_Current>(InStartValue);

    return Cast(TweenEntity);
}

auto
    UCk_Utils_Tween_UE::
    DoAddRequestToTween(
        FCk_Handle_Tween& InTween,
        const auto& InRequest)
    -> FCk_Handle_Tween
{
    InTween.AddOrGet<ck::FFragment_Tween_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InTween;
}

auto
    UCk_Utils_Tween_UE::
    DoChainWithDelay(
        FCk_Handle_Tween& InFirstTween,
        FCk_Handle_Tween& InNextTween,
        float InDelay)
    -> void
{
    if (InDelay <= 0.0f)
    {
        // No delay - direct chain
        InFirstTween.AddOrGet<ck::FFragment_Tween_Chain>().Set_NextTween(InNextTween);
        return;
    }

    // Create Timer for delay
    // Pause the next tween initially
    DoAddRequestToTween(InNextTween, FCk_Request_Tween_Pause{});

    // Create Timer with the delay duration
    const auto TimerParams = FCk_Fragment_Timer_ParamsData{FCk_Time{InDelay}}
        .Set_StartingState(ECk_Timer_State::Paused); // Start paused, will be resumed when first tween completes

    auto DelayTimer = UCk_Utils_Timer_UE::Add(InNextTween, TimerParams);

    // Bind Timer completion to resume the next tween
    ck::UUtils_Signal_OnTimerDone::Bind<&ThisType::OnTimerDone>(
        DelayTimer, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame, ECk_Signal_PostFireBehavior::Unbind);

    // Set up the chain
    InFirstTween.AddOrGet<ck::FFragment_Tween_Chain>().Set_NextTween(InNextTween);
}

// --------------------------------------------------------------------------------------------------------------------
