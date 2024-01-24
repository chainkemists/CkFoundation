#pragma once

#include "CkAnimation/AnimState/CkAnimState_Fragment_Data.h"

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
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Add Feature")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_AnimState_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Has Feature")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Ensure Has Feature")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Get Goal")
    static FCk_AnimState_Goal
    Get_AnimGoal(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Get State")
    static FCk_AnimState_State
    Get_AnimState(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Get Cluster")
    static FCk_AnimState_Cluster
    Get_AnimCluster(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Get Overlay")
    static FCk_AnimState_Overlay
    Get_AnimOverlay(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Request Set Goal")
    static void
    Request_SetAnimGoal(
        FCk_Handle InHandle,
        const FCk_Request_AnimState_SetGoal& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Request Set State")
    static void
    Request_SetAnimState(
        FCk_Handle InHandle,
        const FCk_Request_AnimState_SetState& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Request Set Cluster")
    static void
    Request_SetAnimCluster(
        FCk_Handle InHandle,
        const FCk_Request_AnimState_SetCluster& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName="[AnimState] Request Set Overlay")
    static void
    Request_SetAnimOverlay(
        FCk_Handle InHandle,
        const FCk_Request_AnimState_SetOverlay& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[AnimState] Bind To OnGoalChanged")
    static void
    BindTo_OnGoalChanged(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnGoalChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[AnimState] Unbind From OnGoalChanged")
    static void
    UnbindFrom_OnGoalChanged(
        FCk_Handle InHandle,
        const FCk_Delegate_AnimState_OnGoalChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[AnimState] Bind To OnStateChanged")
    static void
    BindTo_OnStateChanged(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[AnimState] Unbind From OnStateChanged")
    static void
    UnbindFrom_OnStateChanged(
        FCk_Handle InHandle,
        const FCk_Delegate_AnimState_OnStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[AnimState] Bind To OnClusterChanged")
    static void
    BindTo_OnClusterChanged(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnClusterChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[AnimState] Unbind From OnClusterChanged")
    static void
    UnbindFrom_OnClusterChanged(
        FCk_Handle InHandle,
        const FCk_Delegate_AnimState_OnClusterChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[AnimState] Bind To OnOverlayChanged")
    static void
    BindTo_OnOverlayChanged(
        FCk_Handle InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnOverlayChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimState",
              DisplayName = "[AnimState] Unbind From OnOverlayChanged")
    static void
    UnbindFrom_OnOverlayChanged(
        FCk_Handle InHandle,
        const FCk_Delegate_AnimState_OnOverlayChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
