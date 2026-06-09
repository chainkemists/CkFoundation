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

    // Re-drives replication of RESTORED Float-attribute values to clients after a snapshot load. The normal replication
    // trigger (FTag_MayRequireReplication) is transient and consumed before a save, so a restored attribute carries its
    // value but no trigger — the client keeps its Construct default. Keyed on ck::FTag_Snapshot_JustRestored (stamped by
    // CkSnapshot on every restored entity): once the owner's replication driver is re-established (snapshot respawn
    // pass), re-arm the per-component triggers so FProcessor_FloatAttribute_Replicate re-pushes the restored values,
    // then clear the marker. See ck::FTag_Snapshot_JustRestored — every replicated feature needs an analogous processor.
    //
    // The view iterates the clean Current fragment (default deletion policy, tombstone-free) and POINT-QUERIES the
    // marker inside ForEachEntity — a view that lists the in_place marker tag directly yields TOMBSTONE entities
    // (same trap FProcessor_ActorRespawn documents).
    class CKATTRIBUTE_API FProcessor_FloatAttribute_ReplicateOnRestore
        : public ck_exp::TProcessor<FProcessor_FloatAttribute_ReplicateOnRestore, FCk_Handle_FloatAttribute,
            ck::TReadOnly<FFragment_FloatAttribute_Current>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle_FloatAttribute InHandle,
            const FFragment_FloatAttribute_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
