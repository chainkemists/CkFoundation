#include "CkGroundNav_TileBake.h"

#include "CkGroundNav/Bake/CkGroundNav_Clearance.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"
#include "CkGroundNav/Bake/CkGroundNav_Portals.h"
#include "CkGroundNav/Bake/CkGroundNav_Rasterize.h"
#include "CkGroundNav/Bake/CkGroundNav_Walkability.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace tilebake_private
    {
        auto Get_TileCellCount(const FCk_GroundNav_TileBakeParams& InParams) -> int32
        {
            const auto CellSize = InParams._Config.Get_CellSizeUu();

            return CellSize > 0.0f
                ? FMath::Max(1, FMath::CeilToInt32(InParams._Config.Get_TileSizeUu() / CellSize))
                : 0;
        }

        auto Get_AreParamsValid(const FCk_GroundNav_TileBakeParams& InParams) -> bool
        {
            return InParams._Config.Get_IsValid() &&
                   InParams._MaxClearanceUu >= 0.0f &&
                   InParams._MaxZUu > InParams._MinZUu &&
                   Get_TileCellCount(InParams) > 0;
        }

        auto Make_FailedTile(const FCk_GroundNav_TileBakeParams& InParams) -> FCk_GroundNav_Tile
        {
            auto Tile = FCk_GroundNav_Tile{};

            Tile._Coord = InParams._Coord;
            Tile._Epoch = InParams._Epoch;
            Tile._Status = ECk_GroundNav_BuildStatus::Failed;
            Tile._CellSizeUu = InParams._Config.Get_CellSizeUu();
            Tile._MaxClearanceUu = InParams._MaxClearanceUu;

            return Tile;
        }

        /**
         * Demote every span outside the tile's own window so that nothing downstream of clearance can
         * see the halo.
         *
         * The halo has to survive up to and including the distance transform — that is what it is for
         * — and must be gone before the decomposition, or a plate would straddle two tiles and its id
         * would mean two different things depending on which tile you asked.
         */
        auto Do_MaskHalo(
            int32                     InHaloCells,
            int32                     InTileCells,
            FCk_GroundNav_LayerField& InOutLayers) -> void
        {
            for (auto Y = 0; Y < InOutLayers._SizeY; ++Y)
            {
                for (auto X = 0; X < InOutLayers._SizeX; ++X)
                {
                    const auto IsInsideTile =
                        X >= InHaloCells && Y >= InHaloCells &&
                        X < InHaloCells + InTileCells && Y < InHaloCells + InTileCells;

                    if (IsInsideTile)
                    { continue; }

                    for (auto& Layer : InOutLayers._Columns[InOutLayers.Get_ColumnIndex(X, Y)])
                    { Layer = FCk_GroundNav_LayerField::kNoLayer; }
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_HaloCellCount(
            float InMaxClearanceUu,
            float InCellSizeUu)
        -> int32
    {
        if (InCellSizeUu <= 0.0f || InMaxClearanceUu <= 0.0f)
        { return 0; }

        return FMath::CeilToInt32(InMaxClearanceUu / InCellSizeUu);
    }

    auto
        Get_TileBounds(
            const FCk_GroundNav_TileBakeParams& InParams)
        -> FBox
    {
        using namespace tilebake_private;

        const auto CellSize = static_cast<double>(InParams._Config.Get_CellSizeUu());
        const auto SpanUu = static_cast<double>(Get_TileCellCount(InParams)) * CellSize;

        const auto MinX = InParams._FieldOriginXY.X + (static_cast<double>(InParams._Coord._X) * SpanUu);
        const auto MinY = InParams._FieldOriginXY.Y + (static_cast<double>(InParams._Coord._Y) * SpanUu);

        return FBox{
            FVector{MinX, MinY, static_cast<double>(InParams._MinZUu)},
            FVector{MinX + SpanUu, MinY + SpanUu, static_cast<double>(InParams._MaxZUu)}};
    }

    auto
        Get_TileHaloBounds(
            const FCk_GroundNav_TileBakeParams& InParams)
        -> FBox
    {
        const auto CellSize = static_cast<double>(InParams._Config.Get_CellSizeUu());
        const auto HaloUu = static_cast<double>(
            Get_HaloCellCount(InParams._MaxClearanceUu, InParams._Config.Get_CellSizeUu())) * CellSize;

        const auto Bounds = Get_TileBounds(InParams);

        return FBox{
            FVector{Bounds.Min.X - HaloUu, Bounds.Min.Y - HaloUu, Bounds.Min.Z},
            FVector{Bounds.Max.X + HaloUu, Bounds.Max.Y + HaloUu, Bounds.Max.Z}};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoBake_Tile(
            const FCk_GroundNav_GeometryBatch&  InGeometry,
            const FCk_GroundNav_TileBakeParams& InParams,
            FCk_GroundNav_Tile&                 OutTile)
        -> FCk_GroundNav_BakeStageResult
    {
        using namespace tilebake_private;

        auto Result = FCk_GroundNav_BakeStageResult{};

        OutTile = Make_FailedTile(InParams);

        if (NOT Get_AreParamsValid(InParams))
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        const auto CellSize = InParams._Config.Get_CellSizeUu();
        const auto TileCells = Get_TileCellCount(InParams);
        const auto HaloCells = Get_HaloCellCount(InParams._MaxClearanceUu, CellSize);

        const auto HaloBounds = Get_TileHaloBounds(InParams);

        auto Spans = FCk_GroundNav_SpanField{};
        const auto RasterResult = DoRasterizeSpans(
            InGeometry, HaloBounds, InParams._Config, InParams._Profile, Spans);

        if (NOT RasterResult.Get_IsCompleted())
        {
            Result.Set_Status(RasterResult.Get_Status());
            Result.Set_DroppedInputCount(RasterResult.Get_DroppedInputCount());
            return Result;
        }

        auto ProbesSpent = RasterResult.Get_ProbesSpent();

        auto Connections = FCk_GroundNav_ConnectionField{};
        const auto WalkabilityResult = DoFilter_Walkability(InParams._Profile, Spans, Connections);

        if (NOT WalkabilityResult.Get_IsCompleted())
        {
            Result.Set_Status(WalkabilityResult.Get_Status());
            return Result;
        }

        ProbesSpent += WalkabilityResult.Get_ProbesSpent();

        auto Layers = FCk_GroundNav_LayerField{};
        const auto LayerResult = DoExtract_Layers(Spans, Connections, Layers);

        if (NOT LayerResult.Get_IsCompleted())
        {
            Result.Set_Status(LayerResult.Get_Status());
            return Result;
        }

        ProbesSpent += LayerResult.Get_ProbesSpent();

        auto HaloClearance = FCk_GroundNav_ClearanceField{};
        const auto ClearanceResult = DoCompute_Clearance(Layers, CellSize, HaloClearance);

        if (NOT ClearanceResult.Get_IsCompleted())
        {
            Result.Set_Status(ClearanceResult.Get_Status());
            return Result;
        }

        ProbesSpent += ClearanceResult.Get_ProbesSpent();

        Do_MaskHalo(HaloCells, TileCells, Layers);

        auto Plates = FCk_GroundNav_PlateField{};
        const auto PlateResult = DoDecompose_Plates(Spans, Layers, InParams._MergeTunables, Plates);

        if (NOT PlateResult.Get_IsCompleted())
        {
            Result.Set_Status(PlateResult.Get_Status());
            return Result;
        }

        ProbesSpent += PlateResult.Get_ProbesSpent();

        auto Portals = FCk_GroundNav_PortalField{};
        const auto PortalResult = DoExtract_Portals(Spans, Layers, Connections, Plates, HaloClearance, Portals);

        if (NOT PortalResult.Get_IsCompleted())
        {
            Result.Set_Status(PortalResult.Get_Status());
            return Result;
        }

        ProbesSpent += PortalResult.Get_ProbesSpent();

        // ---- Crop to the tile's own window ----------------------------------------------------------

        const auto TileBounds = Get_TileBounds(InParams);
        const auto LayerCount = Layers._LayerCount;
        const auto TileCellCount = TileCells * TileCells;

        OutTile = FCk_GroundNav_Tile{};
        OutTile._Coord = InParams._Coord;
        OutTile._Epoch = InParams._Epoch;
        OutTile._Status = ECk_GroundNav_BuildStatus::Built;
        OutTile._Origin = TileBounds.Min;
        OutTile._CellSizeUu = CellSize;
        OutTile._MaxClearanceUu = InParams._MaxClearanceUu;
        OutTile._SizeX = TileCells;
        OutTile._SizeY = TileCells;
        OutTile._LayerCount = LayerCount;

        OutTile._SurfaceZ.Init(FCk_GroundNav_Tile::kNoSurfaceZ, TileCellCount * LayerCount);

        OutTile._Clearance._SizeX = TileCells;
        OutTile._Clearance._SizeY = TileCells;
        OutTile._Clearance._LayerCount = LayerCount;
        OutTile._Clearance._CellSizeUu = CellSize;
        OutTile._Clearance._Cells.Init(0.0f, TileCellCount * LayerCount);

        for (auto LayerIndex = 0; LayerIndex < LayerCount; ++LayerIndex)
        {
            for (auto Y = 0; Y < TileCells; ++Y)
            {
                for (auto X = 0; X < TileCells; ++X)
                {
                    const auto HaloX = X + HaloCells;
                    const auto HaloY = Y + HaloCells;
                    const auto TileCellIndex = (LayerIndex * TileCellCount) + (Y * TileCells) + X;

                    OutTile._Clearance._Cells[TileCellIndex] = FMath::Min(
                        HaloClearance.Get_ClearanceAt(HaloX, HaloY, LayerIndex), InParams._MaxClearanceUu);

                    auto TopZ = 0.0f;
                    auto Normal = FVector::UpVector;

                    if (NOT Get_CellSurface(Spans, Layers, HaloX, HaloY, LayerIndex, TopZ, Normal))
                    { continue; }

                    OutTile._SurfaceZ[TileCellIndex] = TopZ;
                }
            }
        }

        OutTile._Plates._SizeX = TileCells;
        OutTile._Plates._SizeY = TileCells;
        OutTile._Plates._LayerCount = LayerCount;
        OutTile._Plates._Plates = Plates._Plates;
        OutTile._Plates._CellToPlate.Init(FCk_GroundNav_Plate::kNoPlate, TileCellCount * LayerCount);

        for (auto& Plate : OutTile._Plates._Plates)
        {
            Plate._MinX -= HaloCells;
            Plate._MinY -= HaloCells;
            Plate._MaxX -= HaloCells;
            Plate._MaxY -= HaloCells;
        }

        for (auto LayerIndex = 0; LayerIndex < LayerCount; ++LayerIndex)
        {
            for (auto Y = 0; Y < TileCells; ++Y)
            {
                for (auto X = 0; X < TileCells; ++X)
                {
                    OutTile._Plates._CellToPlate[(LayerIndex * TileCellCount) + (Y * TileCells) + X] =
                        Plates.Get_PlateIndexAt(X + HaloCells, Y + HaloCells, LayerIndex);
                }
            }
        }

        OutTile._Portals = Portals;

        for (auto& Portal : OutTile._Portals._Portals)
        {
            Portal._FromMin -= FIntPoint{HaloCells, HaloCells};
            Portal._FromMax -= FIntPoint{HaloCells, HaloCells};

            // Under the same ceiling as the cells it was derived from. A crossing that advertised more
            // room than any cell either side of it admits would let exactly the agent the cap exists
            // to turn away walk through it.
            Portal._TraversalClearanceUu = FMath::Min(
                Portal._TraversalClearanceUu, InParams._MaxClearanceUu);
        }

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);
        Result.Set_ProbesSpent(ProbesSpent);
        Result.Set_DroppedInputCount(RasterResult.Get_DroppedInputCount());

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
