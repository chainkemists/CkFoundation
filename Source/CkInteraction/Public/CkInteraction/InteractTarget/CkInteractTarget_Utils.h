#pragma once

#include "CkInteraction/InteractTarget/CkInteractTarget_Fragment_Data.h"
#include "CkInteraction/InteractTarget/CkInteractTarget_Fragment.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkInteractTarget_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKINTERACTION_API UCk_Utils_InteractTarget_UE :  public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_InteractTarget_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_InteractTarget);

public:
    using RecordOfInteractTargets_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfInteractTargets>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InteractTarget",
              DisplayName="[Ck][InteractTarget] Add Interaction Target")
    static FCk_Handle_InteractTarget
    Add(
        UPARAM(ref) FCk_Handle& InInteractTargetOwner,
        const FCk_Fragment_InteractTarget_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InteractTarget",
              DisplayName="[Ck][InteractTarget] Add Multiple New Interaction Targets")
    static TArray<FCk_Handle_InteractTarget>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InInteractTargetOwner,
        const FCk_Fragment_MultipleInteractTarget_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InteractTarget",
              DisplayName="Has Interaction Target")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractTarget",
        DisplayName="[Ck][InteractTarget] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_InteractTarget
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractTarget",
        DisplayName="[Ck][InteractTarget] Handle -> InteractTarget Handle",
        meta = (CompactNodeTitle = "<AsInteractTarget>", BlueprintAutocast))
    static FCk_Handle_InteractTarget
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid InteractTarget Handle",
        Category = "Ck|Utils|InteractTarget",
        meta = (CompactNodeTitle = "INVALID_InteractTargetHandle", Keywords = "make"))
    static FCk_Handle_InteractTarget
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractTarget",
        DisplayName="[Ck][InteractTarget] Request Start Interaction")
    static FCk_Handle_InteractTarget
    Request_StartInteraction(
        UPARAM(ref) FCk_Handle_InteractTarget& InInteractTarget,
        const FCk_Try_InteractTarget_StartInteraction& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractTarget",
        DisplayName="[Ck][InteractTarget] Request Cancel Interaction")
    static FCk_Handle_InteractTarget
    Request_CancelInteraction(
        UPARAM(ref) FCk_Handle_InteractTarget& InInteractTarget,
        const FCk_Request_InteractTarget_CancelInteraction& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractTarget",
        DisplayName="[Ck][InteractTarget] Request Cancel All Interactions")
    static FCk_Handle_InteractTarget
    Request_CancelAllInteractions(
        UPARAM(ref) FCk_Handle_InteractTarget& InInteractTarget);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Get Is Enabled")
    static ECk_EnableDisable
    Get_Enabled(
        const FCk_Handle_InteractTarget& InHandle);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Set Is Enabled")
    static void
    Set_Enabled(
        UPARAM(ref) FCk_Handle_InteractTarget& InHandle,
        ECk_EnableDisable InEnabled);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Get Interaction Channel")
    static const FGameplayTag&
    Get_InteractionChannel(
        const FCk_Handle_InteractTarget& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Can Interact With")
    static ECk_CanInteractWithResult
    Get_CanInteractWith(
        const FCk_Handle_InteractTarget& InTarget,
        const FCk_Handle& InSource);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Get Concurrent Interactions Policy")
    static ECk_InteractionTarget_ConcurrentInteractionsPolicy
    Get_ConcurrentInteractionsPolicy(
        const FCk_Handle_InteractTarget& InTarget);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Get Interaction Completion Policy")
    static ECk_Interaction_CompletionPolicy
    Get_InteractionCompletionPolicy(
        const FCk_Handle_InteractTarget& InTarget);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Get Interaction Duration")
    static FCk_Time
    Get_InteractionDuration(
        const FCk_Handle_InteractTarget& InTarget);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractTarget",
        DisplayName="[Ck][InteractTarget] Get Current Interactions")
    static TArray<FCk_Handle_Interaction>
    Get_CurrentInteractions(
        UPARAM(ref) FCk_Handle_InteractTarget& InHandle);

    UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|InteractTarget",
          DisplayName="[Ck][InteractTarget] Try Get Interaction Target")
    static FCk_Handle_InteractTarget
    TryGet(
        const FCk_Handle& InInteractTargetOwner,
        UPARAM(meta = (Categories = "InteractionChannel")) FGameplayTag InInteractionChannel);

    UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|InteractTarget",
          DisplayName="[Ck][InteractTarget] Try Get Interaction from InteractTarget")
    static FCk_Handle_Interaction
    TryGet_Interaction(
        const FCk_Handle_InteractTarget& InTarget,
        const FCk_Handle& InInteractSource);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Bind To OnNewInteraction")
    static FCk_Handle_InteractTarget
    BindTo_OnNewInteraction(
        UPARAM(ref) FCk_Handle_InteractTarget& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_InteractTarget_OnNewInteraction& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Unbind From OnNewInteraction")
    static FCk_Handle_InteractTarget
    UnbindFrom_OnNewInteraction(
        UPARAM(ref) FCk_Handle_InteractTarget& InHandle,
        const FCk_Delegate_InteractTarget_OnNewInteraction& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Bind To OnInteractionFinished")
    static FCk_Handle_InteractTarget
    BindTo_OnInteractionFinished(
        UPARAM(ref) FCk_Handle_InteractTarget& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_InteractTarget_OnInteractionFinished& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractTarget",
        DisplayName = "[Ck][InteractTarget] Unbind From OnInteractionFinished")
    static FCk_Handle_InteractTarget
    UnbindFrom_OnInteractionFinished(
        UPARAM(ref) FCk_Handle_InteractTarget& InHandle,
        const FCk_Delegate_InteractTarget_OnInteractionFinished& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------