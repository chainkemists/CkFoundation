#pragma once

#include "CkAggro/CkAggro_Fragment.h"
#include "CkAggro/CkAggro_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "CkAggro_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKAGGRO_API UCk_Utils_Aggro_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Aggro_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Aggro);

    using RecordOfAggro_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAggro>;

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName="[Ck][Aggro] Add New Aggro")
    static FCk_Handle_Aggro
    Add(
        UPARAM(ref) FCk_Handle_AggroOwner& InHandle,
        const FCk_Handle& InTarget,
        const FCk_Fragment_Aggro_Params& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName="[Ck][Aggro] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Aggro
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Aggro",
        DisplayName="[Ck][Aggro] Handle -> Aggro Handle",
        meta = (CompactNodeTitle = "<AsAggro>", BlueprintAutocast))
    static FCk_Handle_Aggro
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Get Aggro Target")
    static FCk_Handle
    Get_AggroTarget(
        const FCk_Handle_Aggro& InAggro);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Get Aggro Score")
    static float
    Get_AggroScore(
        const FCk_Handle_Aggro& InAggro);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Exclude")
    static FCk_Handle_Aggro
    Request_Exclude(
        UPARAM(ref) FCk_Handle_Aggro& InAggro);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Include")
    static FCk_Handle_Aggro
    Request_Include(
        UPARAM(ref) FCk_Handle_Aggro& InAggro);
};

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
        DisplayName="[Ck][Aggro] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AggroOwner
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][Aggro] Handle -> Aggro Handle",
        meta = (CompactNodeTitle = "<AsAggro>", BlueprintAutocast))
    static FCk_Handle_AggroOwner
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][Aggro] Try Get Aggro by Target")
    static FCk_Handle_Aggro
    TryGet_AggroByTarget(
        const FCk_Handle& InAggroOwnerEntity,
        const FCk_Handle& InTarget);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][Aggro] Get Best Aggro")
    static FCk_Handle_Aggro
    Get_BestAggro(
        const FCk_Handle& InAggroOwnerEntity);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName = "[Ck][Aggro] Bind To OnNewAggroAdded")
    static FCk_Handle
    BindTo_OnNewAggroAdded(
        UPARAM(ref) FCk_Handle& InAggroOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Aggro_OnNewAggroAdded& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName = "[Ck][Aggro] Unbind From OnNewAggroAdded")
    static FCk_Handle
    UnbindFrom_OnNewAggroAdded(
        UPARAM(ref) FCk_Handle& InAggroOwner,
        const FCk_Delegate_Aggro_OnNewAggroAdded& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][Aggro] For Each",
        meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Aggro>
    ForEach_Aggro(
        const FCk_Handle& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Aggro(
        const FCk_Handle& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        const TFunction<void(FCk_Handle_Aggro)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroOwner",
        DisplayName="[Ck][Aggro] For Each (Sorted)",
        meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Aggro>
    ForEach_Aggro_Sorted(
        const FCk_Handle& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        const FInstancedStruct& InOptionalPayload,
        ECk_ScoreSortingPolicy InSortingPolicy,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Aggro_Sorted(
        const FCk_Handle& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        ECk_ScoreSortingPolicy InSortingPolicy,
        const TFunction<void(FCk_Handle_Aggro)>& InFunc) -> void;
};
