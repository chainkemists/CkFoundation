#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttribute_FireSignals = TProcessor_Attribute_FireSignals_CurrentMinMax<
        TFragment_ByteAttribute, FCk_Delegate_ByteAttribute_OnValueChanged>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttribute_MinMaxClamp = TProcessor_Attribute_MinMaxClamp<TFragment_ByteAttribute>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttribute_RecomputeAll = TProcessor_Attribute_RecomputeAll_CurrentMinMax<
        TFragment_ByteAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttributeModifier_ComputeAll = TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<
        TFragment_ByteAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttributeModifier_EndPlayAll = TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax<
        TFragment_ByteAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_ByteAttribute_Replicate = TProcessor_Attribute_Replicate_All<
        TFragment_ByteAttribute, FCk_RepData_ByteAttributes>;

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_ByteAttribute_RetryPendingReplication
        : public ck_exp::TProcessor<FProcessor_ByteAttribute_RetryPendingReplication, FCk_Handle,
            ck::TReadWrite<FFragment_ByteAttribute_PendingReplicationEntries>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle InHandle,
            FFragment_ByteAttribute_PendingReplicationEntries& InPending) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
