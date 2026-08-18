#pragma once

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
    // Scheduling-order dependency only (RunAfter) — forward-declared to keep Persistence free of a Net/ include.
    class FProcessor_ReplicatedFragments_Dispatch;

    // Past this, an entry whose apply keeps returning NotReady is dropped LOUDLY — the feature it targets was
    // never composed. Defined here so one header definition serves both dispatcher TUs (unity-build safe).
#if UE_BUILD_SHIPPING
    constexpr auto PendingApplyTimeoutSeconds = 2.0f;
#else
    constexpr auto PendingApplyTimeoutSeconds = 5.0f;
#endif

    // Load-path sibling of FProcessor_ReplicatedFragments_Dispatch (same Group), draining FFragment_PendingHydration
    // through HydrationApply. Runs on EVERY net mode — a loading standalone/authority world hydrates locally too.
    class CKECS_API FProcessor_Hydration_Dispatch : public ck_exp::TProcessor<
        FProcessor_Hydration_Dispatch,
        FCk_Handle,
        ck::TReadWrite<FFragment_PendingHydration>,
        FTag_Hydration_PendingApply,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_DeferredApply;
        // Last of the FGroup_DeferredApply dispatchers, so it owns clearing FTag_EntityScript_ConstructedThisFrame.
        using RunAfter = TDepList<FProcessor_ReplicatedFragments_Dispatch>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::All;
        using MarkedDirtyBy = FTag_Hydration_PendingApply;
        static constexpr auto LoadPolicy = ECk_ProcessorLoadPolicy::RunsDuringLoad; // load-gate kernel
        static constexpr auto HydrationQuarantinePolicy = ECk_ProcessorHydrationQuarantine::Exempt; // load-gate kernel

    public:
        using TProcessor::TProcessor;

    public:
        // Non-const: clears FTag_EntityScript_ConstructedThisFrame registry-wide after the per-entity loop.
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
    enum class EApplyOutcome : uint8 { Applied, StillPending, DroppedRejected, DroppedTimeout, DroppedNoHandler };

    // Resolves InData's handler and calls HydrationApply. Rejected is terminal immediately; NotReady accumulates
    // InOutPendingForSeconds and past PendingApplyTimeoutSeconds fires the loud ensure and reports DroppedTimeout.
    //
    // A payload that resolves NO handler, or one whose handler never declared a HydrationApply, is
    // DroppedNoHandler — its own outcome rather than DroppedTimeout, because nothing ever waited: the save
    // recorded state this build cannot apply, so it is data loss, terminal on the first call, and ensures on the
    // spot. (The net dispatcher predates this and keeps its own copy.)
    CKECS_API auto
    ApplyOne(
        FCk_Handle& InEntity,
        const FInstancedStruct& InData,
        const TOptional<FInstancedStruct>& InOldData,
        float& InOutPendingForSeconds,
        FCk_Time InDeltaT) -> EApplyOutcome;
}

// --------------------------------------------------------------------------------------------------------------------
