#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKATTRIBUTE_API FProcessor_ByteAttribute_FireSignals : public TProcessor_Attribute_FireSignals<FProcessor_ByteAttribute_FireSignals, FFragment_ByteAttribute, FCk_Delegate_ByteAttribute_OnValueChanged_MC>
    {
    public:
        using TProcessor_Attribute_FireSignals::TProcessor_Attribute_FireSignals;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_ByteAttribute_RecomputeAll : public TProcessor_Attribute_RecomputeAll<FProcessor_ByteAttribute_RecomputeAll, FFragment_ByteAttributeModifier>
    {
    public:
        using TProcessor_Attribute_RecomputeAll::TProcessor_Attribute_RecomputeAll;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_ByteAttributeModifier_Additive_Teardown
        : public TProcessor_AttributeModifier_Additive_Teardown<FProcessor_ByteAttributeModifier_Additive_Teardown, FFragment_ByteAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Additive_Teardown::TProcessor_AttributeModifier_Additive_Teardown;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_ByteAttributeModifier_ComputeAll
        : public TProcessor_AttributeModifier_ComputeAll<FProcessor_ByteAttributeModifier_ComputeAll, FFragment_ByteAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_ComputeAll::TProcessor_AttributeModifier_ComputeAll;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_ByteAttributeModifier_Multiplicative_Teardown
        : public TProcessor_AttributeModifier_Multiplicative_Teardown<FProcessor_ByteAttributeModifier_Multiplicative_Teardown, FFragment_ByteAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Multiplicative_Teardown::TProcessor_AttributeModifier_Multiplicative_Teardown;
    };
}

// --------------------------------------------------------------------------------------------------------------------
