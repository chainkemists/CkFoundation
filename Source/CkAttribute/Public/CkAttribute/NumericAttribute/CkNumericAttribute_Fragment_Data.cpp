#include "CkNumericAttribute_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_NumericAttribute_ParamsData::
    Get_ConstraintsPolicy() const
    -> ECk_NumericAttribute_ConstraintsPolicy
{
    return static_cast<ECk_NumericAttribute_ConstraintsPolicy>(_ConstraintsPolicyFlags);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_NumericAttribute_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_NumericAttribute_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_NumericAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_NumericAttribute_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleNumericAttribute_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleNumericAttribute_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleNumericAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleNumericAttribute_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
