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
              DisplayName="Add New Float Attribute")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_FloatAttribute_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="Add Multiple New Float Attributes")
    static void
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleFloatAttribute_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="Has Float Attribute")
    static bool
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="Has Any Float Attribute")
    static bool
    Has_Any(
        FCk_Handle InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="Ensure Has Float Attribute")
    static bool
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="Ensure Has Any Float Attribute")
    static bool
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="Get All Float Attributes")
    static TArray<FGameplayTag>
    Get_All(
        FCk_Handle InAttributeOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="Get Float Attribute Base Value")
    static float
    Get_BaseValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="Get Float Attribute Bonus Value")
    static float
    Get_BonusValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="Get Float Attribute Final Value")
    static float
    Get_FinalValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName = "Bind To On Float Attribute Value Changed")
    static void
    BindTo_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName = "unbind From On Float Attribute Value Changed")
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
              DisplayName="Add Float Attribute Modifier")
    static void
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="Has Float Attribute Modifier")
    static bool
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="Ensure Has Float Attribute Modifier")
    static bool
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="Remove Float Attribute Modifier")
    static void
    Remove(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);
};

// --------------------------------------------------------------------------------------------------------------------
