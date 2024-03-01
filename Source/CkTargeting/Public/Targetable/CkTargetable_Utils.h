#pragma once

#include "Targetable/CkTargetable_Fragment.h"

#include "CkEcs/Delegates/CkDelegates.h"
#include "CkECS/Handle/CkHandle.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkTargetable_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKTARGETING_API UCk_Utils_Targetable_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Targetable_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Targetable);

private:
    struct RecordOfTargetables_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfTargetables> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targetable",
              DisplayName="[Ck][Targetable] Add New Targetable")
    static FCk_Handle_Targetable
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Targetable_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targetable",
              DisplayName="[Ck][Targetable] Add Multiple New Targetables")
    static TArray<FCk_Handle_Targetable>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_MultipleTargetable_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Targetable",
        DisplayName="[Ck][Targetable] Has Any Targetable")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Targetable",
        DisplayName="[Ck][Targetable] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Targetable
    DoCast(
        FCk_Handle InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Targetable",
        DisplayName="[Ck][Targetable] Handle -> Targetable Handle",
        meta = (CompactNodeTitle = "<AsTargetable>", BlueprintAutocast))
    static FCk_Handle_Targetable
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targetable",
              DisplayName="[Ck][Targetable] Try Get Targetable")
    static FCk_Handle_Targetable
    TryGet_Targetable(
        const FCk_Handle& InTargetableOwnerEntity,
        FGameplayTag InTargetableName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targetable",
              DisplayName="[Ck][Targetable] For Each",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Targetable>
    ForEach_Targetable(
        UPARAM(ref) FCk_Handle& InTargetableOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Targetable(
        FCk_Handle& InTargetableOwnerEntity,
        const TFunction<void(FCk_Handle_Targetable)>& InFunc) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
