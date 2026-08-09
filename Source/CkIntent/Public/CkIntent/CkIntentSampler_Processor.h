#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkInput/CkInputLayer_Fragment.h"

#include "CkIntent/CkIntentSampler_Fragment.h"
#include "CkIntent/CkIntent_ProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /**
     * Buffers every render frame's routed events for the next logic frame to claim.
     *
     * UNRATED on purpose: the router's retention is per-render-frame state and is cleared at the top of the next
     * Route pass, so anything that misses a render frame misses those events for good. This runs on all of them.
     *
     * The view requires the sampler's own fragments, so a source with no sampler composed is never visited and
     * the whole mechanism costs nothing on it.
     */
    class CKINTENT_API FProcessor_IntentSampler_Collect : public ck_exp::TProcessor<
            FProcessor_IntentSampler_Collect,
            FCk_Handle_IntentSampler,
            ck::TReadOnly<FFragment_IntentSampler_Params>,
            ck::TReadWrite<FFragment_IntentSampler_PendingEvents>,
            ck::TReadOnly<FFragment_InputLayer_RoutedThisFrame>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Intent_Collect;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InSampler,
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_PendingEvents& InPending,
            const FFragment_InputLayer_RoutedThisFrame& InRouted) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Appends exactly one record row per logic frame.
     *
     * The cadence is fixed rather than per-render-frame so a recorded pattern means the same thing on every
     * machine: a two-frame tap is two rows at 60 Hz whether the renderer is running at 30 or at 240. Catch-up
     * therefore stays on ReplayMissedTicks (the default) — a sampler that collapsed a hitch into one row would
     * lose every press inside it — but it is BOUNDED, because an unbounded replay would append a second of rows
     * in one frame and hand every consumer above a burst it has no way to pace.
     *
     * Input arrives through the pending buffer rather than the router's retention directly, which is what
     * decouples the record from the frame rate in both directions. Taking ALL of the buffer and clearing is also
     * what makes a replayed catch-up tick harmless: the first tick of the burst has already emptied it.
     */
    class CKINTENT_API FProcessor_IntentSampler_Sample : public ck_exp::TProcessor<
            FProcessor_IntentSampler_Sample,
            FCk_Handle_IntentSampler,
            ck::TReadOnly<FFragment_IntentSampler_Params>,
            ck::TReadWrite<FFragment_IntentSampler_Current>,
            ck::TReadWrite<FFragment_IntentSampler_PendingEvents>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Intent_Sample;

        static constexpr FCk_Time TickRate = ck::time::Hz(60);

        // Four frames of catch-up: enough that an ordinary 30 Hz frame still replays both of its logic steps,
        // small enough that a stall cannot deliver a quarter-second of history in one go.
        static constexpr int32 MaxReplayedTicks = 4;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InSampler,
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_Current& InCurrent,
            FFragment_IntentSampler_PendingEvents& InPending) const -> void;

    private:
        static auto
        DoRecordButtons(
            HandleType InSampler,
            FFragment_IntentSampler_Current& InCurrent,
            const TArray<FCk_InputLayer_RoutedEvent>& InClaimed,
            FCk_Intent_FrameRecord& OutRow) -> void;

        static auto
        DoRecordAxes(
            HandleType InSampler,
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_Current& InCurrent,
            const TArray<FCk_InputLayer_RoutedEvent>& InClaimed,
            FCk_Intent_FrameRecord& OutRow) -> void;

        static auto
        DoRecordOctant(
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_Current& InCurrent,
            FCk_Intent_FrameRecord& OutRow) -> void;

        static auto
        DoRecordSocd(
            HandleType InSampler,
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_Current& InCurrent,
            const TArray<FCk_InputLayer_RoutedEvent>& InClaimed,
            FCk_Intent_FrameRecord& OutRow) -> void;

        static auto
        DoAppendRow(
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_Current& InCurrent,
            const FCk_Intent_FrameRecord& InRow) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
