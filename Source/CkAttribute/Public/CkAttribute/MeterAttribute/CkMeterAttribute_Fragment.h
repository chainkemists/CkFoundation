#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment_Data.h"

#include "CkCore/Meter/CkMeter.h"
#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_Meter_MinValue);
    CK_DEFINE_ECS_TAG(FTag_Meter_MaxValue);
    CK_DEFINE_ECS_TAG(FTag_Meter_CurrentValue);
    CK_DEFINE_ECS_TAG(FTag_Meter_RequiresUpdate);

    // --------------------------------------------------------------------------------------------------------------------

    /*
     * Notes: Meter Attribute, at the moment, is a 'Meta Feature' where it uses Float Attributes internally.
     * This means that there are no Attribute Fragments for the Meter itself. To allow us to work with
     * multiple Meters though, we do need a RecordOfMeterAttributes and a Signal for Attribute Value Changed
     */

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfMeterAttributes, FCk_Handle_FloatAttributeOwner);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKATTRIBUTE_API, OnMeterAttributeValueChanged,
        FCk_Delegate_MeterAttribute_OnValueChanged_MC, FCk_Handle, FCk_Payload_MeterAttribute_OnValueChanged);
}

// --------------------------------------------------------------------------------------------------------------------