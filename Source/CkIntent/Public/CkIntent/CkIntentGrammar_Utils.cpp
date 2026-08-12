#include "CkIntentGrammar_Utils.h"

#include "CkIntent/CkIntent_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_intent_grammar
{
    // Matched case-insensitively, which costs a button the right to be named after one. That trade is deliberate:
    // a case-sensitive keyword would read `Lenient` as a button token and silently drop the modifier the designer
    // wrote, and a silently-ignored modifier is the one failure mode a parser must never have.
    constexpr auto ModifierKeyword_Lenient = TEXT("lenient");
    constexpr auto ModifierKeyword_Level   = TEXT("level");
    constexpr auto ModifierKey_Window      = TEXT("w");
    constexpr auto ModifierKey_Hold        = TEXT("hold");

    // Nine digits is past every frame budget a 60 Hz window can mean and short of int32 overflow, so the length
    // check is what keeps Atoi's answer trustworthy without a wider parse.
    constexpr auto MaxFrameValueDigits = 9;

    // ----------------------------------------------------------------------------------------------------------------

    struct FRejection
    {
        ECk_Intent_ParseError _Error = ECk_Intent_ParseError::None;
        FString _Token;

        auto Get_IsRejected() const -> bool { return _Error != ECk_Intent_ParseError::None; }
    };

    auto
    DoMake_Accepted() -> FRejection
    {
        return FRejection{};
    }

    auto
    DoMake_Rejected(
        ECk_Intent_ParseError InError,
        const FString& InToken) -> FRejection
    {
        return FRejection{InError, InToken};
    }

    // ----------------------------------------------------------------------------------------------------------------

    // The numpad's face read onto the sampler's angle-ordered octants: the keypad's own geometry IS the mapping,
    // which is why there is no table to keep in step with either enum.
    auto
    DoGet_OctantForDigit(
        TCHAR InDigit) -> TOptional<ECk_Intent_Octant>
    {
        switch (InDigit)
        {
            case TEXT('1'): return ECk_Intent_Octant::SW;
            case TEXT('2'): return ECk_Intent_Octant::S;
            case TEXT('3'): return ECk_Intent_Octant::SE;
            case TEXT('4'): return ECk_Intent_Octant::W;
            case TEXT('5'): return ECk_Intent_Octant::Neutral;
            case TEXT('6'): return ECk_Intent_Octant::E;
            case TEXT('7'): return ECk_Intent_Octant::NW;
            case TEXT('8'): return ECk_Intent_Octant::N;
            case TEXT('9'): return ECk_Intent_Octant::NE;
            default: return {};
        }
    }

    // A leading digit is what makes a token a direction run rather than a button, which is the whole of the rule
    // that button names may not start with one — there is no second check to keep consistent with this one.
    auto
    DoGet_IsDigitLeading(
        const FString& InToken) -> bool
    {
        return NOT InToken.IsEmpty() && InToken[0] >= TEXT('0') && InToken[0] <= TEXT('9');
    }

    // Spelled out rather than deferred to FChar::IsAlnum, which answers for the running locale and would quietly
    // admit accented letters into a name space the bake has to match against declared ButtonIds.
    auto
    DoGet_IsButtonNameChar(
        TCHAR InChar) -> bool
    {
        return (InChar >= TEXT('a') && InChar <= TEXT('z')) ||
               (InChar >= TEXT('A') && InChar <= TEXT('Z')) ||
               (InChar >= TEXT('0') && InChar <= TEXT('9')) ||
               InChar == TEXT('_');
    }

    auto
    DoGet_IsButtonNameValid(
        const FString& InName) -> bool
    {
        if (InName.IsEmpty())
        { return false; }

        for (auto Index = int32{0}; Index < InName.Len(); ++Index)
        {
            if (NOT DoGet_IsButtonNameChar(InName[Index]))
            { return false; }
        }

        return true;
    }

    auto
    DoParse_PositiveFrames(
        const FString& InValue) -> TOptional<int32>
    {
        if (InValue.IsEmpty() || InValue.Len() > MaxFrameValueDigits)
        { return {}; }

        for (auto Index = int32{0}; Index < InValue.Len(); ++Index)
        {
            const auto Char = InValue[Index];

            if (Char < TEXT('0') || Char > TEXT('9'))
            { return {}; }
        }

        const auto Frames = FCString::Atoi(*InValue);

        if (Frames <= 0)
        { return {}; }

        return Frames;
    }

    auto
    DoMake_SingleAtomStep(
        FCk_Intent_Atom InAtom) -> FCk_Intent_Step
    {
        auto Atoms = TArray<FCk_Intent_Atom>{};
        Atoms.Add(MoveTemp(InAtom));

        return FCk_Intent_Step{MoveTemp(Atoms)};
    }

    // ----------------------------------------------------------------------------------------------------------------

    /*
     * Expands one whitespace-delimited token into the steps it means, appending them in order.
     *
     * A digit run is a SEQUENCE, and a `+` binds to the run's LAST digit: `236+LP` is therefore three steps ending
     * in a chord, not one four-atom step. The rule is uniform over the token's atoms — a run anywhere in a chord
     * contributes its leading digits as steps BEFORE the chord and its final digit to the chord itself — so there
     * is no position in the token where the same run reads differently.
     */
    auto
    DoExpand_StepToken(
        const FString& InToken,
        TArray<FCk_Intent_Step>& OutSteps) -> FRejection
    {
        constexpr auto CullEmptyAtoms = false;

        auto AtomTokens = TArray<FString>{};
        InToken.ParseIntoArray(AtomTokens, TEXT("+"), CullEmptyAtoms);

        const auto TokenIsChord = AtomTokens.Num() > 1;

        auto ChordAtoms = TArray<FCk_Intent_Atom>{};

        for (const auto& AtomToken : AtomTokens)
        {
            if (AtomToken.IsEmpty())
            { return DoMake_Rejected(ECk_Intent_ParseError::EmptyChordAtom, InToken); }

            if (NOT DoGet_IsDigitLeading(AtomToken))
            {
                if (NOT DoGet_IsButtonNameValid(AtomToken))
                { return DoMake_Rejected(ECk_Intent_ParseError::InvalidButtonName, InToken); }

                auto ButtonAtom = FCk_Intent_Atom::Button(FName{*AtomToken});

                if (TokenIsChord)
                { ChordAtoms.Add(MoveTemp(ButtonAtom)); }
                else
                { OutSteps.Add(DoMake_SingleAtomStep(MoveTemp(ButtonAtom))); }

                continue;
            }

            auto Octants = TArray<ECk_Intent_Octant>{};

            for (auto Index = int32{0}; Index < AtomToken.Len(); ++Index)
            {
                const auto Octant = DoGet_OctantForDigit(AtomToken[Index]);

                if (NOT Octant.IsSet())
                { return DoMake_Rejected(ECk_Intent_ParseError::InvalidDirectionDigit, InToken); }

                Octants.Add(*Octant);
            }

            const auto BoundOctant = Octants.Last();

            for (auto Index = int32{0}; Index < Octants.Num() - 1; ++Index)
            { OutSteps.Add(DoMake_SingleAtomStep(FCk_Intent_Atom::Direction(Octants[Index]))); }

            if (NOT TokenIsChord)
            {
                OutSteps.Add(DoMake_SingleAtomStep(FCk_Intent_Atom::Direction(BoundOctant)));
                continue;
            }

            if (BoundOctant == ECk_Intent_Octant::Neutral)
            { return DoMake_Rejected(ECk_Intent_ParseError::ChordNeutralDirection, InToken); }

            ChordAtoms.Add(FCk_Intent_Atom::Direction(BoundOctant));
        }

        if (NOT TokenIsChord)
        { return DoMake_Accepted(); }

        // Duplicates answer before the two-directions rule, because `6+6` is one atom typed twice and saying so
        // names the fix; "a chord cannot hold two directions" would send the author looking for a second direction
        // they never wrote.
        for (auto Index = int32{0}; Index < ChordAtoms.Num(); ++Index)
        {
            for (auto Other = Index + 1; Other < ChordAtoms.Num(); ++Other)
            {
                if (ChordAtoms[Index] == ChordAtoms[Other])
                { return DoMake_Rejected(ECk_Intent_ParseError::ChordDuplicateAtom, InToken); }
            }
        }

        auto DirectionCount = int32{0};

        for (const auto& Atom : ChordAtoms)
        {
            if (Atom.Get_Kind() == ECk_Intent_AtomKind::Direction)
            { ++DirectionCount; }
        }

        if (DirectionCount > 1)
        { return DoMake_Rejected(ECk_Intent_ParseError::ChordTwoDirections, InToken); }

        OutSteps.Add(FCk_Intent_Step{MoveTemp(ChordAtoms)});

        return DoMake_Accepted();
    }

    // ----------------------------------------------------------------------------------------------------------------

    // First match answers, and that is not a preference: the bake rejects any name whose rows disagree before a
    // single atom is resolved, so every row still standing for a given name carries the identical identity.
    auto
    DoTryGet_ButtonForName(
        const TArray<FCk_Intent_ButtonNameRow>& InRows,
        FName InName) -> TOptional<FCk_Input_ButtonId>
    {
        for (const auto& Row : InRows)
        {
            if (Row.Get_Name() == InName)
            { return Row.Get_Button(); }
        }

        return {};
    }

    /*
     * The buttons a press of which can complete this intent — the button atoms of its LAST step, and nothing else.
     *
     * That omission IS the law the whole deferral model rests on. A button appearing mid-sequence is never visited
     * here, so it can never acquire a resolution row or a verdict, and "a sequence suffix never defers" is not a
     * rule anybody has to remember to enforce — there is no code path that could give it one.
     *
     * An intent whose terminal is a pure direction step answers EMPTY and therefore appears in no resolution row:
     * a press cannot complete it. Nothing rejects that here; a direction-terminated move needs a direction-driven
     * trigger, which is the matcher's to provide.
     */
    auto
    DoGet_TerminalButtons(
        const FCk_Intent_CompiledIntent& InIntent) -> TArray<FCk_Input_ButtonId>
    {
        auto Buttons = TArray<FCk_Input_ButtonId>{};

        if (InIntent.Get_Steps().IsEmpty())
        { return Buttons; }

        for (const auto& Atom : InIntent.Get_Steps().Last().Get_Atoms())
        {
            if (Atom.Get_Kind() == ECk_Intent_AtomKind::Button)
            { Buttons.Add(Atom.Get_Button()); }
        }

        return Buttons;
    }

    /*
     * Whether this intent's terminal needs TWO button presses to complete.
     *
     * Button atoms only — a direction inside a chord is deliberately not counted. A direction is a state the frame
     * record already reports on the very frame the button lands, so `6+LP` completes or fails on the LP press with
     * nothing left in flight; there is no forward ambiguity to wait out and deferring would buy latency for
     * nothing. Two BUTTONS are the case that can still be arriving.
     */
    auto
    DoGet_TerminalNeedsSecondPress(
        const FCk_Intent_CompiledIntent& InIntent) -> bool
    {
        return DoGet_TerminalButtons(InIntent).Num() > 1;
    }

    // One terminal button and every intent it can complete, before ordering.
    struct FTerminalGroup
    {
        FCk_Input_ButtonId _Button;
        TArray<int32> _IntentIndices;
    };

    auto
    DoFindOrAdd_Group(
        TArray<FTerminalGroup>& InGroups,
        const FCk_Input_ButtonId& InButton) -> FTerminalGroup&
    {
        for (auto& Group : InGroups)
        {
            if (Group._Button == InButton)
            { return Group; }
        }

        return InGroups.Emplace_GetRef(FTerminalGroup{InButton, {}});
    }

    /*
     * The group's EDGE members — the only ones a deferral is a question about.
     *
     * A level intent is answered on the press frame by construction: it declares no threshold and its terminal is
     * one button, so there is nothing in flight for a wait to resolve. It is therefore neither a rival that could
     * make a press ambiguous nor a candidate an episode could complete, and it must not lengthen a wait that only
     * its edge neighbours are in.
     */
    auto
    DoGet_EdgeIndices(
        const TArray<FCk_Intent_CompiledIntent>& InIntents,
        const TArray<int32>& InIndices) -> TArray<int32>
    {
        auto EdgeIndices = TArray<int32>{};

        for (const auto& Index : InIndices)
        {
            if (InIntents[Index].Get_Kind() == ECk_Intent_Kind::Edge)
            { EdgeIndices.Add(Index); }
        }

        return EdgeIndices;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentGrammar_UE::
    Parse(
        const FString& InNotation,
        FName InIntentName,
        int32 InPriority,
        const FGameplayTag& InIntentTag)
    -> FCk_Intent_ParseResult
{
    using namespace ck_intent_grammar;

    auto Tokens = TArray<FString>{};
    InNotation.ParseIntoArrayWS(Tokens);

    if (Tokens.IsEmpty())
    { return DoMake_Rejection(ECk_Intent_ParseError::EmptyNotation, {}); }

    auto StepTokens = TArray<FString>{};

    auto WindowFrames = int32{0};
    auto HoldFrames   = int32{0};
    auto Lenience     = ECk_Intent_Lenience::Strict;
    auto Kind         = ECk_Intent_Kind::Edge;
    auto SeenModifier = false;

    for (const auto& Token : Tokens)
    {
        const auto TokenIsLenience = Token.Equals(ModifierKeyword_Lenient, ESearchCase::IgnoreCase);
        const auto TokenIsLevel    = Token.Equals(ModifierKeyword_Level, ESearchCase::IgnoreCase);
        const auto TokenIsKeyed    = Token.Contains(TEXT("="));

        if (NOT TokenIsLenience && NOT TokenIsLevel && NOT TokenIsKeyed)
        {
            if (SeenModifier)
            { return DoMake_Rejection(ECk_Intent_ParseError::ModifierNotTrailing, Token); }

            StepTokens.Add(Token);
            continue;
        }

        SeenModifier = true;

        if (TokenIsLenience)
        {
            if (Lenience == ECk_Intent_Lenience::Lenient)
            { return DoMake_Rejection(ECk_Intent_ParseError::DuplicateModifier, Token); }

            Lenience = ECk_Intent_Lenience::Lenient;
            continue;
        }

        if (TokenIsLevel)
        {
            if (Kind == ECk_Intent_Kind::Level)
            { return DoMake_Rejection(ECk_Intent_ParseError::DuplicateModifier, Token); }

            Kind = ECk_Intent_Kind::Level;
            continue;
        }

        auto Key   = FString{};
        auto Value = FString{};
        Token.Split(TEXT("="), &Key, &Value);

        if (Key.Equals(ModifierKey_Window, ESearchCase::IgnoreCase))
        {
            // A declared window is always positive, so a zero here can only be the not-declared state — there is
            // no second flag that could disagree with the value about whether it was written.
            if (WindowFrames != 0)
            { return DoMake_Rejection(ECk_Intent_ParseError::DuplicateModifier, Token); }

            const auto Frames = DoParse_PositiveFrames(Value);

            if (NOT Frames.IsSet())
            { return DoMake_Rejection(ECk_Intent_ParseError::MalformedWindow, Token); }

            WindowFrames = *Frames;
            continue;
        }

        if (Key.Equals(ModifierKey_Hold, ESearchCase::IgnoreCase))
        {
            if (HoldFrames != 0)
            { return DoMake_Rejection(ECk_Intent_ParseError::DuplicateModifier, Token); }

            const auto Frames = DoParse_PositiveFrames(Value);

            if (NOT Frames.IsSet())
            { return DoMake_Rejection(ECk_Intent_ParseError::MalformedHold, Token); }

            HoldFrames = *Frames;
            continue;
        }

        return DoMake_Rejection(ECk_Intent_ParseError::UnknownModifier, Token);
    }

    if (StepTokens.IsEmpty())
    { return DoMake_Rejection(ECk_Intent_ParseError::NoSteps, {}); }

    auto Steps = TArray<FCk_Intent_Step>{};

    for (const auto& StepToken : StepTokens)
    {
        const auto Rejection = DoExpand_StepToken(StepToken, Steps);

        if (Rejection.Get_IsRejected())
        { return DoMake_Rejection(Rejection._Error, Rejection._Token); }
    }

    // The three level constraints are about the notation as a WHOLE — which token to blame is a question with no
    // honest answer once `level` and the steps disagree — so they reject with an empty token, as NoSteps does.
    const auto KindIsLevel = Kind == ECk_Intent_Kind::Level;

    if (KindIsLevel && HoldFrames != 0)
    { return DoMake_Rejection(ECk_Intent_ParseError::LevelWithHold, {}); }

    if (KindIsLevel && Steps.Num() > 1)
    { return DoMake_Rejection(ECk_Intent_ParseError::LevelWithSequence, {}); }

    const auto TerminalIsOneButton = Steps.Num() == 1 &&
                                     Steps.Last().Get_Atoms().Num() == 1 &&
                                     Steps.Last().Get_Atoms()[0].Get_Kind() == ECk_Intent_AtomKind::Button;

    if (KindIsLevel && NOT TerminalIsOneButton)
    { return DoMake_Rejection(ECk_Intent_ParseError::LevelTerminalNotSingleButton, {}); }

    return DoMake_Acceptance(FCk_Intent_Definition
    {
        InIntentName,
        InIntentTag,
        MoveTemp(Steps),
        WindowFrames,
        HoldFrames,
        InPriority,
        Lenience,
        Kind
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentGrammar_UE::
    Get_TerminalStep(
        const FCk_Intent_Definition& InDefinition)
    -> FCk_Intent_Step
{
    if (InDefinition.Get_Steps().IsEmpty())
    { return {}; }

    return InDefinition.Get_Steps().Last();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentGrammar_UE::
    Bake(
        const TArray<FCk_Intent_Definition>& InDefinitions,
        const TArray<FCk_Intent_ButtonNameRow>& InButtonNames,
        int32 InChordWindowFrames)
    -> FCk_Intent_BakeResult
{
    using namespace ck_intent_grammar;

    if (InChordWindowFrames <= 0)
    { return DoMake_BakeRejection(ECk_Intent_BakeError::NonPositiveChordWindow); }

    if (InDefinitions.IsEmpty())
    { return DoMake_BakeRejection(ECk_Intent_BakeError::NoDefinitions); }

    // The vocabulary has to answer each name with ONE identity, and it is checked before anything reads it. A name
    // repeated against the SAME identity is a caller assembling rows from two overlapping sources and changes
    // nothing; two DIFFERENT identities are two answers to one question, and silently taking either would discard
    // a declaration somebody wrote down.
    for (auto Index = int32{0}; Index < InButtonNames.Num(); ++Index)
    {
        for (auto Other = Index + 1; Other < InButtonNames.Num(); ++Other)
        {
            if (InButtonNames[Index].Get_Name() != InButtonNames[Other].Get_Name())
            { continue; }

            if (InButtonNames[Index].Get_Button() == InButtonNames[Other].Get_Button())
            { continue; }

            auto Rejection = DoMake_BakeRejection(ECk_Intent_BakeError::ConflictingButtonRow);
            Rejection._OffendingButtonName = InButtonNames[Index].Get_Name();
            Rejection._OffendingButton = InButtonNames[Index].Get_Button();
            Rejection._ConflictingButton = InButtonNames[Other].Get_Button();
            return Rejection;
        }
    }

    // Identity first: a nameless or duplicated intent makes every later diagnostic — and every resolution row —
    // ambiguous, so nothing else is worth computing until the names are known to be usable.
    auto SeenNames = TArray<FName>{};

    for (const auto& Definition : InDefinitions)
    {
        if (Definition.Get_Name() == NAME_None)
        { return DoMake_BakeRejection(ECk_Intent_BakeError::UnnamedIntent); }

        if (SeenNames.Contains(Definition.Get_Name()))
        {
            auto Rejection = DoMake_BakeRejection(ECk_Intent_BakeError::DuplicateIntentName);
            Rejection._OffendingIntent = Definition.Get_Name();
            return Rejection;
        }

        SeenNames.Add(Definition.Get_Name());
    }

    auto CompiledIntents = TArray<FCk_Intent_CompiledIntent>{};
    CompiledIntents.Reserve(InDefinitions.Num());

    for (const auto& Definition : InDefinitions)
    {
        auto CompiledSteps = TArray<FCk_Intent_CompiledStep>{};
        CompiledSteps.Reserve(Definition.Get_Steps().Num());

        for (const auto& Step : Definition.Get_Steps())
        {
            auto CompiledAtoms = TArray<FCk_Intent_CompiledAtom>{};
            CompiledAtoms.Reserve(Step.Get_Atoms().Num());

            for (const auto& Atom : Step.Get_Atoms())
            {
                if (Atom.Get_Kind() == ECk_Intent_AtomKind::Direction)
                {
                    CompiledAtoms.Add(FCk_Intent_CompiledAtom::Direction(Atom.Get_Direction()));
                    continue;
                }

                const auto Button = DoTryGet_ButtonForName(InButtonNames, Atom.Get_ButtonName());

                if (NOT Button.IsSet())
                {
                    auto Rejection = DoMake_BakeRejection(ECk_Intent_BakeError::UnknownButtonName);
                    Rejection._OffendingIntent = Definition.Get_Name();
                    Rejection._OffendingButtonName = Atom.Get_ButtonName();
                    return Rejection;
                }

                CompiledAtoms.Add(FCk_Intent_CompiledAtom::Button(*Button));
            }

            CompiledSteps.Add(FCk_Intent_CompiledStep{MoveTemp(CompiledAtoms)});
        }

        CompiledIntents.Add(FCk_Intent_CompiledIntent
        {
            Definition.Get_Name(),
            Definition.Get_IntentTag(),
            MoveTemp(CompiledSteps),
            Definition.Get_WindowFrames(),
            Definition.Get_HoldFrames(),
            Definition.Get_Priority(),
            Definition.Get_Lenience(),
            Definition.Get_Kind()
        });
    }

    auto Groups = TArray<FTerminalGroup>{};

    for (auto Index = int32{0}; Index < CompiledIntents.Num(); ++Index)
    {
        for (const auto& Button : DoGet_TerminalButtons(CompiledIntents[Index]))
        { DoFindOrAdd_Group(Groups, Button)._IntentIndices.Add(Index); }
    }

    // Arbitration has to be a strict total order or it is not an order at all: two intents at the same priority on
    // one terminal make the winner a function of iteration accident. Rejecting names BOTH intents and the button,
    // because the fix is a priority edit and the author needs to know which two to separate.
    for (const auto& Group : Groups)
    {
        for (auto Left = int32{0}; Left < Group._IntentIndices.Num(); ++Left)
        {
            for (auto Right = Left + 1; Right < Group._IntentIndices.Num(); ++Right)
            {
                const auto& LeftIntent  = CompiledIntents[Group._IntentIndices[Left]];
                const auto& RightIntent = CompiledIntents[Group._IntentIndices[Right]];

                if (LeftIntent.Get_Priority() != RightIntent.Get_Priority())
                { continue; }

                auto Rejection = DoMake_BakeRejection(ECk_Intent_BakeError::PriorityTieOnSharedTerminal);
                Rejection._OffendingIntent = LeftIntent.Get_Name();
                Rejection._ConflictingIntent = RightIntent.Get_Name();
                Rejection._OffendingButton = Group._Button;
                return Rejection;
            }
        }
    }

    auto ResolutionTable = TArray<FCk_Intent_ResolutionRow>{};
    auto Deferrals = TArray<FCk_Intent_DeferralRow>{};

    ResolutionTable.Reserve(Groups.Num());

    for (auto& Group : Groups)
    {
        // Distinct priorities are guaranteed by the check above, so this sort is a total order and the resulting
        // row is the same on every machine and every run — which is what lets the matcher read it as arbitration
        // rather than as a suggestion.
        Group._IntentIndices.Sort([&CompiledIntents](const int32& InLeft, const int32& InRight)
        {
            return CompiledIntents[InLeft].Get_Priority() > CompiledIntents[InRight].Get_Priority();
        });

        const auto EdgeIndices = DoGet_EdgeIndices(CompiledIntents, Group._IntentIndices);

        // Deferral is about being WRONG, not about waiting: with one candidate there is no other intent a press
        // could have meant, so nothing is ambiguous no matter how the move is shaped. This single condition is
        // why sharing a terminal, on its own, still defers for nothing.
        const auto HasRivals = EdgeIndices.Num() > 1;

        auto HoldSiblingFrames = int32{0};
        auto NeedsSecondPress  = false;

        if (HasRivals)
        {
            for (const auto& IntentIndex : EdgeIndices)
            {
                HoldSiblingFrames = FMath::Max(HoldSiblingFrames, CompiledIntents[IntentIndex].Get_HoldFrames());
                NeedsSecondPress = NeedsSecondPress || DoGet_TerminalNeedsSecondPress(CompiledIntents[IntentIndex]);
            }
        }

        ResolutionTable.Add(FCk_Intent_ResolutionRow{Group._Button, Group._IntentIndices});

        if (HoldSiblingFrames <= 0 && NOT NeedsSecondPress)
        { continue; }

        auto Verdict = FCk_Intent_DeferralVerdict{};
        Verdict._HoldSiblingFrames = HoldSiblingFrames;
        Verdict._ChordMemberFrames = NeedsSecondPress ? InChordWindowFrames : 0;

        Deferrals.Add(FCk_Intent_DeferralRow{Group._Button, Verdict});
    }

    return DoMake_BakeAcceptance(FCk_Intent_CompiledSet
    {
        MoveTemp(CompiledIntents),
        MoveTemp(ResolutionTable),
        MoveTemp(Deferrals),
        InChordWindowFrames
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentGrammar_UE::
    TryGet_ResolutionRow(
        const FCk_Intent_CompiledSet& InSet,
        const FCk_Input_ButtonId& InTerminalButton)
    -> FCk_Intent_ResolutionRow
{
    for (const auto& Row : InSet.Get_ResolutionTable())
    {
        if (Row.Get_TerminalButton() == InTerminalButton)
        { return Row; }
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentGrammar_UE::
    Get_DeferralVerdict(
        const FCk_Intent_CompiledSet& InSet,
        const FCk_Input_ButtonId& InButton)
    -> FCk_Intent_DeferralVerdict
{
    for (const auto& Row : InSet.Get_Deferrals())
    {
        if (Row.Get_Button() == InButton)
        { return Row.Get_Verdict(); }
    }

    // Absent means no ambiguity was found — the same answer a button the set has never heard of gets, and the
    // reason no-deferral can never be forgotten: it is what happens when nothing wrote a row.
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentGrammar_UE::
    DoMake_Rejection(
        ECk_Intent_ParseError InError,
        const FString& InToken)
    -> FCk_Intent_ParseResult
{
    ck::intent::Verbose(TEXT("Intent notation rejected [{}] at token [{}]"), InError, InToken);

    return FCk_Intent_ParseResult{ECk_SucceededFailed::Failed, FCk_Intent_Definition{}, InError, InToken};
}

auto
    UCk_Utils_IntentGrammar_UE::
    DoMake_Acceptance(
        FCk_Intent_Definition InDefinition)
    -> FCk_Intent_ParseResult
{
    return FCk_Intent_ParseResult
    {
        ECk_SucceededFailed::Succeeded,
        MoveTemp(InDefinition),
        ECk_Intent_ParseError::None,
        FString{}
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentGrammar_UE::
    DoMake_BakeRejection(
        ECk_Intent_BakeError InError)
    -> FCk_Intent_BakeResult
{
    ck::intent::Verbose(TEXT("Intent set rejected at bake [{}]"), InError);

    return FCk_Intent_BakeResult{ECk_SucceededFailed::Failed, FCk_Intent_CompiledSet{}, InError};
}

auto
    UCk_Utils_IntentGrammar_UE::
    DoMake_BakeAcceptance(
        FCk_Intent_CompiledSet InCompiledSet)
    -> FCk_Intent_BakeResult
{
    return FCk_Intent_BakeResult
    {
        ECk_SucceededFailed::Succeeded,
        MoveTemp(InCompiledSet),
        ECk_Intent_BakeError::None
    };
}

// --------------------------------------------------------------------------------------------------------------------
