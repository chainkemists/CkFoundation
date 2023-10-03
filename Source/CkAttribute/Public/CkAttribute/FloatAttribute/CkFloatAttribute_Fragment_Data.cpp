#include "CkFloatAttribute_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_FloatAttribute_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_FloatAttribute_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_FloatAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_FloatAttribute_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleFloatAttribute_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleFloatAttribute_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleFloatAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleFloatAttribute_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
