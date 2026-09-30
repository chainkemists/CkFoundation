#pragma once

#include "CkRotateTowards/CkRotateTowards_Fragment.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkRotateTowards_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_RotateTowards"))
class CKROTATETOWARDS_API UCk_Utils_RotateTowards_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RotateTowards_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_RotateTowards);

    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Adds the feature ONTO the transform entity. A scene-node entity is rotated through its offset, any other
    // transform through Request_SetRotation. The spec's target may be invalid (no target yet).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Add Feature")
    static FCk_Handle_RotateTowards
    Add(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_RotateTowards_Spec& InSpec);

    static auto Has(const FCk_Handle& InHandle) -> bool;

private:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] DoCast", meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_RotateTowards
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] DoCastChecked", meta = (CompactNodeTitle = "<AsRotateTowards>", BlueprintAutocast))
    static FCk_Handle_RotateTowards
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck] Get Invalid RotateTowards Handle",
        meta = (CompactNodeTitle = "INVALID_RotateTowardsHandle", Keywords = "make"))
    static FCk_Handle_RotateTowards
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Get Target")
    static FCk_Handle_Transform
    Get_Target(
        const FCk_Handle_RotateTowards& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Get Has Target")
    static bool
    Get_HasTarget(
        const FCk_Handle_RotateTowards& InHandle);

    // True while every axis is within the reached tolerance of the desired (locked + clamped) rotation.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Get Is At Target")
    static bool
    Get_IsAtTarget(
        const FCk_Handle_RotateTowards& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Get Is Enabled")
    static bool
    Get_IsEnabled(
        const FCk_Handle_RotateTowards& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Get Tunables")
    static FCk_RotateTowards_Tunables
    Get_Tunables(
        const FCk_Handle_RotateTowards& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Get Has Range Clamp")
    static bool
    Get_HasRangeClamp(
        const FCk_Handle_RotateTowards& InHandle);

    // A default (all axes disabled) clamp when none is set.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Get Range Clamp")
    static FCk_RotateTowards_RangeClamp
    Get_RangeClamp(
        const FCk_Handle_RotateTowards& InHandle);

    // The look-at rotation after axis locks and range clamp, from the last Update. Zero without a target.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Get Desired Rotation")
    static FRotator
    Get_DesiredRotation(
        const FCk_Handle_RotateTowards& InHandle);

    // Per-axis signed shortest-arc degrees still to turn (Pitch, Yaw, Roll), from the last Update.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Get Remaining Rotation")
    static FRotator
    Get_RemainingRotation(
        const FCk_Handle_RotateTowards& InHandle);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Request Set Target", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_RotateTowards
    Request_SetTarget(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_SetTarget& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Request Clear Target", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_RotateTowards
    Request_ClearTarget(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_ClearTarget& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Request Update Tunables", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_RotateTowards
    Request_UpdateTunables(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_UpdateTunables& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Request Set Range Clamp", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_RotateTowards
    Request_SetRangeClamp(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_SetRangeClamp& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Request Clear Range Clamp", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_RotateTowards
    Request_ClearRangeClamp(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_ClearRangeClamp& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Request Enable/Disable", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_RotateTowards
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Request_RotateTowards_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Bind To OnTargetChanged")
    static FCk_Handle_RotateTowards
    BindTo_OnTargetChanged(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Unbind From OnTargetChanged")
    static FCk_Handle_RotateTowards
    UnbindFrom_OnTargetChanged(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetChanged& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Bind To OnTargetReached")
    static FCk_Handle_RotateTowards
    BindTo_OnTargetReached(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetReached& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Unbind From OnTargetReached")
    static FCk_Handle_RotateTowards
    UnbindFrom_OnTargetReached(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetReached& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Bind To OnTargetCleared")
    static FCk_Handle_RotateTowards
    BindTo_OnTargetCleared(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetCleared& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|RotateTowards", DisplayName = "[Ck][RotateTowards] Unbind From OnTargetCleared")
    static FCk_Handle_RotateTowards
    UnbindFrom_OnTargetCleared(
        UPARAM(ref) FCk_Handle_RotateTowards& InHandle,
        const FCk_Delegate_RotateTowards_OnTargetCleared& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
