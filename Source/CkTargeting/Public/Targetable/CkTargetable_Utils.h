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
    friend class ck::FProcessor_Targetable_Setup;
    friend class UCk_Utils_Targeter_UE;

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

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Targetable Handle",
        Category = "Ck|Utils|Targetable",
        meta = (CompactNodeTitle = "INVALID_TargetableHandle", Keywords = "make"))
    static FCk_Handle_Targetable
    Get_InvalidHandle() { return {}; };

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
        UPARAM(meta = (Categories = "Targetable")) FGameplayTag InTargetableName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Targetable",
              DisplayName = "[Ck][Targetable] Get Enable/Disable")
    static ECk_EnableDisable
    Get_EnableDisable(
        const FCk_Handle_Targetable& InTargetable);

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

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Targetable] Request Enable/Disable",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Targetable
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Targetable& InTargetable,
        const FCk_Request_Targetable_EnableDisable& InRequest,
        const FCk_Delegate_Targetable_OnEnableDisable& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targetable",
              DisplayName = "[Ck][Targetable] Bind To On Enable/Disable")
    static FCk_Handle_Targetable
    BindTo_OnEnableDisable(
        UPARAM(ref) FCk_Handle_Targetable& InTargetable,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Targetable_OnEnableDisable& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Targetable",
              DisplayName = "[Ck][Targetable] Unbind From On Enable/Disable")
    static FCk_Handle_Targetable
    UnbindFrom_OnEnableDisable(
        UPARAM(ref) FCk_Handle_Targetable& InTargetable,
        const FCk_Delegate_Targetable_OnEnableDisable& InDelegate);

private:
    static auto
    Get_TargetableIsReady(
        const FCk_Handle_Targetable& InTargetable) -> bool;

    static auto
    Set_TargetableIsReady(
        FCk_Handle_Targetable& InTargetable) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
