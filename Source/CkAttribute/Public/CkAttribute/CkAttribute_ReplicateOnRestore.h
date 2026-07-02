#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Per-load done marker for the attribute ReplicateOnRestore pass. TRANSIENT (never captured) and paired with the
    // shared FTag_Snapshot_JustRestored, which this pass must NOT remove — the shared marker is a multi-consumer
    // signal (SM redrive, BB rebind processors, other ReplicateOnRestore passes key on it too).
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_Attribute_RestoreReplicated);

    // ================================================================================================================
    // REPLICATE ON RESTORE — Re-drives replication of RESTORED attribute values to clients after a
    // snapshot load. One template for the whole attribute family (Float/Byte/Integer/Rotator/Vector);
    // per-type instantiations are `using` aliases in each Ck<T>Attribute_Processor.h, registered in the
    // matching .cpp.
    //
    // The normal replication trigger (FTag_MayRequireReplication) is transient and consumed before a
    // save, so a restored attribute carries its value but no trigger — the client would keep its
    // Construct default. Keyed on ck::FTag_Snapshot_JustRestored (stamped by CkSnapshot on every
    // restored entity): once the owner's replication driver is re-established (snapshot respawn pass),
    // re-create the per-owner container entry (Construct is abstained during reconstitution, so the
    // entry Add() normally makes is missing and the Replicate processor's UPDATE path would fail) and
    // re-arm the per-component triggers so the Replicate processor re-pushes the restored values.
    //
    // The view iterates the clean Current fragment (default deletion policy, tombstone-free) and
    // POINT-QUERIES the marker inside ForEachEntity — a view that lists the in_place marker tag
    // directly yields TOMBSTONE entities (same trap FProcessor_ActorRespawn documents).
    // ================================================================================================================

    template <typename T_HandleType, template <ECk_MinMaxCurrent> class T_DerivedAttribute, typename T_RepDataStruct>
    class TProcessor_Attribute_ReplicateOnRestore_All : public ck_exp::TProcessor<
        TProcessor_Attribute_ReplicateOnRestore_All<T_HandleType, T_DerivedAttribute, T_RepDataStruct>,
        T_HandleType,
        TReadOnly<T_DerivedAttribute<ECk_MinMaxCurrent::Current>>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using FragmentType_Current = T_DerivedAttribute<ECk_MinMaxCurrent::Current>;
        using FragmentType_Min     = T_DerivedAttribute<ECk_MinMaxCurrent::Min>;
        using FragmentType_Max     = T_DerivedAttribute<ECk_MinMaxCurrent::Max>;

        // Dependent base — the injected name `TProcessor` is not visible through it, so spell it out.
        using TSuper = ck_exp::TProcessor<
            TProcessor_Attribute_ReplicateOnRestore_All<T_HandleType, T_DerivedAttribute, T_RepDataStruct>,
            T_HandleType,
            TReadOnly<FragmentType_Current>,
            CK_IGNORE_PENDING_KILL>;
        using TimeType = FCk_Time;

    public:
        using Group = FGroup_Gameplay;
        using TSuper::TSuper;

    public:
        auto
        ForEachEntity(
            TimeType /*InDeltaT*/,
            T_HandleType InHandle,
            const FragmentType_Current& /*InCurrent*/) const -> void
        {
            if (NOT InHandle.template Has<FTag_Snapshot_JustRestored>())
            { return; }

            // Done-guard: our OWN transient marker, not removal of the shared JustRestored (which other restore
            // consumers on this entity may still need to observe).
            if (InHandle.template Has<FTag_Attribute_RestoreReplicated>())
            { return; }

            auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

            // Re-arm only once the owner's replication driver is re-established by the snapshot respawn
            // pass — otherwise the downstream Replicate processor's container update would target a
            // missing driver. Leaving both markers untouched retries on the next tick until it exists.
            if (NOT UCk_Utils_EntityReplicationDriver_UE::Has(LifetimeOwner))
            { return; }

            UCk_Utils_Net_UE::TryAddContainerFragment<T_RepDataStruct>(LifetimeOwner);

            InHandle.template AddOrGet<typename FragmentType_Current::FTag_MayRequireReplication>();
            if (InHandle.template Has<FragmentType_Min>())
            { InHandle.template AddOrGet<typename FragmentType_Min::FTag_MayRequireReplication>(); }
            if (InHandle.template Has<FragmentType_Max>())
            { InHandle.template AddOrGet<typename FragmentType_Max::FTag_MayRequireReplication>(); }

            InHandle.template Add<FTag_Attribute_RestoreReplicated>();
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------
