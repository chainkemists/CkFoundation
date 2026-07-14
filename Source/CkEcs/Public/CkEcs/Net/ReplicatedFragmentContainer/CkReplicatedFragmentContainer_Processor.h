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
    //   - lives in FGroup_DeferredApply, which runs after FGroup_Gameplay_Script (where
    //     FProcessor_EntityScript_FinishConstruction lives), so OnConstructed-driven composition
    //     exists before the first dispatch — no per-processor RunAfter needed, and
    //   - FGroup_DeferredApply precedes FGroup_Replication globally, so applied values are visible
    //     before FProcessor_ReplicationDriver_FireOnDependentReplicationComplete broadcasts
    //     OnReplicationComplete in the same frame (fire-gating additionally waits on the drain).
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
// The load-path hydration dispatcher (ck::FProcessor_Hydration_Dispatch) + ck::persistence_apply::ApplyOne moved to
// CkEcs/Persistence/CkPersistenceHydration_Processor.{h,cpp} (split, Phase 5). This header keeps ONLY the net-side
// FProcessor_ReplicatedFragments_Dispatch; the moved hydration dispatcher forward-declares + RunAfters it.

// --------------------------------------------------------------------------------------------------------------------
