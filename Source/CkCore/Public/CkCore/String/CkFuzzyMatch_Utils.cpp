#include "CkFuzzyMatch_Utils.h"

#include <Algo/StableSort.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::fuzzy_private
{
    constexpr auto SequentialBonus    = 15;
    constexpr auto WordBoundaryBonus  = 10;
    constexpr auto FirstCharBonus     = 8;
    constexpr auto CaseMatchBonus     = 2;
    constexpr auto LeadingGapPenalty  = -3;
    constexpr auto GapPenalty         = -1;
    constexpr auto SubstringBaseScore = 100;

    static auto
    IsSeparator(
        TCHAR InChar) -> bool
    {
        return InChar == TEXT('_')
            || InChar == TEXT('-')
            || InChar == TEXT(' ')
            || InChar == TEXT('/')
            || InChar == TEXT('\\')
            || InChar == TEXT('.');
    }

    static auto
    IsCamelBoundary(
        TCHAR InPrev,
        TCHAR InCurr) -> bool
    {
        return FChar::IsLower(InPrev) && FChar::IsUpper(InCurr);
    }

    static auto
    CharsEqual(
        TCHAR InA,
        TCHAR InB,
        ECk_Utils_FuzzyMatch_CaseSensitivity InCaseSensitivity) -> bool
    {
        if (InCaseSensitivity == ECk_Utils_FuzzyMatch_CaseSensitivity::CaseSensitive)
        { return InA == InB; }

        return FChar::ToLower(InA) == FChar::ToLower(InB);
    }

    static auto
    DoMatch_Subsequence(
        FStringView InQuery,
        FStringView InCandidate,
        const FCk_Utils_FuzzyMatch_Params& InParams) -> FCk_Utils_FuzzyMatch_Result
    {
        auto Result = FCk_Utils_FuzzyMatch_Result{};

        if (InQuery.IsEmpty())
        {
            Result.Set_IsMatch(true);
            Result.Set_Score(0);
            return Result;
        }

        if (InCandidate.IsEmpty())
        { return Result; }

        const auto CaseSensitivity = InParams.Get_CaseSensitivity();

        auto Score         = 0;
        auto QueryIdx      = 0;
        int32 PrevMatchIdx = INDEX_NONE;
        auto MatchedIndices = TArray<int32>{};
        MatchedIndices.Reserve(InQuery.Len());

        for (auto CandidateIdx = 0; CandidateIdx < InCandidate.Len(); ++CandidateIdx)
        {
            if (QueryIdx >= InQuery.Len())
            { break; }

            const auto QueryChar     = InQuery[QueryIdx];
            const auto CandidateChar = InCandidate[CandidateIdx];

            if (NOT CharsEqual(QueryChar, CandidateChar, CaseSensitivity))
            {
                if (PrevMatchIdx == INDEX_NONE)
                {
                    Score += LeadingGapPenalty;
                }
                continue;
            }

            auto CharScore = 0;

            if (CandidateIdx == 0)
            {
                CharScore += FirstCharBonus;
            }

            if (CandidateIdx > 0)
            {
                const auto PrevChar = InCandidate[CandidateIdx - 1];
                if (IsSeparator(PrevChar) || IsCamelBoundary(PrevChar, CandidateChar))
                {
                    CharScore += WordBoundaryBonus;
                }
            }

            if (PrevMatchIdx != INDEX_NONE && CandidateIdx == PrevMatchIdx + 1)
            {
                CharScore += SequentialBonus;
            }
            else if (PrevMatchIdx != INDEX_NONE)
            {
                const auto Gap = (CandidateIdx - PrevMatchIdx) - 1;
                CharScore += Gap * GapPenalty;
            }

            if (QueryChar == CandidateChar)
            {
                CharScore += CaseMatchBonus;
            }

            Score += CharScore;
            MatchedIndices.Add(CandidateIdx);
            PrevMatchIdx = CandidateIdx;
            ++QueryIdx;
        }

        if (QueryIdx < InQuery.Len())
        { return Result; }

        Result.Set_IsMatch(true);
        Result.Set_Score(Score);
        Result.Set_MatchedIndices(MoveTemp(MatchedIndices));
        return Result;
    }

    static auto
    DoMatch_Substring(
        FStringView InQuery,
        FStringView InCandidate,
        const FCk_Utils_FuzzyMatch_Params& InParams) -> FCk_Utils_FuzzyMatch_Result
    {
        auto Result = FCk_Utils_FuzzyMatch_Result{};

        if (InQuery.IsEmpty())
        {
            Result.Set_IsMatch(true);
            Result.Set_Score(0);
            return Result;
        }

        const auto CaseSensitivity = InParams.Get_CaseSensitivity();

        int32 FoundIdx = INDEX_NONE;
        const auto MaxStart = InCandidate.Len() - InQuery.Len();
        for (auto Start = 0; Start <= MaxStart; ++Start)
        {
            auto AllMatch = true;
            for (auto Offset = 0; Offset < InQuery.Len(); ++Offset)
            {
                if (NOT CharsEqual(InQuery[Offset], InCandidate[Start + Offset], CaseSensitivity))
                {
                    AllMatch = false;
                    break;
                }
            }
            if (AllMatch)
            {
                FoundIdx = Start;
                break;
            }
        }

        if (FoundIdx == INDEX_NONE)
        { return Result; }

        auto Score = SubstringBaseScore + (InQuery.Len() * SequentialBonus);

        if (FoundIdx == 0)
        {
            Score += FirstCharBonus;
        }
        else
        {
            const auto PrevChar = InCandidate[FoundIdx - 1];
            if (IsSeparator(PrevChar) || IsCamelBoundary(PrevChar, InCandidate[FoundIdx]))
            {
                Score += WordBoundaryBonus;
            }
            Score += FoundIdx * GapPenalty;
        }

        auto MatchedIndices = TArray<int32>{};
        MatchedIndices.Reserve(InQuery.Len());
        for (auto Offset = 0; Offset < InQuery.Len(); ++Offset)
        {
            MatchedIndices.Add(FoundIdx + Offset);
        }

        Result.Set_IsMatch(true);
        Result.Set_Score(Score);
        Result.Set_MatchedIndices(MoveTemp(MatchedIndices));
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::fuzzy::
    Match(
        FStringView InQuery,
        FStringView InCandidate,
        const FCk_Utils_FuzzyMatch_Params& InParams)
    -> FCk_Utils_FuzzyMatch_Result
{
    switch (InParams.Get_Strategy())
    {
        case ECk_Utils_FuzzyMatch_Strategy::Substring:
            return ck::fuzzy_private::DoMatch_Substring(InQuery, InCandidate, InParams);
        case ECk_Utils_FuzzyMatch_Strategy::Subsequence:
        default:
            return ck::fuzzy_private::DoMatch_Subsequence(InQuery, InCandidate, InParams);
    }
}

auto
    ck::fuzzy::
    Score(
        FStringView InQuery,
        FStringView InCandidate,
        const FCk_Utils_FuzzyMatch_Params& InParams)
    -> TOptional<int32>
{
    const auto Result = Match(InQuery, InCandidate, InParams);

    if (NOT Result.Get_IsMatch())
    { return {}; }

    return Result.Get_Score();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FuzzyMatch_UE::
    Match(
        const FString& InQuery,
        const FString& InCandidate,
        const FCk_Utils_FuzzyMatch_Params& InParams)
    -> FCk_Utils_FuzzyMatch_Result
{
    return ck::fuzzy::Match(FStringView{InQuery}, FStringView{InCandidate}, InParams);
}

auto
    UCk_Utils_FuzzyMatch_UE::
    Rank(
        const FString& InQuery,
        const TArray<FString>& InCandidates,
        const FCk_Utils_FuzzyMatch_Params& InParams)
    -> TArray<FCk_Utils_FuzzyMatch_RankedEntry>
{
    auto Ranked = TArray<FCk_Utils_FuzzyMatch_RankedEntry>{};
    Ranked.Reserve(InCandidates.Num());

    const auto Threshold = InParams.Get_MinScoreThreshold();

    for (auto Idx = 0; Idx < InCandidates.Num(); ++Idx)
    {
        const auto& Candidate = InCandidates[Idx];
        auto Result = ck::fuzzy::Match(FStringView{InQuery}, FStringView{Candidate}, InParams);

        if (NOT Result.Get_IsMatch())
        { continue; }

        if (Result.Get_Score() < Threshold)
        { continue; }

        auto Entry = FCk_Utils_FuzzyMatch_RankedEntry{};
        Entry.Set_Candidate(Candidate);
        Entry.Set_OriginalIndex(Idx);
        Entry.Set_Result(MoveTemp(Result));

        Ranked.Emplace(MoveTemp(Entry));
    }

    Algo::StableSort(Ranked, [](const FCk_Utils_FuzzyMatch_RankedEntry& InLhs, const FCk_Utils_FuzzyMatch_RankedEntry& InRhs)
    {
        return InLhs.Get_Result().Get_Score() > InRhs.Get_Result().Get_Score();
    });

    const auto MaxResults = InParams.Get_MaxResults();
    if (MaxResults > 0 && Ranked.Num() > MaxResults)
    {
        Ranked.SetNum(MaxResults);
    }

    return Ranked;
}

// --------------------------------------------------------------------------------------------------------------------
