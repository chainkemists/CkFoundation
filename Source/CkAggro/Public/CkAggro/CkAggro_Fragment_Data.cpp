#include "CkAggro_Fragment_Data.h"

#include "CkAggro/CkAggro_Log.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Aggro_FloatAttribute_Name, TEXT("FloatAttribute.Aggro.Score"));

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_Aggro_Params::
    Get_MinValue() const
    -> float
{
    ck::aggro::ErrorIf(NOT (_MinMax == ECk_MinMax::Min || _MinMax == ECk_MinMax::MinMax),
        TEXT("Attempting to get a Min value of Attribute [{}] where MinMax is set to [{}]. Please address this."),
        TAG_Aggro_FloatAttribute_Name,
        _MinMax);

    return _MinValue;
}

auto
    FCk_Fragment_Aggro_Params::
    Get_MaxValue() const
    -> float
{
    ck::aggro::ErrorIf(NOT (_MinMax == ECk_MinMax::Max || _MinMax == ECk_MinMax::MinMax),
        TEXT("Attempting to get a Max value Attribute [{}] where MinMax is set to [{}]. Please address this."),
        TAG_Aggro_FloatAttribute_Name,
        _MinMax);

    return _MaxValue;
}

// --------------------------------------------------------------------------------------------------------------------
