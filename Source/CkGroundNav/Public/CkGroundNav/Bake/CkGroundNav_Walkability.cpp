#include "CkGroundNav_Walkability.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace walkability_private
    {
        // A span cannot have more dropping sides than it has sides, so this threshold can never be
        // met. It is how a zero ledge sensitivity turns the filter off without a branch in the scan.
        constexpr int32 kLedgeFilterDisabled = kDirectionCount + 1;

        auto Get_RequiredDroppingSides(
            const FCk_GroundNav_AgentProfile& InProfile) -> int32
        {
            const auto Sensitivity = InProfile.Get_LedgeSensitivity();

            if (Sensitivity <= 0.0f)
            { return kLedgeFilterDisabled; }

            return FMath::Clamp(FMath::CeilToInt(1.0f / Sensitivity), 1, kDirectionCount);
        }

        /**
         * Whether the neighbouring column holds anything that stops an agent standing at InTopZ from
         * falling that way.
         *
         * Support is anything reaching at least one step BELOW our feet whose underside starts no
         * higher than our head. The lower bound is what makes this a fall test rather than a
         * difference test: you cannot fall upward, so a wall or a rising ramp beside us is something
         * to stand against, and the ground at the foot of a cliff is not a ledge. The upper bound is
         * what stops a floor far OVERHEAD from counting: a walkway with a bridge above it and a void
         * beside it still reads as the ledge it is.
         */
        auto Get_HasSideSupport(
            const TArray<FCk_GroundNav_Span>& InColumn,
            float                             InTopZ,
            float                             InStepHeight,
            float                             InStandingHeight) -> bool
        {
            for (const auto& Span : InColumn)
            {
                if (Span._MaxZ >= InTopZ - InStepHeight && Span._MinZ <= InTopZ + InStandingHeight)
                { return true; }
            }

            return false;
        }

        /**
         * The span in InColumn an agent standing on InSpan could step onto, or kNoConnection.
         *
         * Ties are broken by smallest height delta, which is deterministic and picks the floor a
         * walker would actually reach first.
         */
        auto Get_ConnectableSpanIndex(
            const TArray<FCk_GroundNav_Span>& InColumn,
            const FCk_GroundNav_Span&         InSpan,
            float                             InStepHeight,
            double                            InMinSlopeChangeDot,
            float                             InRoughPerchToleranceUu) -> int32
        {
            auto BestIndex = FCk_GroundNav_SpanConnections::kNoConnection;
            auto BestDelta = TNumericLimits<float>::Max();

            for (auto Index = 0; Index < InColumn.Num(); ++Index)
            {
                const auto& Candidate = InColumn[Index];

                if (NOT Candidate._IsWalkable)
                { continue; }

                const auto Delta = FMath::Abs(Candidate._MaxZ - InSpan._MaxZ);

                if (Delta > InStepHeight || Delta >= BestDelta)
                { continue; }

                // Two surfaces closer together than the rough-perch tolerance are the same rough
                // surface seen through the lattice, so their normals are allowed to disagree.
                const auto IsRoughPerch = InRoughPerchToleranceUu > 0.0f && Delta <= InRoughPerchToleranceUu;

                if (NOT IsRoughPerch)
                {
                    const auto NormalDot = FVector::DotProduct(
                        InSpan._Normal.Get_Normal(), Candidate._Normal.Get_Normal());

                    if (NormalDot < InMinSlopeChangeDot)
                    { continue; }
                }

                BestIndex = Index;
                BestDelta = Delta;
            }

            return BestIndex;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_DirectionOffset(
            int32 InDirection)
        -> FIntPoint
    {
        switch (InDirection)
        {
            case 0:  return FIntPoint{1, 0};
            case 1:  return FIntPoint{0, 1};
            case 2:  return FIntPoint{-1, 0};
            case 3:  return FIntPoint{0, -1};
            default: return FIntPoint{0, 0};
        }
    }

    auto
        Get_OppositeDirection(
            int32 InDirection)
        -> int32
    {
        return (InDirection + 2) % kDirectionCount;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_SpanConnections::
        Get_ConnectionCount() const
        -> int32
    {
        auto Count = 0;

        for (auto Direction = 0; Direction < kDirectionCount; ++Direction)
        {
            if (Get_IsConnected(Direction))
            { ++Count; }
        }

        return Count;
    }

    auto
        FCk_GroundNav_ConnectionField::
        Get_TotalConnectionCount() const
        -> int32
    {
        auto Count = 0;

        for (const auto& Column : _Columns)
        {
            for (const auto& Connections : Column)
            { Count += Connections.Get_ConnectionCount(); }
        }

        return Count;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoFilter_LowClearance(
            const FCk_GroundNav_AgentProfile& InProfile,
            FCk_GroundNav_SpanField&          InOutSpans)
        -> int32
    {
        const auto StandingHeight = InProfile.Get_StandingHeightUu();
        auto DemotedCount = 0;

        for (auto& Column : InOutSpans._Columns)
        {
            for (auto Index = 0; Index < Column.Num(); ++Index)
            {
                auto& Span = Column[Index];

                if (NOT Span._IsWalkable)
                { continue; }

                const auto Headroom = Column.IsValidIndex(Index + 1)
                    ? Column[Index + 1]._MinZ - Span._MaxZ
                    : TNumericLimits<float>::Max();

                if (Headroom >= StandingHeight)
                { continue; }

                Span._IsWalkable = false;
                ++DemotedCount;
            }
        }

        return DemotedCount;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoFilter_Ledges(
            const FCk_GroundNav_AgentProfile& InProfile,
            FCk_GroundNav_SpanField&          InOutSpans)
        -> int32
    {
        using namespace walkability_private;

        const auto RequiredDroppingSides = Get_RequiredDroppingSides(InProfile);
        const auto StepHeight = InProfile.Get_StepHeightUu();
        const auto StandingHeight = InProfile.Get_StandingHeightUu();

        // Decided against the state on entry, applied afterwards: a demotion must never change the
        // verdict of a span the scan has not reached yet.
        auto Demotions = TArray<FIntVector>{};

        for (auto Y = 0; Y < InOutSpans._SizeY; ++Y)
        {
            for (auto X = 0; X < InOutSpans._SizeX; ++X)
            {
                const auto& Column = InOutSpans.Get_Column(X, Y);

                for (auto Index = 0; Index < Column.Num(); ++Index)
                {
                    const auto& Span = Column[Index];

                    if (NOT Span._IsWalkable)
                    { continue; }

                    auto DroppingSides = 0;

                    for (auto Direction = 0; Direction < kDirectionCount; ++Direction)
                    {
                        const auto Offset = Get_DirectionOffset(Direction);
                        const auto NeighbourX = X + Offset.X;
                        const auto NeighbourY = Y + Offset.Y;

                        if (NOT InOutSpans.Get_IsValidColumn(NeighbourX, NeighbourY))
                        { continue; }

                        if (Get_HasSideSupport(
                            InOutSpans.Get_Column(NeighbourX, NeighbourY), Span._MaxZ, StepHeight,
                            StandingHeight))
                        { continue; }

                        ++DroppingSides;
                    }

                    if (DroppingSides < RequiredDroppingSides)
                    { continue; }

                    Demotions.Emplace(FIntVector{X, Y, Index});
                }
            }
        }

        for (const auto& Demotion : Demotions)
        {
            InOutSpans.Get_MutableColumn(Demotion.X, Demotion.Y)[Demotion.Z]._IsWalkable = false;
        }

        return Demotions.Num();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoBuild_Connections(
            const FCk_GroundNav_AgentProfile& InProfile,
            const FCk_GroundNav_SpanField&    InSpans,
            FCk_GroundNav_ConnectionField&    OutConnections)
        -> int32
    {
        using namespace walkability_private;

        OutConnections = FCk_GroundNav_ConnectionField{};
        OutConnections._SizeX = InSpans._SizeX;
        OutConnections._SizeY = InSpans._SizeY;
        OutConnections._Columns.SetNum(InSpans._Columns.Num());

        for (auto ColumnIndex = 0; ColumnIndex < InSpans._Columns.Num(); ++ColumnIndex)
        {
            OutConnections._Columns[ColumnIndex].SetNum(InSpans._Columns[ColumnIndex].Num());
        }

        const auto StepHeight = InProfile.Get_StepHeightUu();
        const auto RoughPerch = InProfile.Get_RoughPerchToleranceUu();
        const auto MinSlopeChangeDot = FMath::Cos(
            FMath::DegreesToRadians(InProfile.Get_MaxSlopeChangeDegrees()));

        for (auto Y = 0; Y < InSpans._SizeY; ++Y)
        {
            for (auto X = 0; X < InSpans._SizeX; ++X)
            {
                const auto& Column = InSpans.Get_Column(X, Y);
                auto& ConnectionColumn = OutConnections.Get_MutableColumn(X, Y);

                for (auto Index = 0; Index < Column.Num(); ++Index)
                {
                    const auto& Span = Column[Index];

                    if (NOT Span._IsWalkable)
                    { continue; }

                    for (auto Direction = 0; Direction < kDirectionCount; ++Direction)
                    {
                        const auto Offset = Get_DirectionOffset(Direction);
                        const auto NeighbourX = X + Offset.X;
                        const auto NeighbourY = Y + Offset.Y;

                        if (NOT InSpans.Get_IsValidColumn(NeighbourX, NeighbourY))
                        { continue; }

                        ConnectionColumn[Index]._Neighbours[Direction] = Get_ConnectableSpanIndex(
                            InSpans.Get_Column(NeighbourX, NeighbourY), Span, StepHeight,
                            MinSlopeChangeDot, RoughPerch);
                    }
                }
            }
        }

        // Drop any edge the far span does not mirror. Nearest-span tie-breaking is per-span, so in a
        // column holding several reachable floors the two ends can disagree on which one they meant
        // — and a one-way edge would make a flood fill result depend on where it started.
        auto SurvivingCount = 0;

        for (auto Y = 0; Y < OutConnections._SizeY; ++Y)
        {
            for (auto X = 0; X < OutConnections._SizeX; ++X)
            {
                auto& ConnectionColumn = OutConnections.Get_MutableColumn(X, Y);

                for (auto Index = 0; Index < ConnectionColumn.Num(); ++Index)
                {
                    for (auto Direction = 0; Direction < kDirectionCount; ++Direction)
                    {
                        auto& Neighbour = ConnectionColumn[Index]._Neighbours[Direction];

                        if (Neighbour == FCk_GroundNav_SpanConnections::kNoConnection)
                        { continue; }

                        const auto Offset = Get_DirectionOffset(Direction);
                        const auto& FarColumn = OutConnections.Get_Column(X + Offset.X, Y + Offset.Y);
                        const auto Opposite = Get_OppositeDirection(Direction);

                        if (FarColumn[Neighbour]._Neighbours[Opposite] == Index)
                        {
                            ++SurvivingCount;
                            continue;
                        }

                        Neighbour = FCk_GroundNav_SpanConnections::kNoConnection;
                    }
                }
            }
        }

        return SurvivingCount;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoFilter_Walkability(
            const FCk_GroundNav_AgentProfile& InProfile,
            FCk_GroundNav_SpanField&          InOutSpans,
            FCk_GroundNav_ConnectionField&    OutConnections)
        -> FCk_GroundNav_BakeStageResult
    {
        auto Result = FCk_GroundNav_BakeStageResult{};

        if (Get_ProfileRejection(InProfile) != EProfileRejection::None)
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        const auto DemotedByClearance = DoFilter_LowClearance(InProfile, InOutSpans);
        const auto DemotedByLedge = DoFilter_Ledges(InProfile, InOutSpans);

        DoBuild_Connections(InProfile, InOutSpans, OutConnections);

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);
        Result.Set_ProbesSpent(InOutSpans.Get_TotalSpanCount());
        Result.Set_DroppedInputCount(DemotedByClearance + DemotedByLedge);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
