#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttribute_FireSignals = TProcessor_Attribute_FireSignals_CurrentMinMax<
        TFragment_ByteAttribute, FCk_Delegate_ByteAttribute_OnValueChanged_MC>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttribute_MinMaxClamp = TProcessor_Attribute_MinMaxClamp<TFragment_ByteAttribute>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttribute_RecomputeAll = TProcessor_Attribute_RecomputeAll_CurrentMinMax<
        TFragment_ByteAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttributeModifier_ComputeAll = TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<
        TFragment_ByteAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttributeModifier_TeardownAll = TProcessor_AttributeModifier_TeardownAll_CurrentMinMax<
        TFragment_ByteAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttribute_Replicate = TProcessor_Attribute_Replicate_All<
        TFragment_ByteAttribute, UCk_Fragment_ByteAttribute_Rep>;

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
