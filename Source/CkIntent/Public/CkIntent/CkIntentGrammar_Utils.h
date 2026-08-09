#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkIntent/CkIntentCompiledSet_Data.h"
#include "CkIntent/CkIntentGrammar_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkIntentGrammar_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * The compact notation, the one thing that reads it, and the bake that turns definitions into a usable set.
 *
 * A move is authored as a STRING — `"236+LP w=200 lenient"` — rather than as a nested struct a designer assembles
 * in a details panel, because the string is the notation the genre already writes moves in and a reader can see
 * the whole move at once. The struct shape exists downstream of it, produced here, never authored directly.
 *
 * There is exactly ONE parser and this is it: the C++ tests, the Blueprint and AngelScript authoring paths, and
 * the bake's own fixtures all enter here. A second entry point would be a second grammar the moment either one
 * changed, and nothing would say which of them the shipped moves were written against.
 *
 * Parse and Bake live together because they are one pipeline with one story — notation in, activatable set out —
 * and splitting them would split `utils_intent_grammar` into two AngelScript namespaces for what an author
 * experiences as a single surface. They also divide the validation cleanly, and the division is the interesting
 * part: PARSING validates one string in isolation, BAKING validates everything that only a whole set can answer.
 * Whether `LP` is a declared button, whether two moves share a name, whether two priorities tie on a terminal —
 * none of those are answerable from one string, and all of them reject here.
 *
 * Malformed input is a RESULT on both sides, not a programming error: it is what a designer's typo looks like, so
 * both return a rejection reason rather than firing an ensure. The reasons are exhaustive and distinct — see
 * `ECk_Intent_ParseError`, `ECk_Intent_BakeError`, and the reference in the module's Claude.md.
 */
UCLASS(NotBlueprintable)
class CKINTENT_API UCk_Utils_IntentGrammar_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IntentGrammar_UE);

public:
    /**
     * Reads one notation string into a definition, or into the reason it is not one.
     *
     * The name, priority and tag are the definition's IDENTITY and are carried through untouched — they are not
     * part of the notation because they are not part of the move: two projects can share `236+LP` and disagree
     * about what it is called and what it outranks.
     *
     * Accepted, in one line: whitespace-separated steps, then trailing modifiers. A step is a run of numpad digits
     * (each digit its own step), a button name, or atoms joined by `+` into one simultaneous chord. The modifiers
     * are `w=<frames>`, `hold=<frames>` and `lenient`, in any order. The LAST step is the terminal.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentGrammar",
              DisplayName = "[Ck][IntentGrammar] Parse Notation")
    static FCk_Intent_ParseResult
    Parse(
        const FString& InNotation,
        FName InIntentName,
        int32 InPriority,
        const FGameplayTag& InIntentTag);

    /**
     * The step that completes the move — the last one, which is what the matcher's backward scan anchors on.
     *
     * Answers an EMPTY step for a definition with no steps, mirroring the frame record's negative-index answer: a
     * real step always carries at least one atom, so the two can never be confused and there is no separate
     * success flag to forget to check.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentGrammar",
              DisplayName = "[Ck][IntentGrammar] Get Terminal Step")
    static FCk_Intent_Step
    Get_TerminalStep(
        const FCk_Intent_Definition& InDefinition);

public:
    /**
     * Compiles a whole set of definitions into the artifact a matcher runs against: resolved button identities, a
     * per-terminal resolution table ordered by dominance, and the deferral verdicts that say when a press may be
     * acted on.
     *
     * The button vocabulary arrives as ROWS rather than a live map, so the bake needs no world and no entity — a
     * cook step, a validation tool and a test all call it the same way a running game does.
     *
     * `InChordWindowFrames` is the window inside which two atoms of a chord count as pressed together, uniform
     * across the set: the notation has no chord-window syntax and is deliberately kept from growing one. Three
     * frames at the sampler's 60 Hz is roughly the 50 ms most players cannot beat on purpose.
     *
     * ATOMIC: any rejection yields an empty set. A character missing one special silently is a bug that ships; a
     * set that refuses to compile is a bug that gets fixed.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentGrammar",
              DisplayName = "[Ck][IntentGrammar] Bake Compiled Set")
    static FCk_Intent_BakeResult
    Bake(
        const TArray<FCk_Intent_Definition>& InDefinitions,
        const TArray<FCk_Intent_ButtonNameRow>& InButtonNames,
        int32 InChordWindowFrames = 3);

    /**
     * The intents a press of this button can complete, most-dominant first.
     *
     * A button no intent terminates on answers an EMPTY row — indices are what a real row carries, so the two
     * cannot be confused and there is no separate found-flag to forget.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentGrammar",
              DisplayName = "[Ck][IntentGrammar] Try Get Resolution Row")
    static FCk_Intent_ResolutionRow
    TryGet_ResolutionRow(
        const FCk_Intent_CompiledSet& InSet,
        const FCk_Input_ButtonId& InTerminalButton);

    /**
     * How long a press of this button must go unanswered, and why.
     *
     * Not a `TryGet_`: every button has a verdict, and for almost all of them it is the default one — defer for
     * nothing. The set stores only the buttons that DO defer, so a button it has never heard of gets the same
     * answer as a button whose ambiguities were checked and found absent. That equivalence is the point.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentGrammar",
              DisplayName = "[Ck][IntentGrammar] Get Deferral Verdict")
    static FCk_Intent_DeferralVerdict
    Get_DeferralVerdict(
        const FCk_Intent_CompiledSet& InSet,
        const FCk_Input_ButtonId& InButton);

private:
    static auto
    DoMake_Rejection(
        ECk_Intent_ParseError InError,
        const FString& InToken) -> FCk_Intent_ParseResult;

    static auto
    DoMake_Acceptance(
        FCk_Intent_Definition InDefinition) -> FCk_Intent_ParseResult;

    static auto
    DoMake_BakeRejection(
        ECk_Intent_BakeError InError) -> FCk_Intent_BakeResult;

    static auto
    DoMake_BakeAcceptance(
        FCk_Intent_CompiledSet InCompiledSet) -> FCk_Intent_BakeResult;
};

// --------------------------------------------------------------------------------------------------------------------
