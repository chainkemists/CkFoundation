#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment.h"

namespace ck { struct FProcessor_VectorAttribute_MinMaxClamp; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_IntegerAttribute_RecomputeAll : TProcessor_Attribute_RecomputeAll_CurrentMinMax<TFragment_IntegerAttributeModifier>
    {
        using TProcessor_Attribute_RecomputeAll_CurrentMinMax::TProcessor_Attribute_RecomputeAll_CurrentMinMax;
        using Group = FGroup_Gameplay;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_IntegerAttributeModifier_ComputeAll : TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<TFragment_IntegerAttributeModifier>
    {
        using TProcessor_AttributeModifier_ComputeAll_CurrentMinMax::TProcessor_AttributeModifier_ComputeAll_CurrentMinMax;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_IntegerAttribute_RecomputeAll>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_IntegerAttribute_MinMaxClamp : TProcessor_Attribute_MinMaxClamp<TFragment_IntegerAttribute>
    {
        using TProcessor_Attribute_MinMaxClamp::TProcessor_Attribute_MinMaxClamp;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_IntegerAttributeModifier_ComputeAll>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_IntegerAttribute_FireSignals : TProcessor_Attribute_FireSignals_CurrentMinMax<TFragment_IntegerAttribute, FCk_Delegate_IntegerAttribute_OnValueChanged>
    {
        using TProcessor_Attribute_FireSignals_CurrentMinMax::TProcessor_Attribute_FireSignals_CurrentMinMax;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_IntegerAttribute_MinMaxClamp, FProcessor_VectorAttribute_MinMaxClamp>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_IntegerAttributeModifier_EndPlayAll = TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax<
        TFragment_IntegerAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_IntegerAttribute_Replicate = TProcessor_Attribute_Replicate_All<
        TFragment_IntegerAttribute, FCk_RepData_IntegerAttributes>;

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_IntegerAttribute_Refill : TProcessor_Attribute_AccumulatedRefill<TFragment_IntegerAttributeModifier, TFragment_FloatAttribute>
    {
        using TProcessor_Attribute_AccumulatedRefill::TProcessor_Attribute_AccumulatedRefill;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_IntegerAttribute_FireSignals>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_IntegerAttribute_RetryPendingReplication
        : public ck_exp::TProcessor<FProcessor_IntegerAttribute_RetryPendingReplication, FCk_Handle,
            ck::TReadWrite<FFragment_IntegerAttribute_PendingReplicationEntries>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle InHandle,
            FFragment_IntegerAttribute_PendingReplicationEntries& InPending) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
