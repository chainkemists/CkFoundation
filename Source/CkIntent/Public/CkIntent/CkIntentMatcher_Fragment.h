#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkIntent/CkIntentMatcher_Fragment_Data.h"

#include <InputCoreTypes.h>

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_IntentMatcher_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::intent_matcher
{
    // How many scan attempts the diagnostic ring keeps. Small on purpose: this is a near-miss list a human reads
    // after feeling a move not come out, not a profiler trace — and it is only written at all while the CVar is
    // on, so its cost has to stay bounded by something a reader can actually scroll.
    inline constexpr auto ScanDiagnosticsCapacity = 32;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_IntentMatcher_Params = FCk_Fragment_IntentMatcher_ParamsData;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One definition's standing, addressed by its index in the active set's `_Intents`.
     *
     * Rows are a parallel array rather than a map keyed by name or tag: the compiled set is immutable once baked, so
     * an index is stable for exactly as long as the set is active, and the resolution table already addresses
     * intents that way. A row carrying its own copy of the name would be a second place the truth lived.
     *
     * `_PhaseFrame` is the record frame the phase last changed on, whichever phase that was — a completion frame,
     * a failure frame, or the press frame a wait began at. One field rather than three because a row is in exactly
     * one phase and only that phase's frame is meaningful; `TryGet_CompletionFrame` gates on `Completed` so a
     * caller can never read a failure's frame as a completion's.
     *
     * The claim is stamped HERE rather than tracked beside the rows because exclusivity is per definition per
     * layer, and a separate claim table would be a second thing to keep in step with a phase that can change
     * underneath it. ANY phase transition clears the claim: a claim belongs to one completion, and one riding
     * across a transition would let a consumer's stale ownership block everyone from the next one.
     *
     * **The phase is not writable by the processors.** `FIntentMatcher_PhaseWriter` is the only type that can
     * move it, and that writer is the only place the transition signals are broadcast — so a phase cannot change
     * anywhere in this module without its signal going out. That is structural rather than disciplinary, which is
     * the whole reason the friend list is this short. `UCk_Utils_IntentMatcher_UE` is a friend for the CLAIM
     * fields only; it never touches the phase.
     */
    struct CKINTENT_API FIntentMatcher_PhaseRow
    {
    public:
        CK_GENERATED_BODY(FIntentMatcher_PhaseRow);

    public:
        friend class FIntentMatcher_PhaseWriter;
        friend class ::UCk_Utils_IntentMatcher_UE;

    private:
        ECk_Intent_Phase _Phase = ECk_Intent_Phase::Idle;

        int32 _PhaseFrame = INDEX_NONE;

        FCk_Handle _ClaimedBy;

        int32 _ClaimedFrame = INDEX_NONE;

    public:
        CK_PROPERTY_GET(_Phase);
        CK_PROPERTY_GET(_PhaseFrame);
        CK_PROPERTY_GET(_ClaimedBy);
        CK_PROPERTY_GET(_ClaimedFrame);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One terminal button and the physical keys the matcher currently has captures registered for — every key
     * the button's mapping binds, so a keyboard and a gamepad binding on one mapping name both complete it.
     *
     * The keys are CACHED rather than re-resolved at every use, because the cache is what makes drift
     * detectable: a re-derived button map moves the association silently, and comparing the map's fresh answer
     * against what the matcher last registered is the only way to notice without a callback the router does not
     * offer. An EMPTY list is a real state — the button resolved to nothing, so no capture is registered for it.
     */
    struct CKINTENT_API FIntentMatcher_RegisteredCapture
    {
    public:
        CK_GENERATED_BODY(FIntentMatcher_RegisteredCapture);

    public:
        friend class FProcessor_IntentMatcher_HandleRequests;
        friend class FProcessor_IntentMatcher_Match;

    private:
        FCk_Input_ButtonId _Button;

        TArray<FKey> _Keys;

    public:
        CK_PROPERTY_GET(_Button);
        CK_PROPERTY_GET(_Keys);

    public:
        CK_DEFINE_CONSTRUCTORS(FIntentMatcher_RegisteredCapture, _Button, _Keys);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * How long this layer has been delivered a held button — the POLICY-APPLIED count, not the physical one.
     *
     * The physical fact already exists and is not duplicated here: the record's `_Held` says the button is down,
     * frame by frame, for anyone who asks. What the record cannot say is how long it has been down *as far as
     * this layer is concerned*, because delivery is a per-layer question — a modal pushed above the layer stops
     * the layer from being an input consumer while the player's thumb never moves.
     *
     * Keeping the two apart is the whole of [D11]. `_HeldFrames` therefore counts only frames on which the button
     * was both physically down AND still deliverable here, and equals the plain elapsed-since-press count exactly
     * when delivery was never interrupted. Under v1's default policy pair it never has to survive an
     * interruption: delivery loss CANCELS the episode and drops the accumulator, and regaining delivery does not
     * re-arm one — a fresh press edge does.
     */
    struct CKINTENT_API FIntentMatcher_HoldAccumulator
    {
    public:
        CK_GENERATED_BODY(FIntentMatcher_HoldAccumulator);

    public:
        friend class FProcessor_IntentMatcher_Match;

    private:
        FCk_Input_ButtonId _Button;

        int32 _HeldFrames = 0;

    public:
        CK_PROPERTY_GET(_Button);
        CK_PROPERTY_GET(_HeldFrames);

    public:
        CK_DEFINE_CONSTRUCTORS(FIntentMatcher_HoldAccumulator, _Button, _HeldFrames);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One level definition that is currently `Active`, and the two facts its release is decided against.
     *
     * The ANCHOR key is the single key the delivery question is asked about, on exactly the terms a pending
     * episode's `_PressKey` sets: with several devices bound to one button, delivery-loss is per-key — a modal
     * masking only the gamepad key must not release a keyboard hold — and a rebind that moves the anchor off the
     * button releases the row, since the layer will never receive that key again.
     *
     * There is deliberately NO activation frame here. `_PhaseRows[Get_IntentIndex()].Get_PhaseFrame()` already
     * holds it, and a second copy is a second thing that can disagree with the phase it is supposed to describe.
     */
    struct CKINTENT_API FIntentMatcher_ActiveLevel
    {
    public:
        CK_GENERATED_BODY(FIntentMatcher_ActiveLevel);

    public:
        friend class FProcessor_IntentMatcher_Match;

    private:
        int32 _IntentIndex = INDEX_NONE;

        FCk_Input_ButtonId _Button;

        FKey _AnchorKey;

    public:
        CK_PROPERTY_GET(_IntentIndex);
        CK_PROPERTY_GET(_Button);
        CK_PROPERTY_GET(_AnchorKey);

    public:
        CK_DEFINE_CONSTRUCTORS(FIntentMatcher_ActiveLevel, _IntentIndex, _Button, _AnchorKey);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One press the matcher is not allowed to answer yet, and what it is waiting on.
     *
     * The press FRAME is stored rather than a ring offset, because offsets slide as the ring advances and an
     * episode outlives several rows by construction. Converting back is one subtraction against the newest frame
     * index, which is exact for as long as the row is retained — and a press whose row has been evicted could not
     * be scanned anyway.
     *
     * The candidate list is a SNAPSHOT of the terminal's resolution row taken when the episode opened. The set
     * cannot change under a live episode — a swap drops every episode — so the snapshot can never disagree with
     * the table; it is carried so the resolution paths do not re-look-up a row they already have.
     *
     * The two causes are independent flags rather than one mode. A button can be both a hold sibling and a chord
     * member, and collapsing that to a single "reason" would lose whichever one resolved second.
     */
    struct CKINTENT_API FIntentMatcher_PendingEpisode
    {
    public:
        CK_GENERATED_BODY(FIntentMatcher_PendingEpisode);

    public:
        friend class FProcessor_IntentMatcher_Match;

    private:
        FCk_Input_ButtonId _Button;

        // The key the OPENING press arrived on. With several devices bound to one button, delivery-loss is a
        // per-key question — a modal masking only the gamepad key must not cancel a keyboard charge — and the
        // record's held set cannot say which device the player is holding; the press edge could, once.
        FKey _PressKey;

        int32 _PressFrame = INDEX_NONE;

        int32 _ChordWindowFrames = 0;

        int32 _HoldSiblingFrames = 0;

        bool _ChordCauseArmed = false;

        bool _HoldCauseArmed = false;

        TArray<int32> _Candidates;

    public:
        CK_PROPERTY_GET(_Button);
        CK_PROPERTY_GET(_PressKey);
        CK_PROPERTY_GET(_PressFrame);
        CK_PROPERTY_GET(_ChordWindowFrames);
        CK_PROPERTY_GET(_HoldSiblingFrames);
        CK_PROPERTY_GET(_ChordCauseArmed);
        CK_PROPERTY_GET(_HoldCauseArmed);
        CK_PROPERTY_GET(_Candidates);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The active set and everything derived from it, in ONE stable fragment.
     *
     * Never a fragment or an entity per definition: pools are tombstone-mode here, so churning fragment TYPES as a
     * state machine swaps move sets mid-fight is the expensive shape, and the whole argument for an O(1) swap is
     * that activating a set touches one value. It is also what makes the live matcher inspectable as three arrays
     * rather than as a scattering of entities a debugger would have to reassemble.
     *
     * `_LastScannedFrameIndex` is what decouples the scan's cadence from the sampler's. The record advances at a
     * fixed 60 Hz while this runs every render frame, so above that rate most passes have no new row to read and
     * below it several rows arrive at once — remembering the last frame consumed is what lets one pass answer both
     * without re-matching a row or skipping one.
     */
    struct CKINTENT_API FFragment_IntentMatcher_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IntentMatcher_Current);

    public:
        friend class FProcessor_IntentMatcher_HandleRequests;
        friend class FProcessor_IntentMatcher_Match;
        friend class FIntentMatcher_PhaseWriter;
        friend class ::UCk_Utils_IntentMatcher_UE;

    private:
        FCk_Intent_CompiledSet _ActiveSet;

        TArray<FIntentMatcher_PhaseRow> _PhaseRows;

        TArray<FIntentMatcher_RegisteredCapture> _RegisteredCaptures;

        TArray<FIntentMatcher_PendingEpisode> _PendingEpisodes;

        TArray<FIntentMatcher_HoldAccumulator> _HoldAccumulators;

        TArray<FIntentMatcher_ActiveLevel> _ActiveLevels;

        // The near-miss ring, BESIDE the phase rows rather than in them: a row is one definition's current
        // standing, while a diagnostic is one attempt that has already finished — the two have different
        // lifetimes and a row carrying its own history would grow without bound. Written only while
        // `ck.Intent.RecordScanDiagnostics` is on, and a plain overwrite-oldest ring exactly like the sampler's:
        // a slot is meaningless to a reader, which addresses newest-first.
        TArray<FCk_Intent_ScanDiagnostic> _ScanDiagnostics;

        int32 _ScanDiagnosticsNextWrite = 0;

        int32 _ScanDiagnosticsCount = 0;

        int32 _LastScannedFrameIndex = INDEX_NONE;

        // Answered once when the set is activated rather than per row: the level pass allocates before it can
        // discover it has nothing to match, and a set is immutable for as long as it is active, so the answer
        // cannot go stale under it.
        bool _SetHasLevelIntent = false;

    public:
        CK_PROPERTY_GET(_ActiveSet);
        CK_PROPERTY_GET(_PhaseRows);
        CK_PROPERTY_GET(_RegisteredCaptures);
        CK_PROPERTY_GET(_PendingEpisodes);
        CK_PROPERTY_GET(_HoldAccumulators);
        CK_PROPERTY_GET(_ActiveLevels);
        CK_PROPERTY_GET(_ScanDiagnostics);
        CK_PROPERTY_GET(_ScanDiagnosticsNextWrite);
        CK_PROPERTY_GET(_ScanDiagnosticsCount);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKINTENT_API FFragment_IntentMatcher_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_IntentMatcher_Requests);

    public:
        friend class FProcessor_IntentMatcher_HandleRequests;
        friend class ::UCk_Utils_IntentMatcher_UE;

    public:
        using RequestType = std::variant<FCk_Request_IntentMatcher_SwapSet>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The two transition signals, on the MATCHER entity. Intent identity rides the payload; there is no entity per
     * definition and there will not be one — a consumer binds on the matcher handle it already holds and filters
     * by name in the handler.
     *
     * **A late binder under `FireIfPayloadInFlight*` receives only the LAST payload.** Signals are a
     * PRESENTATION surface; the poll surface and the frame record are the authority. A sequence of transitions is
     * reconstructed from those, never from signal replay — nothing here buffers more than one payload, and a
     * consumer that tried to would be building a history the matcher does not keep.
     */
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKINTENT_API, OnIntentPhaseChanged, FCk_Delegate_IntentMatcher_PhaseChanged, FCk_Handle_IntentMatcher, FName, FGameplayTag, ECk_Intent_Phase, ECk_Intent_Phase, int32);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKINTENT_API, OnIntentCompleted, FCk_Delegate_IntentMatcher_Completed, FCk_Handle_IntentMatcher, FName, FGameplayTag, int32);
}

// --------------------------------------------------------------------------------------------------------------------
