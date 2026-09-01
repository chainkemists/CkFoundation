#include "CkGroundNav_Portals.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace portals_private
    {
        // Only the two positive directions are enumerated. Every boundary between two columns is
        // shared by exactly one +X or +Y step, so walking all four would emit each crossing twice.
        constexpr int32 kPositiveDirectionCount = 2;

        /** One cell pair that crosses between two plates, before neighbouring pairs are run together. */
        struct FCrossing
        {
            int32 _Direction = 0;

            // Coordinate along the axis the direction steps along, and along the axis it does not.
            int32 _Fixed = 0;
            int32 _Varying = 0;

            int32 _PlateA = FCk_GroundNav_Plate::kNoPlate;
            int32 _PlateB = FCk_GroundNav_Plate::kNoPlate;

            float _ClearanceUu = 0.0f;
            float _MidZUu = 0.0f;
        };

        auto Get_FromCell(
            int32 InDirection,
            int32 InFixed,
            int32 InVarying) -> FIntPoint
        {
            return InDirection == 0
                ? FIntPoint{InFixed, InVarying}
                : FIntPoint{InVarying, InFixed};
        }

        auto Get_IsSameRun(
            const FCrossing& InPrevious,
            const FCrossing& InCurrent) -> bool
        {
            return InCurrent._Direction == InPrevious._Direction &&
                   InCurrent._Fixed     == InPrevious._Fixed     &&
                   InCurrent._PlateA    == InPrevious._PlateA    &&
                   InCurrent._PlateB    == InPrevious._PlateB    &&
                   InCurrent._Varying   == InPrevious._Varying + 1;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_Portal::
        Get_Endpoints(
            const FCk_GroundNav_SpanField& InSpans,
            FVector&                       OutMinEnd,
            FVector&                       OutMaxEnd) const
        -> void
    {
        // The segment lies on the cell edge the crossing steps over, which is the far side of the
        // _From cells — one cell size past their min corner along the direction.
        const auto MinCorner = InSpans.Get_ColumnMinCorner(_FromMin.X, _FromMin.Y);
        const auto MaxCorner = InSpans.Get_ColumnMinCorner(_FromMax.X + 1, _FromMax.Y + 1);

        if (_Direction == 0)
        {
            OutMinEnd = FVector{MaxCorner.X, MinCorner.Y, static_cast<double>(_MinEndZUu)};
            OutMaxEnd = FVector{MaxCorner.X, MaxCorner.Y, static_cast<double>(_MaxEndZUu)};

            return;
        }

        OutMinEnd = FVector{MinCorner.X, MaxCorner.Y, static_cast<double>(_MinEndZUu)};
        OutMaxEnd = FVector{MaxCorner.X, MaxCorner.Y, static_cast<double>(_MaxEndZUu)};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PortalField::
        Get_PortalsForPlate(
            int32 InPlateIndex) const
        -> TConstArrayView<int32>
    {
        if (InPlateIndex < 0 || InPlateIndex >= _PlateToPortals.Num())
        { return {}; }

        return _PlateToPortals[InPlateIndex];
    }

    auto
        FCk_GroundNav_PortalField::
        Get_OppositePlate(
            int32 InPortalIndex,
            int32 InPlateIndex) const
        -> int32
    {
        if (InPortalIndex < 0 || InPortalIndex >= _Portals.Num())
        { return FCk_GroundNav_Plate::kNoPlate; }

        const auto& Portal = _Portals[InPortalIndex];

        if (Portal._PlateA == InPlateIndex)
        { return Portal._PlateB; }

        if (Portal._PlateB == InPlateIndex)
        { return Portal._PlateA; }

        return FCk_GroundNav_Plate::kNoPlate;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoExtract_Portals(
            const FCk_GroundNav_SpanField&       InSpans,
            const FCk_GroundNav_LayerField&      InLayers,
            const FCk_GroundNav_ConnectionField& InConnections,
            const FCk_GroundNav_PlateField&      InPlates,
            const FCk_GroundNav_ClearanceField&  InClearance,
            FCk_GroundNav_PortalField&           OutPortals)
        -> FCk_GroundNav_BakeStageResult
    {
        using namespace portals_private;

        auto Result = FCk_GroundNav_BakeStageResult{};

        OutPortals = FCk_GroundNav_PortalField{};

        const auto SizeX = InSpans._SizeX;
        const auto SizeY = InSpans._SizeY;

        const auto AreFieldsAligned =
            SizeX > 0 && SizeY > 0 &&
            InLayers._SizeX == SizeX && InLayers._SizeY == SizeY &&
            InConnections._SizeX == SizeX && InConnections._SizeY == SizeY &&
            InPlates._SizeX == SizeX && InPlates._SizeY == SizeY &&
            InClearance._SizeX == SizeX && InClearance._SizeY == SizeY &&
            InPlates._LayerCount == InLayers._LayerCount &&
            InClearance._LayerCount == InLayers._LayerCount;

        if (NOT AreFieldsAligned)
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        OutPortals._PlateToPortals.SetNum(InPlates._Plates.Num());

        auto Crossings = TArray<FCrossing>{};
        auto ProbesSpent = 0;

        for (auto Direction = 0; Direction < kPositiveDirectionCount; ++Direction)
        {
            const auto Offset = Get_DirectionOffset(Direction);

            for (auto Y = 0; Y < SizeY; ++Y)
            {
                for (auto X = 0; X < SizeX; ++X)
                {
                    const auto NeighbourX = X + Offset.X;
                    const auto NeighbourY = Y + Offset.Y;

                    if (NOT InSpans.Get_IsValidColumn(NeighbourX, NeighbourY))
                    { continue; }

                    const auto& SpanColumn = InSpans.Get_Column(X, Y);
                    const auto& LayerColumn = InLayers.Get_Column(X, Y);
                    const auto& ConnectionColumn = InConnections.Get_Column(X, Y);

                    const auto& NeighbourSpanColumn = InSpans.Get_Column(NeighbourX, NeighbourY);
                    const auto& NeighbourLayerColumn = InLayers.Get_Column(NeighbourX, NeighbourY);

                    for (auto SpanIndex = 0; SpanIndex < SpanColumn.Num(); ++SpanIndex)
                    {
                        ++ProbesSpent;

                        const auto Layer = LayerColumn[SpanIndex];

                        if (Layer == FCk_GroundNav_LayerField::kNoLayer)
                        { continue; }

                        const auto NeighbourSpanIndex = ConnectionColumn[SpanIndex]._Neighbours[Direction];

                        if (NeighbourSpanIndex == FCk_GroundNav_SpanConnections::kNoConnection)
                        { continue; }

                        const auto NeighbourLayer = NeighbourLayerColumn[NeighbourSpanIndex];

                        if (NeighbourLayer == FCk_GroundNav_LayerField::kNoLayer)
                        { continue; }

                        const auto PlateA = InPlates.Get_PlateIndexAt(X, Y, Layer);
                        const auto PlateB = InPlates.Get_PlateIndexAt(NeighbourX, NeighbourY, NeighbourLayer);

                        if (PlateA == FCk_GroundNav_Plate::kNoPlate ||
                            PlateB == FCk_GroundNav_Plate::kNoPlate ||
                            PlateA == PlateB)
                        { continue; }

                        auto Crossing = FCrossing{};

                        Crossing._Direction = Direction;
                        Crossing._Fixed = Direction == 0 ? X : Y;
                        Crossing._Varying = Direction == 0 ? Y : X;
                        Crossing._PlateA = PlateA;
                        Crossing._PlateB = PlateB;

                        // An agent has to fit on both sides of the step, so the room a single cell
                        // pair offers is whichever side is tighter.
                        Crossing._ClearanceUu = FMath::Min(
                            InClearance.Get_ClearanceAt(X, Y, Layer),
                            InClearance.Get_ClearanceAt(NeighbourX, NeighbourY, NeighbourLayer));

                        Crossing._MidZUu = 0.5f *
                            (SpanColumn[SpanIndex]._MaxZ + NeighbourSpanColumn[NeighbourSpanIndex]._MaxZ);

                        Crossings.Emplace(Crossing);
                    }
                }
            }
        }

        // Sorting rather than grouping as we go: the enumeration visits every layer of a column
        // before moving on, so crossings of one boundary arrive interleaved with crossings of the
        // boundary a floor above. The sort key is total, so the run order — and every portal index
        // derived from it — is the same for any input that produces the same crossings.
        Crossings.Sort([](const FCrossing& InLeft, const FCrossing& InRight) -> bool
        {
            if (InLeft._Direction != InRight._Direction) { return InLeft._Direction < InRight._Direction; }
            if (InLeft._Fixed     != InRight._Fixed)     { return InLeft._Fixed     < InRight._Fixed; }
            if (InLeft._PlateA    != InRight._PlateA)    { return InLeft._PlateA    < InRight._PlateA; }
            if (InLeft._PlateB    != InRight._PlateB)    { return InLeft._PlateB    < InRight._PlateB; }

            return InLeft._Varying < InRight._Varying;
        });

        for (auto Index = 0; Index < Crossings.Num(); ++Index)
        {
            const auto& Crossing = Crossings[Index];

            const auto IsContinuation = Index > 0 && Get_IsSameRun(Crossings[Index - 1], Crossing);

            const auto Cell = Get_FromCell(Crossing._Direction, Crossing._Fixed, Crossing._Varying);

            if (NOT IsContinuation)
            {
                auto Portal = FCk_GroundNav_Portal{};

                Portal._PlateA = Crossing._PlateA;
                Portal._PlateB = Crossing._PlateB;
                Portal._Direction = Crossing._Direction;
                Portal._FromMin = Cell;
                Portal._FromMax = Cell;
                Portal._MinEndZUu = Crossing._MidZUu;
                Portal._MaxEndZUu = Crossing._MidZUu;
                Portal._TraversalClearanceUu = Crossing._ClearanceUu;

                OutPortals._Portals.Emplace(Portal);

                continue;
            }

            auto& Portal = OutPortals._Portals.Last();

            Portal._FromMax = Cell;
            Portal._MaxEndZUu = Crossing._MidZUu;

            // The widest crossing wins, because the agent chooses where along the interval to cross.
            Portal._TraversalClearanceUu = FMath::Max(Portal._TraversalClearanceUu, Crossing._ClearanceUu);
        }

        for (auto PortalIndex = 0; PortalIndex < OutPortals._Portals.Num(); ++PortalIndex)
        {
            const auto& Portal = OutPortals._Portals[PortalIndex];

            OutPortals._PlateToPortals[Portal._PlateA].Emplace(PortalIndex);
            OutPortals._PlateToPortals[Portal._PlateB].Emplace(PortalIndex);
        }

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);
        Result.Set_ProbesSpent(ProbesSpent);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
