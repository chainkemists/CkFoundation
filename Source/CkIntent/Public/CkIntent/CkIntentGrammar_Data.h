#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkIntent/CkIntentSampler_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkIntentGrammar_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Which of an atom's two payloads is the real one.
 *
 * An atom is a direction OR a button and never both, so the pair is expressed as a mode plus two slots rather
 * than as two independently-readable fields: the unused slot always holds its default, and a consumer that
 * branches on the mode can never read a stale half of the other reading.
 */
UENUM(BlueprintType)
enum class ECk_Intent_AtomKind : uint8
{
    Direction,
    Button
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Intent_AtomKind);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Whether a definition tolerates unmatched directions BETWEEN its steps.
 *
 * Two states, spelled as an enum rather than a flag, because both readings are named at every call site the
 * matcher has: `Strict` says a row between two matched steps may only be holding one of them, `Lenient` says a
 * transient direction the definition does not mention may be skipped over. The constraint is about intervening
 * OCTANTS and nothing else — two neighbouring steps that name no direction constrain nothing either way.
 */
UENUM(BlueprintType)
enum class ECk_Intent_Lenience : uint8
{
    Strict,
    Lenient
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Intent_Lenience);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Why a notation string produced no definition.
 *
 * Every value names a DISTINCT malformation, because a designer reading "this move did not parse" needs to know
 * which token to fix, and a test asserting a rejection needs to prove it rejected for the reason under test
 * rather than incidentally. `None` is the no-rejection-recorded reading: it is what a successful parse carries
 * and what a default-constructed result — one no parse produced — carries too.
 */
UENUM(BlueprintType)
enum class ECk_Intent_ParseError : uint8
{
    None,

    // The notation was empty or nothing but whitespace.
    EmptyNotation,
    // The notation carried modifiers and no steps at all ("w=200 lenient").
    NoSteps,

    // A token shaped like a modifier (it carries an '=') whose key is neither `w` nor `hold`.
    UnknownModifier,
    // The same modifier was declared twice. Last-wins would silently discard an authored value.
    DuplicateModifier,
    // A step token followed a modifier. Modifiers are TRAILING — everything after the first one must be one.
    ModifierNotTrailing,
    // `w=` with a value that is not a positive decimal integer (0, negative, non-numeric, absent, or too long).
    MalformedWindow,
    // `hold=` with a value that is not a positive decimal integer, under the same rules as the window.
    MalformedHold,

    // A token beginning with a digit is a direction run, and every character of one must be a numpad digit 1-9.
    InvalidDirectionDigit,
    // A button token carried a character outside [A-Za-z0-9_].
    InvalidButtonName,
    // A chord carried an empty atom ("6+", "+LP", "6++LP").
    EmptyChordAtom,

    // A chord named the same atom twice. Button names compare case-insensitively, as FName does.
    ChordDuplicateAtom,
    // A chord named two directions. One stick cannot be in two octants at once.
    ChordTwoDirections,
    // A chord named neutral. Neutral is the ABSENCE of a direction, so it cannot be simultaneous with anything.
    ChordNeutralDirection
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Intent_ParseError);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One indivisible thing a step asks for: a direction, or a button by NAME.
 *
 * Directions are the sampler's own `ECk_Intent_Octant` vocabulary, so a definition and a frame record speak the
 * same eight-way language with nothing to translate at match time. The numpad digits the notation is written in
 * map onto it positionally — 6 = E, 9 = NE, 8 = N, 7 = NW, 4 = W, 1 = SW, 2 = S, 3 = SE, 5 = Neutral — which is
 * the octant enum's own angle order read off a numpad's face.
 *
 * A button is an unresolved NAME and stays one through the whole of parsing. Resolving `LP` to a ButtonId needs a
 * name -> ButtonId table that only the bake is given, and a parser that guessed would either invent an identity
 * (a second minting authority beside the ButtonMap) or fail on a notation that is perfectly well-formed and
 * simply names a button this project has not declared yet. Neither is the parser's call to make.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_Atom
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Intent_Atom);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_AtomKind _Kind = ECk_Intent_AtomKind::Direction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_Octant _Direction = ECk_Intent_Octant::Neutral;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FName _ButtonName = NAME_None;

public:
    CK_PROPERTY_GET(_Kind);
    CK_PROPERTY_GET(_Direction);
    CK_PROPERTY_GET(_ButtonName);

public:
    static auto
    Direction(
        ECk_Intent_Octant InOctant) -> FCk_Intent_Atom
    {
        auto Atom = FCk_Intent_Atom{};
        Atom._Kind = ECk_Intent_AtomKind::Direction;
        Atom._Direction = InOctant;
        return Atom;
    }

    static auto
    Button(
        FName InButtonName) -> FCk_Intent_Atom
    {
        auto Atom = FCk_Intent_Atom{};
        Atom._Kind = ECk_Intent_AtomKind::Button;
        Atom._ButtonName = InButtonName;
        return Atom;
    }

public:
    // The slot the kind does not select always holds its default, so comparing every field IS comparing the
    // payload — there is no state in which two atoms agree on kind and payload but disagree here.
    auto operator==(
        const FCk_Intent_Atom& InOther) const -> bool
    {
        return _Kind == InOther._Kind && _Direction == InOther._Direction && _ButtonName == InOther._ButtonName;
    }

    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(FCk_Intent_Atom);

public:
    FCk_Intent_Atom() = default;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * One position in a sequence: everything that must be true at the same moment for the sequence to advance.
 *
 * A step holds a SET of atoms rather than a direction-or-chord union, because a chord is not a different kind of
 * step — it is a step with more than one atom, and a matcher that walked two shapes would have to say which of
 * them a one-atom chord was. `Get_IsChord` is therefore a reading of the set, not a stored flag that could go
 * stale against it.
 *
 * A step with no atoms is the "no such step" value — what `Get_TerminalStep` answers for a definition that has
 * no steps. The parser never produces one inside a definition.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_Step
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Intent_Step);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Intent_Atom> _Atoms;

public:
    CK_PROPERTY_GET(_Atoms);

public:
    auto Get_IsChord() const -> bool { return _Atoms.Num() > 1; }
    auto Get_IsEmpty() const -> bool { return _Atoms.IsEmpty(); }

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Intent_Step, _Atoms);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * One authored move: the sequence, its modifiers, and the identity arbitration will refer to it by.
 *
 * The LAST step is the TERMINAL — the input that completes the move — and every earlier step is a prefix the
 * matcher scans backwards from it. That is a property of the list, not a separate field, so a definition cannot
 * carry a terminal that disagrees with its own steps.
 *
 * `_WindowFrames` and `_HoldFrames` are in LOGIC FRAMES at the sampler's fixed 60 Hz cadence, never milliseconds:
 * the record they will be matched against is indexed in frames, and a millisecond budget would mean a different
 * number of rows on every machine. Zero is the not-declared reading for both, which is unambiguous because the
 * parser rejects a declared zero — a real declaration always carries a real, positive number.
 *
 * The gameplay tag is CARRIED, not interpreted: nothing here mints, registers or validates a tag, because a tag
 * namespace is a project's to declare. The name and the priority are equally uninterpreted at parse time — an
 * unnamed definition and a tied priority are both real problems, and both are the BAKE's to reject, because only
 * the bake sees the whole set a name has to be unique within and a priority has to be unambiguous against.
 *
 * **Definitions are produced ONLY by the parser.** There are no setters and no public field constructor; the one
 * constructor that can populate an instance is private, and `UCk_Utils_IntentGrammar_UE` is its only friend. A
 * default-constructed definition is therefore the only one anybody else can make, and it is empty — which is the
 * "no definition" value a rejected parse carries.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_Definition
{
    GENERATED_BODY()

    friend class UCk_Utils_IntentGrammar_UE;

public:
    CK_GENERATED_BODY(FCk_Intent_Definition);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FName _Name = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _IntentTag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Intent_Step> _Steps;

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
    auto Get_IsEmpty() const -> bool { return _Steps.IsEmpty(); }

public:
    FCk_Intent_Definition() = default;

private:
    FCk_Intent_Definition(
        FName InName,
        FGameplayTag InIntentTag,
        TArray<FCk_Intent_Step> InSteps,
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
 * What one notation string parsed to: a definition, or the reason there isn't one.
 *
 * The outcome is a MODE and the error is its value, the house optionality shape — so there is no way to read a
 * definition without having read whether there is one, and no separate flag that can go stale against the payload.
 * A rejected result carries an EMPTY definition, never a partially-built one: a notation is accepted whole or not
 * at all, the same atomicity every composition in this codebase promises.
 *
 * `_ErrorToken` is the offending token verbatim, empty when the rejection is about the notation as a whole (an
 * empty string, a step list that turned out to be all modifiers). It exists so an authoring tool can point at the
 * text the designer has to fix instead of making them re-derive it from the reason.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_ParseResult
{
    GENERATED_BODY()

    friend class UCk_Utils_IntentGrammar_UE;

public:
    CK_GENERATED_BODY(FCk_Intent_ParseResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_SucceededFailed _Outcome = ECk_SucceededFailed::Failed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Intent_Definition _Definition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_ParseError _Error = ECk_Intent_ParseError::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _ErrorToken;

public:
    CK_PROPERTY_GET(_Outcome);
    CK_PROPERTY_GET(_Definition);
    CK_PROPERTY_GET(_Error);
    CK_PROPERTY_GET(_ErrorToken);

public:
    auto Get_IsSucceeded() const -> bool { return _Outcome == ECk_SucceededFailed::Succeeded; }

public:
    FCk_Intent_ParseResult() = default;

private:
    FCk_Intent_ParseResult(
        ECk_SucceededFailed InOutcome,
        FCk_Intent_Definition InDefinition,
        ECk_Intent_ParseError InError,
        FString InErrorToken)
        : _Outcome(InOutcome)
        , _Definition(MoveTemp(InDefinition))
        , _Error(InError)
        , _ErrorToken(MoveTemp(InErrorToken))
    { }
};

// --------------------------------------------------------------------------------------------------------------------
