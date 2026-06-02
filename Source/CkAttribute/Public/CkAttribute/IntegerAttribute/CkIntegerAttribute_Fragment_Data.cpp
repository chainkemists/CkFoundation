#include "CkIntegerAttribute_Fragment_Data.h"

#include "CkAttribute/CkAttribute_Log.h"

#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_IntegerAttribute_ParamsData);
CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_IntegerAttributeRefill_ParamsData);

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Label_IntegerAttribute, TEXT("IntegerAttribute"));

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_IntegerAttribute_ParamsData::
    Get_MinValue() const
    -> int32
{
    ck::attribute::ErrorIf(NOT (_MinMax == ECk_MinMax::Min || _MinMax == ECk_MinMax::MinMax),
        TEXT("Attempting to get a Min value of Attribute [{}] where MinMax is set to [{}]. Please address this."),
         _Name,
         _MinMax);

    return _MinValue;
}

auto
    FCk_Fragment_IntegerAttribute_ParamsData::
    Get_MaxValue() const
    -> int32
{
    ck::attribute::ErrorIf(NOT (_MinMax == ECk_MinMax::Max || _MinMax == ECk_MinMax::MinMax),
        TEXT("Attempting to get a Max value Attribute [{}] where MinMax is set to [{}]. Please address this."),
         _Name,
         _MinMax);

    return _MaxValue;
}

// --------------------------------------------------------------------------------------------------------------------
