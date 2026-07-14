#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Fragment.h"


// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_RotatorAttribute_RecomputeAll : TProcessor_Attribute_RecomputeAll_CurrentMinMax<TFragment_RotatorAttributeModifier>
    {
        using TProcessor_Attribute_RecomputeAll_CurrentMinMax::TProcessor_Attribute_RecomputeAll_CurrentMinMax;
        using Group = FGroup_Gameplay;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_RotatorAttributeModifier_ComputeAll : TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<TFragment_RotatorAttributeModifier>
    {
        using TProcessor_AttributeModifier_ComputeAll_CurrentMinMax::TProcessor_AttributeModifier_ComputeAll_CurrentMinMax;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_RotatorAttribute_RecomputeAll>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_RotatorAttribute_MinMaxClamp : TProcessor_Attribute_MinMaxClamp<TFragment_RotatorAttribute>
    {
        using TProcessor_Attribute_MinMaxClamp::TProcessor_Attribute_MinMaxClamp;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_RotatorAttributeModifier_ComputeAll>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_RotatorAttribute_FireSignals : TProcessor_Attribute_FireSignals_CurrentMinMax<TFragment_RotatorAttribute, FCk_Delegate_RotatorAttribute_OnValueChanged>
    {
        using TProcessor_Attribute_FireSignals_CurrentMinMax::TProcessor_Attribute_FireSignals_CurrentMinMax;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_RotatorAttribute_MinMaxClamp>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_RotatorAttributeModifier_EndPlayAll = TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax<
        TFragment_RotatorAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_RotatorAttribute_Replicate = TProcessor_Attribute_Replicate_All<
        TFragment_RotatorAttribute, FCk_RepData_RotatorAttributes>;

    // --------------------------------------------------------------------------------------------------------------------

    // (FProcessor_RotatorAttribute_ReplicateOnRestore removed — restore re-seed is now the generic
    //  FProcessor_Persistence_ReDriveOnRestore + ck::attribute_restore Produce/SeedContainer.)

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
