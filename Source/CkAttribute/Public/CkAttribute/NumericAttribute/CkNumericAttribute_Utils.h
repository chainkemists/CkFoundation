#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment.h"
#include "CkAttribute/NumericAttribute/CkNumericAttribute_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkNumericAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_NumericAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_NumericAttribute_UE);

public:
    class FloatAttribute_Utils : public ck::TUtils_Attribute<ck::FFragment_FloatAttribute> {};
    class RecordOfFloatAttributes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfFloatAttributes> {};

    class RecordOfMeterAttributes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfMeterAttributes> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName="[Ck][NumericAttribute] Add New Attribute")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_NumericAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName="[Ck][NumericAttribute] Add Multiple New Attributes")
    static void
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleNumericAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName="[Ck][NumericAttribute] Has Attribute")
    static bool
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName="[Ck][NumericAttribute] Has Any Attribute")
    static bool
    Has_Any(
        FCk_Handle InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName="[Ck][NumericAttribute] Ensure Has Attribute")
    static bool
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

        UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName="[Ck][NumericAttribute] Ensure Has Any Attribute")
    static bool
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName="[Ck][NumericAttribute] Get Current Value")
    static float
    Get_CurrentValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName="[Ck][NumericAttribute] Get  Min Value")
    static float
    Get_MinValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        bool& OutHasMinValue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName="[Ck][NumericAttribute] Get Max Value")
    static float
    Get_MaxValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        bool& OutHasMaxValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName = "[Ck][NumericAttribute] Bind To OnValueChanged")
    static void
    BindTo_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_NumericAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Numeric",
              DisplayName = "[Ck][NumericAttribute] Unbind From OnValueChanged")
    static void
    UnbindFrom_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_NumericAttribute_OnValueChanged& InDelegate);

private:
    static auto
    OnFloatAttribute_ValueChanged(
        FCk_Handle InFloatAttributeOwnerEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged) -> void;

    static auto
    OnMeterAttribute_ValueChanged(
        FCk_Handle InMeterAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_NumericAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_NumericAttributeModifier_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Numeric",
              DisplayName="[Ck][NumericAttribute] Add Modifier")
    static void
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_NumericAttributeModifier_ParamsData& InParams);

    // TODO: Implement the following functions
    //UFUNCTION(BlueprintPure,
    //          Category = "Ck|Utils|AttributeModifier|Numeric",
    //          DisplayName="Has Numeric Attribute Modifier")
    //static bool
    //Has(
    //    FCk_Handle InAttributeOwnerEntity,
    //    FGameplayTag InAttributeName,
    //    FGameplayTag InModifierName);

    //UFUNCTION(BlueprintPure,
    //          Category = "Ck|Utils|AttributeModifier|Numeric",
    //          DisplayName="Ensure Has Numeric Attribute Modifier")
    //static bool
    //Ensure(
    //    FCk_Handle InAttributeOwnerEntity,
    //    FGameplayTag InAttributeName,
    //    FGameplayTag InModifierName);

    //UFUNCTION(BlueprintCallable,
    //          Category = "Ck|Utils|AttributeModifier|Numeric",
    //          DisplayName="Ensure Has Numeric Attribute Modifier")
    //static void
    //Remove(
    //    FCk_Handle InAttributeOwnerEntity,
    //    FGameplayTag InAttributeName,
    //    FGameplayTag InModifierName);
};
