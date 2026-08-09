#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkIntent/CkIntentMatcher_Fragment.h"
#include "CkIntent/CkIntentSampler_Fragment_Data.h"
#include "CkIntent/CkIntent_ProcessorGroups.h"

#include <Templates/Function.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /**
     * The ONE thing in this module that can move a phase, and therefore the one place the transition signals are
     * broadcast.
     *
     * `FIntentMatcher_PhaseRow` names this type as the only friend that reaches its phase fields, so "a phase
     * cannot change without its signal" is enforced by the compiler rather than by everyone remembering. Every
     * caller — a wait opening, a move completing, a loser settling, a latch decaying, a set being swapped out —
     * comes through here.
     *
     * It is a class rather than a free function because friendship is granted to types, and a free function in a
     * helper namespace could not be given the access this needs without opening the row to the whole namespace.
     */
    class CKINTENT_API FIntentMatcher_PhaseWriter
    {
    public:
        /**
         * Moves one row to a phase and broadcasts the transition. A call that changes neither the phase nor the
         * frame is a no-op and is silent — a re-completion on a LATER frame is a real event and is not.
         *
         * The claim is cleared by every transition this performs: a claim belongs to one completion.
         */
        static auto
        Set_Phase(
            FCk_Handle_IntentMatcher InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            int32 InIntentIndex,
            ECk_Intent_Phase InNewPhase,
            int32 InFrame) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Everything the backward scan resolves once per pass instead of once per candidate.
     *
     * The layer's priority is carried rather than looked up because the visibility predicate asks for it on every
     * routed event of every row it walks, and a matcher's own priority cannot change while the pass runs. The
     * latest frame index is carried for the same reason and one more: a pending episode remembers the FRAME its
     * press landed on, and turning that back into a ring offset is a subtraction against exactly this number.
     */
    struct CKINTENT_API FIntentMatcher_ScanContext
    {
    public:
        CK_GENERATED_BODY(FIntentMatcher_ScanContext);

    private:
        FCk_Handle_IntentSampler _Sampler;

        FCk_Handle_InputButtonMap _ButtonMap;

        FCk_Handle_InputLayer _Layer;

        int32 _LayerPriority = 0;

        int32 _LatestFrameIndex = INDEX_NONE;

        int32 _LatchDecayFrames = 0;

        // Read ONCE per pass from the CVar rather than at each scan: the switch cannot change mid-tick, and a
        // per-scan console lookup would be the cost the opt-in exists to avoid.
        bool _RecordDiagnostics = false;

    public:
        CK_PROPERTY_GET(_Sampler);
        CK_PROPERTY_GET(_ButtonMap);
        CK_PROPERTY_GET(_Layer);
        CK_PROPERTY_GET(_LayerPriority);
        CK_PROPERTY_GET(_LatestFrameIndex);
        CK_PROPERTY_GET(_LatchDecayFrames);
        CK_PROPERTY_GET(_RecordDiagnostics);

    public:
        CK_DEFINE_CONSTRUCTORS(FIntentMatcher_ScanContext, _Sampler, _ButtonMap, _Layer, _LayerPriority, _LatestFrameIndex, _LatchDecayFrames, _RecordDiagnostics);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Set swaps drain here, in the first CkIntent group of the frame.
     *
     * The group is what makes the ordering a guarantee rather than a hope: the scan lives two groups later, so a set
     * activated this frame is the set this frame's scan runs, and a matcher can never match half of one set and half
     * of another. The capture edits a swap enqueues land on the layer one frame later — the ordinary deferred-edit
     * contract, since CkInput drains those before routing and this runs after it.
     */
    class CKINTENT_API FProcessor_IntentMatcher_HandleRequests : public ck_exp::TProcessor<
            FProcessor_IntentMatcher_HandleRequests,
            FCk_Handle_IntentMatcher,
            ck::TReadOnly<FFragment_IntentMatcher_Params>,
            ck::TReadWrite<FFragment_IntentMatcher_Current>,
            ck::TReadWrite<FFragment_IntentMatcher_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Intent_Collect;
        using MarkedDirtyBy = FFragment_IntentMatcher_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMatcher,
            const FFragment_IntentMatcher_Params& InParams,
            FFragment_IntentMatcher_Current& InCurrent,
            FFragment_IntentMatcher_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InMatcher,
            const FFragment_IntentMatcher_Params& InParams,
            FFragment_IntentMatcher_Current& InCurrent,
            const FCk_Request_IntentMatcher_SwapSet& InRequest) -> ECk_Request_OperationResult;
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CKINTENT_API FProcessor_IntentMatcher_CancelPendingRequests : public ck_exp::TProcessor<
            FProcessor_IntentMatcher_CancelPendingRequests,
            FCk_Handle_IntentMatcher,
            ck::TReadOnly<FFragment_IntentMatcher_Requests>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMatcher,
            const FFragment_IntentMatcher_Requests& InRequests) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Matches the active set against every record row written since the last pass, and carries pending presses
     * forward across them.
     *
     * UNRATED on purpose, while the record it reads advances at a fixed 60 Hz. Rating this to the same cadence would
     * put two independently-phased accumulators in the same pipeline and give the pair no defined alignment; reading
     * the record's own monotonic frame index instead is exact at every frame rate. Above 60 Hz most passes find no
     * new row and do nothing; below it several rows arrive at once and every one of them is processed, oldest first,
     * because a sequence's steps are ordered in time and so is a hold.
     *
     * Every row is processed in the same three moves: live episodes advance first (a chord's partner or a hold's
     * threshold may land on this row), then new press edges open episodes or resolve immediately, then any episode
     * still waiting on an intent that just completed elsewhere is settled. Nothing else owns pending resolution —
     * there is no second processor and no group of its own, because an episode is a fact about rows and this is the
     * one place rows are read.
     *
     * Capture re-resolution rides the same pass. The button map re-derives without telling anyone, so the only way to
     * notice a rebind is to compare the map's current answer against what the matcher last registered — a handful of
     * comparisons per matcher, and deliberately not a delegate: a processor binding a UObject delegate would own a
     * lifetime it has no way to end.
     */
    class CKINTENT_API FProcessor_IntentMatcher_Match : public ck_exp::TProcessor<
            FProcessor_IntentMatcher_Match,
            FCk_Handle_IntentMatcher,
            ck::TReadOnly<FFragment_IntentMatcher_Params>,
            ck::TReadWrite<FFragment_IntentMatcher_Current>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Intent_Match;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMatcher,
            const FFragment_IntentMatcher_Params& InParams,
            FFragment_IntentMatcher_Current& InCurrent) const -> void;

    private:
        static auto
        DoRefreshCaptureResolutions(
            FCk_Handle_InputLayer& InLayer,
            const FCk_Handle_InputButtonMap& InButtonMap,
            const FFragment_IntentMatcher_Params& InParams,
            FFragment_IntentMatcher_Current& InCurrent) -> void;

        static auto
        DoProcessRow(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            int32 InOffset) -> void;

        static auto
        DoScanRowForNewPresses(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            const FCk_Intent_FrameRecord& InRow,
            int32 InOffset,
            const TArray<FCk_Input_ButtonId>& InButtonsSpokenFor,
            TArray<int32>& OutCompletedThisRow) -> void;

        // Answers whether the episode is FINISHED and should be dropped by the caller — the caller owns the array,
        // so nothing here can remove an entry out from under an iteration.
        static auto
        DoAdvanceEpisode(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            const FCk_Intent_FrameRecord& InRow,
            int32 InOffset,
            FIntentMatcher_PendingEpisode& InEpisode,
            TArray<int32>& OutCompletedThisRow) -> bool;

        static auto
        DoTryResolveEpisode(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            const FIntentMatcher_PendingEpisode& InEpisode,
            TFunctionRef<bool(const FCk_Intent_CompiledIntent&)> InCandidateFilter,
            int32 InTerminalOffset,
            int32 InResolutionFrame,
            TArray<int32>& OutCompletedThisRow) -> bool;

        static auto
        DoOpenEpisode(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FCk_Intent_FrameRecord& InRow,
            const FCk_Input_ButtonId& InButton,
            const FCk_Intent_DeferralVerdict& InVerdict,
            const TArray<int32>& InCandidates) -> void;

        static auto
        DoPurgeEpisodesResolvedElsewhere(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const TArray<int32>& InCompletedThisRow,
            int32 InFrame) -> void;

        static auto
        DoCompleteIntent(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            int32 InIntentIndex,
            int32 InFrame,
            TArray<int32>& OutCompletedThisRow) -> void;

        static auto
        DoFailEpisode(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_PendingEpisode& InEpisode,
            int32 InFrame) -> void;

        // Candidates that were waiting and did not win go back to Idle rather than to Failed: Failed is reserved
        // for an ambiguity that answered NOTHING, and marking every loser would make a tap press permanently
        // brand its hold sibling.
        static auto
        DoSettleLosingRows(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_PendingEpisode& InEpisode,
            int32 InFrame) -> void;

        static auto
        DoAdvanceHoldAccumulator(
            FFragment_IntentMatcher_Current& InCurrent,
            const FCk_Input_ButtonId& InButton) -> void;

        static auto
        DoDropHoldAccumulator(
            FFragment_IntentMatcher_Current& InCurrent,
            const FCk_Input_ButtonId& InButton) -> void;

        static auto
        DoGet_AccumulatedHoldFrames(
            const FFragment_IntentMatcher_Current& InCurrent,
            const FCk_Input_ButtonId& InButton) -> int32;

        // Runs LAST on every row, so a completion stamped this frame is never decayed by the same pass that
        // produced it, and a decay is evaluated against the frame it actually falls on rather than against
        // whichever frame the renderer happened to catch up on.
        // Runs the scan and, when diagnostics are armed, files what it saw. Every scan site goes through here so
        // the ring cannot end up describing only some of them.
        static auto
        DoRunScan(
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            const FCk_Intent_CompiledIntent& InIntent,
            int32 InTerminalOffset) -> bool;

        static auto
        DoPushScanDiagnostic(
            FFragment_IntentMatcher_Current& InCurrent,
            FCk_Intent_ScanDiagnostic InDiagnostic) -> void;

        static auto
        DoDecayLatches(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            int32 InFrame) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
