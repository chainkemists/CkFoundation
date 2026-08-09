#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkInput/CkInputLayer_Fragment_Data.h"

#include "CkIntent/CkIntentCompiledSet_Data.h"

#include "CkIntentMatcher_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKINTENT_API FCk_Handle_IntentMatcher : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_IntentMatcher); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_IntentMatcher);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Where one definition of the active set currently stands.
 *
 * The order is the life of one press: nothing, then maybe a wait, then an answer either way. `Pending` exists
 * only where the baked verdict said a press cannot be acted on yet — a tap that might become a hold, a button
 * that might become a chord — and it is the honest reading of that window: the input happened, the answer has
 * not.
 *
 * `Completed` and `Failed` are both LATCHED and both name the frame they resolved on. Nothing decays either back
 * to `Idle` (Phase 6 owns decay), so a poller reads "how did this last resolve, and on which frame" rather than
 * "is it resolving right now" — comparing that frame against the frame the poller cares about is what turns a
 * latch into an edge. A new press on the same terminal supersedes the previous answer, exactly as a second
 * completion replaces the first.
 *
 * `Failed` is reserved for an ambiguity that resolved with NOTHING matching — the input was real, was waited on,
 * and answered no move. A candidate that merely LOST arbitration returns to `Idle` instead: every tap press would
 * otherwise permanently mark its hold sibling as failed, which is noise rather than information.
 */
UENUM(BlueprintType)
enum class ECk_Intent_Phase : uint8
{
    Idle,
    Pending,
    Completed,
    Failed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Intent_Phase);

// --------------------------------------------------------------------------------------------------------------------

/**
 * What a matcher is, beyond the set it happens to be holding.
 *
 * The capture behavior is the only knob, and it is per-matcher rather than per-set: whether an intent layer masks
 * the layers below it is a property of what that layer IS (a gameplay layer consumes, an observer watches), not of
 * which move list it is currently running. `Consume` is the default because a matcher that did not mask would let
 * every intent key fall through to whatever sits underneath it, which is almost never what a move set means.
 *
 * There is deliberately no input-source field. The layer this feature is composed on already names its source, and
 * a second spelling of the same thing is a second spelling that can disagree with the first the moment either is
 * edited.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Fragment_IntentMatcher_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_IntentMatcher_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InputLayer_CaptureBehavior _CaptureBehavior = ECk_InputLayer_CaptureBehavior::Consume;

    // How long a resolved latch stands before it returns to Idle, in the record's own logic frames. 20 is a
    // third of a second at the sampler's 60 Hz — the generous end of an input buffer, long enough that a
    // consumer running a frame or two behind still sees the completion, short enough that nobody claims it
    // later in the fight. Rejected at composition if it is not positive: a latch that expires instantly is
    // unpollable, and one that never expires is the stale-claim hazard the decay exists to remove.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _LatchDecayFrames = 20;

public:
    CK_PROPERTY_GET(_CaptureBehavior);
    CK_PROPERTY(_LatchDecayFrames);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_IntentMatcher_ParamsData, _CaptureBehavior);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Activate a compiled set on a matcher, replacing whatever it was running.
 *
 * The payload is the whole set and nothing else, because a set is values all the way down: activating one is an
 * assignment, not a rebuild, and there is no second parameter that could describe half of it.
 *
 * A DEFAULT-CONSTRUCTED set is the deactivation request. It is not a rejection and not a special mode — an empty
 * set has no terminals to capture and no definitions to complete, which is exactly what "this layer no longer
 * matches anything" means, so the two cases are one code path.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Request_IntentMatcher_SwapSet : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IntentMatcher_SwapSet);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IntentMatcher_SwapSet);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Intent_CompiledSet _CompiledSet;

public:
    CK_PROPERTY_GET(_CompiledSet);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IntentMatcher_SwapSet, _CompiledSet);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Take exclusive ownership of a completed intent, so a second consumer polling the same frame cannot also act on
 * it.
 *
 * The intent is named rather than indexed because an index is an artifact of the bake a consumer never sees, and
 * named by NAME rather than by tag for the same reason the phase poll's `_ByName` form is the one v1 uses: no set
 * baked today carries a tag.
 *
 * The claimant is a handle rather than a bool because "already claimed" is not the interesting question — WHO
 * claimed it is, and it is what makes a re-claim by the same consumer idempotent instead of a failure.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Request_IntentMatcher_Claim : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IntentMatcher_Claim);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IntentMatcher_Claim);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _IntentName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Claimant;

public:
    CK_PROPERTY_GET(_IntentName);
    CK_PROPERTY_GET(_Claimant);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IntentMatcher_Claim, _IntentName, _Claimant);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Every phase transition on the matcher, complete enough to be the only thing a consumer needs.
 *
 * The intent's IDENTITY rides the payload rather than the signal living on a per-intent entity: consumers bind on
 * the matcher handle they already hold and filter by name in the handler when they care about one move. Minting an
 * entity per definition would buy a narrower subscription at the cost of a lifetime for every move a character can
 * express, and of consumers having to resolve one before they could listen at all.
 *
 * Both phases are carried because a transition is the interesting thing, not the destination — `Completed -> Idle`
 * is a latch expiring and `Pending -> Idle` is a move losing arbitration, and a payload that named only the new
 * phase could not tell them apart.
 */
DECLARE_DYNAMIC_DELEGATE_SixParams(
    FCk_Delegate_IntentMatcher_PhaseChanged,
    FCk_Handle_IntentMatcher, InMatcher,
    FName, InIntentName,
    FGameplayTag, InIntentTag,
    ECk_Intent_Phase, InPreviousPhase,
    ECk_Intent_Phase, InNewPhase,
    int32, InFrame);

// --------------------------------------------------------------------------------------------------------------------

/**
 * The presentational case, which is most of them: a move just landed.
 *
 * A strict subset of what `PhaseChanged` already reports, and it exists anyway because "play the animation when
 * the special comes out" should not require a consumer to write a two-phase comparison it can get wrong. It fires
 * on every transition INTO `Completed`, including a re-completion of a move that was already latched.
 */
DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_IntentMatcher_Completed,
    FCk_Handle_IntentMatcher, InMatcher,
    FName, InIntentName,
    FGameplayTag, InIntentTag,
    int32, InFrame);

// --------------------------------------------------------------------------------------------------------------------

/** How one backward scan ended. Two outcomes because a scan either found its whole definition behind it or did not. */
UENUM(BlueprintType)
enum class ECk_Intent_ScanOutcome : uint8
{
    Matched,
    FailedAtStep
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Intent_ScanOutcome);

// --------------------------------------------------------------------------------------------------------------------

/**
 * What became of one step of a walked definition.
 *
 * The three failures are separated because they are three different things to fix, and "the move did not come
 * out" is the same sentence for all of them. A terminal that was not satisfied means the press itself was wrong
 * for this move — the direction was not held, or the chord's partner was not down. A window that ran out means
 * the input WAS there in spirit and the player was too slow, which is a tuning answer. Contiguity broken means
 * the player let the stick pass through a direction the definition does not mention, which is a lenience answer.
 */
UENUM(BlueprintType)
enum class ECk_Intent_ScanStepOutcome : uint8
{
    Matched,

    // The single row this step was evaluated against did not satisfy it. Only a terminal can end this way — every
    // earlier step is sought across a span of rows rather than tested against one.
    NotSatisfied,

    // The walk ran out of rows to look at before this step matched: the definition's `w=` frames were spent, or
    // the ring stopped retaining that far back.
    WindowExhausted,

    // Strict lenience: a row between two matched steps held neither of their octants, so the sequence was not
    // contiguous and the walk refused to skip over it.
    ContiguityBroken
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Intent_ScanStepOutcome);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One step of one scan, as it was walked.
 *
 * `_FramesExamined` is the answer to "by how many frames did this miss": the number of record rows the walk read
 * while seeking this step, including the one that matched it. For a terminal it is always one — a terminal is
 * tested against the press row and nowhere else. For the step a scan died on it is how far back the walk got
 * before giving up, which read against the definition's `w=` is the whole diagnosis.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_ScanStepDiagnostic
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Intent_ScanStepDiagnostic);

private:
    // Index into the DEFINITION's steps, not into the walk. The walk runs backwards, so this is what lets a
    // reader put the entries back in the order the move is written.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _StepIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_ScanStepOutcome _Outcome = ECk_Intent_ScanStepOutcome::Matched;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _MatchedAtFrame = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _FramesExamined = 0;

public:
    CK_PROPERTY_GET(_StepIndex);
    CK_PROPERTY_GET(_Outcome);
    CK_PROPERTY_GET(_MatchedAtFrame);
    CK_PROPERTY_GET(_FramesExamined);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Intent_ScanStepDiagnostic, _StepIndex, _Outcome, _MatchedAtFrame, _FramesExamined);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * One completed backward scan, recorded so a debugger can answer "why did nothing happen when I pressed that".
 *
 * A failed scan otherwise leaves NO trace anywhere — the matcher simply declines to complete, and the near-miss
 * that a player felt is invisible to every other surface. This is the only record of it.
 *
 * `_Steps` are in WALK order: the terminal first, then each earlier step as the backwards walk reached it. That
 * is the order the evidence was gathered in, and it stops at the step the scan died on — a scan that failed
 * three steps from the front has nothing to say about the two it never looked for. `_StepIndex` carries the
 * definition's own numbering for a reader who wants the move's order back.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_ScanDiagnostic
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Intent_ScanDiagnostic);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _IntentName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _IntentTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _TerminalFrame = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_ScanOutcome _Outcome = ECk_Intent_ScanOutcome::FailedAtStep;

    // The definition index of the step the walk died on, INDEX_NONE on a match. Duplicated from the last entry
    // of `_Steps` on purpose: it is the one field a near-miss list shows without expanding anything.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _FailedStepIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Intent_ScanStepDiagnostic> _Steps;

public:
    CK_PROPERTY(_IntentName);
    CK_PROPERTY(_IntentTag);
    CK_PROPERTY(_TerminalFrame);
    CK_PROPERTY(_Outcome);
    CK_PROPERTY(_FailedStepIndex);
    CK_PROPERTY(_Steps);

public:
    FCk_Intent_ScanDiagnostic() = default;
};

// --------------------------------------------------------------------------------------------------------------------
