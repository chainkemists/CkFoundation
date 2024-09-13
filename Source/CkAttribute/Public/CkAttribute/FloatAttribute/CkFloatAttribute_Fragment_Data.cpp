#include "CkFloatAttribute_Fragment_Data.h"

#include "CkAttribute/CkAttribute_Log.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_FloatAttribute, TEXT("FloatAttribute"));

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
    FCk_Fragment_FloatAttribute_ParamsData::
    Get_MinValue() const
    -> float
{
    ck::attribute::ErrorIf(NOT (_MinMax == ECk_MinMax::Min || _MinMax == ECk_MinMax::MinMax),
        TEXT("Attempting to get a Min value of Attribute [{}] where MinMax is set to [{}]. Please address this."),
         _Name,
         _MinMax);

    return _MinValue;
}

auto
    FCk_Fragment_FloatAttribute_ParamsData::
    Get_MaxValue() const
    -> float
{
    ck::attribute::ErrorIf(NOT (_MinMax == ECk_MinMax::Max || _MinMax == ECk_MinMax::MinMax),
        TEXT("Attempting to get a Max value Attribute [{}] where MinMax is set to [{}]. Please address this."),
         _Name,
         _MinMax);

    return _MaxValue;
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
