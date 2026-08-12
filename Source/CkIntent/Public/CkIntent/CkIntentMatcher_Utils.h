#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkIntent/CkIntentMatcher_Fragment.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkIntentMatcher_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * The thing that turns a compiled set and a frame record into "that move just happened".
 *
 * A matcher is composed on an INPUT LAYER, and the layer is not incidental — it is the whole arbitration story. A
 * set only ever matches the events VISIBLE to its layer, so a modal that consumes above it silences every move
 * underneath it without either side knowing the other exists, and two layers each carrying their own set (the
 * player's moves, the vehicle's) neither see nor cancel each other. That is also why nothing here can be reached
 * without a matcher handle in the lookup path: there is no global "did the player just quarter-circle", because
 * the question is meaningless without saying WHERE it was asked.
 *
 * Activating a set is one deferred request and one value assignment. Capture registration follows the set — the
 * matcher registers a `Key` capture for each of the set's terminal buttons, resolved through the source's button
 * map — and re-resolution follows the map, so a rebind moves the physical key with no edit to any definition.
 *
 * The read surface is a POLL, deliberately, and it is latched: a completed intent names the frame it completed on
 * and stays that way until another completion replaces it. A consumer that needs an edge compares that frame
 * against the frame it last acted on.
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IntentMatcher"))
class CKINTENT_API UCk_Utils_IntentMatcher_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IntentMatcher_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IntentMatcher);

public:
    /**
     * Composes a matcher onto an input layer. Rejects a handle that is not one and an entity that already carries a
     * matcher, each rejection leaving nothing composed.
     *
     * The matcher starts with NO active set: it captures nothing, matches nothing, and answers `Idle` for every
     * question until a set is swapped in. There is no set to declare here because a set is baked, not authored in
     * a details panel, and a composition that took one would be an activation nothing could tell apart from the
     * request that exists for it.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Add Feature")
    static FCk_Handle_IntentMatcher
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_IntentMatcher_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_IntentMatcher
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Handle -> IntentMatcher Handle",
              meta = (CompactNodeTitle = "<AsIntentMatcher>", BlueprintAutocast))
    static FCk_Handle_IntentMatcher
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid IntentMatcher Handle",
              Category = "Ck|Utils|IntentMatcher",
              meta = (CompactNodeTitle = "INVALID_IntentMatcherHandle", Keywords = "make"))
    static FCk_Handle_IntentMatcher
    Get_InvalidHandle() { return {}; }

public:
    /**
     * Where a definition of the active set stands, addressed by the tag it was baked with.
     *
     * An intent the active set does not carry answers `Idle`, the same as one that carries it and has not completed.
     * The two are deliberately not distinguished: `Idle` is what a consumer acts on either way, and a phase enum
     * that grew an `Unknown` value would make every caller branch on a case only a typo produces. Ask
     * `Get_ActiveIntentCount` if what you need to know is whether a set is loaded at all.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Get Intent Phase")
    static ECk_Intent_Phase
    Get_IntentPhase(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag);

    /**
     * The same reading, addressed by the definition's name.
     *
     * The tag-keyed form above is the ruled primary surface, but the `Intent.*` tag namespace is still an open
     * question and nothing mints one yet — so every set baked today carries an EMPTY tag, which matches no row, and
     * the name form is what a v1 consumer and every test actually calls.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Get Intent Phase By Name")
    static ECk_Intent_Phase
    Get_IntentPhase_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName);

    /**
     * The record frame a completion landed on, or INDEX_NONE for any row that is not `Completed` — including one
     * still `Pending`, one that resolved `Failed`, and an intent the active set does not carry.
     *
     * Gating on the phase is what keeps the reading unambiguous: rows carry one frame for whichever phase they
     * are in, and a caller that could read a failure's frame as a completion's would act on a move that did not
     * happen.
     *
     * The frame index is the whole answer to "was this THIS frame or an old latch": it is the sampler's own
     * monotonic logic-frame counter, the same number `FCk_Intent_FrameRecord::Get_FrameIndex` reports, so a
     * consumer compares it against the frame it last acted on rather than against wall-clock time.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Try Get Completion Frame")
    static int32
    TryGet_CompletionFrame(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Try Get Completion Frame By Name")
    static int32
    TryGet_CompletionFrame_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName);

    /**
     * The record frame a LEVEL intent went `Active` on, or INDEX_NONE for any row that is not `Active` right now —
     * including one that was active and has since been released, and an intent the active set does not carry.
     *
     * The completion-frame gate, applied to the other lifecycle. A level row holds one frame like every other row,
     * so once it is released that frame names the release; a caller reading it as an activation would believe a
     * hold is still going. What it buys while the row IS active is the duration: this frame subtracted from the
     * record's latest is how long the layer has had the input, in the sampler's own logic frames.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Try Get Activation Frame")
    static int32
    TryGet_ActivationFrame(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Try Get Activation Frame By Name")
    static int32
    TryGet_ActivationFrame_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName);

    /** Whether a non-empty set is currently active. False after a deactivating swap and before the first one. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Get Has Active Set")
    static bool
    Get_HasActiveSet(
        const FCk_Handle_IntentMatcher& InMatcher);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Get Active Intent Count")
    static int32
    Get_ActiveIntentCount(
        const FCk_Handle_IntentMatcher& InMatcher);

    /** Whether a claimable intent has already been claimed. False for every row that is neither `Completed` nor `Active`. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Get Is Claimed")
    static bool
    Get_IsClaimed(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Get Is Claimed By Name")
    static bool
    Get_IsClaimed_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName);

    /**
     * Who holds the claim on a completed or an active intent, or an invalid handle when nobody does.
     *
     * The claimant is the entity that asked, not a token the matcher minted: a consumer that already holds the
     * handle it claimed with can compare directly instead of remembering a receipt.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Try Get Claimed By")
    static FCk_Handle
    TryGet_ClaimedBy(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Try Get Claimed By Name")
    static FCk_Handle
    TryGet_ClaimedBy_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName);

    /**
     * Whether `ck.Intent.RecordScanDiagnostics` is on. Global, not per-matcher: it is a debugging switch, and a
     * per-matcher one would leave a reader wondering which matcher they forgot to arm.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Get Scan Diagnostics Enabled")
    static bool
    Get_ScanDiagnosticsEnabled();

    /**
     * The last few backward scans this matcher ran, NEWEST FIRST — the near-miss list.
     *
     * A failed scan leaves no other trace anywhere: the matcher simply declines to complete, so a move a player
     * felt should have come out is invisible to the record, the phase rows and the signals alike. This is the
     * only surface that can answer "which step had nothing behind it, and by how many frames".
     *
     * EMPTY unless `ck.Intent.RecordScanDiagnostics` was on while the scans ran. Recording is opt-in because the
     * ring is written on every scan attempt, and a chord window polls one attempt per frame — the honest record
     * of a busy matcher is more entries than anyone wants to pay for by default.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Get Scan Diagnostics")
    static TArray<FCk_Intent_ScanDiagnostic>
    Get_ScanDiagnostics(
        const FCk_Handle_IntentMatcher& InMatcher);

    /**
     * The physical keys the matcher currently has captures registered for, one per resolved terminal button.
     *
     * This is the matcher's own bookkeeping, NOT the layer's live capture set: a capture edit is deferred, so the
     * layer does not carry a just-registered row until the request drains, while this answers the moment the swap
     * or the re-resolution decided it. A terminal whose button resolves to nothing contributes no key.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Get Registered Capture Keys")
    static TArray<FKey>
    Get_RegisteredCaptureKeys(
        const FCk_Handle_IntentMatcher& InMatcher);

public:
    /**
     * Activates a compiled set, replacing whatever the matcher was running. Deferred, and ATOMIC: every terminal
     * button in the set is resolved against the source's button map first, and a single one that resolves to no key
     * — a button the map never minted, or a mapped button the player left unbound — rejects the WHOLE swap with
     * `Failed`, leaving the previous set active and its captures untouched.
     *
     * That is a RESULT rather than a diagnostic on purpose. An unbound mapping is a state a player can put the game
     * in from a settings screen, so the caller is told and decides; a definition naming a button that does not
     * exist at all was already rejected at bake time and cannot reach here.
     *
     * A default-constructed set DEACTIVATES the matcher — captures removed, rows cleared — and completes
     * `Succeeded`, because that is what the caller asked for.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Request Swap Set",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IntentMatcher
    Request_SwapSet(
        UPARAM(ref) FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Request_IntentMatcher_SwapSet& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /**
     * Takes exclusive ownership of a completed — or of a currently ACTIVE level — intent. **IMMEDIATE, not deferred** — the mutation lands on the
     * calling stack and the completion delegate fires synchronously after it, which is the house immediate-mutator
     * escape hatch and is declared as such.
     *
     * The reason is the whole point of the mechanism: a deferred claim makes same-frame double-claim a race, and
     * two pollers must observe the first claim SYNCHRONOUSLY. Nothing else about exclusivity survives a frame of
     * lag.
     *
     * Outcomes, all of them decided against one row:
     * - Completed or Active, and unclaimed -> `Succeeded`, claim stamped
     * - Completed or Active, already claimed by the SAME claimant -> `Succeeded`, stamp untouched (idempotent; the
     *   caller's intent already holds, and re-stamping would move a claim frame nobody asked to move)
     * - Completed or Active, claimed by ANOTHER entity -> `Failed`. That is the exclusion working, not an error.
     * - `Idle` / `Pending` / `Failed` -> `Failed`. There is nothing to take ownership of.
     * - a name the active set does not carry -> `Failed`
     * - an invalid matcher or an invalid claimant -> `Failed_NotEnqueued` (rejected at the boundary, same call
     *   stack, nothing touched)
     *
     * There is deliberately no claim that reaches DOWN the layer stack. That request is `handled == true` reborn;
     * the mechanism for "block the layers below me" remains a capture edit. Two consumers on the SAME layer are
     * ordered by processor order, which is accepted and documented — an intent that needs single-consumer
     * semantics lives behind one consumer.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Request Claim",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IntentMatcher
    Request_Claim(
        UPARAM(ref) FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Request_IntentMatcher_Claim& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    /**
     * Every phase transition on this matcher, with the intent named in the payload — bind once and filter by name
     * in the handler. There is no per-intent handle to resolve and there will not be one.
     *
     * **Signals are the PRESENTATION surface; the poll surface is the authority.** A late binder under
     * `FireIfPayloadInFlight*` receives only the LAST payload — a sequence of transitions is reconstructed from
     * `Get_IntentPhase*` and the frame record, never from signal replay. Nothing here buffers more than one
     * payload, so a consumer that tried would be reading a history the matcher does not keep.
     *
     * The default policy replays only a payload fired THIS frame, which is the one policy that cannot make the
     * signal contradict the poll: a latch cannot decay inside the frame it was stamped on. `FireIfPayloadInFlight`
     * is available and is the right choice for a consumer that binds a frame or two late — with the caveat that
     * the replayed payload may already have been decayed away, so check the frame it carries.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Bind To OnIntentPhaseChanged")
    static FCk_Handle_IntentMatcher
    BindTo_OnIntentPhaseChanged(
        UPARAM(ref) FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Delegate_IntentMatcher_PhaseChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Unbind From OnIntentPhaseChanged")
    static FCk_Handle_IntentMatcher
    UnbindFrom_OnIntentPhaseChanged(
        UPARAM(ref) FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Delegate_IntentMatcher_PhaseChanged& InDelegate);

    /** The presentational subset: fires on every transition INTO `Completed`. Same replay law as above. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Bind To OnIntentCompleted")
    static FCk_Handle_IntentMatcher
    BindTo_OnIntentCompleted(
        UPARAM(ref) FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Delegate_IntentMatcher_Completed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentMatcher",
              DisplayName = "[Ck][IntentMatcher] Unbind From OnIntentCompleted")
    static FCk_Handle_IntentMatcher
    UnbindFrom_OnIntentCompleted(
        UPARAM(ref) FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Delegate_IntentMatcher_Completed& InDelegate);

private:
    static auto
    DoGet_IntentIndex_ByTag(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag) -> int32;

    static auto
    DoGet_IntentIndex_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName) -> int32;

    static auto
    DoGet_CompletionFrame(
        const FCk_Handle_IntentMatcher& InMatcher,
        int32 InIntentIndex) -> int32;

    static auto
    DoGet_ActivationFrame(
        const FCk_Handle_IntentMatcher& InMatcher,
        int32 InIntentIndex) -> int32;

    static auto
    DoGet_ClaimedBy(
        const FCk_Handle_IntentMatcher& InMatcher,
        int32 InIntentIndex) -> FCk_Handle;

    static auto
    DoGet_LatestRecordFrame(
        const FCk_Handle_IntentMatcher& InMatcher) -> int32;
};

// --------------------------------------------------------------------------------------------------------------------
