#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_FireSignals = TProcessor_Attribute_FireSignals_CurrentMinMax<
        TFragment_FloatAttribute, FCk_Delegate_FloatAttribute_OnValueChanged_MC>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_MinMaxClamp = TProcessor_Attribute_MinMaxClamp<TFragment_FloatAttribute>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_RecomputeAll = TProcessor_Attribute_RecomputeAll_CurrentMinMax<
        TFragment_FloatAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttributeModifier_ComputeAll = TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<
        TFragment_FloatAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttributeModifier_TeardownAll = TProcessor_AttributeModifier_TeardownAll_CurrentMinMax<
        TFragment_FloatAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_Replicate = TProcessor_Attribute_Replicate_All<
        TFragment_FloatAttribute, UCk_Fragment_FloatAttribute_Rep>;

        // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_Refill = TProcessor_Attribute_Refill<TFragment_FloatAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
