#pragma once

#include "CkAnimation/AnimPlan/CkAnimPlan_Fragment.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include <InstancedStruct.h>

#include "CkAnimPlan_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_AnimPlan_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKANIMATION_API UCk_Utils_AnimPlan_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AnimPlan_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AnimPlan);

private:
    struct RecordOfAnimPlans_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAnimPlans> {};

public:
    friend class ck::FProcessor_AnimPlan_HandleRequests;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimPlan] Add New AnimPlan")
    static FCk_Handle_AnimPlan
    Add(
        UPARAM(ref) FCk_Handle& InAnimPlanOwnerEntity,
        const FCk_Fragment_AnimPlan_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimPlan",
              DisplayName="[Ck][AnimPlan] Add Multiple New AnimPlans")
    static TArray<FCk_Handle_AnimPlan>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InAnimPlanOwnerEntity,
        const FCk_Fragment_MultipleAnimPlan_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InAnimPlanOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimPlan",
              DisplayName="[Ck][AnimPlan] Has Any AnimPlan")
    static bool
    Has_Any(
        const FCk_Handle& InAnimPlanOwnerEntity);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AnimPlan",
        DisplayName="[Ck][AnimPlan] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AnimPlan
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AnimPlan",
        DisplayName="[Ck][AnimPlan] Handle -> AnimPlan Handle",
        meta = (CompactNodeTitle = "<AsAnimPlan>", BlueprintAutocast))
    static FCk_Handle_AnimPlan
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid AnimPlan Handle",
        Category = "Ck|Utils|AnimPlan",
        meta = (CompactNodeTitle = "INVALID_AnimPlanHandle", Keywords = "make"))
    static FCk_Handle_AnimPlan
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimPlan",
              DisplayName="[Ck][AnimPlan] Try Get AnimPlan")
    static FCk_Handle_AnimPlan
    TryGet_AnimPlan(
        const FCk_Handle& InAnimPlanOwnerEntity,
        UPARAM(meta = (Categories = "AnimPlan.Goal")) FGameplayTag InAnimPlanGoalName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimPlan",
              DisplayName="[Ck][AnimPlan] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_AnimPlan>
    ForEach_AnimPlan(
        UPARAM(ref) FCk_Handle& InAnimPlanOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

    static auto
    ForEach_AnimPlan(
        FCk_Handle& InAnimPlanOwnerEntity,
        const TFunction<void(FCk_Handle_AnimPlan&)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimPlan",
              DisplayName="[Ck][AnimPlan] Get Anim Goal",
              meta = (BlueprintThreadSafe))
    static FCk_AnimPlan_Goal
    Get_AnimGoal(
        const FCk_Handle_AnimPlan& InAnimPlanEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimPlan",
              DisplayName="[Ck][AnimPlan] Get Anim Cluster",
              meta = (BlueprintThreadSafe))
    static FCk_AnimPlan_Cluster
    Get_AnimCluster(
        const FCk_Handle_AnimPlan& InAnimPlanEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AnimPlan",
              DisplayName="[Ck][AnimPlan] Get Anim State",
              meta = (BlueprintThreadSafe))
    static FCk_AnimPlan_State
    Get_AnimState(
        const FCk_Handle_AnimPlan& InAnimPlanEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimPlan] Request Update Anim Cluster")
    static FCk_Handle_AnimPlan
    Request_UpdateAnimCluster(
        UPARAM(ref) FCk_Handle_AnimPlan& InHandle,
        const FCk_Request_AnimPlan_UpdateAnimCluster& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AnimPlan] Request Update Anim State")
    static FCk_Handle_AnimPlan
    Request_UpdateAnimState(
        UPARAM(ref) FCk_Handle_AnimPlan& InHandle,
        const FCk_Request_AnimPlan_UpdateAnimState& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimPlan",
              DisplayName = "[Ck][AnimPlan] Bind To OnPlanChanged")
    static FCk_Handle_AnimPlan
    BindTo_OnGoalChanged(
        UPARAM(ref) FCk_Handle_AnimPlan& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AnimPlan_OnPlanChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AnimPlan",
              DisplayName = "[Ck][AnimPlan] Unbind From OnPlanChanged")
    static FCk_Handle_AnimPlan
    UnbindFrom_OnGoalChanged(
        UPARAM(ref) FCk_Handle_AnimPlan& InHandle,
        const FCk_Delegate_AnimPlan_OnPlanChanged& InDelegate);

private:
    static auto
    Request_TryReplicateAnimPlan(
        FCk_Handle_AnimPlan& InAnimPlanEntity) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

