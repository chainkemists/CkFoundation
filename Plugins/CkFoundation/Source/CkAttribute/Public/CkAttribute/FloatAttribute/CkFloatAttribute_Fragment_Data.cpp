#include "CkFloatAttribute_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_FloatAttributes_ParamsData_PDA::
    Get_Value_Implementation() const
    -> FCk_Fragment_FloatAttributes_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_FloatAttributes_ParamsData_Literal_PDA::
    Get_Value_Implementation() const
    -> FCk_Fragment_FloatAttributes_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
