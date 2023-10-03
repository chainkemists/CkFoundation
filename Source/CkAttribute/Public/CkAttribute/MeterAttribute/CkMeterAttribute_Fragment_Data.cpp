#include "CkMeterAttribute_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_MeterAttributeModifier_ParamsData::
    Get_ModifierPolicy() const
    -> ECk_MeterAttributeModifier_Policy
{
    return static_cast<ECk_MeterAttributeModifier_Policy>(_ModifierPolicyFlags);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MeterAttribute_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MeterAttribute_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MeterAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MeterAttribute_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleMeterAttribute_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleMeterAttribute_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleMeterAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleMeterAttribute_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
