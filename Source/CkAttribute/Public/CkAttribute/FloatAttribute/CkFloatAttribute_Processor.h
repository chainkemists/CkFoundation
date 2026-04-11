#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"

namespace ck { struct FProcessor_VectorAttribute_MinMaxClamp; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_FloatAttribute_RecomputeAll : TProcessor_Attribute_RecomputeAll_CurrentMinMax<TFragment_FloatAttributeModifier>
    {
        using TProcessor_Attribute_RecomputeAll_CurrentMinMax::TProcessor_Attribute_RecomputeAll_CurrentMinMax;
        using Group = FGroup_Gameplay;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_FloatAttributeModifier_ComputeAll : TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<TFragment_FloatAttributeModifier>
    {
        using TProcessor_AttributeModifier_ComputeAll_CurrentMinMax::TProcessor_AttributeModifier_ComputeAll_CurrentMinMax;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_FloatAttribute_RecomputeAll>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_FloatAttribute_MinMaxClamp : TProcessor_Attribute_MinMaxClamp<TFragment_FloatAttribute>
    {
        using TProcessor_Attribute_MinMaxClamp::TProcessor_Attribute_MinMaxClamp;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_FloatAttributeModifier_ComputeAll>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_FloatAttribute_FireSignals : TProcessor_Attribute_FireSignals_CurrentMinMax<TFragment_FloatAttribute, FCk_Delegate_FloatAttribute_OnValueChanged>
    {
        using TProcessor_Attribute_FireSignals_CurrentMinMax::TProcessor_Attribute_FireSignals_CurrentMinMax;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_FloatAttribute_MinMaxClamp, FProcessor_VectorAttribute_MinMaxClamp>;
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttributeModifier_EndPlayAll = TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax<
        TFragment_FloatAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_FloatAttribute_Replicate = TProcessor_Attribute_Replicate_All<
        TFragment_FloatAttribute, FCk_RepData_FloatAttributes>;

    // --------------------------------------------------------------------------------------------------------------------

    struct FProcessor_FloatAttribute_Refill : TProcessor_Attribute_Refill<TFragment_FloatAttributeModifier>
    {
        using TProcessor_Attribute_Refill::TProcessor_Attribute_Refill;
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_FloatAttribute_FireSignals>;
    };

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
