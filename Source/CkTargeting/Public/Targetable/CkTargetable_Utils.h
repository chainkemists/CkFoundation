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
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
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
        UPARAM(ref) FCk_Handle& InHandle,
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
              DisplayName = "[Ck][Targetable] Get Is Valid (BasicInfo)",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    IsValid(
        const FCk_Targetable_BasicInfo& InTargetableInfo);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targetable",
              DisplayName = "[Ck][Targetable] BasicInfo == BasicInfo",
              meta = (CompactNodeTitle = "==", KeyWords = "==,equal"))
    static bool
    IsEqual(
        const FCk_Targetable_BasicInfo& InTargetableInfoA,
        const FCk_Targetable_BasicInfo& InTargetableInfoB);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targetable",
              DisplayName = "[Ck][Targetable] BasicInfo != BasicInfo",
              meta = (CompactNodeTitle = "!=", KeyWords = "!=,not,equal"))
    static bool
    IsNotEqual(
        const FCk_Targetable_BasicInfo& InTargetableInfoA,
        const FCk_Targetable_BasicInfo& InTargetableInfoB);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targetable",
              DisplayName="[Ck][Targetable] Try Get Targetable")
    static FCk_Handle_Targetable
    TryGet_Targetable(
        const FCk_Handle& InTargetableOwnerEntity,
        FGameplayTag InTargetableName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targetable",
              DisplayName="[Ck][Targetable] Get Targetability Tags")
    static FGameplayTagContainer
    Get_TargetabilityTags(
        const FCk_Handle_Targetable& InTargetable);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targetable",
              DisplayName="[Ck][Targetable] Get Attachment Params")
    static FCk_Targetable_AttachmentParams
    Get_AttachmentParams(
        const FCk_Handle_Targetable& InTargetable);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targetable",
              DisplayName="[Ck][Targetable] Get Attachment Node")
    static USceneComponent*
    Get_AttachmentNode(
        const FCk_Handle_Targetable& InTargetable);

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
