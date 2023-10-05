#include "CkNumericAttribute_Utils.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Utils.h"

#include <Kismet/KismetMathLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NumericAttribute_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_NumericAttribute_ParamsData& InParams)
    -> void
{
    const auto& ConstraintsPolicy = InParams.Get_ConstraintsPolicy();
    const auto& AttributeName = InParams.Get_AttributeName();
    const auto& AttributeStartingValue = InParams.Get_AttributeStartingValue();

    const auto& HasMinimumValue = EnumHasAnyFlags(ConstraintsPolicy, ECk_NumericAttribute_ConstraintsPolicy::HasMinimumValue);
    const auto& HasMaximumValue = EnumHasAnyFlags(ConstraintsPolicy, ECk_NumericAttribute_ConstraintsPolicy::HasMaximumValue);

    if (HasMinimumValue || HasMaximumValue)
    {
        constexpr float DefaultMinimumValue = -100000.0f;
        constexpr float DefaultMaximumValue = 100000.0f;

        const auto& MinimumValue = HasMinimumValue ? InParams.Get_AttributeMinimumValue() : DefaultMinimumValue;
        const auto& MaximumValue = HasMaximumValue ? InParams.Get_AttributeMaximumValue() : DefaultMaximumValue;

        const auto& StartingPercentage = UKismetMathLibrary::MapRangeClamped(AttributeStartingValue, MinimumValue, MaximumValue, 0.0f, 1.0f);

        UCk_Utils_MeterAttribute_UE::Add
        (
            InHandle,
            FCk_Fragment_MeterAttribute_ParamsData
            {
                AttributeName,
                FCk_Meter
                {
                    FCk_Meter_Params
                    {
                        FCk_Meter_Capacity{MaximumValue}
                        .Set_MinCapacity(MinimumValue)
                    }
                    .Set_StartingPercentage(FCk_FloatRange_0to1{static_cast<ck::FFragment_FloatAttribute::AttributeDataType>(StartingPercentage)})
                }
            }
        );
    }
    else
    {
        UCk_Utils_FloatAttribute_UE::Add(InHandle, FCk_Fragment_FloatAttribute_ParamsData{AttributeName, AttributeStartingValue});
    }
}

auto
    UCk_Utils_NumericAttribute_UE::
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleNumericAttribute_ParamsData& InParams)
    -> void
{
    for (const auto& param : InParams.Get_NumericAttributeParams())
    {
        Add(InHandle, param);
    }
}

auto
    UCk_Utils_NumericAttribute_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    return UCk_Utils_FloatAttribute_UE::Has(InAttributeOwnerEntity, InAttributeName) || UCk_Utils_MeterAttribute_UE::Has(InAttributeOwnerEntity, InAttributeName);
}

auto
    UCk_Utils_NumericAttribute_UE::
    Has_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    return UCk_Utils_FloatAttribute_UE::Has_Any(InAttributeOwnerEntity) || UCk_Utils_MeterAttribute_UE::Has_Any(InAttributeOwnerEntity);
}

auto
    UCk_Utils_NumericAttribute_UE::
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName), TEXT("Handle [{}] does NOT have a Numeric Attribute [{}]"), InAttributeOwnerEntity, InAttributeName)
    { return false; }

    return true;
}

auto
    UCk_Utils_NumericAttribute_UE::
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have any Numeric Attribute [{}]"), InAttributeOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_NumericAttribute_UE::
    Get_CurrentValue(
        FCk_Handle   InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> float
{
    if (NOT Ensure(InAttributeOwnerEntity, InAttributeName))
    { return {}; }

    if (UCk_Utils_FloatAttribute_UE::Has(InAttributeOwnerEntity, InAttributeName))
    { return UCk_Utils_FloatAttribute_UE::Get_FinalValue(InAttributeOwnerEntity, InAttributeName); }

    return UCk_Utils_MeterAttribute_UE::Get_FinalValue(InAttributeOwnerEntity, InAttributeName).Get_Value().Get_CurrentValue();
}

auto
    UCk_Utils_NumericAttribute_UE::
    Get_MinValue(
        FCk_Handle   InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        bool&        OutHasMinValue)
    -> float
{
    OutHasMinValue = UCk_Utils_MeterAttribute_UE::Has(InAttributeOwnerEntity, InAttributeName);

    if (OutHasMinValue)
    {
        return UCk_Utils_MeterAttribute_UE::Get_FinalValue(InAttributeOwnerEntity, InAttributeName).Get_Params().Get_Capacity().Get_MinCapacity();
    }

    return {};
}

auto
    UCk_Utils_NumericAttribute_UE::
    Get_MaxValue(
        FCk_Handle   InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        bool&        OutHasMaxValue)
    -> float
{
    OutHasMaxValue = UCk_Utils_MeterAttribute_UE::Has(InAttributeOwnerEntity, InAttributeName);

    if (OutHasMaxValue)
    {
        return UCk_Utils_MeterAttribute_UE::Get_FinalValue(InAttributeOwnerEntity, InAttributeName).Get_Params().Get_Capacity().Get_MaxCapacity();
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
