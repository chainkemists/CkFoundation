#pragma once

#include "CkDynamic/CkDynamic_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ---- Replicate (Server-side) ----

    // Pushes the current payload of every replicated dynamic fragment on the entity into the
    // replication driver. Runs on authority, gated by FTag_DynamicFragment_MayRequireReplication
    // (set on Add-with-Replicates and by Request_MarkReplicationDirty). The tag is cleared per-entity
    // only once the push succeeds, so an entity that opted in before its driver existed keeps retrying.
    class CKDYNAMIC_API FProcessor_DynamicFragment_Replicate : public ck_exp::TProcessor<
            FProcessor_DynamicFragment_Replicate,
            FCk_Handle,
            ck::TReadOnly<FFragment_DynamicFragment_ReplicatedTypes>,
            FTag_DynamicFragment_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_DynamicFragment_MayRequireReplication;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_DynamicFragment_ReplicatedTypes& InReplicatedTypes) const -> void;
    };

}

// --------------------------------------------------------------------------------------------------------------------
