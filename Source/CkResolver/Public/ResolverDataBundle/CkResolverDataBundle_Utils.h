#pragma once

#include "CkECS/Handle/CkHandle.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkNet/CkNet_Utils.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment_Data.h"

#include "CkResolverDataBundle_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FFragment_ResolverDataBundle_Current;
    struct FFragment_ResolverDataBundle_Params;

    class FProcessor_ResolverDataBundle_StartNewPhase;
    class FProcessor_ResolverDataBundle_HandleRequests;
    class FProcessor_ResolverDataBundle_ResolveOperations;
    class FProcessor_ResolverDataBundle_Calculate;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKRESOLVER_API UCk_Utils_ResolverDataBundle_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ResolverDataBundle_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ResolverDataBundle);

public:
    friend class ck::FProcessor_ResolverDataBundle_StartNewPhase;
    friend class ck::FProcessor_ResolverDataBundle_HandleRequests;
    friend class ck::FProcessor_ResolverDataBundle_ResolveOperations;
    friend class ck::FProcessor_ResolverDataBundle_Calculate;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ResolverDataBundle",
              DisplayName="[Ck][ResolverDataBundle] Create New")
    static FCk_Handle_ResolverDataBundle
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        UPARAM(meta=(Categories = "Resolver.DataBundle.Name")) FGameplayTag InName,
        const FCk_Fragment_ResolverDataBundle_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ResolverDataBundle",
              DisplayName="[Ck][ResolverDataBundle] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ResolverDataBundle
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Handle -> ResolverDataBundle Handle",
        meta = (CompactNodeTitle = "<AsResolverDataBundle>", BlueprintAutocast))
    static FCk_Handle_ResolverDataBundle
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Get Name")
    static FGameplayTag
    Get_Name(
        const FCk_Handle_ResolverDataBundle& InDataBundle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Get Instigator")
    static FCk_Handle_ResolverSource
    Get_Instigator(
        const FCk_Handle_ResolverDataBundle& InDataBundle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Get Target")
    static FCk_Handle_ResolverTarget
    Get_Target(
        const FCk_Handle_ResolverDataBundle& InDataBundle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Get Causer")
    static FCk_Handle
    Get_Causer(
        const FCk_Handle_ResolverDataBundle& InDataBundle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Get Phases")
    static TArray<FCk_Fragment_ResolverDataBundle_PhaseInfo>
    Get_Phases(
        const FCk_Handle_ResolverDataBundle& InDataBundle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Get Current Phase")
    static FGameplayTag
    Get_CurrentPhase(
        const FCk_Handle_ResolverDataBundle& InDataBundle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Get Next Phase")
    static FGameplayTag
    Get_NextPhase(
        const FCk_Handle_ResolverDataBundle& InDataBundle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Get Final Value")
    static float
    Get_FinalValue(
        const FCk_Handle_ResolverDataBundle& InDataBundle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Get Metadata")
    static FGameplayTagContainer
    Get_Metadata(
        const FCk_Handle_ResolverDataBundle& InDataBundle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Get Resolved Payload")
    static FCk_Payload_ResolverDataBundle_Resolved
    Get_Resolved_Payload(
        const FCk_Handle_ResolverDataBundle& InDataBundle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Request Add Operation (Modifier)")
    static FCk_Handle_ResolverDataBundle
    Request_AddOperation_Modifier(
        UPARAM(ref) FCk_Handle_ResolverDataBundle& InDataBundle,
        ECk_ResolverDataBundle_PhaseSelection InPhase,
        const FRequest_ResolverDataBundle_ModifierOperation& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Request Add Operation (Metadata)")
    static FCk_Handle_ResolverDataBundle
    Request_AddOperation_Metadata(
        UPARAM(ref) FCk_Handle_ResolverDataBundle& InDataBundle,
        ECk_ResolverDataBundle_PhaseSelection InPhase,
        const FRequest_ResolverDataBundle_MetadataOperation& InRequest);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Bind to OnPhaseStart")
    static FCk_Handle_ResolverDataBundle
    BindTo_OnPhaseStart(
        UPARAM(ref) FCk_Handle_ResolverDataBundle& InDataBundle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ResolverDataBundle_OnPhaseStart& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Unbind from OnPhaseStart")
    static FCk_Handle_ResolverDataBundle
    UnbindFrom_OnPhaseStart(
        UPARAM(ref) FCk_Handle_ResolverDataBundle& InDataBundle,
        const FCk_Delegate_ResolverDataBundle_OnPhaseStart& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Bind to OnPhaseComplete")
    static FCk_Handle_ResolverDataBundle
    BindTo_OnPhaseComplete(
        UPARAM(ref) FCk_Handle_ResolverDataBundle& InDataBundle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ResolverDataBundle_OnPhaseComplete& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Unbind from OnPhaseComplete")
    static FCk_Handle_ResolverDataBundle
    UnbindFrom_OnPhaseComplete(
        UPARAM(ref) FCk_Handle_ResolverDataBundle& InDataBundle,
        const FCk_Delegate_ResolverDataBundle_OnPhaseComplete& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Bind to OnAllPhasesComplete")
    static FCk_Handle_ResolverDataBundle
    BindTo_OnAllPhasesComplete(
        UPARAM(ref) FCk_Handle_ResolverDataBundle& InDataBundle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ResolverDataBundle_OnAllPhasesComplete& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ResolverDataBundle",
        DisplayName="[Ck][ResolverDataBundle] Unbind from OnAllPhasesComplete")
    static FCk_Handle_ResolverDataBundle
    UnbindFrom_OnAllPhasesComplete(
        UPARAM(ref) FCk_Handle_ResolverDataBundle& InDataBundle,
        const FCk_Delegate_ResolverDataBundle_OnAllPhasesComplete& InDelegate);

private:
    [[nodiscard]]
    static auto
    DoGet_ResolverDataComponentAttributeName(
        ECk_ResolverDataBundle_ModifierComponent InResolverDataComponent) -> FGameplayTag;

    [[maybe_unused]]
    static auto
    DoAddPendingOperation(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle,
        const FCk_ResolverDataBundle_ModifierOperation& InOperation) -> FCk_Handle_ResolverDataBundle;

    [[maybe_unused]]
    static auto
    DoAddPendingOperation(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle,
        const FCk_ResolverDataBundle_ModifierOperation_Conditional& InOperation) -> FCk_Handle_ResolverDataBundle;

    [[maybe_unused]]
    static auto
    DoAddPendingOperation(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle,
        const FCk_ResolverDataBundle_MetadataOperation& InOperation) -> FCk_Handle_ResolverDataBundle;

    [[maybe_unused]]
    static auto
    DoAddPendingOperation(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle,
        const FCk_ResolverDataBundle_MetadataOperation_Conditional& InOperation) -> FCk_Handle_ResolverDataBundle;

    [[maybe_unused]]
    static auto
    DoMarkBundle_AsOperationsResolved(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle) -> FCk_Handle_ResolverDataBundle;

    [[maybe_unused]]
    static auto
    DoMarkBundle_AsCalculateDone(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle) -> FCk_Handle_ResolverDataBundle;

    static auto
    DoTryStartNewPhase(
        FCk_Handle_ResolverDataBundle& InResolverDataBundle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
