#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_FireSignals = TProcessor_Attribute_FireSignals_CurrentMinMax<
        TFragment_FloatAttribute, FCk_Delegate_FloatAttribute_OnValueChanged>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_MinMaxClamp = TProcessor_Attribute_MinMaxClamp<TFragment_FloatAttribute>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_RecomputeAll = TProcessor_Attribute_RecomputeAll_CurrentMinMax<
        TFragment_FloatAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttributeModifier_ComputeAll = TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<
        TFragment_FloatAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttributeModifier_EndPlayAll = TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax<
        TFragment_FloatAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_Replicate = TProcessor_Attribute_Replicate_All<
        TFragment_FloatAttribute, FCk_RepData_FloatAttributes>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_Refill = TProcessor_Attribute_Refill<TFragment_FloatAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_FloatAttribute_RetryPendingReplication
        : public ck_exp::TProcessor<FProcessor_FloatAttribute_RetryPendingReplication, FCk_Handle,
            ck::TReadWrite<FFragment_FloatAttribute_PendingReplicationEntries>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle InHandle,
            FFragment_FloatAttribute_PendingReplicationEntries& InPending) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
