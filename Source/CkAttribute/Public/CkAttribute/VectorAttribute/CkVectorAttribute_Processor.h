#pragma once

#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_VectorAttribute_RecomputeAll : TProcessor_Attribute_RecomputeAll_CurrentMinMax<TFragment_VectorAttributeModifier>
    {
        using TProcessor_Attribute_RecomputeAll_CurrentMinMax::TProcessor_Attribute_RecomputeAll_CurrentMinMax;
        using Group = FGroup_Gameplay;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_VectorAttributeModifier_ComputeAll : TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<TFragment_VectorAttributeModifier>
    {
        using TProcessor_AttributeModifier_ComputeAll_CurrentMinMax::TProcessor_AttributeModifier_ComputeAll_CurrentMinMax;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_VectorAttribute_RecomputeAll>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_VectorAttribute_MinMaxClamp : TProcessor_Attribute_MinMaxClamp<TFragment_VectorAttribute>
    {
        using TProcessor_Attribute_MinMaxClamp::TProcessor_Attribute_MinMaxClamp;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_VectorAttributeModifier_ComputeAll>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_VectorAttribute_FireSignals : TProcessor_Attribute_FireSignals_CurrentMinMax<TFragment_VectorAttribute, FCk_Delegate_VectorAttribute_OnValueChanged>
    {
        using TProcessor_Attribute_FireSignals_CurrentMinMax::TProcessor_Attribute_FireSignals_CurrentMinMax;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_VectorAttribute_MinMaxClamp>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_VectorAttributeModifier_EndPlayAll = TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax<
        TFragment_VectorAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_VectorAttribute_Replicate = TProcessor_Attribute_Replicate_All<
        TFragment_VectorAttribute, FCk_RepData_VectorAttributes>;

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_VectorAttribute_RetryPendingReplication
        : public ck_exp::TProcessor<FProcessor_VectorAttribute_RetryPendingReplication, FCk_Handle,
            ck::TReadWrite<FFragment_VectorAttribute_PendingReplicationEntries>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle InHandle,
            FFragment_VectorAttribute_PendingReplicationEntries& InPending) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
