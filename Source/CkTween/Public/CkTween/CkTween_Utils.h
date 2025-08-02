#pragma once

#include "CkTween_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkTween_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Handle_Transform;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKTWEEN_API UCk_Utils_Tween_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Tween_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Tween);

public:
    // ============================================================================================================
    // CORE TWEEN CREATION
    // ============================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Float")
    static FCk_Handle_Tween
    Create_TweenFloat(
        UPARAM(ref) FCk_Handle& InOwner,
        float InStartValue,
        float InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        float InDelay = 0.0f);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Vector")
    static FCk_Handle_Tween
    Create_TweenVector(
        UPARAM(ref) FCk_Handle& InOwner,
        FVector InStartValue,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        float InDelay = 0.0f);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Rotator")
    static FCk_Handle_Tween
    Create_TweenRotator(
        UPARAM(ref) FCk_Handle& InOwner,
        FRotator InStartValue,
        FRotator InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        float InDelay = 0.0f);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Linear Color")
    static FCk_Handle_Tween
    Create_TweenLinearColor(
        UPARAM(ref) FCk_Handle& InOwner,
        FLinearColor InStartValue,
        FLinearColor InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        float InDelay = 0.0f);

    // ============================================================================================================
    // TRANSFORM SHORTCUTS
    // ============================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Location")
    static FCk_Handle_Tween
    Create_TweenEntityLocation(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        float InDelay = 0.0f);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Location Add")
    static FCk_Handle_Tween
    Create_TweenEntityLocationAdd(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        FVector InOffsetValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        float InDelay = 0.0f);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Rotation")
    static FCk_Handle_Tween
    Create_TweenEntityRotation(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        FRotator InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        float InDelay = 0.0f);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Scale")
    static FCk_Handle_Tween
    Create_TweenEntityScale(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        FVector InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        float InDelay = 0.0f);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Create Tween Entity Transform")
    static FCk_TweenTransformResult
    Create_TweenEntityTransform(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        FTransform InEndTransform,
        float InDuration,
        ECk_TweenEasing InEasing = ECk_TweenEasing::OutCubic,
        float InDelay = 0.0f);

    // ============================================================================================================
    // CHAINING OPERATIONS
    // ============================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Chain Tween")
    static FCk_Handle_Tween
    ChainTween(
        UPARAM(ref) FCk_Handle_Tween& InFirstTween,
        UPARAM(ref) FCk_Handle_Tween& InNextTween);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Chain Tween Transform")
    static FCk_TweenTransformResult
    ChainTween_Transform(
        UPARAM(ref) FCk_TweenTransformResult& InFirstTransformTween,
        UPARAM(ref) FCk_TweenTransformResult& InNextTransformTween);

    // ============================================================================================================
    // CONTROL OPERATIONS - Individual Tweens
    // ============================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Pause")
    static FCk_Handle_Tween
    Pause(
        UPARAM(ref) FCk_Handle_Tween& InTween);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Resume")
    static FCk_Handle_Tween
    Resume(
        UPARAM(ref) FCk_Handle_Tween& InTween);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Stop")
    static FCk_Handle_Tween
    Stop(
        UPARAM(ref) FCk_Handle_Tween& InTween);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Restart")
    static FCk_Handle_Tween
    Restart(
        UPARAM(ref) FCk_Handle_Tween& InTween);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Set Time Multiplier")
    static FCk_Handle_Tween
    SetTimeMultiplier(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        float InMultiplier);

    // ============================================================================================================
    // CONTROL OPERATIONS - Transform Tweens
    // ============================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Pause Transform Tween")
    static FCk_TweenTransformResult
    Pause_TransformTween(
        UPARAM(ref) FCk_TweenTransformResult& InTransformTween);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Resume Transform Tween")
    static FCk_TweenTransformResult
    Resume_TransformTween(
        UPARAM(ref) FCk_TweenTransformResult& InTransformTween);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Stop Transform Tween")
    static FCk_TweenTransformResult
    Stop_TransformTween(
        UPARAM(ref) FCk_TweenTransformResult& InTransformTween);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Restart Transform Tween")
    static FCk_TweenTransformResult
    Restart_TransformTween(
        UPARAM(ref) FCk_TweenTransformResult& InTransformTween);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Set Time Multiplier Transform Tween")
    static FCk_TweenTransformResult
    SetTimeMultiplier_TransformTween(
        UPARAM(ref) FCk_TweenTransformResult& InTransformTween,
        float InMultiplier);

    // ============================================================================================================
    // GETTERS
    // ============================================================================================================

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

    // ============================================================================================================
    // TWEEN VALUE HELPERS
    // ============================================================================================================

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

    // ============================================================================================================
    // SIGNAL BINDING
    // ============================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Bind To OnUpdate")
    static FCk_Handle_Tween
    BindTo_OnUpdate(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Tween_OnUpdate& InDelegate);

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
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Tween_OnComplete& InDelegate);

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
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Tween_OnLoop& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Tween",
        DisplayName = "[Ck][Tween] Unbind From OnLoop")
    static FCk_Handle_Tween
    UnbindFrom_OnLoop(
        UPARAM(ref) FCk_Handle_Tween& InTween,
        const FCk_Delegate_Tween_OnLoop& InDelegate);

    // ============================================================================================================
    // CAST OPERATIONS
    // ============================================================================================================

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

private:
    // Internal helper functions
    static auto DoCreateTween(
        FCk_Handle& InOwner,
        const FCk_TweenValue& InStartValue,
        const FCk_TweenValue& InEndValue,
        float InDuration,
        ECk_TweenEasing InEasing,
        float InDelay,
        FGameplayTag InTweenName = TAG_Tween) -> FCk_Handle_Tween;

    static auto DoAddRequestToTween(
        FCk_Handle_Tween& InTween,
        const auto& InRequest) -> FCk_Handle_Tween;
};

// --------------------------------------------------------------------------------------------------------------------
