#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include "CkCore/TypeConverter/CkTypeConverter.h"
#include "CkSignal/CkSignal_Macros.h"

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

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL(CKATTRIBUTE_API, OnFloatAttributeValueChanged, FCk_Handle, FCk_Payload_FloatAttribute_OnValueChanged);
    CK_DEFINE_SIGNAL_UTILS(CKATTRIBUTE_API, OnFloatAttributeValueChanged);
    CK_DEFINE_SIGNAL_WITH_DELEGATE(CKATTRIBUTE_API, OnFloatAttributeValueChanged, FCk_Delegate_FloatAttribute_OnValueChanged_MC, FCk_Handle, FCk_Payload_FloatAttribute_OnValueChanged);
    CK_DEFINE_SIGNAL_WITH_DELEGATE_UTILS(CKATTRIBUTE_API, OnFloatAttributeValueChanged);
}

// --------------------------------------------------------------------------------------------------------------------