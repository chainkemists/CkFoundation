#include "CkMeterAttribute_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MeterAttributes_ParamsData_PDA::
    Get_Value_Implementation() const
    -> FCk_Fragment_MeterAttributes_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MeterAttributes_ParamsData_Literal_PDA::
    Get_Value_Implementation() const
    -> FCk_Fragment_MeterAttributes_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
