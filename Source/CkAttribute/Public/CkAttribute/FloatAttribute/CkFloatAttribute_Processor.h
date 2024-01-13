#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKATTRIBUTE_API FProcessor_FloatAttribute_FireSignals : public TProcessor_Attribute_FireSignals<FProcessor_FloatAttribute_FireSignals, FFragment_FloatAttribute, FCk_Delegate_FloatAttribute_OnValueChanged_MC>
    {
    public:
        using TProcessor_Attribute_FireSignals::TProcessor_Attribute_FireSignals;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_FloatAttribute_RecomputeAll : public TProcessor_Attribute_RecomputeAll<FProcessor_FloatAttribute_RecomputeAll, FFragment_FloatAttributeModifier>
    {
    public:
        using TProcessor_Attribute_RecomputeAll::TProcessor_Attribute_RecomputeAll;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_FloatAttributeModifier_Additive_Teardown
        : public TProcessor_AttributeModifier_Additive_Teardown<FProcessor_FloatAttributeModifier_Additive_Teardown, FFragment_FloatAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Additive_Teardown::TProcessor_AttributeModifier_Additive_Teardown;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_FloatAttributeModifier_ComputeAll
        : public TProcessor_AttributeModifier_ComputeAll<FProcessor_FloatAttributeModifier_ComputeAll, FFragment_FloatAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_ComputeAll::TProcessor_AttributeModifier_ComputeAll;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_FloatAttributeModifier_Multiplicative_Teardown
        : public TProcessor_AttributeModifier_Multiplicative_Teardown<FProcessor_FloatAttributeModifier_Multiplicative_Teardown, FFragment_FloatAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Multiplicative_Teardown::TProcessor_AttributeModifier_Multiplicative_Teardown;
    };
}

// --------------------------------------------------------------------------------------------------------------------
