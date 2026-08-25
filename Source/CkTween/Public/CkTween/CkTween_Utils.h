#pragma once

#include "CkTween_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Fragment_Data.h"

#include "CkTimer/CkTimer_Fragment_Data.h"

#include "CkSpline/CkSpline_Fragment_Data.h"

#include <Curves/CurveFloat.h>

#include "CkTween_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Handle_Transform;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Tween"))
class CKTWEEN_API UCk_Utils_Tween_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Tween_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Tween);

public:
    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween (Float)")
    static FCk_Handle_Tween
    Create_TweenFloat(
        UPARAM(ref) FCk_Handle& InOwner,
        float InStartValue,
        float InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween (Vector)")
    static FCk_Handle_Tween
    Create_TweenVector(
        UPARAM(ref) FCk_Handle& InOwner,
        FVector InStartValue,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween (Rotator)")
    static FCk_Handle_Tween
    Create_TweenRotator(
        UPARAM(ref) FCk_Handle& InOwner,
        FRotator InStartValue,
        FRotator InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Linear (Color)")
    static FCk_Handle_Tween
    Create_TweenLinearColor(
        UPARAM(ref) FCk_Handle& InOwner,
        FLinearColor InStartValue,
        FLinearColor InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity (Location)")
    static FCk_Handle_Tween
    Create_TweenEntityLocation(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity (Rotation)")
    static FCk_Handle_Tween
    Create_TweenEntityRotation(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        FRotator InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity (Scale)")
    static FCk_Handle_Tween
    Create_TweenEntityScale(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity (Transform)")
    static FCk_TweenTransformResult
    Create_TweenEntityTransform(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        FTransform InEndTransform,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    // Scene nodes driven by a parent own their world transform. These variants tween the relative
    // offset instead, so the scene-node processor remains the sole world-transform writer.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween SceneNode Offset (Location)")
    static FCk_Handle_Tween
    Create_TweenSceneNodeOffsetLocation(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween SceneNode Offset (Rotation)")
    static FCk_Handle_Tween
    Create_TweenSceneNodeOffsetRotation(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        FRotator InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween SceneNode Offset (Scale)")
    static FCk_Handle_Tween
    Create_TweenSceneNodeOffsetScale(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween SceneNode Offset (Transform)")
    static FCk_TweenTransformResult
    Create_TweenSceneNodeOffsetTransform(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        FTransform InEndTransform,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Location (Follow Target)")
    static FCk_Handle_Tween
    Create_TweenEntityLocation_FollowTarget(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        UPARAM(ref) FCk_Handle_Transform& InTargetEntity,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Rotation (Follow Target)")
    static FCk_Handle_Tween
    Create_TweenEntityRotation_FollowTarget(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        UPARAM(ref) FCk_Handle_Transform& InTargetEntity,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Scale (Follow Target)")
    static FCk_Handle_Tween
    Create_TweenEntityScale_FollowTarget(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        UPARAM(ref) FCk_Handle_Transform& InTargetEntity,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Transform (Follow Target)")
    static FCk_TweenTransformResult
    Create_TweenEntityTransform_FollowTarget(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        UPARAM(ref) FCk_Handle_Transform& InTargetEntity,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Transform (Follow Spline)")
    static FCk_Handle_Tween
    Create_TweenEntityTransform_FollowSpline(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        const FCk_Handle_Spline& InSpline,
        float InDuration,
        ECk_Tween_SplineOrientation InOrientation = ECk_Tween_SplineOrientation::OrientToSpline,
        ECk_TweenEasing InEasing = ECk_TweenEasing::Linear,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    // --------------------------------------------------------------------------------------------------------------------

    // Curve-DRIVEN rotation: the channels emit DEGREES (X/Y/Z mapping to Pitch/Yaw/Roll) and are
    // composed onto the entity's current rotation, captured now and held for the tween's life. A
    // null channel contributes 0, so a single-axis wobble authors one curve.
    //
    // This is the creator to reach for when the motion RETURNS to where it began (a shake, a
    // wobble, a recoil). The Start->End creators above cannot express one at any easing: with
    // Start == End, Interpolate(V, V, alpha) is V for every alpha, so the prop never moves.
    //
    // Because the pose is recomputed from the immutable base every frame it is ABSOLUTE, never
    // accumulated -- so re-triggering (Restart) cannot walk the prop away from its rest pose.
    //
    // InDuration <= 0 derives the duration from the curves' own last key, so the length cannot
    // drift out of sync with the authored curve.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Rotation (Curve Offset)")
    static FCk_Handle_Tween
    Create_TweenEntityRotation_CurveOffset(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        const FCk_TweenCurveChannels& InChannels,
        float InDuration = 0.0f,
        ECk_TweenCurveTimeInput InTimeInput = ECk_TweenCurveTimeInput::ElapsedSeconds,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    // Curve-DRIVEN location: sibling of Create_TweenEntityRotation_CurveOffset. The channels emit
    // WORLD-axis units of offset added onto the entity's location, captured now and held for the
    // tween's life. Same contract throughout: a null channel contributes 0, InDuration <= 0
    // derives the duration from the curves' last key, and the absolute-recompute guarantee means
    // re-triggering (Restart) cannot walk the entity away from its rest position.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Location (Curve Offset)")
    static FCk_Handle_Tween
    Create_TweenEntityLocation_CurveOffset(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        const FCk_TweenCurveChannels& InChannels,
        float InDuration = 0.0f,
        ECk_TweenCurveTimeInput InTimeInput = ECk_TweenCurveTimeInput::ElapsedSeconds,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween SceneNode Offset Rotation (Curve Offset)")
    static FCk_Handle_Tween
    Create_TweenSceneNodeOffsetRotation_CurveOffset(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        const FCk_TweenCurveChannels& InChannels,
        float InDuration = 0.0f,
        ECk_TweenCurveTimeInput InTimeInput = ECk_TweenCurveTimeInput::ElapsedSeconds,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween SceneNode Offset Location (Curve Offset)")
    static FCk_Handle_Tween
    Create_TweenSceneNodeOffsetLocation_CurveOffset(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        const FCk_TweenCurveChannels& InChannels,
        float InDuration = 0.0f,
        ECk_TweenCurveTimeInput InTimeInput = ECk_TweenCurveTimeInput::ElapsedSeconds,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    // Curve-DRIVEN float: the curve's output is the tween's value outright. Start/End play no part.
    // InDuration <= 0 derives the duration from the curve's last key.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween (Float Curve)")
    static FCk_Handle_Tween
    Create_TweenFloat_Curve(
        UPARAM(ref) FCk_Handle& InOwner,
        UCurveFloat* InCurve,
        float InDuration = 0.0f,
        ECk_TweenCurveTimeInput InTimeInput = ECk_TweenCurveTimeInput::ElapsedSeconds,
        ECk_TweenLoopType InLoopType = ECk_TweenLoopType::None,
        int32 InLoopCount = 0,
        float InYoyoDelay = 0.0f,
        ECk_TweenCompletionBehavior InCompletionBehavior = ECk_TweenCompletionBehavior::DoNothing);

    // Replaces the ECk_TweenEasing table with an authored progress -> eased-progress curve on an
    // existing tween. This still interpolates Start->End, so it does NOT enable a shake -- for that
    // use Create_TweenEntityRotation_CurveOffset. The eased alpha is clamped to [0, 1], so an
    // overshooting curve shape will not actually overshoot.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Set Easing Curve")
    static FCk_Handle_Tween
    SetEasingCurve(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        UCurveFloat* InCurve);

    // Latest last-key time across the channels' curves; 0 when none are set. This is the
    // duration a curve-driven tween derives for itself, exposed for callers that need the number.
    // A channel authored but not resident ensures and counts as unset, so this and the tween the
    // caller goes on to create can never disagree about what is actually playing.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Get Curves Max Time")
    static float
    Get_CurvesMaxTime(
        const FCk_TweenCurveChannels& InChannels);

    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Chain Tween")
    static FCk_Handle_Tween
    ChainTween(
        UPARAM(ref) FCk_Handle_Tween& InFirstTween,
        UPARAM(ref) FCk_Handle_Tween& InNextTween,
        float InDelay = 0.0f);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Chain Tween Transform")
    static FCk_TweenTransformResult
    ChainTween_Transform(
        UPARAM(ref) FCk_TweenTransformResult& InFirstTransformTween,
        UPARAM(ref) FCk_TweenTransformResult& InNextTransformTween,
        float InDelay = 0.0f);

    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Pause",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Tween
    Pause(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Resume",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Tween
    Resume(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // InBehavior carries no C++ default: the trailing completion delegate cannot have one (UHT
    // cannot parse a delegate default), and C++ forbids a defaulted parameter before it.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Stop",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Tween
    Stop(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        ECk_TweenStopBehavior InBehavior,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Restart",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Tween
    Restart(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Set Time Multiplier",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Tween
    SetTimeMultiplier(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        float InMultiplier,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // --------------------------------------------------------------------------------------------------------------------

    // Each *_TransformTween below fans one call out to three independent tween entities. The
    // completion delegate must fire exactly once, so it rides the Location tween's request only;
    // all three are drained in the same pass of FProcessor_Tween_HandleRequests, so that one
    // completion faithfully reports when the fan-out was processed.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Pause Transform Tween",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_TweenTransformResult
    Pause_TransformTween(
        UPARAM(ref) FCk_TweenTransformResult& InTransformTween,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Resume Transform Tween",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_TweenTransformResult
    Resume_TransformTween(
        UPARAM(ref) FCk_TweenTransformResult& InTransformTween,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // InBehavior carries no C++ default — see Stop.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Stop Transform Tween",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_TweenTransformResult
    Stop_TransformTween(
        UPARAM(ref) FCk_TweenTransformResult& InTransformTween,
        ECk_TweenStopBehavior InBehavior,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Restart Transform Tween",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_TweenTransformResult
    Restart_TransformTween(
        UPARAM(ref) FCk_TweenTransformResult& InTransformTween,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Set Time Multiplier Transform Tween",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_TweenTransformResult
    SetTimeMultiplier_TransformTween(
        UPARAM(ref) FCk_TweenTransformResult& InTransformTween,
        float InMultiplier,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Get State")
    static ECk_TweenState
    Get_State(
        const FCk_Handle_Tween& InTween);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Get Progress")
    static FCk_FloatRange_0to1
    Get_Progress(
        const FCk_Handle_Tween& InTween);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Get Current Value")
    static FCk_TweenValue
    Get_CurrentValue(
        const FCk_Handle_Tween& InTween);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Get Current Loop")
    static int32
    Get_CurrentLoop(
        const FCk_Handle_Tween& InTween);

    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][TweenValue] Is Float")
    static bool
    TweenValue_IsFloat(
        const FCk_TweenValue& InValue);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][TweenValue] Is Vector")
    static bool
    TweenValue_IsVector(
        const FCk_TweenValue& InValue);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][TweenValue] Is Rotator")
    static bool
    TweenValue_IsRotator(
        const FCk_TweenValue& InValue);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][TweenValue] Is Linear Color")
    static bool
    TweenValue_IsLinearColor(
        const FCk_TweenValue& InValue);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][TweenValue] Is Transform Handle")
    static bool
    TweenValue_IsTransformHandle(
        const FCk_TweenValue& InValue);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][TweenValue] Get As Float")
    static float
    TweenValue_GetAsFloat(
        const FCk_TweenValue& InValue);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][TweenValue] Get As Vector")
    static FVector
    TweenValue_GetAsVector(
        const FCk_TweenValue& InValue);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][TweenValue] Get As Rotator")
    static FRotator
    TweenValue_GetAsRotator(
        const FCk_TweenValue& InValue);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][TweenValue] Get As Linear Color")
    static FLinearColor
    TweenValue_GetAsLinearColor(
        const FCk_TweenValue& InValue);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][TweenValue] Get As Transform Handle")
    static FCk_Handle_Transform
    TweenValue_GetAsTransformHandle(
        const FCk_TweenValue& InValue);

    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Bind To OnUpdate")
    static FCk_Handle_Tween
    BindTo_OnUpdate(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        const FCk_Delegate_Tween_OnUpdate& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Unbind From OnUpdate")
    static FCk_Handle_Tween
    UnbindFrom_OnUpdate(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        const FCk_Delegate_Tween_OnUpdate& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Bind To OnComplete")
    static FCk_Handle_Tween
    BindTo_OnComplete(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        const FCk_Delegate_Tween_OnComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Unbind From OnComplete")
    static FCk_Handle_Tween
    UnbindFrom_OnComplete(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        const FCk_Delegate_Tween_OnComplete& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Bind To OnLoop")
    static FCk_Handle_Tween
    BindTo_OnLoop(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        const FCk_Delegate_Tween_OnLoop& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Unbind From OnLoop")
    static FCk_Handle_Tween
    UnbindFrom_OnLoop(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        const FCk_Delegate_Tween_OnLoop& InDelegate);

    // --------------------------------------------------------------------------------------------------------------------

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Tween
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Handle -> Tween Handle",
        meta = (CompactNodeTitle = "<AsTween>", BlueprintAutocast))
    static FCk_Handle_Tween
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Tween Handle",
        Category = "Ck|Tween",
        meta = (CompactNodeTitle = "INVALID_TweenHandle", Keywords = "make"))
    static FCk_Handle_Tween
    Get_InvalidHandle() { return {}; }

public:
    static auto OnTimerDone(
        FCk_Handle_Timer InTimer,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT) -> void;

private:
    static auto DoCreateTween(
        FCk_Handle& InOwner,
        const FCk_TweenValue& InStartValue,
        const FCk_TweenValue& InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing,
        ECk_TweenLoopType InLoopType,
        int32 InLoopCount,
        float InYoyoDelay,
        ECk_TweenTarget InTarget,
        ECk_TweenCompletionBehavior InCompletionBehavior) -> FCk_Handle_Tween;

    static auto DoAddRequestToTween(
        FCk_Handle_Tween& InTween,
        const auto& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Tween;

    static auto DoChainWithDelay(
        FCk_Handle_Tween& InFirstTween,
        FCk_Handle_Tween& InNextTween,
        float InDelay) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
