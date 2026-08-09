#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkInput/CkInputButtonMap_Fragment_Data.h"

#include "CkIntent/CkIntentGrammar_Data.h"

#include "CkIntentCompiledSet_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Why a set of definitions could not be compiled.
 *
 * Every value names a DISTINCT defect, for the same reason the parse errors do: a rejection a caller cannot act
 * on is barely better than a crash. `None` is the no-rejection reading — what a successful bake carries, and what
 * a default-constructed result no bake produced carries too.
 */
UENUM(BlueprintType)
enum class ECk_Intent_BakeError : uint8
{
    None,

    // Nothing to compile. An empty set is never what a caller meant, and a matcher handed one would silently
    // answer nothing forever.
    NoDefinitions,
    // The chord window must be a real number of frames — see `FCk_Intent_CompiledSet::_ChordWindowFrames`.
    NonPositiveChordWindow,

    // A definition carried no name. Names are how arbitration, debugging and every later diagnostic refer to an
    // intent, and the parser deliberately does not police them — this is where that is caught.
    UnnamedIntent,
    // Two definitions carried the same name. Which one a resolution table entry meant would be unanswerable.
    DuplicateIntentName,

    // A button atom named something the supplied rows do not map. Unknown at BAKE time is a rejection precisely so
    // it can never be a runtime surprise.
    UnknownButtonName,
    // Two rows answer one name with DIFFERENT identities. Picking either would silently discard a declaration the
    // caller wrote; the same name against the same identity is idempotent and accepted.
    ConflictingButtonRow,

    // Two intents share a terminal button at the SAME priority, so which of them a press resolves to is undefined.
    // Arbitration over one terminal has to be a strict total order or it is not arbitration, only iteration luck.
    PriorityTieOnSharedTerminal
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Intent_BakeError);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One atom of a compiled step: a direction, or a button as a resolved IDENTITY.
 *
 * The difference from `FCk_Intent_Atom` is the whole point of the bake — a name became a `FCk_Input_ButtonId`, so
 * nothing downstream ever has to resolve anything again, and a rebind that moves which key produces that button
 * changes nothing here. There is deliberately no name left on the atom: carrying both would be two spellings of
 * one identity, and the moment a map re-derived, only one of them would still be true.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_CompiledAtom
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Intent_CompiledAtom);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_AtomKind _Kind = ECk_Intent_AtomKind::Direction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_Octant _Direction = ECk_Intent_Octant::Neutral;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _Button;

public:
    CK_PROPERTY_GET(_Kind);
    CK_PROPERTY_GET(_Direction);
    CK_PROPERTY_GET(_Button);

public:
    static auto
    Direction(
        ECk_Intent_Octant InOctant) -> FCk_Intent_CompiledAtom
    {
        auto Atom = FCk_Intent_CompiledAtom{};
        Atom._Kind = ECk_Intent_AtomKind::Direction;
        Atom._Direction = InOctant;
        return Atom;
    }

    static auto
    Button(
        FCk_Input_ButtonId InButton) -> FCk_Intent_CompiledAtom
    {
        auto Atom = FCk_Intent_CompiledAtom{};
        Atom._Kind = ECk_Intent_AtomKind::Button;
        Atom._Button = MoveTemp(InButton);
        return Atom;
    }

public:
    auto operator==(
        const FCk_Intent_CompiledAtom& InOther) const -> bool
    {
        return _Kind == InOther._Kind && _Direction == InOther._Direction && _Button == InOther._Button;
    }

    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(FCk_Intent_CompiledAtom);

public:
    FCk_Intent_CompiledAtom() = default;
};

// --------------------------------------------------------------------------------------------------------------------

/** One position of a compiled sequence — the resolved twin of `FCk_Intent_Step`, with the same set-of-atoms shape. */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_CompiledStep
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Intent_CompiledStep);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Intent_CompiledAtom> _Atoms;

public:
    CK_PROPERTY_GET(_Atoms);

public:
    auto Get_IsChord() const -> bool { return _Atoms.Num() > 1; }
    auto Get_IsEmpty() const -> bool { return _Atoms.IsEmpty(); }

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Intent_CompiledStep, _Atoms);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * One compiled move: everything the matcher needs and nothing it does not.
 *
 * This does NOT carry the `FCk_Intent_Definition` it came from. A compiled intent holding both the unresolved and
 * the resolved step list would be two answers to "what does this move ask for", and the matcher would have to
 * know — forever — which of them was authoritative. The notation is the authoring input; this is the artifact.
 *
 * Identity and modifiers are copied across verbatim: they were never notation-dependent, and re-deriving them
 * would be a second reading of the same authored value.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_CompiledIntent
{
    GENERATED_BODY()

    friend class UCk_Utils_IntentGrammar_UE;

public:
    CK_GENERATED_BODY(FCk_Intent_CompiledIntent);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FName _Name = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _IntentTag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Intent_CompiledStep> _Steps;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _WindowFrames = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _HoldFrames = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_Lenience _Lenience = ECk_Intent_Lenience::Strict;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_IntentTag);
    CK_PROPERTY_GET(_Steps);
    CK_PROPERTY_GET(_WindowFrames);
    CK_PROPERTY_GET(_HoldFrames);
    CK_PROPERTY_GET(_Priority);
    CK_PROPERTY_GET(_Lenience);

public:
    FCk_Intent_CompiledIntent() = default;

private:
    FCk_Intent_CompiledIntent(
        FName InName,
        FGameplayTag InIntentTag,
        TArray<FCk_Intent_CompiledStep> InSteps,
        int32 InWindowFrames,
        int32 InHoldFrames,
        int32 InPriority,
        ECk_Intent_Lenience InLenience)
        : _Name(InName)
        , _IntentTag(MoveTemp(InIntentTag))
        , _Steps(MoveTemp(InSteps))
        , _WindowFrames(InWindowFrames)
        , _HoldFrames(InHoldFrames)
        , _Priority(InPriority)
        , _Lenience(InLenience)
    { }
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * How long a press of one button must go unanswered before the matcher may act on it, and why.
 *
 * **Zero is the whole design.** A default-constructed verdict defers for nothing, and that is what the
 * overwhelming majority of buttons get — including every button that is merely a terminal shared by several
 * intents, and every button that appears mid-sequence in a longer move. Deferral is not computed and then found
 * to be unnecessary; it is ABSENT unless one of the two forward ambiguities below was found, which is what makes
 * "a sequence suffix never defers" a structural property of the bake rather than a special case someone has to
 * remember to keep working.
 *
 * The two causes are independent and may both apply, which is why they are two named frame counts and not one
 * enum: a button can simultaneously have a hold sibling and be a chord member, and collapsing that to a single
 * cause would lose whichever one the matcher was not asked about. Zero means that cause does not apply — the same
 * not-declared convention `_WindowFrames` and `_HoldFrames` use, and unambiguous for the same reason (a real
 * deferral always carries a real, positive number of frames).
 *
 * How to combine them, when both apply, is the MATCHER's call and deliberately not answered here.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_DeferralVerdict
{
    GENERATED_BODY()

    friend class UCk_Utils_IntentGrammar_UE;

public:
    CK_GENERATED_BODY(FCk_Intent_DeferralVerdict);

private:
    // The longest hold any intent sharing this terminal declares. A tap-and-hold pair on one button cannot be
    // told apart until the hold threshold has passed or the button came up.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _HoldSiblingFrames = 0;

    // The set's chord window. This button completes a chord in one intent and some OTHER intent's terminal too,
    // so a lone press cannot yet say which — the partner press may still be in flight.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _ChordMemberFrames = 0;

public:
    CK_PROPERTY_GET(_HoldSiblingFrames);
    CK_PROPERTY_GET(_ChordMemberFrames);

public:
    auto Get_IsDeferred() const -> bool { return _HoldSiblingFrames > 0 || _ChordMemberFrames > 0; }

public:
    auto operator==(
        const FCk_Intent_DeferralVerdict& InOther) const -> bool
    {
        return _HoldSiblingFrames == InOther._HoldSiblingFrames && _ChordMemberFrames == InOther._ChordMemberFrames;
    }

    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(FCk_Intent_DeferralVerdict);

public:
    FCk_Intent_DeferralVerdict() = default;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * One terminal button and the intents a press of it can complete, most-dominant first.
 *
 * Indices address the set's own `_Intents` array rather than repeating names and priorities here. A row carrying
 * its own copy of either would be a second place the truth lived, and the set is immutable once baked — so an
 * index is stable for exactly as long as the row is.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_ResolutionRow
{
    GENERATED_BODY()

    friend class UCk_Utils_IntentGrammar_UE;

public:
    CK_GENERATED_BODY(FCk_Intent_ResolutionRow);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _TerminalButton;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<int32> _IntentIndices;

public:
    CK_PROPERTY_GET(_TerminalButton);
    CK_PROPERTY_GET(_IntentIndices);

public:
    FCk_Intent_ResolutionRow() = default;

private:
    FCk_Intent_ResolutionRow(
        FCk_Input_ButtonId InTerminalButton,
        TArray<int32> InIntentIndices)
        : _TerminalButton(MoveTemp(InTerminalButton))
        , _IntentIndices(MoveTemp(InIntentIndices))
    { }
};

// --------------------------------------------------------------------------------------------------------------------

/** One button that DOES defer, and by how much. Buttons absent from the array defer for nothing. */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_DeferralRow
{
    GENERATED_BODY()

    friend class UCk_Utils_IntentGrammar_UE;

public:
    CK_GENERATED_BODY(FCk_Intent_DeferralRow);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _Button;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Intent_DeferralVerdict _Verdict;

public:
    CK_PROPERTY_GET(_Button);
    CK_PROPERTY_GET(_Verdict);

public:
    FCk_Intent_DeferralRow() = default;

private:
    FCk_Intent_DeferralRow(
        FCk_Input_ButtonId InButton,
        FCk_Intent_DeferralVerdict InVerdict)
        : _Button(MoveTemp(InButton))
        , _Verdict(InVerdict)
    { }
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * One name and the button identity it stands for — the bake's whole view of a project's button vocabulary.
 *
 * Rows rather than a live map handle, deliberately: the bake is a pure function over values and must be callable
 * with no world, no entity and no composed ButtonMap at all. A caller that HAS a map builds rows from it; a test,
 * a cook step or a validation tool supplies rows it made up. Neither can tell the bake apart.
 *
 * A name may be declared more than once so long as every row agrees on the identity — a caller stitching rows
 * from two overlapping sources should not have to de-duplicate first. Rows that DISAGREE are rejected, because
 * one name with two answers has no correct resolution to pick.
 *
 * This is the one type here a caller is meant to construct — it is the bake's INPUT.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_ButtonNameRow
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Intent_ButtonNameRow);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Name = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _Button;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_Button);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Intent_ButtonNameRow, _Name, _Button);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * A validated, self-contained set of moves: what a character is currently able to express.
 *
 * Everything the matcher reads at runtime is here, as VALUES — no handles, no pointers, no back-references to the
 * definitions or the map it was built from. That is what makes activating a set an assignment rather than a
 * rebuild: a copy of a compiled set is indistinguishable from the original, so a state machine can swap one in
 * mid-fight without anything re-resolving, re-sorting or re-validating.
 *
 * It is also why the set is baked ONCE and never edited. Every table in it is derived from every intent in it —
 * add one move and the resolution ordering, the deferral verdicts and the priority-tie verdict can all change —
 * so there is no coherent "add an intent" operation, only a rebake.
 *
 * `_ChordWindowFrames` is uniform across the set rather than per-move: the notation has no chord-window syntax
 * and the grammar is deliberately kept from growing one. It is the window inside which two atoms of a chord count
 * as simultaneous, and it is what a chord-membership deferral waits out.
 *
 * **Compiled sets are produced ONLY by the bake**, on the same terms definitions are produced only by the parser:
 * no setters, no public field constructor, one private constructor whose only friend is
 * `UCk_Utils_IntentGrammar_UE`. The empty set anyone can default-construct is the "no set" value a rejected bake
 * carries.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_CompiledSet
{
    GENERATED_BODY()

    friend class UCk_Utils_IntentGrammar_UE;

public:
    CK_GENERATED_BODY(FCk_Intent_CompiledSet);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Intent_CompiledIntent> _Intents;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Intent_ResolutionRow> _ResolutionTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Intent_DeferralRow> _Deferrals;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _ChordWindowFrames = 0;

public:
    CK_PROPERTY_GET(_Intents);
    CK_PROPERTY_GET(_ResolutionTable);
    CK_PROPERTY_GET(_Deferrals);
    CK_PROPERTY_GET(_ChordWindowFrames);

public:
    auto Get_IsEmpty() const -> bool { return _Intents.IsEmpty(); }

public:
    FCk_Intent_CompiledSet() = default;

private:
    FCk_Intent_CompiledSet(
        TArray<FCk_Intent_CompiledIntent> InIntents,
        TArray<FCk_Intent_ResolutionRow> InResolutionTable,
        TArray<FCk_Intent_DeferralRow> InDeferrals,
        int32 InChordWindowFrames)
        : _Intents(MoveTemp(InIntents))
        , _ResolutionTable(MoveTemp(InResolutionTable))
        , _Deferrals(MoveTemp(InDeferrals))
        , _ChordWindowFrames(InChordWindowFrames)
    { }
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * What one bake produced: a compiled set, or the reason there isn't one.
 *
 * Same shape as `FCk_Intent_ParseResult` — outcome as the mode, error as its value, and an EMPTY set on
 * rejection. One bad definition invalidates the whole set rather than yielding the moves that happened to
 * compile: a character with a silently missing special is a bug that ships, where a set that refuses to compile
 * is a bug that gets fixed.
 *
 * The context fields are populated per reason and are otherwise their unset value:
 *   `UnknownButtonName`           -> `_OffendingIntent`, `_OffendingButtonName`
 *   `UnnamedIntent`               -> nothing (there is no name to give)
 *   `DuplicateIntentName`         -> `_OffendingIntent` (the name both definitions claimed)
 *   `PriorityTieOnSharedTerminal` -> `_OffendingIntent`, `_ConflictingIntent`, `_OffendingButton`
 *   `ConflictingButtonRow`        -> `_OffendingButtonName`, `_OffendingButton`, `_ConflictingButton`
 *   `NoDefinitions` / `NonPositiveChordWindow` -> nothing
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_BakeResult
{
    GENERATED_BODY()

    friend class UCk_Utils_IntentGrammar_UE;

public:
    CK_GENERATED_BODY(FCk_Intent_BakeResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_SucceededFailed _Outcome = ECk_SucceededFailed::Failed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Intent_CompiledSet _CompiledSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_BakeError _Error = ECk_Intent_BakeError::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FName _OffendingIntent = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FName _ConflictingIntent = NAME_None;

    // The name that did not resolve. It stays a NAME on purpose: minting a ButtonId to describe a button that has
    // no identity would be inventing the very thing the rejection is about.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FName _OffendingButtonName = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _OffendingButton;

    // The second identity a contested name was answered with. Both are carried because the fix is to delete one
    // of them, and an author shown only one cannot tell which row they are looking at.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _ConflictingButton;

public:
    CK_PROPERTY_GET(_Outcome);
    CK_PROPERTY_GET(_CompiledSet);
    CK_PROPERTY_GET(_Error);
    CK_PROPERTY_GET(_OffendingIntent);
    CK_PROPERTY_GET(_ConflictingIntent);
    CK_PROPERTY_GET(_OffendingButtonName);
    CK_PROPERTY_GET(_OffendingButton);
    CK_PROPERTY_GET(_ConflictingButton);

public:
    auto Get_IsSucceeded() const -> bool { return _Outcome == ECk_SucceededFailed::Succeeded; }

public:
    FCk_Intent_BakeResult() = default;

private:
    FCk_Intent_BakeResult(
        ECk_SucceededFailed InOutcome,
        FCk_Intent_CompiledSet InCompiledSet,
        ECk_Intent_BakeError InError)
        : _Outcome(InOutcome)
        , _CompiledSet(MoveTemp(InCompiledSet))
        , _Error(InError)
    { }
};

// --------------------------------------------------------------------------------------------------------------------
