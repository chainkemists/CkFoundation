#pragma once

#include <GameplayTagContainer.h>

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment_Data.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkVectorAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_VectorAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VectorAttribute_UE);

public:
    friend class UCk_Utils_VectorAttributeModifier_UE;

private:
    class VectorAttribute_Utils : public ck::TUtils_Attribute<ck::FFragment_VectorAttribute> {};
    class RecordOfVectorAttributes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfVectorAttributes> {};

public:
    friend class UCk_Utils_VectorAttributes_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Add New Attribute")
    static FCk_Handle_VectorAttributeOwner
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_VectorAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Add Multiple New Attributes")
    static FCk_Handle_VectorAttributeOwner
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleVectorAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Has Attribute")
    static bool
    Has_Attribute(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag      InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Has Any Attribute")
    static bool
    Has_Any_Attribute(
        const FCk_Handle& InAttributeOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Has Feature")
    static bool
    Has(
        const FCk_Handle& InEntity);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Attribute|Vector",
        DisplayName="[Ck][VectorAttribute] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VectorAttributeOwner
    Cast(
        const FCk_Handle&    InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Attribute|Vector",
        DisplayName="[Ck][VectorAttribute] Handle -> VectorAttributeOwner Handle",
        meta = (CompactNodeTitle = "As VectorAttributeOwner", BlueprintAutocast))
    static FCk_Handle_VectorAttributeOwner
    Conv_HandleToVectorAttributeOwner(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle>
    ForEach_VectorAttribute(
        UPARAM(ref) FCk_Handle_VectorAttributeOwner& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_VectorAttribute(
        UPARAM(ref) FCk_Handle_VectorAttributeOwner& InAttributeOwner,
        const TFunction<void(FCk_Handle_VectorAttribute)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] For Each If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_VectorAttribute>
    ForEach_VectorAttribute_If(
        UPARAM(ref) FCk_Handle_VectorAttributeOwner& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_VectorAttribute_If(
        UPARAM(ref) FCk_Handle_VectorAttributeOwner& InAttributeOwner,
        const TFunction<void(FCk_Handle_VectorAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_VectorAttribute)>& InPredicate) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Get Base Value")
    static FVector
    Get_BaseValue(
        const FCk_Handle_VectorAttributeOwner& InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Get Bonus Value")
    static FVector
    Get_BonusValue(
        const FCk_Handle_VectorAttributeOwner& InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Get Final Value")
    static FVector
    Get_FinalValue(
        const FCk_Handle_VectorAttributeOwner& InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Request Override Base Value")
    static void
    Request_Override(
        UPARAM(ref) FCk_Handle_VectorAttributeOwner& InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FVector InNewBaseValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName = "[Ck][VectorAttribute] Bind To OnValueChanged")
    static void
    BindTo_OnValueChanged(
        UPARAM(ref) FCk_Handle_VectorAttributeOwner& InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VectorAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName = "[Ck][VectorAttribute] Unbind From OnValueChanged")
    static void
    UnbindFrom_OnValueChanged(
        UPARAM(ref) FCk_Handle_VectorAttributeOwner& InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_VectorAttribute_OnValueChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_VectorAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VectorAttributeModifier_UE);

private:
    class VectorAttributeModifier_Utils : public ck::TUtils_AttributeModifier<ck::FFragment_VectorAttributeModifier> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Vector",
              DisplayName="[Ck][VectorAttribute] Add Modifier")
    static void
    Add(
        UPARAM(ref) FCk_Handle_VectorAttributeOwner& InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_VectorAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Vector",
              DisplayName="[Ck][VectorAttribute] Has Modifier")
    static bool
    Has(
        const FCk_Handle_VectorAttributeOwner& InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Vector",
              DisplayName="[Ck][VectorAttribute] Remove Modifier")
    static void
    Remove(
        UPARAM(ref) FCk_Handle_VectorAttributeOwner& InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);
};

// --------------------------------------------------------------------------------------------------------------------
