#include "CkIntentMatcher_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Payload/CkPayload.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkInput/CkInputButtonMap_Utils.h"
#include "CkInput/CkInputLayer_Utils.h"

#include "CkIntent/CkIntentGrammar_Utils.h"
#include "CkIntent/CkIntentMatcher_Utils.h"
#include "CkIntent/CkIntentSampler_Utils.h"
#include "CkIntent/CkIntent_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_GROUP(ck::FGroup_Intent_Match);

CK_REGISTER_PROCESSOR(ck::FProcessor_IntentMatcher_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntentMatcher_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntentMatcher_Match);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_intent_matcher_processor
{
    /**
     * Whether one routed event was ever offered to this layer.
     *
     * The three outcomes are the router's three terminal paths and each answers differently. `PassedThrough` means
     * nothing ENDED the walk, so every layer on the way down — including this one — was offered it. `DroppedNoOwner`
     * is a release whose press owner is gone: it never walked at all. `ConsumedByLayer` is the only interesting
     * case, and the rule is positional: a consumer at or below this layer's priority means the walk reached us
     * first, while a consumer above means it stopped before it got here.
     */
    auto
        Get_IsEventVisibleToLayer(
            const FCk_InputLayer_RoutedEvent& InRouted,
            const ck::FIntentMatcher_ScanContext& InContext)
        -> bool
    {
        switch (InRouted.Get_Outcome())
        {
            case ECk_InputLayer_DeliveryOutcome::PassedThrough:
            { return true; }
            case ECk_InputLayer_DeliveryOutcome::DroppedNoOwner:
            { return false; }
            case ECk_InputLayer_DeliveryOutcome::ConsumedByLayer:
            {
                const auto& Consumer = InRouted.Get_ConsumingLayer();

                if (Consumer == InContext.Get_Layer())
                { return true; }

                // A consumer destroyed since the row was written cannot be ranked, and the honest reading is that
                // it masked us: the row says something ended the walk and it was not this layer.
                if (ck::Is_NOT_Valid(Consumer))
                { return false; }

                return UCk_Utils_InputLayer_UE::Get_Priority(Consumer) <= InContext.Get_LayerPriority();
            }
            default:
            {
                CK_INVALID_ENUM(InRouted.Get_Outcome());
                return false;
            }
        }
    }

    /**
     * Whether a press of this key would still be DELIVERED to the layer if it happened right now.
     *
     * The visibility predicate above answers for an event that was routed; this answers for a hold, which routes
     * nothing after its press edge. A held button produces no rows at all, so the only honest reading of "does
     * this layer still have the input" is to ask the question routing would ask: does any layer above this one
     * currently declare a Consume capture that would end the walk first.
     *
     * Evaluated live rather than remembered, because the mask can appear at any frame of a hold — a modal pushed
     * mid-charge is exactly the case [D15]'s delivery-loss policy exists for.
     */
    auto
        Get_IsKeyDeliverableToLayer(
            const ck::FIntentMatcher_ScanContext& InContext,
            const FKey& InKey)
        -> bool
    {
        auto Source = UCk_Utils_InputLayer_UE::Get_InputSource(InContext.Get_Layer());

        if (ck::Is_NOT_Valid(Source))
        { return false; }

        auto MaskedFromAbove = false;

        Source.View<
            ck::FFragment_InputLayer_Params,
            ck::FFragment_InputLayer_Current,
            ck::TExclude<ck::FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>().ForEach(
            [&](
                FCk_Entity InEntity,
                const ck::FFragment_InputLayer_Params& InLayerParams,
                const ck::FFragment_InputLayer_Current& InLayerCurrent)
            {
                if (MaskedFromAbove)
                { return; }

                if (InLayerParams.Get_InputSource() != Source)
                { return; }

                if (InLayerParams.Get_Priority() <= InContext.Get_LayerPriority())
                { return; }

                MaskedFromAbove = InLayerCurrent.Get_Captures().ContainsByPredicate(
                [&](const FCk_InputLayer_Capture& InCapture) -> bool
                {
                    if (InCapture.Get_Behavior() != ECk_InputLayer_CaptureBehavior::Consume)
                    { return false; }

                    return InCapture.Get_MatchMode() == ECk_InputLayer_CaptureMatch::CatchAll ||
                           InCapture.Get_Key() == InKey;
                });
            });

        return NOT MaskedFromAbove;
    }

    // Whether a hold on this key would still be reaching the layer right now: the player has it down, and nothing
    // above declares a Consume that would end the walk for it.
    auto
        Get_IsKeyHeldAndDeliverable(
            const ck::FIntentMatcher_ScanContext& InContext,
            const TArray<FKey>& InHeldKeys,
            const FKey& InKey)
        -> bool
    {
        return InKey.IsValid() && InHeldKeys.Contains(InKey) && Get_IsKeyDeliverableToLayer(InContext, InKey);
    }

    /**
     * The key a level row can be anchored to right now — invalid when the button has none, which is the only state
     * that can end the row.
     *
     * PRIMARY FIRST, walking the map's own key order, so two matchers reading one profile re-home onto the same key
     * and a re-anchor is not a function of which device the sweep happened to look at first.
     */
    auto
        TryGet_AnchorableKey(
            const ck::FIntentMatcher_ScanContext& InContext,
            const TArray<FKey>& InHeldKeys,
            const TArray<FKey>& InButtonKeys)
        -> FKey
    {
        for (const auto& Key : InButtonKeys)
        {
            if (Get_IsKeyHeldAndDeliverable(InContext, InHeldKeys, Key))
            { return Key; }
        }

        return FKey{EKeys::Invalid};
    }

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The key of a press of this button on this row that the layer was allowed to see — an invalid key when
     * there is none, which is also the "was it visible at all" answer.
     *
     * The row names buttons while the delivery outcomes name KEYS, so the map is consulted to bridge them —
     * against EVERY key the button currently resolves to, since a press on any bound device is a press of the
     * button. That makes the answer a statement about the CURRENT resolution rather than the one in force when
     * the row was written — a one-frame skew across a rebind, and the alternative would be storing a key on
     * every edge the record already describes in button space.
     */
    auto
        TryGet_VisiblePressKey(
            const FCk_Intent_FrameRecord& InRow,
            const FCk_Input_ButtonId& InButton,
            const ck::FIntentMatcher_ScanContext& InContext)
        -> FKey
    {
        if (NOT InRow.Get_Pressed().Contains(InButton))
        { return FKey{EKeys::Invalid}; }

        const auto Keys = ck::IsValid(InContext.Get_ButtonMap())
            ? UCk_Utils_InputButtonMap_UE::Get_KeysForButton(InContext.Get_ButtonMap(), InButton)
            : TArray<FKey>{};

        if (Keys.IsEmpty())
        { return FKey{EKeys::Invalid}; }

        const auto* Found = InRow.Get_RoutedEvents().FindByPredicate(
        [&](const FCk_InputLayer_RoutedEvent& InRouted) -> bool
        {
            return Keys.Contains(InRouted.Get_Event().Get_Key()) &&
                   InRouted.Get_Event().Get_EventType() == ECk_InputSource_EventType::Pressed &&
                   Get_IsEventVisibleToLayer(InRouted, InContext);
        });

        return Found != nullptr ? Found->Get_Event().Get_Key() : FKey{EKeys::Invalid};
    }

    auto
        Get_IsButtonPressVisible(
            const FCk_Intent_FrameRecord& InRow,
            const FCk_Input_ButtonId& InButton,
            const ck::FIntentMatcher_ScanContext& InContext)
        -> bool
    {
        return TryGet_VisiblePressKey(InRow, InButton, InContext).IsValid();
    }

    // ----------------------------------------------------------------------------------------------------------------

    // A step carries at most one direction — the grammar rejects two in a chord — so the first one found is the
    // step's direction, and an unset answer means the step is buttons only.
    auto
        TryGet_StepOctant(
            const FCk_Intent_CompiledStep& InStep)
        -> TOptional<ECk_Intent_Octant>
    {
        for (const auto& Atom : InStep.Get_Atoms())
        {
            if (Atom.Get_Kind() == ECk_Intent_AtomKind::Direction)
            { return Atom.Get_Direction(); }
        }

        return {};
    }

    auto
        Get_IsLevelIntent(
            const FCk_Intent_CompiledIntent& InIntent)
        -> bool
    {
        return InIntent.Get_Kind() == ECk_Intent_Kind::Level;
    }

    // How many BUTTONS the move's terminal asks for at once. More than one is the definition of a chord terminal
    // and therefore of an intent that cannot complete until a partner press arrives — a direction in the chord
    // does not count, because the record already reports the direction on the very frame of the press.
    auto
        Get_TerminalButtonAtomCount(
            const FCk_Intent_CompiledIntent& InIntent)
        -> int32
    {
        if (InIntent.Get_Steps().IsEmpty())
        { return 0; }

        auto Count = 0;

        for (const auto& Atom : InIntent.Get_Steps().Last().Get_Atoms())
        {
            if (Atom.Get_Kind() == ECk_Intent_AtomKind::Button)
            { ++Count; }
        }

        return Count;
    }

    /**
     * Whether one row satisfies one step, every atom of it at once.
     *
     * `InHeldCountsForButtons` is the terminal-versus-prefix distinction and it is the only one. A chord terminal
     * asks whether several buttons were down TOGETHER, and a partner pressed a frame earlier is still down on the
     * terminal's row — so held-ness counts there. A prefix step asks for an INPUT the player made, and a button
     * that simply stayed down is not another press of it.
     */
    auto
        Get_StepMatchesRow(
            const FCk_Intent_FrameRecord& InRow,
            const FCk_Intent_CompiledStep& InStep,
            const ck::FIntentMatcher_ScanContext& InContext,
            bool InHeldCountsForButtons)
        -> bool
    {
        if (InStep.Get_IsEmpty())
        { return false; }

        for (const auto& Atom : InStep.Get_Atoms())
        {
            switch (Atom.Get_Kind())
            {
                case ECk_Intent_AtomKind::Direction:
                {
                    if (InRow.Get_Octant() != Atom.Get_Direction())
                    { return false; }

                    break;
                }
                case ECk_Intent_AtomKind::Button:
                {
                    if (Get_IsButtonPressVisible(InRow, Atom.Get_Button(), InContext))
                    { break; }

                    // Held-ness carries no delivery outcome of its own — it is a physical fact the record derives
                    // from every claimed event regardless of who received it — so it is taken at face value, and
                    // only press EDGES are filtered by visibility.
                    if (InHeldCountsForButtons && InRow.Get_Held().Contains(Atom.Get_Button()))
                    { break; }

                    return false;
                }
                default:
                {
                    CK_INVALID_ENUM(Atom.Get_Kind());
                    return false;
                }
            }
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    // Stamps the outcome fields a scan can only know once it has stopped walking. Separate from the step
    // recording because every exit the scan has reaches it, and a missed exit is a half-written entry.
    auto
        Finish_Diagnostic(
            FCk_Intent_ScanDiagnostic& OutDiagnostic,
            TArray<FCk_Intent_ScanStepDiagnostic> InWalkedSteps,
            int32 InFailedStepIndex)
        -> void
    {
        OutDiagnostic.Set_Steps(MoveTemp(InWalkedSteps));
        OutDiagnostic.Set_FailedStepIndex(InFailedStepIndex);
        OutDiagnostic.Set_Outcome(InFailedStepIndex == INDEX_NONE
            ? ECk_Intent_ScanOutcome::Matched
            : ECk_Intent_ScanOutcome::FailedAtStep);
    }

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The D7 backward scan: the terminal row, then every earlier step walked last-to-first over older rows.
     *
     * Backwards is the whole design. A forward matcher has to keep one partial match per candidate alive across
     * frames and decide when to abandon each; scanning back from a press that already happened asks one question
     * of a record that already exists, and a move either has its prefix behind it or it does not.
     *
     * A step consumes the most recent row that satisfies it and the walk never revisits, so steps cannot reorder:
     * `236` matched against a record holding 6 then 3 then 2 finds them in exactly that order or fails.
     */
    auto
        Get_ScanSucceeds(
            const FCk_Intent_CompiledIntent& InIntent,
            int32 InTerminalOffset,
            const ck::FIntentMatcher_ScanContext& InContext,
            FCk_Intent_ScanDiagnostic& OutDiagnostic)
        -> bool
    {
        const auto& Steps = InIntent.Get_Steps();

        if (Steps.IsEmpty())
        { return false; }

        const auto TerminalRow = UCk_Utils_IntentSampler_UE::TryGet_FrameAtOffset(
            InContext.Get_Sampler(), InTerminalOffset);

        const auto Recording = InContext.Get_RecordDiagnostics();
        const auto TerminalStepIndex = Steps.Num() - 1;

        auto WalkedSteps = TArray<FCk_Intent_ScanStepDiagnostic>{};

        if (Recording)
        {
            OutDiagnostic.Set_IntentName(InIntent.Get_Name());
            OutDiagnostic.Set_IntentTag(InIntent.Get_IntentTag());
            OutDiagnostic.Set_TerminalFrame(TerminalRow.Get_FrameIndex());
        }

        constexpr auto TerminalAcceptsHeldButtons = true;

        // A terminal is tested against the press row and nowhere else, so its examined count is always one.
        constexpr auto TerminalRowsExamined = 1;

        if (NOT Get_StepMatchesRow(TerminalRow, Steps.Last(), InContext, TerminalAcceptsHeldButtons))
        {
            if (Recording)
            {
                WalkedSteps.Emplace(FCk_Intent_ScanStepDiagnostic{TerminalStepIndex,
                    ECk_Intent_ScanStepOutcome::NotSatisfied, INDEX_NONE, TerminalRowsExamined});

                Finish_Diagnostic(OutDiagnostic, MoveTemp(WalkedSteps), TerminalStepIndex);
            }

            return false;
        }

        if (Recording)
        {
            WalkedSteps.Emplace(FCk_Intent_ScanStepDiagnostic{TerminalStepIndex,
                ECk_Intent_ScanStepOutcome::Matched, TerminalRow.Get_FrameIndex(), TerminalRowsExamined});
        }

        if (Steps.Num() == 1)
        {
            if (Recording)
            { Finish_Diagnostic(OutDiagnostic, MoveTemp(WalkedSteps), INDEX_NONE); }

            return true;
        }

        // The window counts logic frames INCLUDING the terminal's, so `w=1` is a move that completes on one row.
        // An undeclared window is bounded only by what the ring still holds, which is the honest limit — a step
        // whose row has been evicted cannot be proven either way.
        const auto RetainedRows = UCk_Utils_IntentSampler_UE::Get_FrameCount(InContext.Get_Sampler());
        const auto WindowFrames = InIntent.Get_WindowFrames();

        const auto OldestOffset = WindowFrames > 0
            ? FMath::Min(InTerminalOffset + WindowFrames - 1, RetainedRows - 1)
            : RetainedRows - 1;

        constexpr auto PrefixAcceptsHeldButtons = false;

        auto StepIndex = Steps.Num() - 2;
        auto LastMatchedOctant = TryGet_StepOctant(Steps.Last());
        auto FramesExaminedForStep = 0;

        for (auto Offset = InTerminalOffset + 1; Offset <= OldestOffset; ++Offset)
        {
            const auto Row = UCk_Utils_IntentSampler_UE::TryGet_FrameAtOffset(InContext.Get_Sampler(), Offset);

            if (Row.Get_FrameIndex() < 0)
            {
                // The ring stopped retaining this far back, which to the walk is the same answer the window
                // running out gives: there are no more rows to look at.
                if (Recording)
                {
                    WalkedSteps.Emplace(FCk_Intent_ScanStepDiagnostic{StepIndex,
                        ECk_Intent_ScanStepOutcome::WindowExhausted, INDEX_NONE, FramesExaminedForStep});

                    Finish_Diagnostic(OutDiagnostic, MoveTemp(WalkedSteps), StepIndex);
                }

                return false;
            }

            ++FramesExaminedForStep;

            const auto& Step = Steps[StepIndex];

            if (Get_StepMatchesRow(Row, Step, InContext, PrefixAcceptsHeldButtons))
            {
                if (Recording)
                {
                    WalkedSteps.Emplace(FCk_Intent_ScanStepDiagnostic{StepIndex,
                        ECk_Intent_ScanStepOutcome::Matched, Row.Get_FrameIndex(), FramesExaminedForStep});
                }

                LastMatchedOctant = TryGet_StepOctant(Step);
                --StepIndex;
                FramesExaminedForStep = 0;

                if (StepIndex < 0)
                {
                    if (Recording)
                    { Finish_Diagnostic(OutDiagnostic, MoveTemp(WalkedSteps), INDEX_NONE); }

                    return true;
                }

                continue;
            }

            if (InIntent.Get_Lenience() == ECk_Intent_Lenience::Lenient)
            { continue; }

            // Strict wants the octant run contiguous, so a row sitting between two matched steps may only be
            // holding one of them. Two steps that name no direction at all constrain nothing here: lenience is a
            // statement about unmatched DIRECTIONS, and between two buttons there are none to be unmatched.
            const auto SeekOctant = TryGet_StepOctant(Steps[StepIndex]);

            if (NOT LastMatchedOctant.IsSet() && NOT SeekOctant.IsSet())
            { continue; }

            const auto RowHoldsANeighbouringStep =
                (LastMatchedOctant.IsSet() && Row.Get_Octant() == *LastMatchedOctant) ||
                (SeekOctant.IsSet()        && Row.Get_Octant() == *SeekOctant);

            if (NOT RowHoldsANeighbouringStep)
            {
                if (Recording)
                {
                    WalkedSteps.Emplace(FCk_Intent_ScanStepDiagnostic{StepIndex,
                        ECk_Intent_ScanStepOutcome::ContiguityBroken, INDEX_NONE, FramesExaminedForStep});

                    Finish_Diagnostic(OutDiagnostic, MoveTemp(WalkedSteps), StepIndex);
                }

                return false;
            }
        }

        if (Recording)
        {
            WalkedSteps.Emplace(FCk_Intent_ScanStepDiagnostic{StepIndex,
                ECk_Intent_ScanStepOutcome::WindowExhausted, INDEX_NONE, FramesExaminedForStep});

            Finish_Diagnostic(OutDiagnostic, MoveTemp(WalkedSteps), StepIndex);
        }

        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_DistinctValidKeys(
            const TArray<ck::FIntentMatcher_RegisteredCapture>& InEntries)
        -> TArray<FKey>
    {
        auto Keys = TArray<FKey>{};

        for (const auto& Entry : InEntries)
        {
            for (const auto& Key : Entry.Get_Keys())
            {
                if (NOT Key.IsValid())
                { continue; }

                Keys.AddUnique(Key);
            }
        }

        return Keys;
    }

    // Order-insensitive: a rebind that reorders a button's slots without changing the key SET moves nothing a
    // capture cares about, and reporting it as drift would log a transition no edit follows.
    auto
        Get_KeySetsMatch(
            const TArray<FKey>& InA,
            const TArray<FKey>& InB)
        -> bool
    {
        if (InA.Num() != InB.Num())
        { return false; }

        for (const auto& Key : InA)
        {
            if (NOT InB.Contains(Key))
            { return false; }
        }

        return true;
    }

    auto
        Format_Keys(
            const TArray<FKey>& InKeys)
        -> FString
    {
        return FString::JoinBy(InKeys, TEXT(", "), [](const FKey& InKey) { return InKey.ToString(); });
    }

    /**
     * Turns two key sets into the smallest set of capture edits that gets from one to the other.
     *
     * Diffing rather than remove-all-then-add-all because two terminal buttons may legitimately resolve to one key:
     * a blanket removal would drop a capture the surviving button still needs, and the layer would stop seeing a key
     * the set still names.
     */
    auto
        ApplyCaptureEdits(
            FCk_Handle_InputLayer& InLayer,
            ECk_InputLayer_CaptureBehavior InBehavior,
            const TArray<FKey>& InOldKeys,
            const TArray<FKey>& InNewKeys)
        -> void
    {
        for (const auto& Key : InOldKeys)
        {
            if (InNewKeys.Contains(Key))
            { continue; }

            UCk_Utils_InputLayer_UE::Request_RemoveCapture(InLayer,
                FCk_Request_InputLayer_RemoveCapture{ECk_InputLayer_CaptureMatch::Key, Key}, {});
        }

        for (const auto& Key : InNewKeys)
        {
            if (InOldKeys.Contains(Key))
            { continue; }

            UCk_Utils_InputLayer_UE::Request_AddCapture(InLayer,
                FCk_Request_InputLayer_AddCapture{
                    UCk_Utils_InputLayer_UE::Make_KeyCapture(Key, InBehavior)}, {});
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FIntentMatcher_PhaseWriter::
        Set_Phase(
            FCk_Handle_IntentMatcher InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            int32 InIntentIndex,
            ECk_Intent_Phase InNewPhase,
            int32 InFrame)
        -> void
    {
        if (NOT InCurrent._PhaseRows.IsValidIndex(InIntentIndex))
        { return; }

        auto& PhaseRow = InCurrent._PhaseRows[InIntentIndex];

        const auto PreviousPhase = PhaseRow._Phase;

        // A re-completion on a LATER frame is a real event even though the phase is unchanged, so the frame is
        // half of what makes a transition. Only a call that moves neither is silent.
        if (PreviousPhase == InNewPhase && PhaseRow._PhaseFrame == InFrame)
        { return; }

        PhaseRow._Phase = InNewPhase;
        PhaseRow._PhaseFrame = InFrame;

        // A claim belongs to ONE completion. Clearing here rather than at each call site is what makes that true
        // of every transition, including the decay that exists to stop a stale completion being claimable.
        PhaseRow._ClaimedBy = {};
        PhaseRow._ClaimedFrame = INDEX_NONE;

        const auto& Intent = InCurrent._ActiveSet.Get_Intents()[InIntentIndex];

        UUtils_Signal_OnIntentPhaseChanged::Broadcast(InMatcher, MakePayload(
            InMatcher, Intent.Get_Name(), Intent.Get_IntentTag(), PreviousPhase, InNewPhase, InFrame));

        if (InNewPhase != ECk_Intent_Phase::Completed)
        { return; }

        UUtils_Signal_OnIntentCompleted::Broadcast(InMatcher, MakePayload(
            InMatcher, Intent.Get_Name(), Intent.Get_IntentTag(), InFrame));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IntentMatcher_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMatcher,
            const FFragment_IntentMatcher_Params& InParams,
            FFragment_IntentMatcher_Current& InCurrent,
            FFragment_IntentMatcher_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InMatcher, Result);

            Result = DoHandleRequest(InMatcher, InParams, InCurrent, InRequest);
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InMatcher.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_IntentMatcher_HandleRequests::
        DoHandleRequest(
            HandleType InMatcher,
            const FFragment_IntentMatcher_Params& InParams,
            FFragment_IntentMatcher_Current& InCurrent,
            const FCk_Request_IntentMatcher_SwapSet& InRequest)
        -> ECk_Request_OperationResult
    {
        auto Layer = UCk_Utils_InputLayer_UE::CastChecked(InMatcher);

        const auto& Set = InRequest.Get_CompiledSet();

        auto Desired = TArray<FIntentMatcher_RegisteredCapture>{};

        if (NOT Set.Get_IsEmpty())
        {
            const auto ButtonMap = UCk_Utils_InputButtonMap_UE::Cast(
                UCk_Utils_InputLayer_UE::Get_InputSource(Layer));

            for (const auto& ResolutionRow : Set.Get_ResolutionTable())
            {
                const auto& Terminal = ResolutionRow.Get_TerminalButton();

                const auto Keys = ck::IsValid(ButtonMap)
                    ? UCk_Utils_InputButtonMap_UE::Get_KeysForButton(ButtonMap, Terminal)
                    : TArray<FKey>{};

                // Atomic on EMPTINESS: a terminal NO key produces means the set describes a move the player
                // could not make, and half-activating it would leave a matcher whose captures and whose
                // definitions disagree. A terminal that still resolves on one device while another slot sits
                // unbound activates with the keys it has — a partial binding is a state the player can produce
                // from a settings screen, not a defective set.
                if (Keys.IsEmpty())
                {
                    intent::Verbose
                    (
                        TEXT("IntentMatcher [{}] rejected a set swap: terminal button [{}|{}] resolves to no key "
                             "on this source, so a press of it could never arrive. The previous set stays active"),
                        InMatcher, Terminal.Get_Tier(), Terminal.Get_Name()
                    );

                    return ECk_Request_OperationResult::Failed;
                }

                Desired.Emplace(FIntentMatcher_RegisteredCapture{Terminal, Keys});
            }
        }

        const auto OldKeys = ck_intent_matcher_processor::Get_DistinctValidKeys(InCurrent._RegisteredCaptures);
        const auto NewKeys = ck_intent_matcher_processor::Get_DistinctValidKeys(Desired);

        ck_intent_matcher_processor::ApplyCaptureEdits(Layer, InParams.Get_CaptureBehavior(), OldKeys, NewKeys);

        // The outgoing set's rows are about to stop existing, and a poller reads an intent the set no longer
        // carries as Idle — so the phase observably moved and the signal owes an account of it. Done BEFORE the
        // set is replaced, because the payload names the intent and only the outgoing set can still name it.
        for (auto Index = 0; Index < InCurrent._PhaseRows.Num(); ++Index)
        {
            if (InCurrent._PhaseRows[Index].Get_Phase() == ECk_Intent_Phase::Idle)
            { continue; }

            FIntentMatcher_PhaseWriter::Set_Phase(InMatcher, InCurrent, Index, ECk_Intent_Phase::Idle, INDEX_NONE);
        }

        InCurrent._RegisteredCaptures = MoveTemp(Desired);
        InCurrent._ActiveSet = Set;
        InCurrent._LastScannedFrameIndex = INDEX_NONE;
        InCurrent._SetHasLevelIntent = Set.Get_Intents().ContainsByPredicate(
        [](const FCk_Intent_CompiledIntent& InIntent) -> bool
        {
            return ck_intent_matcher_processor::Get_IsLevelIntent(InIntent);
        });

        // Episodes are indices into the set that just went away, so they cannot survive it — and nothing is
        // reported for them: a swap is not an answer to the press that was waiting, it is the question being
        // withdrawn. Active levels go the same way, and their Active -> Idle signal already rode the non-Idle
        // sweep above.
        InCurrent._PendingEpisodes.Reset();
        InCurrent._HoldAccumulators.Reset();
        InCurrent._ActiveLevels.Reset();

        InCurrent._PhaseRows.Reset();
        InCurrent._PhaseRows.SetNum(Set.Get_Intents().Num());

        intent::Verbose
        (
            TEXT("IntentMatcher [{}] activated a set of [{}] intents over [{}] terminal keys"),
            InMatcher, Set.Get_Intents().Num(), NewKeys.Num()
        );

        return ECk_Request_OperationResult::Succeeded;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IntentMatcher_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMatcher,
            const FFragment_IntentMatcher_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InMatcher, InRequests.Get_Requests());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IntentMatcher_Match::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMatcher,
            const FFragment_IntentMatcher_Params& InParams,
            FFragment_IntentMatcher_Current& InCurrent) const
        -> void
    {
        auto Layer = UCk_Utils_InputLayer_UE::CastChecked(InMatcher);

        const auto Source = UCk_Utils_InputLayer_UE::Get_InputSource(Layer);
        const auto ButtonMap = UCk_Utils_InputButtonMap_UE::Cast(Source);

        DoRefreshCaptureResolutions(Layer, ButtonMap, InParams, InCurrent);

        if (InCurrent._ActiveSet.Get_IsEmpty())
        { return; }

        const auto Sampler = UCk_Utils_IntentSampler_UE::Cast(Source);

        if (ck::Is_NOT_Valid(Sampler))
        {
            // No further row will ever be written, so no sweep can notice the release: a row left Active here
            // stays Active — and CLAIMABLE — for the rest of the matcher's life, against a hold whose input has
            // stopped existing. The last frame this matcher consumed is the honest one to name it on.
            DoDeactivateAllLevelRows(InMatcher, InCurrent, InCurrent._LastScannedFrameIndex,
                FString{TEXT("the input source no longer carries a frame record")});

            return;
        }

        const auto LatestFrameIndex = UCk_Utils_IntentSampler_UE::Get_LatestFrame(Sampler).Get_FrameIndex();

        if (LatestFrameIndex < 0)
        { return; }

        const auto RetainedRows = UCk_Utils_IntentSampler_UE::Get_FrameCount(Sampler);

        // A matcher that has never scanned takes only the newest row: a set activated now must not complete on
        // presses the player made before it existed. Afterwards the gap between the record's frame index and the
        // last one consumed is exactly how many rows arrived, clamped to what the ring still holds.
        const auto UnscannedRows = InCurrent._LastScannedFrameIndex == INDEX_NONE
            ? 1
            : FMath::Min(LatestFrameIndex - InCurrent._LastScannedFrameIndex, RetainedRows);

        InCurrent._LastScannedFrameIndex = LatestFrameIndex;

        const auto Context = FIntentMatcher_ScanContext
        {
            Sampler, ButtonMap, Layer, UCk_Utils_InputLayer_UE::Get_Priority(Layer), LatestFrameIndex,
            InParams.Get_LatchDecayFrames(), UCk_Utils_IntentMatcher_UE::Get_ScanDiagnosticsEnabled()
        };

        // Oldest first: a later row's press may complete a move whose prefix a nearer row carries, and scanning
        // the newest first would read that prefix as part of a frame that had not happened yet. A pending episode
        // needs the same order for a different reason — a hold is counted in rows.
        for (auto Offset = UnscannedRows - 1; Offset >= 0; --Offset)
        {
            DoProcessRow(InMatcher, InCurrent, Context, Offset);
        }
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoRefreshCaptureResolutions(
            FCk_Handle_InputLayer& InLayer,
            const FCk_Handle_InputButtonMap& InButtonMap,
            const FFragment_IntentMatcher_Params& InParams,
            FFragment_IntentMatcher_Current& InCurrent)
        -> void
    {
        if (InCurrent._RegisteredCaptures.IsEmpty())
        { return; }

        auto Refreshed = TArray<FIntentMatcher_RegisteredCapture>{};
        Refreshed.Reserve(InCurrent._RegisteredCaptures.Num());

        auto AnyAssociationMoved = false;

        for (const auto& Registered : InCurrent._RegisteredCaptures)
        {
            const auto FreshKeys = ck::IsValid(InButtonMap)
                ? UCk_Utils_InputButtonMap_UE::Get_KeysForButton(InButtonMap, Registered.Get_Button())
                : TArray<FKey>{};

            if (NOT ck_intent_matcher_processor::Get_KeySetsMatch(FreshKeys, Registered.Get_Keys()))
            {
                AnyAssociationMoved = true;

                // Once per transition rather than once per tick, and it falls out of only logging where the
                // comparison actually changed — a terminal that stays unbound is silent after the first line.
                intent::Verbose
                (
                    TEXT("IntentMatcher on InputLayer [{}]: terminal button [{}|{}] moved from keys [{}] to keys "
                         "[{}]; its captures follow on the next routing pass"),
                    InLayer, Registered.Get_Button().Get_Tier(), Registered.Get_Button().Get_Name(),
                    ck_intent_matcher_processor::Format_Keys(Registered.Get_Keys()),
                    ck_intent_matcher_processor::Format_Keys(FreshKeys)
                );
            }

            Refreshed.Emplace(FIntentMatcher_RegisteredCapture{Registered.Get_Button(), FreshKeys});
        }

        if (NOT AnyAssociationMoved)
        { return; }

        const auto OldKeys = ck_intent_matcher_processor::Get_DistinctValidKeys(InCurrent._RegisteredCaptures);
        const auto NewKeys = ck_intent_matcher_processor::Get_DistinctValidKeys(Refreshed);

        ck_intent_matcher_processor::ApplyCaptureEdits(InLayer, InParams.Get_CaptureBehavior(), OldKeys, NewKeys);

        InCurrent._RegisteredCaptures = MoveTemp(Refreshed);
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoProcessRow(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            int32 InOffset)
        -> void
    {
        const auto Row = UCk_Utils_IntentSampler_UE::TryGet_FrameAtOffset(InContext.Get_Sampler(), InOffset);

        if (Row.Get_FrameIndex() < 0)
        { return; }

        auto CompletedThisRow = TArray<int32>{};
        auto ButtonsSpokenFor = TArray<FCk_Input_ButtonId>{};

        // Live episodes first: a chord's partner and a hold's threshold both land on THIS row, and a press that
        // is already being waited on must not open a second episode below.
        for (auto Index = InCurrent._PendingEpisodes.Num() - 1; Index >= 0; --Index)
        {
            auto& Episode = InCurrent._PendingEpisodes[Index];

            ButtonsSpokenFor.AddUnique(Episode._Button);

            if (NOT DoAdvanceEpisode(InMatcher, InCurrent, InContext, Row, InOffset, Episode, CompletedThisRow))
            { continue; }

            DoDropHoldAccumulator(InCurrent, Episode._Button);
            InCurrent._PendingEpisodes.RemoveAt(Index);
        }

        DoScanRowForNewPresses(InMatcher, InCurrent, InContext, Row, InOffset, ButtonsSpokenFor, CompletedThisRow);

        // Deliberately outside the spoken-for bookkeeping: a level row is not an episode candidate and a press
        // that opened an episode for its edge siblings still activates the level move on the same button.
        DoUpdateLevelRows(InMatcher, InCurrent, InContext, Row, InOffset);

        DoPurgeEpisodesResolvedElsewhere(InMatcher, InCurrent, CompletedThisRow, Row.Get_FrameIndex());

        DoDecayLatches(InMatcher, InCurrent, InContext, Row.Get_FrameIndex());
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoScanRowForNewPresses(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            const FCk_Intent_FrameRecord& InRow,
            int32 InOffset,
            const TArray<FCk_Input_ButtonId>& InButtonsSpokenFor,
            TArray<int32>& OutCompletedThisRow)
        -> void
    {
        const auto& Set = InCurrent._ActiveSet;

        for (const auto& PressedButton : InRow.Get_Pressed())
        {
            if (InButtonsSpokenFor.Contains(PressedButton))
            { continue; }

            const auto PressKey = ck_intent_matcher_processor::TryGet_VisiblePressKey(InRow, PressedButton, InContext);

            if (NOT PressKey.IsValid())
            { continue; }

            const auto ResolutionRow = UCk_Utils_IntentGrammar_UE::TryGet_ResolutionRow(Set, PressedButton);

            if (ResolutionRow.Get_IntentIndices().IsEmpty())
            { continue; }

            const auto Verdict = UCk_Utils_IntentGrammar_UE::Get_DeferralVerdict(Set, PressedButton);

            if (Verdict.Get_IsDeferred())
            {
                // A level index can never enter an episode's candidate list: `Pending` is a phase it has no way
                // to leave — nothing resolves it later — and a wait it joined would answer for a press its own
                // lifecycle already answered on this very frame.
                auto EdgeCandidates = TArray<int32>{};

                for (const auto IntentIndex : ResolutionRow.Get_IntentIndices())
                {
                    if (NOT Set.Get_Intents().IsValidIndex(IntentIndex))
                    { continue; }

                    if (ck_intent_matcher_processor::Get_IsLevelIntent(Set.Get_Intents()[IntentIndex]))
                    { continue; }

                    EdgeCandidates.Add(IntentIndex);
                }

                if (EdgeCandidates.IsEmpty())
                { continue; }

                DoOpenEpisode(InMatcher, InCurrent, InRow, PressedButton, PressKey, Verdict, EdgeCandidates);

                // A chord whose partner was ALREADY down completes on the press row itself, so the freshly opened
                // episode gets one advance against the row that opened it rather than waiting for the next one.
                const auto OpenedIndex = InCurrent._PendingEpisodes.Num() - 1;

                if (DoAdvanceEpisode(InMatcher, InCurrent, InContext, InRow, InOffset,
                        InCurrent._PendingEpisodes[OpenedIndex], OutCompletedThisRow))
                {
                    DoDropHoldAccumulator(InCurrent, InCurrent._PendingEpisodes[OpenedIndex]._Button);
                    InCurrent._PendingEpisodes.RemoveAt(OpenedIndex);
                }

                continue;
            }

            // The row is already ordered most-dominant first, so first-match IS the arbiter and there is no
            // second sort here that could disagree with the one the bake settled.
            for (const auto IntentIndex : ResolutionRow.Get_IntentIndices())
            {
                if (NOT InCurrent._PhaseRows.IsValidIndex(IntentIndex))
                { continue; }

                if (ck_intent_matcher_processor::Get_IsLevelIntent(Set.Get_Intents()[IntentIndex]))
                { continue; }

                if (OutCompletedThisRow.Contains(IntentIndex))
                { break; }

                if (NOT DoRunScan(InCurrent, InContext, Set.Get_Intents()[IntentIndex], InOffset))
                { continue; }

                DoCompleteIntent(InMatcher, InCurrent, IntentIndex, InRow.Get_FrameIndex(), OutCompletedThisRow);
                break;
            }
        }
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoUpdateLevelRows(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            const FCk_Intent_FrameRecord& InRow,
            int32 InOffset)
        -> void
    {
        // A set with no level intent can neither activate one nor be holding one — a swap clears `_ActiveLevels` —
        // so every press of every row below would pay a map lookup and a resolution-row copy to discover there was
        // never any work. Answered at swap time, because the set cannot change while it is active.
        if (NOT InCurrent._SetHasLevelIntent)
        { return; }

        const auto& Set = InCurrent._ActiveSet;

        for (const auto& PressedButton : InRow.Get_Pressed())
        {
            const auto PressKey = ck_intent_matcher_processor::TryGet_VisiblePressKey(InRow, PressedButton, InContext);

            if (NOT PressKey.IsValid())
            { continue; }

            const auto ResolutionRow = UCk_Utils_IntentGrammar_UE::TryGet_ResolutionRow(Set, PressedButton);

            for (const auto IntentIndex : ResolutionRow.Get_IntentIndices())
            {
                if (NOT InCurrent._PhaseRows.IsValidIndex(IntentIndex))
                { continue; }

                if (NOT ck_intent_matcher_processor::Get_IsLevelIntent(Set.Get_Intents()[IntentIndex]))
                { continue; }

                // Already active: a second press of a button that is still down cannot start a hold that never
                // ended, and re-entering would restamp a phase frame the consumer measures the hold from.
                if (Get_IsLevelActive(InCurrent, IntentIndex))
                { continue; }

                if (NOT DoRunScan(InCurrent, InContext, Set.Get_Intents()[IntentIndex], InOffset))
                { continue; }

                DoActivateLevelRow(InMatcher, InCurrent, IntentIndex, PressedButton, PressKey, InRow.Get_FrameIndex());
            }
        }

        if (InCurrent._ActiveLevels.IsEmpty())
        { return; }

        const auto HeldKeys = UCk_Utils_IntentSampler_UE::Get_HeldKeys(InContext.Get_Sampler());

        // Backwards so a release cannot move an entry the sweep has not reached yet.
        for (auto Index = InCurrent._ActiveLevels.Num() - 1; Index >= 0; --Index)
        {
            auto& Active = InCurrent._ActiveLevels[Index];

            // Cheapest first, and it subsumes everything below it: a button nothing holds has no key left to
            // anchor to, so neither the map lookup nor the delivery walk has a question to answer.
            if (NOT InRow.Get_Held().Contains(Active._Button))
            {
                DoDeactivateLevelRow(InMatcher, InCurrent, Index, InRow.Get_FrameIndex(),
                    FString{TEXT("the button is no longer held")});

                continue;
            }

            const auto ButtonKeys = ck::IsValid(InContext.Get_ButtonMap())
                ? UCk_Utils_InputButtonMap_UE::Get_KeysForButton(InContext.Get_ButtonMap(), Active._Button)
                : TArray<FKey>{};

            if (ButtonKeys.Contains(Active._AnchorKey) &&
                ck_intent_matcher_processor::Get_IsKeyHeldAndDeliverable(InContext, HeldKeys, Active._AnchorKey))
            { continue; }

            // The anchor exists to make delivery-loss a PER-KEY question — a modal masking the gamepad must not
            // release a keyboard hold — and it is NOT a second held-union. A player who let go of the anchor while
            // still holding another bound key is still holding the button, so a mask or a rebind aimed at the key
            // they already released must not end the hold they are actually on. The anchor therefore MOVES onto
            // whichever bound key is both held and deliverable, and only a button with none left ends the row.
            const auto ReAnchoredKey = ck_intent_matcher_processor::TryGet_AnchorableKey(
                InContext, HeldKeys, ButtonKeys);

            if (NOT ReAnchoredKey.IsValid())
            {
                DoDeactivateLevelRow(InMatcher, InCurrent, Index, InRow.Get_FrameIndex(),
                    FString{TEXT("this layer no longer receives any held key of the button")});

                continue;
            }

            intent::Verbose
            (
                TEXT("IntentMatcher [{}] re-anchored level intent [{}] from key [{}] to key [{}] on record frame "
                     "[{}]"),
                InMatcher, InCurrent._ActiveSet.Get_Intents()[Active._IntentIndex].Get_Name(),
                Active._AnchorKey.ToString(), ReAnchoredKey.ToString(), InRow.Get_FrameIndex()
            );

            Active._AnchorKey = ReAnchoredKey;
        }
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoActivateLevelRow(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            int32 InIntentIndex,
            const FCk_Input_ButtonId& InButton,
            const FKey& InAnchorKey,
            int32 InFrame)
        -> void
    {
        // Entry first, broadcast second. `Set_Phase` fires `OnIntentPhaseChanged` from inside itself, and a handler
        // that reads back through the matcher must not see a phase the active-level bookkeeping does not yet agree
        // with.
        InCurrent._ActiveLevels.Emplace(FIntentMatcher_ActiveLevel{InIntentIndex, InButton, InAnchorKey});

        FIntentMatcher_PhaseWriter::Set_Phase(
            InMatcher, InCurrent, InIntentIndex, ECk_Intent_Phase::Active, InFrame);

        intent::Verbose
        (
            TEXT("IntentMatcher [{}] activated level intent [{}] on button [{}|{}] anchored to key [{}] on record "
                 "frame [{}]"),
            InMatcher, InCurrent._ActiveSet.Get_Intents()[InIntentIndex].Get_Name(),
            InButton.Get_Tier(), InButton.Get_Name(), InAnchorKey.ToString(), InFrame
        );
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoDeactivateLevelRow(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            int32 InActiveLevelIndex,
            int32 InFrame,
            const FString& InReason)
        -> void
    {
        const auto IntentIndex = InCurrent._ActiveLevels[InActiveLevelIndex]._IntentIndex;

        // Removal first, broadcast second — the same reason activation emplaces first: a handler reading back
        // through the matcher would otherwise see `Idle` beside a row still listed as active.
        InCurrent._ActiveLevels.RemoveAt(InActiveLevelIndex);

        FIntentMatcher_PhaseWriter::Set_Phase(
            InMatcher, InCurrent, IntentIndex, ECk_Intent_Phase::Idle, InFrame);

        intent::Verbose
        (
            TEXT("IntentMatcher [{}] released level intent [{}] on record frame [{}]: {}"),
            InMatcher, InCurrent._ActiveSet.Get_Intents()[IntentIndex].Get_Name(), InFrame, InReason
        );
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoDeactivateAllLevelRows(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            int32 InFrame,
            const FString& InReason)
        -> void
    {
        for (auto Index = InCurrent._ActiveLevels.Num() - 1; Index >= 0; --Index)
        {
            DoDeactivateLevelRow(InMatcher, InCurrent, Index, InFrame, InReason);
        }
    }

    auto
        FProcessor_IntentMatcher_Match::
        Get_IsLevelActive(
            const FFragment_IntentMatcher_Current& InCurrent,
            int32 InIntentIndex)
        -> bool
    {
        return InCurrent._ActiveLevels.ContainsByPredicate(
        [&](const FIntentMatcher_ActiveLevel& InActive) -> bool
        {
            return InActive.Get_IntentIndex() == InIntentIndex;
        });
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoAdvanceEpisode(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            const FCk_Intent_FrameRecord& InRow,
            int32 InOffset,
            FIntentMatcher_PendingEpisode& InEpisode,
            TArray<int32>& OutCompletedThisRow)
        -> bool
    {
        const auto ButtonKeys = ck::IsValid(InContext.Get_ButtonMap())
            ? UCk_Utils_InputButtonMap_UE::Get_KeysForButton(InContext.Get_ButtonMap(), InEpisode._Button)
            : TArray<FKey>{};

        // [D15]'s default pair, the Cancel half: an episode whose input this layer no longer receives is over.
        // Conservative and loud on purpose — the alternative is a charge that silently completes out of a menu.
        // Evaluated against the KEY THE PRESS ARRIVED ON, not whichever key the button resolves to first: with
        // several devices bound to one button, a modal masking only the gamepad key must not cancel a keyboard
        // charge — and masking the key the player is actually holding must, whichever slot it sits in. A rebind
        // that moves the opening key off the button ends the episode the same way: the layer will never receive
        // that key again.
        const auto DeliveryIsIntact = InEpisode._PressKey.IsValid() &&
                                      ButtonKeys.Contains(InEpisode._PressKey) &&
                                      ck_intent_matcher_processor::Get_IsKeyDeliverableToLayer(
                                          InContext, InEpisode._PressKey);

        if (NOT DeliveryIsIntact)
        {
            intent::Verbose
            (
                TEXT("IntentMatcher [{}] cancelled the pending press of button [{}|{}]: this layer no longer "
                     "receives its key. The physical hold is unaffected and still on the record"),
                InMatcher, InEpisode._Button.Get_Tier(), InEpisode._Button.Get_Name()
            );

            DoFailEpisode(InMatcher, InCurrent, InEpisode, InRow.Get_FrameIndex());
            return true;
        }

        const auto ButtonIsDown = InRow.Get_Held().Contains(InEpisode._Button) ||
            ck_intent_matcher_processor::Get_IsButtonPressVisible(InRow, InEpisode._Button, InContext);

        // The press row is elapsed-zero, so only the rows AFTER it add to the count. That is what makes a
        // threshold of N mean "still down N frames later" and makes the completion frame press + N exactly.
        if (ButtonIsDown && InRow.Get_FrameIndex() > InEpisode._PressFrame)
        { DoAdvanceHoldAccumulator(InCurrent, InEpisode._Button); }

        const auto HeldFrames = DoGet_AccumulatedHoldFrames(InCurrent, InEpisode._Button);
        const auto ElapsedFrames = InRow.Get_FrameIndex() - InEpisode._PressFrame;
        const auto PressOffset = InContext.Get_LatestFrameIndex() - InEpisode._PressFrame;

        // Both causes run CONCURRENTLY and neither is assumed shorter; whichever answers first ends the episode.
        // The chord is asked first because a chord that actually completed on this row is the most specific
        // evidence available, and it is the only branch whose terminal is THIS row rather than the press row.
        if (InEpisode._ChordCauseArmed && ElapsedFrames <= InEpisode._ChordWindowFrames)
        {
            const auto ChordCandidates = [](const FCk_Intent_CompiledIntent& InIntent) -> bool
            {
                return ck_intent_matcher_processor::Get_TerminalButtonAtomCount(InIntent) > 1;
            };

            if (DoTryResolveEpisode(InMatcher, InCurrent, InContext, InEpisode, ChordCandidates,
                    InOffset, InRow.Get_FrameIndex(), OutCompletedThisRow))
            { return true; }
        }

        if (InEpisode._HoldCauseArmed && ButtonIsDown)
        {
            const auto HoldCandidatesAtThreshold = [HeldFrames](const FCk_Intent_CompiledIntent& InIntent) -> bool
            {
                return InIntent.Get_HoldFrames() > 0 && HeldFrames >= InIntent.Get_HoldFrames();
            };

            if (DoTryResolveEpisode(InMatcher, InCurrent, InContext, InEpisode, HoldCandidatesAtThreshold,
                    PressOffset, InRow.Get_FrameIndex(), OutCompletedThisRow))
            { return true; }
        }

        // The button came up, so nothing can become a hold any more; or it outlasted the longest threshold any
        // sibling declares, so no hold that could still win is left. Either way the hold ambiguity is spent.
        if (InEpisode._HoldCauseArmed && (NOT ButtonIsDown || HeldFrames >= InEpisode._HoldSiblingFrames))
        { InEpisode._HoldCauseArmed = false; }

        if (InEpisode._ChordCauseArmed && ElapsedFrames > InEpisode._ChordWindowFrames)
        { InEpisode._ChordCauseArmed = false; }

        if (InEpisode._ChordCauseArmed || InEpisode._HoldCauseArmed)
        { return false; }

        // Every cause is spent. The press answers on the evidence it always had — its OWN row — while the
        // completion frame names the frame the wait ended on: the latency was paid, the history was not rewritten.
        const auto UnconditionalCandidates = [](const FCk_Intent_CompiledIntent& InIntent) -> bool
        {
            return InIntent.Get_HoldFrames() == 0;
        };

        if (DoTryResolveEpisode(InMatcher, InCurrent, InContext, InEpisode, UnconditionalCandidates,
                PressOffset, InRow.Get_FrameIndex(), OutCompletedThisRow))
        { return true; }

        DoFailEpisode(InMatcher, InCurrent, InEpisode, InRow.Get_FrameIndex());
        return true;
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoTryResolveEpisode(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            const FIntentMatcher_PendingEpisode& InEpisode,
            TFunctionRef<bool(const FCk_Intent_CompiledIntent&)> InCandidateFilter,
            int32 InTerminalOffset,
            int32 InResolutionFrame,
            TArray<int32>& OutCompletedThisRow)
        -> bool
    {
        const auto& Intents = InCurrent._ActiveSet.Get_Intents();

        for (const auto IntentIndex : InEpisode._Candidates)
        {
            if (NOT InCurrent._PhaseRows.IsValidIndex(IntentIndex))
            { continue; }

            if (NOT InCandidateFilter(Intents[IntentIndex]))
            { continue; }

            if (NOT DoRunScan(InCurrent, InContext, Intents[IntentIndex], InTerminalOffset))
            { continue; }

            DoCompleteIntent(InMatcher, InCurrent, IntentIndex, InResolutionFrame, OutCompletedThisRow);
            DoSettleLosingRows(InMatcher, InCurrent, InEpisode, InResolutionFrame);

            return true;
        }

        return false;
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoOpenEpisode(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FCk_Intent_FrameRecord& InRow,
            const FCk_Input_ButtonId& InButton,
            const FKey& InPressKey,
            const FCk_Intent_DeferralVerdict& InVerdict,
            const TArray<int32>& InCandidates)
        -> void
    {
        auto Episode = FIntentMatcher_PendingEpisode{};

        Episode._Button = InButton;
        Episode._PressKey = InPressKey;
        Episode._PressFrame = InRow.Get_FrameIndex();
        Episode._ChordWindowFrames = InVerdict.Get_ChordMemberFrames();
        Episode._HoldSiblingFrames = InVerdict.Get_HoldSiblingFrames();
        Episode._ChordCauseArmed = InVerdict.Get_ChordMemberFrames() > 0;
        Episode._HoldCauseArmed = InVerdict.Get_HoldSiblingFrames() > 0;
        Episode._Candidates = InCandidates;

        for (const auto IntentIndex : Episode._Candidates)
        {
            FIntentMatcher_PhaseWriter::Set_Phase(
                InMatcher, InCurrent, IntentIndex, ECk_Intent_Phase::Pending, Episode._PressFrame);
        }

        // RequireRePress, the gain half of [D15]'s default pair: the count belongs to THIS press and starts at
        // zero, whatever the button happened to be doing before the layer was allowed to see it.
        DoDropHoldAccumulator(InCurrent, InButton);
        InCurrent._HoldAccumulators.Emplace(FIntentMatcher_HoldAccumulator{InButton, 0});

        intent::Verbose
        (
            TEXT("IntentMatcher [{}] is holding the press of button [{}|{}] on frame [{}] pending [{}] hold "
                 "frames and [{}] chord frames"),
            InMatcher, InButton.Get_Tier(), InButton.Get_Name(), Episode._PressFrame,
            Episode._HoldSiblingFrames, Episode._ChordWindowFrames
        );

        InCurrent._PendingEpisodes.Emplace(MoveTemp(Episode));
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoPurgeEpisodesResolvedElsewhere(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const TArray<int32>& InCompletedThisRow,
            int32 InFrame)
        -> void
    {
        if (InCompletedThisRow.IsEmpty())
        { return; }

        for (auto Index = InCurrent._PendingEpisodes.Num() - 1; Index >= 0; --Index)
        {
            const auto& Episode = InCurrent._PendingEpisodes[Index];

            // An intent that just completed answers every episode that was waiting on it: the same press cannot
            // still be ambiguous between candidates one of which has already been decided.
            const auto WasAnsweredElsewhere = Episode._Candidates.ContainsByPredicate(
            [&](int32 InIntentIndex) -> bool
            {
                return InCompletedThisRow.Contains(InIntentIndex);
            });

            if (NOT WasAnsweredElsewhere)
            { continue; }

            DoSettleLosingRows(InMatcher, InCurrent, Episode, InFrame);
            DoDropHoldAccumulator(InCurrent, Episode._Button);

            InCurrent._PendingEpisodes.RemoveAt(Index);
        }
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoCompleteIntent(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            int32 InIntentIndex,
            int32 InFrame,
            TArray<int32>& OutCompletedThisRow)
        -> void
    {
        FIntentMatcher_PhaseWriter::Set_Phase(
            InMatcher, InCurrent, InIntentIndex, ECk_Intent_Phase::Completed, InFrame);

        OutCompletedThisRow.AddUnique(InIntentIndex);

        intent::Verbose
        (
            TEXT("IntentMatcher [{}] completed intent [{}] on record frame [{}]"),
            InMatcher, InCurrent._ActiveSet.Get_Intents()[InIntentIndex].Get_Name(), InFrame
        );
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoFailEpisode(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_PendingEpisode& InEpisode,
            int32 InFrame)
        -> void
    {
        for (const auto IntentIndex : InEpisode._Candidates)
        {
            if (NOT InCurrent._PhaseRows.IsValidIndex(IntentIndex))
            { continue; }

            // Only the rows THIS episode put into the wait are its to answer; one that has since been completed
            // by another press keeps its latch.
            if (InCurrent._PhaseRows[IntentIndex].Get_Phase() != ECk_Intent_Phase::Pending)
            { continue; }

            FIntentMatcher_PhaseWriter::Set_Phase(
                InMatcher, InCurrent, IntentIndex, ECk_Intent_Phase::Failed, InFrame);
        }

        intent::Verbose
        (
            TEXT("IntentMatcher [{}] resolved the pending press of button [{}|{}] on frame [{}] with no move "
                 "matching"),
            InMatcher, InEpisode._Button.Get_Tier(), InEpisode._Button.Get_Name(), InFrame
        );
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoSettleLosingRows(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_PendingEpisode& InEpisode,
            int32 InFrame)
        -> void
    {
        for (const auto IntentIndex : InEpisode._Candidates)
        {
            if (NOT InCurrent._PhaseRows.IsValidIndex(IntentIndex))
            { continue; }

            if (InCurrent._PhaseRows[IntentIndex].Get_Phase() != ECk_Intent_Phase::Pending)
            { continue; }

            FIntentMatcher_PhaseWriter::Set_Phase(
                InMatcher, InCurrent, IntentIndex, ECk_Intent_Phase::Idle, InFrame);
        }
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoAdvanceHoldAccumulator(
            FFragment_IntentMatcher_Current& InCurrent,
            const FCk_Input_ButtonId& InButton)
        -> void
    {
        const auto Index = InCurrent._HoldAccumulators.IndexOfByPredicate(
        [&](const FIntentMatcher_HoldAccumulator& InAccumulator) -> bool
        {
            return InAccumulator.Get_Button() == InButton;
        });

        if (Index == INDEX_NONE)
        { return; }

        ++InCurrent._HoldAccumulators[Index]._HeldFrames;
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoDropHoldAccumulator(
            FFragment_IntentMatcher_Current& InCurrent,
            const FCk_Input_ButtonId& InButton)
        -> void
    {
        InCurrent._HoldAccumulators.RemoveAll(
        [&](const FIntentMatcher_HoldAccumulator& InAccumulator) -> bool
        {
            return InAccumulator.Get_Button() == InButton;
        });
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoGet_AccumulatedHoldFrames(
            const FFragment_IntentMatcher_Current& InCurrent,
            const FCk_Input_ButtonId& InButton)
        -> int32
    {
        const auto Index = InCurrent._HoldAccumulators.IndexOfByPredicate(
        [&](const FIntentMatcher_HoldAccumulator& InAccumulator) -> bool
        {
            return InAccumulator.Get_Button() == InButton;
        });

        if (Index == INDEX_NONE)
        { return 0; }

        return InCurrent._HoldAccumulators[Index].Get_HeldFrames();
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoRunScan(
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            const FCk_Intent_CompiledIntent& InIntent,
            int32 InTerminalOffset)
        -> bool
    {
        // Built unconditionally and left empty when recording is off: an empty TArray allocates nothing, so the
        // switched-off cost is one stack struct rather than a branch at every recording site inside the scan.
        auto Diagnostic = FCk_Intent_ScanDiagnostic{};

        const auto Matched = ck_intent_matcher_processor::Get_ScanSucceeds(
            InIntent, InTerminalOffset, InContext, Diagnostic);

        if (InContext.Get_RecordDiagnostics())
        { DoPushScanDiagnostic(InCurrent, MoveTemp(Diagnostic)); }

        return Matched;
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoPushScanDiagnostic(
            FFragment_IntentMatcher_Current& InCurrent,
            FCk_Intent_ScanDiagnostic InDiagnostic)
        -> void
    {
        // The write index and the array length advance together while the ring fills, so one branch grows the
        // storage and the other overwrites — never both.
        if (InCurrent._ScanDiagnostics.Num() < intent_matcher::ScanDiagnosticsCapacity)
        { InCurrent._ScanDiagnostics.Emplace(MoveTemp(InDiagnostic)); }
        else
        { InCurrent._ScanDiagnostics[InCurrent._ScanDiagnosticsNextWrite] = MoveTemp(InDiagnostic); }

        InCurrent._ScanDiagnosticsNextWrite =
            (InCurrent._ScanDiagnosticsNextWrite + 1) % intent_matcher::ScanDiagnosticsCapacity;

        InCurrent._ScanDiagnosticsCount =
            FMath::Min(InCurrent._ScanDiagnosticsCount + 1, intent_matcher::ScanDiagnosticsCapacity);
    }

    auto
        FProcessor_IntentMatcher_Match::
        DoDecayLatches(
            HandleType InMatcher,
            FFragment_IntentMatcher_Current& InCurrent,
            const FIntentMatcher_ScanContext& InContext,
            int32 InFrame)
        -> void
    {
        for (auto Index = 0; Index < InCurrent._PhaseRows.Num(); ++Index)
        {
            const auto& PhaseRow = InCurrent._PhaseRows[Index];

            // Only a RESOLVED latch expires. `Pending` carries its own windows and would be cut short by a second
            // clock it knows nothing about; `Idle` has nothing to expire.
            const auto PhaseIsALatch = PhaseRow.Get_Phase() == ECk_Intent_Phase::Completed ||
                                       PhaseRow.Get_Phase() == ECk_Intent_Phase::Failed;

            if (NOT PhaseIsALatch)
            { continue; }

            if (InFrame - PhaseRow.Get_PhaseFrame() < InContext.Get_LatchDecayFrames())
            { continue; }

            // The REAL decay frame, not INDEX_NONE. The payload's frame is the only thing a consumer can measure
            // a transition against, and a row's stored frame is never read as a completion frame — the poll gates
            // on the PHASE, so an Idle row carrying the frame it went idle on leaks nothing and says something.
            FIntentMatcher_PhaseWriter::Set_Phase(
                InMatcher, InCurrent, Index, ECk_Intent_Phase::Idle, InFrame);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
