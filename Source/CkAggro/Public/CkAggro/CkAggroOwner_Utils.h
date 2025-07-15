#pragma once

#include "CkAggro/CkAggroOwner_Fragment_Data.h"
#include "CkAggro/CkAggro_Fragment.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkEcs/CkEcs_Utils.h"

#include "CkEcs/Record/CkRecord_Utils.h"

#include "CkAggroOwner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKAGGRO_API UCk_Utils_AggroOwner_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AggroOwner_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AggroOwner);

public:
    using RecordOfAggro_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAggro>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroOwner",
              DisplayName="[Ck][AggroOwner] Add Feature")
    static FCk_Handle_AggroOwner
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AggroOwner_Params& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][AggroOwner] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AggroOwner
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][AggroOwner] Handle -> Aggro Owner Handle",
        meta = (CompactNodeTitle = "<AsAggroOwner>", BlueprintAutocast))
    static FCk_Handle_AggroOwner
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid AggroOwner Handle",
        Category = "Ck|Utils|AggroOwner",
        meta = (CompactNodeTitle = "INVALID_AggroOwnerHandle", Keywords = "make"))
    static FCk_Handle_AggroOwner
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][AggroOwner] Try Get Aggro by Target")
    static FCk_Handle_Aggro
    TryGet_AggroByTarget(
        const FCk_Handle_AggroOwner& InAggroOwnerEntity,
        const FCk_Handle& InTarget);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][AggroOwner] Get Best Aggro")
    static FCk_Handle_Aggro
    Get_BestAggro(
        const FCk_Handle_AggroOwner& InAggroOwnerEntity);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName = "[Ck][AggroOwner] Bind To OnNewAggroAdded")
    static FCk_Handle
    BindTo_OnNewAggroAdded(
        UPARAM(ref) FCk_Handle_AggroOwner& InAggroOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Aggro_OnNewAggroAdded& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName = "[Ck][AggroOwner] Unbind From OnNewAggroAdded")
    static FCk_Handle
    UnbindFrom_OnNewAggroAdded(
        UPARAM(ref) FCk_Handle_AggroOwner& InAggroOwner,
        const FCk_Delegate_Aggro_OnNewAggroAdded& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName = "[Ck][AggroOwner] Bind To OnAggroChanged")
    static FCk_Handle
    BindTo_OnAggroChanged(
        UPARAM(ref) FCk_Handle_AggroOwner& InAggroOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Aggro_OnAggroChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName = "[Ck][AggroOwner] Unbind From OnAggroChanged")
    static FCk_Handle
    UnbindFrom_OnAggroChanged(
        UPARAM(ref) FCk_Handle_AggroOwner& InAggroOwner,
        const FCk_Delegate_Aggro_OnAggroChanged& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][AggroOwner] For Each",
        meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Aggro>
    ForEach_Aggro(
        const FCk_Handle_AggroOwner& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Aggro(
        const FCk_Handle_AggroOwner& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        const TFunction<void(FCk_Handle_Aggro)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][AggroOwner] For Each (Sorted)",
        meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Aggro>
    ForEach_Aggro_Sorted(
        const FCk_Handle_AggroOwner& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        const FInstancedStruct& InOptionalPayload,
        ECk_ScoreSortingPolicy InSortingPolicy,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Aggro_Sorted(
        const FCk_Handle_AggroOwner& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        ECk_ScoreSortingPolicy InSortingPolicy,
        const TFunction<void(FCk_Handle_Aggro)>& InFunc) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
