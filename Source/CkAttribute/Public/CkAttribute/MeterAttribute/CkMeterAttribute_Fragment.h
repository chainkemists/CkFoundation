#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment_Data.h"

#include "CkCore/Meter/CkMeter.h"
#include "CkCore/TypeConverter/CkTypeConverter.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKATTRIBUTE_API FFragment_MeterAttribute : public TFragment_Attribute<FCk_Meter>
    {
        using TFragment_Attribute::TFragment_Attribute;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKATTRIBUTE_API FFragment_MeterAttributeModifier : public TFragment_AttributeModifier<FFragment_MeterAttribute>
    {
        using TFragment_AttributeModifier::TFragment_AttributeModifier;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfMeterAttributes : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TAttributeModifierOperators<FCk_Meter>
    {
        static auto Add(FCk_Meter InA, FCk_Meter InB) -> FCk_Meter
        {
            return InA + InB;
        };

        static auto Multiply(FCk_Meter InA, FCk_Meter InB) -> FCk_Meter
        {
            return InA * InB;
        };
    };


    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_MeterAttribute>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_MeterAttribute>& InPayload) const
        {
            return FCk_Payload_MeterAttribute_OnValueChanged
            {
                InPayload.Get_BaseValue(),
                InPayload.Get_FinalValue()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    using UUtils_Signal_OnMeterAttributeValueChanged = TUtils_Signal_OnAttributeValueChanged<
        FFragment_MeterAttribute, FCk_Delegate_MeterAttribute_OnValueChanged_MC>;
}

// --------------------------------------------------------------------------------------------------------------------