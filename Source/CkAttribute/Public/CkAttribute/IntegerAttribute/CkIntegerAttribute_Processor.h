#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_IntegerAttribute_FireSignals = TProcessor_Attribute_FireSignals_CurrentMinMax<
        TFragment_IntegerAttribute, FCk_Delegate_IntegerAttribute_OnValueChanged_MC>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_IntegerAttribute_MinMaxClamp = TProcessor_Attribute_MinMaxClamp<TFragment_IntegerAttribute>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_IntegerAttribute_RecomputeAll = TProcessor_Attribute_RecomputeAll_CurrentMinMax<
        TFragment_IntegerAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_IntegerAttributeModifier_ComputeAll = TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<
        TFragment_IntegerAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_IntegerAttributeModifier_EndPlayAll = TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax<
        TFragment_IntegerAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_IntegerAttribute_Replicate = TProcessor_Attribute_Replicate_All<
        TFragment_IntegerAttribute, UCk_Fragment_IntegerAttribute_Rep>;

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
