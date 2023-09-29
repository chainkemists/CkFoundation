#include "CkFloatAttribute_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Multiple_FloatAttribute_ParamsData_PDA::
    Get_Value_Implementation() const
    -> TArray<FCk_Fragment_FloatAttribute_ParamsData>
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Multiple_FloatAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation() const
    -> TArray<FCk_Fragment_FloatAttribute_ParamsData>
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
