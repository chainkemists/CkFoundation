#pragma once

// Load-path hydration dispatcher — split out of Net/ReplicatedFragmentContainer/ (saveload-v3-ergonomics Phase 5).

#include "CkEcs/Persistence/CkPersistenceHydration.h"

#include "CkEcs/EntityScript/CkEntityScript_Processor.h" // FTag_EntityScript_ConstructedThisFrame
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCore/Time/CkTime.h" // FCk_Time (ApplyOne)

#include <Misc/Optional.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Scheduling-order dependency only (RunAfter): the hydration dispatcher runs LAST in FGroup_DeferredApply,
    // after the net dispatcher which STAYS in Net/ReplicatedFragmentContainer_Processor.h. Forward-declared to keep
    // Persistence free of a Net/ include; if the scheduler ever needs the complete type here, include the Net
    // processor header instead and note it "scheduling-order dependency only".
    class FProcessor_ReplicatedFragments_Dispatch;

    // Past this, an entry whose apply keeps returning NotReady is dropped LOUDLY — the feature it targets was never
    // composed. Mirrors the EntityScript pending-replication timeout (CkEntityScript_Processor.cpp). Shared by the
    // net dispatcher (Net/ReplicatedFragmentContainer_Processor.cpp) and the hydration ApplyOne below — defined here
    // so a single header definition serves both TUs (unity-build safe).
#if UE_BUILD_SHIPPING
    constexpr auto PendingApplyTimeoutSeconds = 2.0f;
#else
    constexpr auto PendingApplyTimeoutSeconds = 5.0f;
#endif

    // Load-path sibling of FProcessor_ReplicatedFragments_Dispatch: drains an entity's FFragment_PendingHydration
    // queue through the SAME handler HydrationApply, but runs on EVERY net mode (a loading standalone/authority world
    // hydrates locally, not just clients). Same Group as the net dispatcher (FGroup_DeferredApply).
    // The load path enqueues FFragment_PendingHydration entries; this processor drains them.
    class CKECS_API FProcessor_Hydration_Dispatch : public ck_exp::TProcessor<
        FProcessor_Hydration_Dispatch,
        FCk_Handle,
        ck::TReadWrite<FFragment_PendingHydration>,
        FTag_Hydration_PendingApply,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_DeferredApply;
        // Runs LAST among the FGroup_DeferredApply dispatchers so it can clear FTag_EntityScript_ConstructedThisFrame
        // after both have skipped this-frame-composed entities in the main pass.
        using RunAfter = TDepList<FProcessor_ReplicatedFragments_Dispatch>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::All;
        using MarkedDirtyBy = FTag_Hydration_PendingApply;
        static constexpr auto LoadPolicy = ECk_ProcessorLoadPolicy::RunsDuringLoad; // load-gate kernel

    public:
        using TProcessor::TProcessor;

    public:
        // Overrides the base view-iterating tick to clear FTag_EntityScript_ConstructedThisFrame registry-wide
        // AFTER the per-entity loop. Non-const — it mutates the registry.
        auto
        DoTick(
            TimeType InDeltaT) -> void;

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
    // Resolves InData's handler and calls HydrationApply. On NotReady, accumulates InOutPendingForSeconds; past the
    // shared 5s/2s window fires the loud ensure and reports DroppedTimeout. A missing/HydrationApply-less handler
    // (registry churn) reports DroppedTimeout. InOldData is the last-applied payload (unset on first apply).
    CKECS_API auto
    ApplyOne(
        FCk_Handle& InEntity,
        const FInstancedStruct& InData,
        const TOptional<FInstancedStruct>& InOldData,
        float& InOutPendingForSeconds,
        FCk_Time InDeltaT) -> EApplyOutcome;
}

// --------------------------------------------------------------------------------------------------------------------
