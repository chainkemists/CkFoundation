#include "CkGroundNav_Query_Projection.h"

#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace projection_private
    {
        struct FProjectionCandidate
        {
            bool _Found = false;

            int32 _Band = 0;
            double _HorizontalUu = 0.0;
            double _VerticalUu = 0.0;

            FIntPoint _FieldCell = FIntPoint::ZeroValue;

            FCk_GroundNav_SurfaceRef _Surface;

            float _SurfaceZUu = 0.0f;
            float _ClearanceUu = 0.0f;
        };

        auto Get_IsBetterCandidate(
            const FProjectionCandidate& InBest,
            int32                       InBand,
            double                      InHorizontalUu,
            double                      InVerticalUu) -> bool
        {
            if (NOT InBest._Found)
            { return true; }

            if (InBand != InBest._Band)
            { return InBand < InBest._Band; }

            if (InHorizontalUu != InBest._HorizontalUu)
            { return InHorizontalUu < InBest._HorizontalUu; }

            return InVerticalUu < InBest._VerticalUu;
        }

        auto Get_IsWithinMode(
            double                        InDeltaZUu,
            ECk_NavSurface_ProjectionMode InMode) -> bool
        {
            switch (InMode)
            {
                case ECk_NavSurface_ProjectionMode::Down:
                { return InDeltaZUu <= 0.0; }

                case ECk_NavSurface_ProjectionMode::Up:
                { return InDeltaZUu >= 0.0; }

                default:
                { return true; }
            }
        }

        auto Get_IsWithinReach(
            double InDeltaZUu,
            float  InUpExtentUu,
            float  InDownExtentUu) -> bool
        {
            if (InDeltaZUu > 0.0)
            { return InDeltaZUu <= static_cast<double>(InUpExtentUu); }

            if (InDeltaZUu < 0.0)
            { return -InDeltaZUu <= static_cast<double>(InDownExtentUu); }

            return true;
        }

        // The cells at Chebyshev distance exactly InRing, in Y-then-X ascending order and each one once.
        auto DoCollect_RingCells(
            const FIntPoint&   InCentre,
            int32              InRing,
            TArray<FIntPoint>& OutCells) -> void
        {
            OutCells.Reset();

            if (InRing <= 0)
            {
                OutCells.Add(InCentre);
                return;
            }

            for (auto OffsetY = -InRing; OffsetY <= InRing; ++OffsetY)
            {
                const auto IsEdgeRow = OffsetY == -InRing || OffsetY == InRing;

                for (auto OffsetX = -InRing; OffsetX <= InRing; ++OffsetX)
                {
                    if (NOT IsEdgeRow && OffsetX != -InRing && OffsetX != InRing)
                    { continue; }

                    OutCells.Add(FIntPoint{InCentre.X + OffsetX, InCentre.Y + OffsetY});
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ProjectPoint(
            const FCk_GroundNav_Field&           InField,
            const FCk_GroundNav_ProjectionQuery& InQuery)
        -> FCk_GroundNav_ProjectionResult
    {
        using namespace projection_private;

        auto Result = FCk_GroundNav_ProjectionResult{};

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;

            return Result;
        }

        const auto& Params = InField._Params;

        const auto CellSize = static_cast<double>(Params._Config.Get_CellSizeUu());
        const auto StepHeight = static_cast<double>(Params._Profile.Get_StepHeightUu());
        const auto HorizontalExtent = static_cast<double>(InQuery._HorizontalExtentUu);

        const auto QueryXY = FVector2D{InQuery._Location.X, InQuery._Location.Y};
        const auto CentreCell = Get_FieldCellAt(Params, QueryXY);

        const auto MaxRing = CellSize > 0.0 && HorizontalExtent > 0.0
            ? FMath::CeilToInt32(HorizontalExtent / CellSize)
            : 0;

        auto TouchedTiles = TArray<int32>{};
        auto RingCells = TArray<FIntPoint>{};
        auto Best = FProjectionCandidate{};

        for (auto Ring = 0; Ring <= MaxRing; ++Ring)
        {
            DoCollect_RingCells(CentreCell, Ring, RingCells);

            for (const auto& Cell : RingCells)
            {
                // A ring's corners reach past the box. A cell outside the box cannot hold the answer, so
                // it is not billed to its tile either — otherwise a distant unbuilt tile would turn a
                // NoSurface into an Unbuilt.
                const auto HorizontalUu = Get_HorizontalDistanceToCell(Params, Cell, QueryXY);

                if (HorizontalUu > HorizontalExtent)
                { continue; }

                const auto Address = Get_CellAddress(InField, Cell);

                if (NOT Address.Get_IsValid())
                { continue; }

                TouchedTiles.AddUnique(Address._TileIndex);

                if (Get_TileStatus(InField, Address._TileIndex) != ECk_GroundNav_BuildStatus::Built)
                {
                    Result._Cost._TouchedUnbuiltTile = true;
                    continue;
                }

                const auto& Tile = InField._Tiles[Address._TileIndex];

                for (auto Layer = 0; Layer < Tile._LayerCount; ++Layer)
                {
                    ++Result._Cost._CellsRead;

                    auto Surface = FCk_GroundNav_SurfaceRef{};
                    auto SurfaceZUu = 0.0f;
                    auto ClearanceUu = 0.0f;

                    if (NOT Get_SurfaceAt(InField, Address, Layer, Surface, SurfaceZUu, ClearanceUu))
                    { continue; }

                    if (NOT Get_IsAdmitted(ClearanceUu, InQuery._Agent))
                    { continue; }

                    const auto DeltaZUu = static_cast<double>(SurfaceZUu) - InQuery._Location.Z;

                    if (NOT Get_IsWithinMode(DeltaZUu, InQuery._Mode))
                    { continue; }

                    if (NOT Get_IsWithinReach(DeltaZUu, InQuery._UpExtentUu, InQuery._DownExtentUu))
                    { continue; }

                    const auto VerticalUu = FMath::Abs(DeltaZUu);
                    const auto Band = StepHeight > 0.0
                        ? FMath::FloorToInt32(VerticalUu / StepHeight)
                        : 0;

                    if (NOT Get_IsBetterCandidate(Best, Band, HorizontalUu, VerticalUu))
                    { continue; }

                    Best._Found = true;
                    Best._Band = Band;
                    Best._HorizontalUu = HorizontalUu;
                    Best._VerticalUu = VerticalUu;
                    Best._FieldCell = Cell;
                    Best._Surface = Surface;
                    Best._SurfaceZUu = SurfaceZUu;
                    Best._ClearanceUu = ClearanceUu;
                }
            }

            // The nearest any cell of the next ring can be is Ring cells away, and nothing beats band zero.
            if (Best._Found && Best._Band == 0 && Best._HorizontalUu < static_cast<double>(Ring) * CellSize)
            { break; }
        }

        Result._Cost._TilesTouched = TouchedTiles.Num();

        if (NOT Best._Found)
        {
            Result._Status = Result._Cost._TouchedUnbuiltTile
                ? ECk_NavSurface_QueryStatus::Unbuilt
                : ECk_NavSurface_QueryStatus::NoSurface;

            return Result;
        }

        const auto ClosestXY = Get_ClosestPointInCellXY(Params, Best._FieldCell, QueryXY);

        Result._Status = ECk_NavSurface_QueryStatus::Success;
        Result._Location = FVector{ClosestXY.X, ClosestXY.Y, static_cast<double>(Best._SurfaceZUu)};
        Result._SurfaceNormal = Get_SurfaceNormal(InField, Best._Surface);
        Result._Surface = Best._Surface;
        Result._ClearanceUu = Best._ClearanceUu;

        return Result;
    }

    auto
        Get_ProjectPoints_Batch(
            const FCk_GroundNav_Field&                     InField,
            TConstArrayView<FCk_GroundNav_ProjectionQuery> InQueries,
            TArrayView<FCk_GroundNav_ProjectionResult>     OutResults)
        -> void
    {
        const auto StorageIsLargeEnough = OutResults.Num() >= InQueries.Num();
        CK_ENSURE_IF_NOT(StorageIsLargeEnough,
            TEXT("GroundNav projection batch has storage for [{}] results for [{}] queries"),
            OutResults.Num(), InQueries.Num())
        { return; }

        for (auto Index = 0; Index < InQueries.Num(); ++Index)
        { OutResults[Index] = Get_ProjectPoint(InField, InQueries[Index]); }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsNavigable(
            const FCk_GroundNav_Field&            InField,
            const FCk_GroundNav_IsNavigableQuery& InQuery)
        -> FCk_GroundNav_IsNavigableResult
    {
        auto Result = FCk_GroundNav_IsNavigableResult{};

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;

            return Result;
        }

        const auto QueryXY = FVector2D{InQuery._Location.X, InQuery._Location.Y};

        auto Candidates = TArray<FCk_GroundNav_CellAddress, TInlineAllocator<4>>{};
        Get_CellAddressesAt(InField, QueryXY, Candidates);

        if (Candidates.IsEmpty())
        {
            Result._Status = ECk_NavSurface_QueryStatus::NoSurface;

            return Result;
        }

        const auto ToleranceUu = static_cast<double>(InQuery._VerticalToleranceUu);

        auto TouchedTiles = TArray<int32, TInlineAllocator<4>>{};
        auto Found = false;
        auto BestVerticalUu = 0.0;

        for (const auto& Address : Candidates)
        {
            TouchedTiles.AddUnique(Address._TileIndex);

            if (Get_TileStatus(InField, Address._TileIndex) != ECk_GroundNav_BuildStatus::Built)
            {
                Result._Cost._TouchedUnbuiltTile = true;
                continue;
            }

            const auto& Tile = InField._Tiles[Address._TileIndex];

            for (auto Layer = 0; Layer < Tile._LayerCount; ++Layer)
            {
                ++Result._Cost._CellsRead;

                auto Surface = FCk_GroundNav_SurfaceRef{};
                auto SurfaceZUu = 0.0f;
                auto ClearanceUu = 0.0f;

                if (NOT Get_SurfaceAt(InField, Address, Layer, Surface, SurfaceZUu, ClearanceUu))
                { continue; }

                if (NOT Get_IsAdmitted(ClearanceUu, InQuery._Agent))
                { continue; }

                const auto VerticalUu = FMath::Abs(static_cast<double>(SurfaceZUu) - InQuery._Location.Z);

                if (VerticalUu > ToleranceUu)
                { continue; }

                if (Found && VerticalUu >= BestVerticalUu)
                { continue; }

                Found = true;
                BestVerticalUu = VerticalUu;

                Result._Surface = Surface;
                Result._SurfaceZUu = SurfaceZUu;
                Result._ClearanceUu = ClearanceUu;
            }
        }

        Result._Cost._TilesTouched = TouchedTiles.Num();

        if (Found)
        {
            Result._Status = ECk_NavSurface_QueryStatus::Success;

            return Result;
        }

        Result._Status = Result._Cost._TouchedUnbuiltTile
            ? ECk_NavSurface_QueryStatus::Unbuilt
            : ECk_NavSurface_QueryStatus::NoSurface;

        return Result;
    }

    auto
        Get_IsNavigable_Batch(
            const FCk_GroundNav_Field&                      InField,
            TConstArrayView<FCk_GroundNav_IsNavigableQuery> InQueries,
            TArrayView<FCk_GroundNav_IsNavigableResult>     OutResults)
        -> void
    {
        const auto StorageIsLargeEnough = OutResults.Num() >= InQueries.Num();
        CK_ENSURE_IF_NOT(StorageIsLargeEnough,
            TEXT("GroundNav navigable batch has storage for [{}] results for [{}] queries"),
            OutResults.Num(), InQueries.Num())
        { return; }

        for (auto Index = 0; Index < InQueries.Num(); ++Index)
        { OutResults[Index] = Get_IsNavigable(InField, InQueries[Index]); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
