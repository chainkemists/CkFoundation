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
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
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
        UPARAM(ref) FCk_Handle& InHandle,
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
              DisplayName = "[Ck][Targeter] Get Is Valid (BasicInfo)",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    IsValid(
        const FCk_Targeter_BasicInfo& InTargeterInfo);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targeter",
              DisplayName = "[Ck][Targeter] BasicInfo == BasicInfo",
              meta = (CompactNodeTitle = "==", KeyWords = "==,equal"))
    static bool
    IsEqual(
        const FCk_Targeter_BasicInfo& InTargeterInfoA,
        const FCk_Targeter_BasicInfo& InTargeterInfoB);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targeter",
              DisplayName = "[Ck][Targeter] BasicInfo != BasicInfo",
              meta = (CompactNodeTitle = "!=", KeyWords = "!=,not,equal"))
    static bool
    IsNotEqual(
        const FCk_Targeter_BasicInfo& InTargeterInfoA,
        const FCk_Targeter_BasicInfo& InTargeterInfoB);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targeter",
              DisplayName="[Ck][Targeter] Try Get Targeter")
    static FCk_Handle_Targeter
    TryGet_Targeter(
        const FCk_Handle& InTargeterOwnerEntity,
        UPARAM(meta = (Categories = "Targeter")) FGameplayTag InTargeterName);

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
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targeter",
              DisplayName="[Ck][Targeter] Extract Owners (From TargetList)")
    static TArray<FCk_Handle>
    Get_ExtractOwners_FromTargetList(
        const FCk_Targeter_TargetList& InTargetList);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targeter",
              DisplayName="[Ck][Targeter] Extract Targetables (From TargetList)")
    static TArray<FCk_Handle_Targetable>
    Get_ExtractTargetables_FromTargetList(
        const FCk_Targeter_TargetList& InTargetList);

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
              DisplayName = "[Ck][Targeter] Filter Targets By Predicate",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static FCk_Targeter_TargetList
    FilterTargets_ByPredicate(
        const FCk_Targeter_BasicInfo& InTargeter,
        const FCk_Targeter_TargetList& InTargetList,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InTargeter_InTarget_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targeter",
              DisplayName = "[Ck][Targeter] Sort Targets By Predicate",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static void
    SortTargets_ByPredicate(
        const FCk_Targeter_BasicInfo& InTargeter,
        UPARAM(ref) FCk_Targeter_TargetList& InTargetList,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InTargeter_In2Targets_OutResult InPredicate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targeter",
              DisplayName = "[Ck][Targeter] Intersect Targets",
              meta = (CompactNodeTitle = "Intersect_Targets"))
    static FCk_Targeter_TargetList
    IntersectTargets(
        const FCk_Targeter_TargetList& InTargetListA,
        const FCk_Targeter_TargetList& InTargetListB);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targeter",
              DisplayName = "[Ck][Targeter] Except Targets",
              meta = (CompactNodeTitle = "Except_Targets"))
    static FCk_Targeter_TargetList
    ExceptTargets(
        const FCk_Targeter_TargetList& InTargetListA,
        const FCk_Targeter_TargetList& InTargetListB);

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
