#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKATTRIBUTE_API FProcessor_VectorAttribute_FireSignals : public TProcessor_Attribute_FireSignals<FProcessor_VectorAttribute_FireSignals, FFragment_VectorAttribute, FCk_Delegate_VectorAttribute_OnValueChanged_MC>
    {
    public:
        using TProcessor_Attribute_FireSignals::TProcessor_Attribute_FireSignals;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_VectorAttribute_OverrideBaseValue : public TProcessor_Attribute_OverrideBaseValue<FProcessor_VectorAttribute_OverrideBaseValue, FFragment_VectorAttribute>
    {
    public:
        using TProcessor_Attribute_OverrideBaseValue::TProcessor_Attribute_OverrideBaseValue;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_VectorAttribute_RecomputeAll : public TProcessor_Attribute_RecomputeAll<FProcessor_VectorAttribute_RecomputeAll, FFragment_VectorAttributeModifier>
    {
    public:
        using TProcessor_Attribute_RecomputeAll::TProcessor_Attribute_RecomputeAll;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_VectorAttributeModifier_Additive_Teardown
        : public TProcessor_AttributeModifier_Additive_Teardown<FProcessor_VectorAttributeModifier_Additive_Teardown, FFragment_VectorAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Additive_Teardown::TProcessor_AttributeModifier_Additive_Teardown;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_VectorAttributeModifier_ComputeAll
        : public TProcessor_AttributeModifier_ComputeAll<FProcessor_VectorAttributeModifier_ComputeAll, FFragment_VectorAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_ComputeAll::TProcessor_AttributeModifier_ComputeAll;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_VectorAttributeModifier_Multiplicative_Teardown
        : public TProcessor_AttributeModifier_Multiplicative_Teardown<FProcessor_VectorAttributeModifier_Multiplicative_Teardown, FFragment_VectorAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Multiplicative_Teardown::TProcessor_AttributeModifier_Multiplicative_Teardown;
    };
}

// --------------------------------------------------------------------------------------------------------------------
