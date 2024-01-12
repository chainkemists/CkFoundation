#include "CkVectorAttribute_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_VectorAttribute_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_VectorAttribute_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_VectorAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_VectorAttribute_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleVectorAttribute_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleVectorAttribute_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleVectorAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleVectorAttribute_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
