#pragma once

#include "CkEcs/EntityScript/CkEntityScript_Processor.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Single client-side dispatch site: net receive and driver link only mark entries pending
    // (FTag_RepFragments_PendingApply); this applies them. Its FGroup_DeferredApply placement is the
    // load-bearing part (after OnConstructed composition, before FGroup_Replication) — CkEcs/CLAUDE.md.
    class CKECS_API FProcessor_ReplicatedFragments_Dispatch : public ck_exp::TProcessor<
        FProcessor_ReplicatedFragments_Dispatch,
        FCk_Handle,
        ck::TReadOnly<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>,
        FTag_RepFragments_PendingApply,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_DeferredApply;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;
        using MarkedDirtyBy = FTag_RepFragments_PendingApply;
        static constexpr auto LoadPolicy = ECk_ProcessorLoadPolicy::RunsDuringLoad; // load-gate kernel

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>& InDriver) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
