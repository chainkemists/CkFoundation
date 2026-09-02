#include "CkGroundNav_Clearance.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace clearance_private
    {
        // Large enough that no reachable chamfer sum comes near it, small enough that adding a step
        // cost cannot overflow.
        constexpr int32 kUnreached = TNumericLimits<int32>::Max() / 4;

        constexpr int32 kNoSpan = -1;

        auto Get_Distance(
            const TArray<int32>& InDistances,
            int32                InSizeX,
            int32                InSizeY,
            int32                InX,
            int32                InY) -> int32
        {
            // Outside the field is blocked, not unknown. See the header on why the border must seed
            // the sweep and what a tiled bake owes because of it.
            if (InX < 0 || InY < 0 || InX >= InSizeX || InY >= InSizeY)
            { return 0; }

            return InDistances[(InY * InSizeX) + InX];
        }

        auto Get_DirectionBetween(
            int32 InFromX,
            int32 InFromY,
            int32 InToX,
            int32 InToY) -> int32
        {
            const auto DeltaX = InToX - InFromX;
            const auto DeltaY = InToY - InFromY;

            if (DeltaX == 1 && DeltaY == 0)
            { return 0; }

            if (DeltaX == 0 && DeltaY == 1)
            { return 1; }

            if (DeltaX == -1 && DeltaY == 0)
            { return 2; }

            return 3;
        }

        /**
         * One layer's view of the connection field: which span each column contributes to the layer,
         * and whether two orthogonal neighbours are linked. Everything else in the sweep is grid
         * arithmetic; this is the one place walkability is consulted.
         */
        struct FLayerLinks
        {
        public:
            const FCk_GroundNav_ConnectionField* _Connections = nullptr;

            int32 _SizeX = 0;
            int32 _SizeY = 0;

            // kNoSpan where the layer has no surface in the column.
            TArray<int32> _SpanOfCell;

        public:
            auto Get_IsInside(int32 InX, int32 InY) const -> bool
            {
                return InX >= 0 && InY >= 0 && InX < _SizeX && InY < _SizeY;
            }

            auto Get_SpanAt(int32 InX, int32 InY) const -> int32
            {
                return Get_IsInside(InX, InY) ? _SpanOfCell[(InY * _SizeX) + InX] : kNoSpan;
            }

            /** Whether the layer's span in one column names the layer's span in an orthogonal neighbour. */
            auto Get_IsLinked(
                int32  InFromX,
                int32  InFromY,
                int32  InToX,
                int32  InToY,
                int32& InOutProbes) const -> bool
            {
                const auto FromSpan = Get_SpanAt(InFromX, InFromY);
                const auto ToSpan = Get_SpanAt(InToX, InToY);

                if (FromSpan == kNoSpan || ToSpan == kNoSpan)
                { return false; }

                ++InOutProbes;

                const auto Direction = Get_DirectionBetween(InFromX, InFromY, InToX, InToY);

                return _Connections->Get_Column(InFromX, InFromY)[FromSpan]._Neighbours[Direction] == ToSpan;
            }

            /**
             * A diagonal neighbour is linked when either two-step path around the corner is, cell by
             * cell. Four neighbours, not eight, is the bake's rule for stepping; a diagonal here is
             * only the chamfer metric's shortcut for two orthogonal steps, so it needs those steps.
             */
            auto Get_IsLinkedDiagonally(
                int32  InFromX,
                int32  InFromY,
                int32  InToX,
                int32  InToY,
                int32& InOutProbes) const -> bool
            {
                const auto ViaRow = Get_IsLinked(InFromX, InFromY, InToX, InFromY, InOutProbes) &&
                                    Get_IsLinked(InToX, InFromY, InToX, InToY, InOutProbes);

                if (ViaRow)
                { return true; }

                return Get_IsLinked(InFromX, InFromY, InFromX, InToY, InOutProbes) &&
                       Get_IsLinked(InFromX, InToY, InToX, InToY, InOutProbes);
            }
        };
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ChamferDistance(
            int32 InDeltaX,
            int32 InDeltaY)
        -> int32
    {
        const auto AbsX = FMath::Abs(InDeltaX);
        const auto AbsY = FMath::Abs(InDeltaY);

        const auto Diagonal = FMath::Min(AbsX, AbsY);
        const auto Straight = FMath::Max(AbsX, AbsY) - Diagonal;

        return (Diagonal * kChamferDiagonalCost) + (Straight * kChamferOrthogonalCost);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_ClearanceField::
        Get_MaxClearance() const
        -> float
    {
        auto Max = 0.0f;

        for (const auto& Cell : _Cells)
        { Max = FMath::Max(Max, Cell); }

        return Max;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoCompute_Clearance(
            const FCk_GroundNav_LayerField&      InLayers,
            const FCk_GroundNav_ConnectionField& InConnections,
            float                                InCellSizeUu,
            FCk_GroundNav_ClearanceField&        OutClearance)
        -> FCk_GroundNav_BakeStageResult
    {
        using namespace clearance_private;

        auto Result = FCk_GroundNav_BakeStageResult{};

        const auto ShapesAgree =
            InConnections._SizeX == InLayers._SizeX &&
            InConnections._SizeY == InLayers._SizeY &&
            InConnections._Columns.Num() == InLayers._Columns.Num();

        if (InCellSizeUu <= 0.0f || InLayers._SizeX <= 0 || InLayers._SizeY <= 0 || NOT ShapesAgree)
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        const auto SizeX = InLayers._SizeX;
        const auto SizeY = InLayers._SizeY;
        const auto CellCount = SizeX * SizeY;

        OutClearance = FCk_GroundNav_ClearanceField{};
        OutClearance._SizeX = SizeX;
        OutClearance._SizeY = SizeY;
        OutClearance._LayerCount = InLayers._LayerCount;
        OutClearance._CellSizeUu = InCellSizeUu;
        OutClearance._Cells.Init(0.0f, CellCount * InLayers._LayerCount);

        // One cell of chamfer cost buys one cell size of world clearance, which is what makes an
        // isolated cell read as exactly one cell size rather than zero.
        const auto WorldPerUnit = InCellSizeUu / static_cast<float>(kChamferOrthogonalCost);

        auto Distances = TArray<int32>{};
        auto ProbesSpent = 0;

        auto Links = FLayerLinks{};
        Links._Connections = &InConnections;
        Links._SizeX = SizeX;
        Links._SizeY = SizeY;

        for (auto LayerIndex = 0; LayerIndex < InLayers._LayerCount; ++LayerIndex)
        {
            Distances.Reset();
            Distances.Init(0, CellCount);

            Links._SpanOfCell.Reset();
            Links._SpanOfCell.Init(kNoSpan, CellCount);

            for (auto Y = 0; Y < SizeY; ++Y)
            {
                for (auto X = 0; X < SizeX; ++X)
                {
                    ++ProbesSpent;

                    const auto& Column = InLayers.Get_Column(X, Y);
                    const auto CellIndex = (Y * SizeX) + X;

                    for (auto SpanIndex = 0; SpanIndex < Column.Num(); ++SpanIndex)
                    {
                        if (Column[SpanIndex] != LayerIndex)
                        { continue; }

                        Links._SpanOfCell[CellIndex] = SpanIndex;
                        break;
                    }

                    Distances[CellIndex] = Links._SpanOfCell[CellIndex] != kNoSpan ? kUnreached : 0;
                }
            }

            // A neighbour contributes its own distance only where the body could step to it. A
            // walkable neighbour it is not linked to — a wall top, a ledge, another storey's floor
            // packed into this layer — contributes as a blocked cell would: the boundary between them
            // is the obstacle, one step away.
            const auto Do_Relax = [&](int32 InX, int32 InY, int32 InFromX, int32 InFromY, int32 InCost, bool InIsDiagonal) -> void
            {
                ++ProbesSpent;

                auto& Current = Distances[(InY * SizeX) + InX];

                if (Current == 0)
                { return; }

                const auto IsLinked = InIsDiagonal
                    ? Links.Get_IsLinkedDiagonally(InFromX, InFromY, InX, InY, ProbesSpent)
                    : Links.Get_IsLinked(InFromX, InFromY, InX, InY, ProbesSpent);

                const auto FromDistance = IsLinked ? Get_Distance(Distances, SizeX, SizeY, InFromX, InFromY) : 0;

                Current = FMath::Min(Current, FromDistance + InCost);
            };

            constexpr auto Straight = false;
            constexpr auto Diagonal = true;

            for (auto Y = 0; Y < SizeY; ++Y)
            {
                for (auto X = 0; X < SizeX; ++X)
                {
                    Do_Relax(X, Y, X - 1, Y,     kChamferOrthogonalCost, Straight);
                    Do_Relax(X, Y, X,     Y - 1, kChamferOrthogonalCost, Straight);
                    Do_Relax(X, Y, X - 1, Y - 1, kChamferDiagonalCost,   Diagonal);
                    Do_Relax(X, Y, X + 1, Y - 1, kChamferDiagonalCost,   Diagonal);
                }
            }

            for (auto Y = SizeY - 1; Y >= 0; --Y)
            {
                for (auto X = SizeX - 1; X >= 0; --X)
                {
                    Do_Relax(X, Y, X + 1, Y,     kChamferOrthogonalCost, Straight);
                    Do_Relax(X, Y, X,     Y + 1, kChamferOrthogonalCost, Straight);
                    Do_Relax(X, Y, X + 1, Y + 1, kChamferDiagonalCost,   Diagonal);
                    Do_Relax(X, Y, X - 1, Y + 1, kChamferDiagonalCost,   Diagonal);
                }
            }

            const auto PlaneOffset = LayerIndex * CellCount;

            for (auto CellIndex = 0; CellIndex < CellCount; ++CellIndex)
            {
                ++ProbesSpent;

                OutClearance._Cells[PlaneOffset + CellIndex] =
                    static_cast<float>(Distances[CellIndex]) * WorldPerUnit;
            }
        }

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);

        // A probe here is one cell read: the column scan that seeds a cell, each neighbour a chamfer
        // pass relaxes it against, each connection consulted to decide whether that neighbour counts
        // as ground or as wall, and the distance read back out as world clearance.
        Result.Set_ProbesSpent(ProbesSpent);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
