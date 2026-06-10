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
    // Single client-side dispatch site for replicated fragment container entries whose handler
    // speaks the Apply/Remove contract. Net receive and driver link only mark entries pending
    // (FTag_RepFragments_PendingApply on the associated entity); this processor applies them.
    //
    // Scheduling contract (the load-bearing part):
    //   - runs AFTER FProcessor_EntityScript_FinishConstruction (same group), so OnConstructed-
    //     driven composition exists before the first dispatch, and
    //   - FGroup_Gameplay_Script precedes FGroup_Replication globally, so applied values are
    //     visible before FProcessor_ReplicationDriver_FireOnDependentReplicationComplete
    //     broadcasts OnReplicationComplete in the same frame.
    class CKECS_API FProcessor_ReplicatedFragments_Dispatch : public ck_exp::TProcessor<
        FProcessor_ReplicatedFragments_Dispatch,
        FCk_Handle,
        ck::TReadOnly<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>,
        FTag_RepFragments_PendingApply,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Script;
        using RunAfter = TDepList<FProcessor_EntityScript_FinishConstruction>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;
        using MarkedDirtyBy = FTag_RepFragments_PendingApply;

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
