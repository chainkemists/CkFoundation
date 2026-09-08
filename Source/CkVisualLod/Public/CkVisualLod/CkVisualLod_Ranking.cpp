#include "CkVisualLod/CkVisualLod_Ranking.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::visual_lod
{
    auto
        Get_AreRenderBandsValid(
            const TArray<FVisualLod_RenderBandRange>& InBands)
        -> bool
    {
        if (InBands.IsEmpty())
        { return true; }

        if (InBands[0]._DistanceThreshold != 0.0f)
        { return false; }

        for (auto Index = 0; Index < InBands.Num(); ++Index)
        {
            const auto& Band = InBands[Index];
            if (NOT FMath::IsFinite(Band._DistanceThreshold) || NOT FMath::IsFinite(Band._ReturnHysteresis)
                || Band._DistanceThreshold < 0.0f || Band._ReturnHysteresis < 0.0f)
            { return false; }

            if (Index == 0)
            { continue; }

            const auto& Previous = InBands[Index - 1];
            if (Band._DistanceThreshold <= Previous._DistanceThreshold
                || Band._ReturnHysteresis >= Band._DistanceThreshold - Previous._DistanceThreshold)
            { return false; }
        }

        return true;
    }

    auto
        Get_RenderBandIndex(
            const TArray<FVisualLod_RenderBandRange>& InBands,
            float InDistance,
            int32 InCurrentBandIndex)
        -> int32
    {
        if (InBands.IsEmpty())
        { return INDEX_NONE; }

        if (NOT Get_AreRenderBandsValid(InBands))
        { return INDEX_NONE; }

        const auto Distance = FMath::Max(InDistance, 0.0f);
        if (NOT InBands.IsValidIndex(InCurrentBandIndex))
        {
            auto Result = 0;
            while (Result + 1 < InBands.Num() && Distance >= InBands[Result + 1]._DistanceThreshold)
            { ++Result; }
            return Result;
        }

        auto Result = InCurrentBandIndex;
        while (Result + 1 < InBands.Num() && Distance >= InBands[Result + 1]._DistanceThreshold)
        { ++Result; }

        while (Result > 0 && Distance < InBands[Result]._DistanceThreshold - InBands[Result]._ReturnHysteresis)
        { --Result; }

        return Result;
    }

    auto
        Get_IsInView(
            const FVector& InEntityLocation,
            const FVisualLod_LocalView& InView,
            float InAlwaysInViewDistance,
            float InDistance)
        -> bool
    {
        if (NOT InView._IsValid)
        { return false; }

        if (InDistance < InAlwaysInViewDistance)
        { return true; }

        const auto Dir = (InEntityLocation - InView._Location).GetSafeNormal();
        if (Dir.IsNearlyZero())
        { return true; }

        return static_cast<float>(Dir.Dot(InView._Forward)) >= InView._CosHalfCone;
    }

    auto
        Select_Flips(
            const TArray<FVisualLod_RankEntry>& InCandidates,
            const TArray<FVisualLod_RankEntry>& InIncumbents,
            int32 InFreeBudget,
            int32 InMaxPreempts,
            float InPreemptDistanceMargin)
        -> FVisualLod_Selection
    {
        auto Result = FVisualLod_Selection{};

        const auto Budget   = FMath::Max(InFreeBudget, 0);
        const auto Preempts = FMath::Max(InMaxPreempts, 0);

        const auto Order = Get_RankOrder(InCandidates, Budget + Preempts);

        auto Cursor = 0;
        while (Cursor < Order.Num() && Result._PromoteIndices.Num() < Budget)
        {
            Result._PromoteIndices.Add(InCandidates[Order[Cursor]]._Index);
            ++Cursor;
        }

        auto ChosenIncumbents = TArray<int32>{};
        while (Cursor < Order.Num() && ChosenIncumbents.Num() < Preempts)
        {
            const auto WorstIdx = Find_WorstIncumbent(InIncumbents, ChosenIncumbents);
            if (WorstIdx < 0)
            { break; }

            // Strictly better, or the incumbent keeps its slot: an equal-ish challenger would
            // otherwise trade places with it every tick and pay a crossfade each time
            const auto& Cand = InCandidates[Order[Cursor]];
            const auto& Inc  = InIncumbents[WorstIdx];
            const auto Beats = (Cand._InView && NOT Inc._InView)
                || (Cand._InView == Inc._InView && Cand._Distance + InPreemptDistanceMargin < Inc._Distance);

            // Sorted best-first, so no later candidate can win one this one lost
            if (NOT Beats)
            { break; }

            Result._PreemptDemoteIndices.Add(Inc._Index);
            ChosenIncumbents.Add(WorstIdx);
            ++Cursor;
        }

        return Result;
    }

    auto
        Get_IsBetterCandidate(
            const FVisualLod_RankEntry& InLhs,
            const FVisualLod_RankEntry& InRhs)
        -> bool
    {
        if (InLhs._InView != InRhs._InView)
        { return InLhs._InView; }

        return InLhs._Distance < InRhs._Distance;
    }

    auto
        Get_IsWorseIncumbent(
            const FVisualLod_RankEntry& InLhs,
            const FVisualLod_RankEntry& InRhs)
        -> bool
    {
        if (InLhs._InView != InRhs._InView)
        { return NOT InLhs._InView; }

        return InLhs._Distance > InRhs._Distance;
    }

    auto
        Get_RankOrder(
            const TArray<FVisualLod_RankEntry>& InEntries,
            int32 InCount)
        -> TArray<int32>
    {
        auto Order = TArray<int32>{};
        Order.Reserve(InEntries.Num());
        for (auto Idx = 0; Idx < InEntries.Num(); ++Idx)
        { Order.Add(Idx); }

        const auto Sorted = FMath::Clamp(InCount, 0, Order.Num());
        for (auto Outer = 0; Outer < Sorted; ++Outer)
        {
            auto Best = Outer;
            for (auto Inner = Outer + 1; Inner < Order.Num(); ++Inner)
            {
                if (Get_IsBetterCandidate(InEntries[Order[Inner]], InEntries[Order[Best]]))
                { Best = Inner; }
            }

            if (Best == Outer)
            { continue; }

            Swap(Order[Outer], Order[Best]);
        }

        Order.SetNum(Sorted);
        return Order;
    }

    auto
        Find_WorstIncumbent(
            const TArray<FVisualLod_RankEntry>& InIncumbents,
            const TArray<int32>& InAlreadyChosen)
        -> int32
    {
        auto Worst = static_cast<int32>(INDEX_NONE);
        for (auto Idx = 0; Idx < InIncumbents.Num(); ++Idx)
        {
            if (InAlreadyChosen.Contains(Idx))
            { continue; }

            if (Worst < 0 || Get_IsWorseIncumbent(InIncumbents[Idx], InIncumbents[Worst]))
            { Worst = Idx; }
        }

        return Worst;
    }
}

// --------------------------------------------------------------------------------------------------------------------
