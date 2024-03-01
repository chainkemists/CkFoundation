#pragma once

#include "Targeter/CkTargeter_Fragment.h"

#include "CkEcs/Delegates/CkDelegates.h"
#include "CkECS/Handle/CkHandle.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkTargeter_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Targeter_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKTARGETING_API UCk_Utils_Targeter_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Targeter_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Targeter);

private:
    friend class ck::FProcessor_Targeter_HandleRequests;

private:
    struct RecordOfTargeters_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfTargeters> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targeter",
              DisplayName="[Ck][Targeter] Add New Targeter")
    static FCk_Handle_Targeter
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Targeter_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targeter",
              DisplayName="[Ck][Targeter] Add Multiple New Targeters")
    static TArray<FCk_Handle_Targeter>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_MultipleTargeter_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Targeter",
        DisplayName="[Ck][Targeter] Has Any Targeter")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Targeter",
        DisplayName="[Ck][Targeter] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Targeter
    DoCast(
        FCk_Handle InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Targeter",
        DisplayName="[Ck][Targeter] Handle -> Targeter Handle",
        meta = (CompactNodeTitle = "<AsTargeter>", BlueprintAutocast))
    static FCk_Handle_Targeter
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targeter",
              DisplayName="[Ck][Targeter] Try Get Targeter")
    static FCk_Handle_Targeter
    TryGet_Targeter(
        const FCk_Handle& InTargeterOwnerEntity,
        FGameplayTag InTargeterName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targeter",
              DisplayName="[Ck][Targeter] Get Can Target")
    static bool
    Get_CanTarget(
        const FCk_Handle_Targeter& InTargeter,
        const FCk_Handle_Targetable& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targeter",
              DisplayName="[Ck][Targeter] Get Targeting Status")
    static ECk_Targeter_TargetingStatus
    Get_TargetingStatus(
        const FCk_Handle_Targeter& InTargeter);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targeter",
              DisplayName="[Ck][Targeter] For Each",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Targeter>
    ForEach_Targeter(
        UPARAM(ref) FCk_Handle& InTargeterOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Targeter(
        FCk_Handle& InTargeterOwnerEntity,
        const TFunction<void(FCk_Handle_Targeter)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targeter",
              DisplayName = "[Ck][Targeter] Request Start Targeting")
    static FCk_Handle_Targeter
    Request_Start(
        UPARAM(ref) FCk_Handle_Targeter& InTargeter);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targeter",
              DisplayName = "[Ck][Targeter] Request Stop Targeting")
    static FCk_Handle_Targeter
    Request_Stop(
        UPARAM(ref) FCk_Handle_Targeter& InTargeter);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targeter",
              DisplayName = "[Ck][Targeter] Request Query Targets",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Targeter
    Request_QueryTargets(
        UPARAM(ref) FCk_Handle_Targeter& InTargeter,
        const FCk_Request_Targeter_QueryTargets& InRequest,
        const FCk_Delegate_Targeter_OnTargetsQueried& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targeter",
              DisplayName = "[Ck][Targeter] Bind To OnTargetsQueried")
    static FCk_Handle_Targeter
    BindTo_OnTargetsQueried(
        UPARAM(ref) FCk_Handle_Targeter& InTargeterHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Targeter_OnTargetsQueried& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targeter",
              DisplayName = "[Ck][Targeter] Unbind From OnTargetsQueried")
    static FCk_Handle_Targeter
    UnbindFrom_OnTargetsQueried(
        UPARAM(ref) FCk_Handle_Targeter& InTargeterHandle,
        const FCk_Delegate_Targeter_OnTargetsQueried& InDelegate);

private:
    static auto
    Request_Cleanup(
        FCk_Handle_Targeter& InTargeter) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
