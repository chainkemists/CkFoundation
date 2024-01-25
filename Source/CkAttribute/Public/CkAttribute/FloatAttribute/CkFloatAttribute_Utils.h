#pragma once

#include <GameplayTagContainer.h>

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkFloatAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_FloatAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatAttribute_UE);

public:
    friend class UCk_Utils_FloatAttributeModifier_UE;

private:
    class FloatAttribute_Utils : public ck::TUtils_Attribute<ck::FFragment_FloatAttribute> {};
    class RecordOfFloatAttributes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfFloatAttributes> {};

public:
    friend class UCk_Utils_FloatAttributes_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Add New Attribute")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_FloatAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Add Multiple New Attributes")
    static void
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleFloatAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Has Attribute")
    static bool
    Has(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag      InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Has Any Attribute")
    static bool
    Has_Any(
        const FCk_Handle& InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Ensure Has Attribute")
    static bool
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Ensure Has Any Attribute")
    static bool
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle>
    ForEach_FloatAttribute(
        FCk_Handle InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_FloatAttribute(
        const FCk_Handle&                  InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] For Each If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle>
    ForEach_FloatAttribute_If(
        FCk_Handle InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_FloatAttribute_If(
        const FCk_Handle&                  InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc,
        const TFunction<bool(FCk_Handle)>& InPredicate) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Get Base Value")
    static float
    Get_BaseValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Get Bonus Value")
    static float
    Get_BonusValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Get Final Value")
    static float
    Get_FinalValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Request Override Base Value")
    static void
    Request_Override(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        float InNewBaseValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName = "[Ck][FloatAttribute] Bind To OnValueChanged")
    static void
    BindTo_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName = "[Ck][FloatAttribute] Unbind From OnValueChanged")
    static void
    UnbindFrom_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_FloatAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatAttributeModifier_UE);

private:
    class FloatAttributeModifier_Utils : public ck::TUtils_AttributeModifier<ck::FFragment_FloatAttributeModifier> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] Add Modifier")
    static void
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] Has Modifier")
    static bool
    Has(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag      InAttributeName,
        FGameplayTag      InModifierName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] Ensure Has Modifier")
    static bool
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] Remove Modifier")
    static void
    Remove(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);
};

// --------------------------------------------------------------------------------------------------------------------
