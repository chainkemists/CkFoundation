#pragma once

#include <GameplayTagContainer.h>

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment_Data.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkByteAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_ByteAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ByteAttribute_UE);

public:
    friend class UCk_Utils_ByteAttributeModifier_UE;

private:
    class ByteAttribute_Utils : public ck::TUtils_Attribute<ck::FFragment_ByteAttribute> {};
    class RecordOfByteAttributes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfByteAttributes> {};

public:
    friend class UCk_Utils_ByteAttributes_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Add New Attribute")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ByteAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Add Multiple New Attributes")
    static void
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleByteAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Has Attribute")
    static bool
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Has Any Attribute")
    static bool
    Has_Any(
        FCk_Handle InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Ensure Has Attribute")
    static bool
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Ensure Has Any Attribute")
    static bool
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle>
    ForEach_ByteAttribute(
        FCk_Handle InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_ByteAttribute(
        const FCk_Handle&                  InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] For Each If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle>
    ForEach_ByteAttribute_If(
        FCk_Handle InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_ByteAttribute_If(
        FCk_Handle InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc,
        const TFunction<bool(FCk_Handle)>& InPredicate) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Get Base Value")
    static uint8
    Get_BaseValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Get Bonus Value")
    static uint8
    Get_BonusValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Get Final Value")
    static uint8
    Get_FinalValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Request Override Base Value")
    static void
    Request_Override(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        uint8 InNewBaseValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName = "[Ck][ByteAttribute] Bind To OnValueChanged")
    static void
    BindTo_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ByteAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName = "[Ck][ByteAttribute] Unbind From OnValueChanged")
    static void
    UnbindFrom_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_ByteAttribute_OnValueChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_ByteAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ByteAttributeModifier_UE);

private:
    class ByteAttributeModifier_Utils : public ck::TUtils_AttributeModifier<ck::FFragment_ByteAttributeModifier> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Byte",
              DisplayName="[Ck][ByteAttribute] Add Modifier")
    static void
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_ByteAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Byte",
              DisplayName="[Ck][ByteAttribute] Has Modifier")
    static bool
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Byte",
              DisplayName="[Ck][ByteAttribute] Ensure Has Modifier")
    static bool
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Byte",
              DisplayName="[Ck][ByteAttribute] Remove Modifier")
    static void
    Remove(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);
};

// --------------------------------------------------------------------------------------------------------------------
