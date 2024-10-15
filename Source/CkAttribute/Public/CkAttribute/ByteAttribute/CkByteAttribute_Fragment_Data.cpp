#include "CkByteAttribute_Fragment_Data.h"

#include "CkAttribute/CkAttribute_Log.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_ByteAttribute, TEXT("ByteAttribute"));

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_ByteAttribute_ParamsData::
    Get_MinValue() const
    -> uint8
{
    ck::attribute::ErrorIf(NOT (_MinMax == ECk_MinMax::Min || _MinMax == ECk_MinMax::MinMax),
        TEXT("Attempting to get a Min value of Attribute [{}] where MinMax is set to [{}]. Please address this."),
         _Name,
         _MinMax);

    return _MinValue;
}

auto
    FCk_Fragment_ByteAttribute_ParamsData::
    Get_MaxValue() const
    -> uint8
{
    ck::attribute::ErrorIf(NOT (_MinMax == ECk_MinMax::Max || _MinMax == ECk_MinMax::MinMax),
        TEXT("Attempting to get a Max value Attribute [{}] where MinMax is set to [{}]. Please address this."),
         _Name,
         _MinMax);

    return _MaxValue;
}

// --------------------------------------------------------------------------------------------------------------------
