#include "CkVectorAttribute_Fragment_Data.h"

#include "CkAttribute/CkAttribute_Log.h"

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
    FCk_Fragment_VectorAttribute_ParamsData::
    Get_MinValue() const
    -> FVector
{
    ck::attribute::ErrorIf(NOT (_Component == ECk_MinMax::Min || _Component == ECk_MinMax::MinMax),
        TEXT("Attempting to get a Min value of Attribute [{}] where MinMax is set to [{}]. Please address this."),
         _Component,
         _Name);

    return _MinValue;
}

auto
    FCk_Fragment_VectorAttribute_ParamsData::
    Get_MaxValue() const
    -> FVector
{
    ck::attribute::ErrorIf(NOT (_Component == ECk_MinMax::Max || _Component == ECk_MinMax::MinMax),
        TEXT("Attempting to get a Max value Attribute [{}] where MinMax is set to [{}]. Please address this."),
         _Component,
         _Name);

    return _MaxValue;
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
