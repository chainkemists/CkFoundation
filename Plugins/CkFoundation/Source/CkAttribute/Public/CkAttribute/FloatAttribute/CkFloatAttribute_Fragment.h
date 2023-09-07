#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include "CkCore/TypeConverter/CkTypeConverter.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKATTRIBUTE_API FFragment_FloatAttribute : public TFragment_Attribute<float>
    {
        using TFragment_Attribute::TFragment_Attribute;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKATTRIBUTE_API FFragment_FloatAttributeModifier : public TFragment_AttributeModifier<FFragment_FloatAttribute>
    {
        using TFragment_AttributeModifier::TFragment_AttributeModifier;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfFloatAttributes : public FFragment_RecordOfEntities {};

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TAttributeModifierOperators<float>
    {
        static auto Add(float InA, float InB) -> float
        {
            return InA + InB;
        };

        static auto Multiply(float InA, float InB) -> float
        {
            return InA * InB;
        };
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_FloatAttribute>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_FloatAttribute>& InPayload) const
        {
            return FCk_Payload_FloatAttribute_OnValueChanged
            {
                InPayload.Get_Handle(),
                InPayload.Get_BaseValue(),
                InPayload.Get_FinalValue()
            };
        }
    };

    using UUtils_Signal_UnrealMulticast_OnFloatAttributeValueChanged =
        TUtils_Signal_UnrealMulticast_OnAttributeValueChanged<ck::FFragment_FloatAttribute, FCk_Delegate_FloatAttribute_OnValueChanged_MC>;
}

// --------------------------------------------------------------------------------------------------------------------