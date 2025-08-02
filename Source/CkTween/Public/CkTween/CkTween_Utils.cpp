#include "CkTween_Utils.h"

#include "CkCore/Math/ValueRange/CkValueRange_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkTween/CkTween_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Tween_UE::Create_TweenFloat(
    FCk_Handle& InOwner,
    float InStartValue,
    float InEndValue,
    float InDuration,
    ECk_TweenEasing InEasing,
    float InDelay) -> FCk_Handle_Tween
{
    return DoCreateTween(InOwner, FCk_TweenValue{InStartValue}, FCk_TweenValue{InEndValue}, InDuration, InEasing, InDelay);
}

auto UCk_Utils_Tween_UE::Create_TweenVector(
    FCk_Handle& InOwner,
    FVector InStartValue,
    FVector InEndValue,
    float InDuration,
    ECk_TweenEasing InEasing,
    float InDelay) -> FCk_Handle_Tween
{
    return DoCreateTween(InOwner, FCk_TweenValue{InStartValue}, FCk_TweenValue{InEndValue}, InDuration, InEasing, InDelay);
}

auto UCk_Utils_Tween_UE::Create_TweenRotator(
    FCk_Handle& InOwner,
    FRotator InStartValue,
    FRotator InEndValue,
    float InDuration,
    ECk_TweenEasing InEasing,
    float InDelay) -> FCk_Handle_Tween
{
    return DoCreateTween(InOwner, FCk_TweenValue{InStartValue}, FCk_TweenValue{InEndValue}, InDuration, InEasing, InDelay);
}

auto UCk_Utils_Tween_UE::Create_TweenLinearColor(
    FCk_Handle& InOwner,
    FLinearColor InStartValue,
    FLinearColor InEndValue,
    float InDuration,
    ECk_TweenEasing InEasing,
    float InDelay) -> FCk_Handle_Tween
{
    return DoCreateTween(InOwner, FCk_TweenValue{InStartValue}, FCk_TweenValue{InEndValue}, InDuration, InEasing, InDelay);
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Tween_UE::Create_TweenEntityLocation(
    FCk_Handle_Transform& InEntity,
    FVector InEndValue,
    float InDuration,
    ECk_TweenEasing InEasing,
    float InDelay) -> FCk_Handle_Tween
{
    const auto StartValue = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InEntity).GetLocation();
    return DoCreateTween(InEntity, FCk_TweenValue{StartValue}, FCk_TweenValue{InEndValue}, InDuration, InEasing, InDelay, TAG_Tween_Location);
}

auto UCk_Utils_Tween_UE::Create_TweenEntityLocationAdd(
    FCk_Handle_Transform& InEntity,
    FVector InOffsetValue,
    float InDuration,
    ECk_TweenEasing InEasing,
    float InDelay) -> FCk_Handle_Tween
{
    const auto StartValue = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InEntity).GetLocation();
    const auto EndValue = StartValue + InOffsetValue;
    return DoCreateTween(InEntity, FCk_TweenValue{StartValue}, FCk_TweenValue{EndValue}, InDuration, InEasing, InDelay, TAG_Tween_Location);
}

auto UCk_Utils_Tween_UE::Create_TweenEntityRotation(
    FCk_Handle_Transform& InEntity,
    FRotator InEndValue,
    float InDuration,
    ECk_TweenEasing InEasing,
    float InDelay) -> FCk_Handle_Tween
{
    const auto StartValue = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InEntity).GetRotation().Rotator();
    return DoCreateTween(InEntity, FCk_TweenValue{StartValue}, FCk_TweenValue{InEndValue}, InDuration, InEasing, InDelay, TAG_Tween_Rotation);
}

auto UCk_Utils_Tween_UE::Create_TweenEntityScale(
    FCk_Handle_Transform& InEntity,
    FVector InEndValue,
    float InDuration,
    ECk_TweenEasing InEasing,
    float InDelay) -> FCk_Handle_Tween
{
    const auto StartValue = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InEntity).GetScale3D();
    return DoCreateTween(InEntity, FCk_TweenValue{StartValue}, FCk_TweenValue{InEndValue}, InDuration, InEasing, InDelay, TAG_Tween_Scale);
}

auto UCk_Utils_Tween_UE::Create_TweenEntityTransform(
    FCk_Handle_Transform& InEntity,
    FTransform InEndTransform,
    float InDuration,
    ECk_TweenEasing InEasing,
    float InDelay) -> FCk_TweenTransformResult
{
    const auto LocationTween = Create_TweenEntityLocation(InEntity, InEndTransform.GetLocation(), InDuration, InEasing, InDelay);
    const auto RotationTween = Create_TweenEntityRotation(InEntity, InEndTransform.GetRotation().Rotator(), InDuration, InEasing, InDelay);
    const auto ScaleTween = Create_TweenEntityScale(InEntity, InEndTransform.GetScale3D(), InDuration, InEasing, InDelay);

    return FCk_TweenTransformResult{LocationTween, RotationTween, ScaleTween};
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Tween_UE::ChainTween(
    FCk_Handle_Tween& InFirstTween,
    FCk_Handle_Tween& InNextTween) -> FCk_Handle_Tween
{
    if (ck::Is_NOT_Valid(InFirstTween) || ck::Is_NOT_Valid(InNextTween))
    { return InFirstTween; }

    // Find the last tween in the chain starting from InFirstTween
    auto CurrentTween = InFirstTween;
    auto LastTween = InFirstTween;

    while (ck::IsValid(CurrentTween))
    {
        const auto& Params = CurrentTween.Get<ck::FFragment_Tween_Params>();
        if (NOT Params.Get_NextTween().IsSet())
        { break; }

        LastTween = CurrentTween;
        CurrentTween = Params.Get_NextTween().GetValue();
    }

    // Add InNextTween to the end of the chain
    LastTween.Get<ck::FFragment_Tween_Params>().Set_NextTween(InNextTween);

    return InFirstTween;
}

auto UCk_Utils_Tween_UE::ChainTween_Transform(
    FCk_TweenTransformResult& InFirstTransformTween,
    FCk_TweenTransformResult& InNextTransformTween) -> FCk_TweenTransformResult
{
    // Chain each component
    auto LocationTween = InFirstTransformTween.Get_LocationTween();
    auto RotationTween = InFirstTransformTween.Get_RotationTween();
    auto ScaleTween = InFirstTransformTween.Get_ScaleTween();

    auto NextLocationTween = InNextTransformTween.Get_LocationTween();
    auto NextRotationTween = InNextTransformTween.Get_RotationTween();
    auto NextScaleTween = InNextTransformTween.Get_ScaleTween();

    ChainTween(LocationTween, NextLocationTween);
    ChainTween(RotationTween, NextRotationTween);
    ChainTween(ScaleTween, NextScaleTween);

    return InFirstTransformTween;
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Tween_UE::Pause(FCk_Handle_Tween& InTween) -> FCk_Handle_Tween
{
    return DoAddRequestToTween(InTween, FCk_Request_Tween_Pause{});
}

auto UCk_Utils_Tween_UE::Resume(FCk_Handle_Tween& InTween) -> FCk_Handle_Tween
{
    return DoAddRequestToTween(InTween, FCk_Request_Tween_Resume{});
}

auto UCk_Utils_Tween_UE::Stop(FCk_Handle_Tween& InTween) -> FCk_Handle_Tween
{
    return DoAddRequestToTween(InTween, FCk_Request_Tween_Stop{});
}

auto UCk_Utils_Tween_UE::Restart(FCk_Handle_Tween& InTween) -> FCk_Handle_Tween
{
    return DoAddRequestToTween(InTween, FCk_Request_Tween_Restart{});
}

auto UCk_Utils_Tween_UE::SetTimeMultiplier(FCk_Handle_Tween& InTween, float InMultiplier) -> FCk_Handle_Tween
{
    return DoAddRequestToTween(InTween, FCk_Request_Tween_SetTimeMultiplier{InMultiplier});
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Tween_UE::Pause_TransformTween(FCk_TweenTransformResult& InTransformTween) -> FCk_TweenTransformResult
{
    auto LocationTween = InTransformTween.Get_LocationTween();
    auto RotationTween = InTransformTween.Get_RotationTween();
    auto ScaleTween = InTransformTween.Get_ScaleTween();

    Pause(LocationTween);
    Pause(RotationTween);
    Pause(ScaleTween);

    return InTransformTween;
}

auto UCk_Utils_Tween_UE::Resume_TransformTween(FCk_TweenTransformResult& InTransformTween) -> FCk_TweenTransformResult
{
    auto LocationTween = InTransformTween.Get_LocationTween();
    auto RotationTween = InTransformTween.Get_RotationTween();
    auto ScaleTween = InTransformTween.Get_ScaleTween();

    Resume(LocationTween);
    Resume(RotationTween);
    Resume(ScaleTween);

    return InTransformTween;
}

auto UCk_Utils_Tween_UE::Stop_TransformTween(FCk_TweenTransformResult& InTransformTween) -> FCk_TweenTransformResult
{
    auto LocationTween = InTransformTween.Get_LocationTween();
    auto RotationTween = InTransformTween.Get_RotationTween();
    auto ScaleTween = InTransformTween.Get_ScaleTween();

    Stop(LocationTween);
    Stop(RotationTween);
    Stop(ScaleTween);

    return InTransformTween;
}

auto UCk_Utils_Tween_UE::Restart_TransformTween(FCk_TweenTransformResult& InTransformTween) -> FCk_TweenTransformResult
{
    auto LocationTween = InTransformTween.Get_LocationTween();
    auto RotationTween = InTransformTween.Get_RotationTween();
    auto ScaleTween = InTransformTween.Get_ScaleTween();

    Restart(LocationTween);
    Restart(RotationTween);
    Restart(ScaleTween);

    return InTransformTween;
}

auto UCk_Utils_Tween_UE::SetTimeMultiplier_TransformTween(FCk_TweenTransformResult& InTransformTween, float InMultiplier) -> FCk_TweenTransformResult
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

auto UCk_Utils_Tween_UE::Get_State(const FCk_Handle_Tween& InTween) -> ECk_TweenState
{
    return InTween.Get<ck::FFragment_Tween_Current>().Get_State();
}

auto UCk_Utils_Tween_UE::Get_Progress(const FCk_Handle_Tween& InTween) -> FCk_FloatRange_0to1
{
    const auto& Params = InTween.Get<ck::FFragment_Tween_Params>();
    const auto& Current = InTween.Get<ck::FFragment_Tween_Current>();

    if (Params.Get_Duration() <= 0.0f)
    { return UCk_Utils_FloatRange_0to1_UE::Make_FloatRange_0to1(1.0f); }

    const auto Progress = FMath::Clamp(Current.Get_CurrentTime() / Params.Get_Duration(), 0.0f, 1.0f);
    return UCk_Utils_FloatRange_0to1_UE::Make_FloatRange_0to1(Progress);
}

auto UCk_Utils_Tween_UE::Get_CurrentValue(const FCk_Handle_Tween& InTween) -> FCk_TweenValue
{
    return InTween.Get<ck::FFragment_Tween_Current>().Get_CurrentValue();
}

auto UCk_Utils_Tween_UE::Get_CurrentLoop(const FCk_Handle_Tween& InTween) -> int32
{
    return InTween.Get<ck::FFragment_Tween_Current>().Get_CurrentLoop();
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Tween_UE::TweenValue_IsFloat(const FCk_TweenValue& InValue) -> bool
{
    return InValue.IsFloat();
}

auto UCk_Utils_Tween_UE::TweenValue_IsVector(const FCk_TweenValue& InValue) -> bool
{
    return InValue.IsVector();
}

auto UCk_Utils_Tween_UE::TweenValue_IsRotator(const FCk_TweenValue& InValue) -> bool
{
    return InValue.IsRotator();
}

auto UCk_Utils_Tween_UE::TweenValue_IsLinearColor(const FCk_TweenValue& InValue) -> bool
{
    return InValue.IsLinearColor();
}

auto UCk_Utils_Tween_UE::TweenValue_GetAsFloat(const FCk_TweenValue& InValue) -> float
{
    return InValue.GetAsFloat();
}

auto UCk_Utils_Tween_UE::TweenValue_GetAsVector(const FCk_TweenValue& InValue) -> FVector
{
    return InValue.GetAsVector();
}

auto UCk_Utils_Tween_UE::TweenValue_GetAsRotator(const FCk_TweenValue& InValue) -> FRotator
{
    return InValue.GetAsRotator();
}

auto UCk_Utils_Tween_UE::TweenValue_GetAsLinearColor(const FCk_TweenValue& InValue) -> FLinearColor
{
    return InValue.GetAsLinearColor();
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Tween_UE::BindTo_OnUpdate(
    FCk_Handle_Tween& InTween,
    ECk_Signal_BindingPolicy InBindingPolicy,
    ECk_Signal_PostFireBehavior InPostFireBehavior,
    const FCk_Delegate_Tween_OnUpdate& InDelegate) -> FCk_Handle_Tween
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnTweenUpdate, InTween, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTween;
}

auto UCk_Utils_Tween_UE::UnbindFrom_OnUpdate(
    FCk_Handle_Tween& InTween,
    const FCk_Delegate_Tween_OnUpdate& InDelegate) -> FCk_Handle_Tween
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnTweenUpdate, InTween, InDelegate);
    return InTween;
}

auto UCk_Utils_Tween_UE::BindTo_OnComplete(
    FCk_Handle_Tween& InTween,
    ECk_Signal_BindingPolicy InBindingPolicy,
    ECk_Signal_PostFireBehavior InPostFireBehavior,
    const FCk_Delegate_Tween_OnComplete& InDelegate) -> FCk_Handle_Tween
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnTweenComplete, InTween, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTween;
}

auto UCk_Utils_Tween_UE::UnbindFrom_OnComplete(
    FCk_Handle_Tween& InTween,
    const FCk_Delegate_Tween_OnComplete& InDelegate) -> FCk_Handle_Tween
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnTweenComplete, InTween, InDelegate);
    return InTween;
}

auto UCk_Utils_Tween_UE::BindTo_OnLoop(
    FCk_Handle_Tween& InTween,
    ECk_Signal_BindingPolicy InBindingPolicy,
    ECk_Signal_PostFireBehavior InPostFireBehavior,
    const FCk_Delegate_Tween_OnLoop& InDelegate) -> FCk_Handle_Tween
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

auto UCk_Utils_Tween_UE::DoCreateTween(
    FCk_Handle& InOwner,
    const FCk_TweenValue& InStartValue,
    const FCk_TweenValue& InEndValue,
    float InDuration,
    ECk_TweenEasing InEasing,
    float InDelay,
    FGameplayTag InTweenName) -> FCk_Handle_Tween
{
    auto TweenEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);

    auto Params = FCk_Fragment_Tween_ParamsData{}
        .Set_StartValue(InStartValue)
        .Set_EndValue(InEndValue)
        .Set_Duration(InDuration)
        .Set_Easing(InEasing)
        .Set_Delay(InDelay)
        .Set_TweenName(InTweenName);

    TweenEntity.Add<ck::FFragment_Tween_Params>(Params);
    TweenEntity.Add<ck::FFragment_Tween_Current>();
    TweenEntity.Add<ck::FTag_Tween_NeedsSetup>();

    return Cast(TweenEntity);
}

auto UCk_Utils_Tween_UE::DoAddRequestToTween(FCk_Handle_Tween& InTween, const auto& InRequest) -> FCk_Handle_Tween
{
    InTween.AddOrGet<ck::FFragment_Tween_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InTween;
}

// --------------------------------------------------------------------------------------------------------------------
