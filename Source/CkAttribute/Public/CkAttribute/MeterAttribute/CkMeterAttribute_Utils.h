#pragma once

#include <GameplayTagContainer.h>

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment_Data.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkMeterAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_MeterAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_MeterAttribute_UE);

public:
    friend class UCk_Utils_MeterAttributeModifier_UE;

private:
    class MeterAttribute_Utils : public ck::TUtils_Attribute<ck::FFragment_MeterAttribute> {};
    class RecordOfMeterAttributes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfMeterAttributes> {};

public:
    friend class UCk_Utils_MeterAttributes_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="Add Meter Attributes")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Provider_MeterAttributes_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="Has Meter Attribute")
    static bool
    Has(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="Ensure Has Meter Attribute")
    static bool
    Ensure(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="Get Meter Attribute Base Value")
    static FCk_Meter
    Get_BaseValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="Get Meter Attribute Bonus Value")
    static FCk_Meter
    Get_BonusValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="Get Meter Attribute Final Value")
    static FCk_Meter
    Get_FinalValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName = "Bind To On Meter Attribute Value Changed")
    static void
    BindTo_OnValueChanged(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName = "unbind From On Meter Attribute Value Changed")
    static void
    UnbindFrom_OnValueChanged(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_MeterAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_MeterAttributeModifier_UE);

private:
    class MeterAttributeModifier_Utils : public ck::TUtils_AttributeModifier<ck::FFragment_MeterAttributeModifier> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Meter",
              DisplayName="Add Meter Attribute Modifier")
    static void
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_MeterAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Meter",
              DisplayName="Has Meter Attribute Modifier")
    static bool
    Has(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Meter",
              DisplayName="Ensure Has Meter Attribute Modifier")
    static bool
    Ensure(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Meter",
              DisplayName="Remove Meter Attribute Modifier")
    static void
    Remove(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity);
};

// --------------------------------------------------------------------------------------------------------------------
