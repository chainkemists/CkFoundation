#include "CkMeterAttribute_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Multiple_MeterAttribute_ParamsData_PDA::
    Get_Value_Implementation() const
    -> TArray<FCk_Fragment_MeterAttribute_ParamsData>
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_MeterAttributeModifier_ParamsData::
    Get_ModifierPolicy() const
    -> ECk_MeterAttributeModifier_Policy
{
    return static_cast<ECk_MeterAttributeModifier_Policy>(_ModifierPolicyFlags);
}

auto
    UCk_Provider_Multiple_MeterAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation() const
    -> TArray<FCk_Fragment_MeterAttribute_ParamsData>
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
