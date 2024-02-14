#pragma once

#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_VectorAttribute_FireSignals = TProcessor_Attribute_FireSignals_CurrentMinMax<
        TFragment_VectorAttribute, FCk_Delegate_VectorAttribute_OnValueChanged_MC>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_VectorAttribute_MinMaxClamp = TProcessor_Attribute_MinMaxClamp<TFragment_VectorAttribute>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_VectorAttribute_RecomputeAll = TProcessor_Attribute_RecomputeAll_CurrentMinMax<
        TFragment_VectorAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_VectorAttributeModifier_ComputeAll = TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<
        TFragment_VectorAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_VectorAttributeModifier_TeardownAll = TProcessor_AttributeModifier_TeardownAll_CurrentMinMax<
        TFragment_VectorAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
