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

    // --------------------------------------------------------------------------------------------------------------------

    // Load-path sibling of FProcessor_ReplicatedFragments_Dispatch: drains an entity's FFragment_PendingHydration
    // queue through the SAME handler Apply, but runs on EVERY net mode (a loading standalone/authority world
    // hydrates locally, not just clients). Same Group as the net dispatcher (they move together in Phase 2).
    // DORMANT in Phase 1: nothing enqueues FFragment_PendingHydration yet — Phase 3B's load path does.
    class CKECS_API FProcessor_Hydration_Dispatch : public ck_exp::TProcessor<
        FProcessor_Hydration_Dispatch,
        FCk_Handle,
        ck::TReadWrite<FFragment_PendingHydration>,
        FTag_Hydration_PendingApply,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Script;
        using RunAfter = TDepList<FProcessor_EntityScript_FinishConstruction>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::All;
        using MarkedDirtyBy = FTag_Hydration_PendingApply;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PendingHydration& InPending) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::persistence_apply
{
    // Result of applying one payload this tick.
    enum class EApplyOutcome : uint8 { Applied, StillPending, DroppedTimeout };

    // Shared handler-resolve + Apply + NotReady/timeout core used by FProcessor_Hydration_Dispatch (the net
    // dispatcher predates it and keeps its own inline copy — see CkReplicatedFragmentContainer_Processor.cpp).
    // Resolves InData's handler and calls Apply. On NotReady, accumulates InOutPendingForSeconds; past the shared
    // 5s/2s window fires the loud ensure and reports DroppedTimeout. A missing/Apply-less handler (registry churn)
    // reports DroppedTimeout. InOldData is the last-applied payload (unset on first apply).
    CKECS_API auto
    ApplyOne(
        FCk_Handle& InEntity,
        const FInstancedStruct& InData,
        const TOptional<FInstancedStruct>& InOldData,
        float& InOutPendingForSeconds,
        FCk_Time InDeltaT) -> EApplyOutcome;
}

// --------------------------------------------------------------------------------------------------------------------
