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

    struct FTagMeter_MinValue {};
    struct FTagMeter_MaxValue {};
    struct FTagMeter_CurrentValue {};
    struct FTagMeter_RequiresUpdate {};

    // --------------------------------------------------------------------------------------------------------------------

    /*
     * Notes: Meter Attribute, at the moment, is a 'Meta Feature' where it uses Float Attributes internally.
     * This means that there are no Attribute Fragments for the Meter itself. To allow us to work with
     * multiple Meters though, we do need a RecordOfMeterAttributes and a Signal for Attribute Value Changed
     */

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfMeterAttributes : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKATTRIBUTE_API, OnMeterAttributeValueChanged,
        FCk_Delegate_MeterAttribute_OnValueChanged_MC, FCk_Handle, FCk_Payload_MeterAttribute_OnValueChanged);
}

// --------------------------------------------------------------------------------------------------------------------