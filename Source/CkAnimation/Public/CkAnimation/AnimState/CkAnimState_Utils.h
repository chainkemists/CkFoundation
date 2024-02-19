#pragma once

#include "CkAnimation/AnimState/CkAnimState_Fragment_Data.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkECS/Handle/CkHandle.h"

#include "CkNet/CkNet_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkAnimState_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKANIMATION_API UCk_Utils_AnimState_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AnimState_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimState] Add Feature")
    static void
    Add(
        UPARAM(ref) FCk_Handle_UnderConstruction& InHandle,
        const FCk_Fragment_AnimState_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimState",
              DisplayName="[Ck][AnimState] Has Feature")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimState",
              DisplayName="[Ck][AnimState] Ensure Has Feature")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimState] Get Goal")
    static FCk_AnimState_Goal
    Get_AnimGoal(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimState] Get State")
    static FCk_AnimState_State
    Get_AnimState(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimState] Get Cluster")
    static FCk_AnimState_Cluster
    Get_AnimCluster(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimState] Get Overlay")
    static FCk_AnimState_Overlay
    Get_AnimOverlay(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimState] Request Set Goal")
    static void
    Request_SetAnimGoal(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_AnimState_SetGoal& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimState] Request Set State")
    static void
    Request_SetAnimState(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_AnimState_SetState& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimState] Request Set Cluster")
    static void
    Request_SetAnimCluster(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_AnimState_SetCluster& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimState] Request Set Overlay")
    static void
    Request_SetAnimOverlay(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Request_AnimState_SetOverlay& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[Ck][AnimState] Bind To OnGoalChanged")
    static void
    BindTo_OnGoalChanged(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnGoalChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[Ck][AnimState] Unbind From OnGoalChanged")
    static void
    UnbindFrom_OnGoalChanged(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_AnimState_OnGoalChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[Ck][AnimState] Bind To OnStateChanged")
    static void
    BindTo_OnStateChanged(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[Ck][AnimState] Unbind From OnStateChanged")
    static void
    UnbindFrom_OnStateChanged(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_AnimState_OnStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[Ck][AnimState] Bind To OnClusterChanged")
    static void
    BindTo_OnClusterChanged(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnClusterChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[Ck][AnimState] Unbind From OnClusterChanged")
    static void
    UnbindFrom_OnClusterChanged(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_AnimState_OnClusterChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[Ck][AnimState] Bind To OnOverlayChanged")
    static void
    BindTo_OnOverlayChanged(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnOverlayChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[Ck][AnimState] Unbind From OnOverlayChanged")
    static void
    UnbindFrom_OnOverlayChanged(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_AnimState_OnOverlayChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
