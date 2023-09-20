#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKATTRIBUTE_API FProcessor_MeterAttribute_FireSignals : public TProcessor_Attribute_FireSignals<FProcessor_MeterAttribute_FireSignals, FFragment_MeterAttribute, FCk_Delegate_MeterAttribute_OnValueChanged_MC>
    {
    public:
        using TProcessor_Attribute_FireSignals::TProcessor_Attribute_FireSignals;
    };

    // --------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_MeterAttribute_RecomputeAll : public TProcessor_Attribute_RecomputeAll<FProcessor_MeterAttribute_RecomputeAll, FFragment_MeterAttributeModifier>
    {
    public:
        using TProcessor_Attribute_RecomputeAll::TProcessor_Attribute_RecomputeAll;
    };

    // --------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_MeterAttributeModifier_Additive_Compute
        : public TProcessor_AttributeModifier_Additive_Compute<FProcessor_MeterAttributeModifier_Additive_Compute, FFragment_MeterAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Additive_Compute::TProcessor_AttributeModifier_Additive_Compute;
    };

    // --------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_MeterAttributeModifier_Additive_Teardown
        : public TProcessor_AttributeModifier_Additive_Teardown<FProcessor_MeterAttributeModifier_Additive_Teardown, FFragment_MeterAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Additive_Teardown::TProcessor_AttributeModifier_Additive_Teardown;
    };

    // --------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_MeterAttributeModifier_Multiplicative_Compute
        : public TProcessor_AttributeModifier_Multiplicative_Compute<FProcessor_MeterAttributeModifier_Multiplicative_Compute, FFragment_MeterAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Multiplicative_Compute::TProcessor_AttributeModifier_Multiplicative_Compute;
    };

    // --------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_MeterAttributeModifier_Multiplicative_Teardown
        : public TProcessor_AttributeModifier_Multiplicative_Teardown<FProcessor_MeterAttributeModifier_Multiplicative_Teardown, FFragment_MeterAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Multiplicative_Teardown::TProcessor_AttributeModifier_Multiplicative_Teardown;
    };
}

// --------------------------------------------------------------------------------------------------------------------
