#include "CkGroundNav_Clearance.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace clearance_private
    {
        // Large enough that no reachable chamfer sum comes near it, small enough that adding a step
        // cost cannot overflow.
        constexpr int32 kUnreached = TNumericLimits<int32>::Max() / 4;

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
            const FCk_GroundNav_LayerField& InLayers,
            float                           InCellSizeUu,
            FCk_GroundNav_ClearanceField&   OutClearance)
        -> FCk_GroundNav_BakeStageResult
    {
        using namespace clearance_private;

        auto Result = FCk_GroundNav_BakeStageResult{};

        if (InCellSizeUu <= 0.0f || InLayers._SizeX <= 0 || InLayers._SizeY <= 0)
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

        for (auto LayerIndex = 0; LayerIndex < InLayers._LayerCount; ++LayerIndex)
        {
            Distances.Reset();
            Distances.Init(0, CellCount);

            for (auto Y = 0; Y < SizeY; ++Y)
            {
                for (auto X = 0; X < SizeX; ++X)
                {
                    ++ProbesSpent;

                    const auto IsWalkable = InLayers.Get_OccupancyAt(X, Y, LayerIndex) > 0;

                    Distances[(Y * SizeX) + X] = IsWalkable ? kUnreached : 0;
                }
            }

            const auto Do_Relax = [&](int32 InX, int32 InY, int32 InFromX, int32 InFromY, int32 InCost) -> void
            {
                ++ProbesSpent;

                auto& Current = Distances[(InY * SizeX) + InX];
                const auto Candidate = Get_Distance(Distances, SizeX, SizeY, InFromX, InFromY) + InCost;

                Current = FMath::Min(Current, Candidate);
            };

            for (auto Y = 0; Y < SizeY; ++Y)
            {
                for (auto X = 0; X < SizeX; ++X)
                {
                    Do_Relax(X, Y, X - 1, Y,     kChamferOrthogonalCost);
                    Do_Relax(X, Y, X,     Y - 1, kChamferOrthogonalCost);
                    Do_Relax(X, Y, X - 1, Y - 1, kChamferDiagonalCost);
                    Do_Relax(X, Y, X + 1, Y - 1, kChamferDiagonalCost);
                }
            }

            for (auto Y = SizeY - 1; Y >= 0; --Y)
            {
                for (auto X = SizeX - 1; X >= 0; --X)
                {
                    Do_Relax(X, Y, X + 1, Y,     kChamferOrthogonalCost);
                    Do_Relax(X, Y, X,     Y + 1, kChamferOrthogonalCost);
                    Do_Relax(X, Y, X + 1, Y + 1, kChamferDiagonalCost);
                    Do_Relax(X, Y, X - 1, Y + 1, kChamferDiagonalCost);
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

        // A probe here is one cell read: the occupancy that seeds a cell, each of the four neighbours
        // a chamfer pass relaxes it against, and the distance read back out as world clearance.
        Result.Set_ProbesSpent(ProbesSpent);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
