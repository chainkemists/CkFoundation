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
    UCk_Provider_Multiple_MeterAttribute_ParamsData_Literal_PDA::
    Get_Value_Implementation() const
    -> TArray<FCk_Fragment_MeterAttribute_ParamsData>
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
