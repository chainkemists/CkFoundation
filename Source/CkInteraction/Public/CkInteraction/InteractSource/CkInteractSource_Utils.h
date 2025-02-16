#pragma once

#include "CkInteraction/InteractSource/CkInteractSource_Fragment_Data.h"
#include "CkInteraction/InteractSource/CkInteractSource_Fragment.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Fragment_Data.h"

#include "CkInteractSource_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKINTERACTION_API UCk_Utils_InteractSource_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_InteractSource_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_InteractSource);

public:
    using RecordOfInteractSources_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfInteractSources>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InteractSource",
              DisplayName="[Ck][InteractSource] Add Interaction Sources")
    static FCk_Handle_InteractSource
    Add(
        UPARAM(ref) FCk_Handle& InInteractSourceOwner,
        const FCk_Fragment_InteractSource_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InteractSource",
              DisplayName="[Ck][InteractSource] Add Multiple New Interaction Source")
    static TArray<FCk_Handle_InteractSource>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InInteractSourceOwner,
        const FCk_Fragment_MultipleInteractSource_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InteractSource",
              DisplayName="Has Interaction Source")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractSource",
        DisplayName="[Ck][InteractSource] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_InteractSource
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractSource",
        DisplayName="[Ck][InteractSource] Handle -> InteractSource Handle",
        meta = (CompactNodeTitle = "<AsInteractSource>", BlueprintAutocast))
    static FCk_Handle_InteractSource
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid InteractSource Handle",
        Category = "Ck|Utils|InteractSource",
        meta = (CompactNodeTitle = "INVALID_InteractSourceHandle", Keywords = "make"))
    static FCk_Handle_InteractSource
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractSource",
        DisplayName="[Ck][InteractSource] Request Cancel Interaction")
    static FCk_Handle_InteractSource
    Request_CancelInteraction(
        UPARAM(ref) FCk_Handle_InteractSource& InInteractSource,
        const FCk_Request_InteractSource_CancelInteraction& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractSource",
        DisplayName="[Ck][InteractSource] Request Cancel All Interactions")
    static FCk_Handle_InteractSource
    Request_CancelAllInteractions(
        UPARAM(ref) FCk_Handle_InteractSource& InInteractSource);

    // This is only meant to be called by InteractTarget
    static FCk_Handle_InteractSource
    Request_StartInteraction(
        UPARAM(ref) FCk_Handle_InteractSource& InInteractSource,
        const FCk_Request_InteractSource_StartInteraction& InRequest);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractSource",
        DisplayName = "[Ck][InteractSource] Get Interaction Channel")
    static const FGameplayTag&
    Get_InteractionChannel(
        const FCk_Handle_InteractSource& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractSource",
        DisplayName = "[Ck][InteractSource] Get Concurrent Interactions Policy")
    static ECk_InteractionSource_ConcurrentInteractionsPolicy
    Get_ConcurrentInteractionsPolicy(
        const FCk_Handle_InteractSource& InHandle);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractSource",
        DisplayName="[Ck][InteractSource] Get Current Interactions")
    static TArray<FCk_Handle_Interaction>
    Get_CurrentInteractions(
        const FCk_Handle_InteractSource& InHandle,
        ECk_InteractionSource_SortingPolicy InSortingPolicy = ECk_InteractionSource_SortingPolicy::ClosestToFarthest);

    static TArray<FCk_Handle_Interaction>
    Get_PendingInteractions(
        const FCk_Handle_InteractSource& InHandle);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractSource",
        DisplayName="[Ck][InteractSource] Try Get Current Interactions (By Target)")
    static FCk_Handle_Interaction
    TryGet_CurrentInteractions_ByTarget(
        const FCk_Handle_InteractSource& InHandle,
        const FCk_Handle& InTarget);

    UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|InteractSource",
          DisplayName="[Ck][InteractSource] Try Get Interaction Source")
    static FCk_Handle_InteractSource
    TryGet(
        const FCk_Handle& InInteractSourceOwner,
        UPARAM(meta = (Categories = "InteractionChannel")) FGameplayTag InInteractionChannel);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractSource",
        DisplayName = "[Ck][InteractSource] Bind To OnNewInteraction")
    static FCk_Handle_InteractSource
    BindTo_OnNewInteraction(
        UPARAM(ref) FCk_Handle_InteractSource& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_InteractSource_OnNewInteraction& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractSource",
        DisplayName = "[Ck][InteractSource] Unbind From OnNewInteraction")
    static FCk_Handle_InteractSource
    UnbindFrom_OnNewInteraction(
        UPARAM(ref) FCk_Handle_InteractSource& InHandle,
        const FCk_Delegate_InteractSource_OnNewInteraction& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractSource",
        DisplayName = "[Ck][InteractSource] Bind To OnInteractionFinished")
    static FCk_Handle_InteractSource
    BindTo_OnInteractionFinished(
        UPARAM(ref) FCk_Handle_InteractSource& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_InteractSource_OnInteractionFinished& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractSource",
        DisplayName = "[Ck][InteractSource] Unbind From OnInteractionFinished")
    static FCk_Handle_InteractSource
    UnbindFrom_OnInteractionFinished(
        UPARAM(ref) FCk_Handle_InteractSource& InHandle,
        const FCk_Delegate_InteractSource_OnInteractionFinished& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------