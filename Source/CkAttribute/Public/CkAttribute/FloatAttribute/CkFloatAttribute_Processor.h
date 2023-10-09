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

    class CKATTRIBUTE_API FProcessor_FloatAttributeModifier_RevokableAdditive_Compute
        : public TProcessor_AttributeModifier_RevokableAdditive_Compute<FProcessor_FloatAttributeModifier_RevokableAdditive_Compute, FFragment_FloatAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_RevokableAdditive_Compute::TProcessor_AttributeModifier_RevokableAdditive_Compute;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_FloatAttributeModifier_NotRevokableAdditive_Compute
        : public TProcessor_AttributeModifier_NotRevokableAdditive_Compute<FProcessor_FloatAttributeModifier_NotRevokableAdditive_Compute, FFragment_FloatAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_NotRevokableAdditive_Compute::TProcessor_AttributeModifier_NotRevokableAdditive_Compute;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_FloatAttributeModifier_Additive_Teardown
        : public TProcessor_AttributeModifier_Additive_Teardown<FProcessor_FloatAttributeModifier_Additive_Teardown, FFragment_FloatAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_Additive_Teardown::TProcessor_AttributeModifier_Additive_Teardown;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_FloatAttributeModifier_RevokableMultiplicative_Compute
        : public TProcessor_AttributeModifier_RevokableMultiplicative_Compute<FProcessor_FloatAttributeModifier_RevokableMultiplicative_Compute, FFragment_FloatAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_RevokableMultiplicative_Compute::TProcessor_AttributeModifier_RevokableMultiplicative_Compute;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_FloatAttributeModifier_NotRevokableMultiplicative_Compute
        : public TProcessor_AttributeModifier_NotRevokableMultiplicative_Compute<FProcessor_FloatAttributeModifier_NotRevokableMultiplicative_Compute, FFragment_FloatAttributeModifier>
    {
    public:
        using TProcessor_AttributeModifier_NotRevokableMultiplicative_Compute::TProcessor_AttributeModifier_NotRevokableMultiplicative_Compute;
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
