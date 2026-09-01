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
                   // Strictly positive: a zero cap gives a zero-cell halo, which costs BOTH the
                   // cross-tile connectivity (no stub's far column is in the lattice) and correct
                   // ledge filtering at every tile border (an out-of-lattice neighbour reads as
                   // support). Neither failure is visible in the output — the field just reads
                   // blocked at every seam.
                   InParams._MaxClearanceUu > 0.0f &&
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

        /** A stub plus where in the halo lattice it came from, so its plate can be filled in later. */
        struct FStubSite
        {
            FCk_GroundNav_SeamStub _Stub;

            int32 _HaloX = 0;
            int32 _HaloY = 0;
            int32 _Layer = 0;
        };

        auto Get_EdgeCell(
            int32 InDirection,
            int32 InAlong,
            int32 InTileCells) -> FIntPoint
        {
            const auto Last = InTileCells - 1;

            switch (InDirection)
            {
                case 0:  return FIntPoint{Last, InAlong};
                case 1:  return FIntPoint{InAlong, Last};
                case 2:  return FIntPoint{0, InAlong};
                default: return FIntPoint{InAlong, 0};
            }
        }

        /**
         * Record what the halo knows about every crossing that leaves the tile.
         *
         * Runs BEFORE the halo is masked away, because that is the only moment the neighbouring ground
         * and the connection field that reaches it both exist. What survives is a description of the
         * crossing, not a reference into anything: two tiles are composed later by comparing
         * descriptions, never by re-deriving adjacency.
         */
        auto Do_CollectSeamStubs(
            const FCk_GroundNav_SpanField&       InSpans,
            const FCk_GroundNav_LayerField&      InLayers,
            const FCk_GroundNav_ConnectionField& InConnections,
            const FCk_GroundNav_ClearanceField&  InClearance,
            int32                                InHaloCells,
            int32                                InTileCells,
            float                                InMaxClearanceUu,
            TArray<FStubSite>&                   OutSites) -> void
        {
            OutSites.Reset();

            for (auto Direction = 0; Direction < kDirectionCount; ++Direction)
            {
                const auto Offset = Get_DirectionOffset(Direction);

                for (auto Along = 0; Along < InTileCells; ++Along)
                {
                    const auto EdgeCell = Get_EdgeCell(Direction, Along, InTileCells);

                    const auto HaloX = EdgeCell.X + InHaloCells;
                    const auto HaloY = EdgeCell.Y + InHaloCells;

                    const auto FarX = HaloX + Offset.X;
                    const auto FarY = HaloY + Offset.Y;

                    if (NOT InSpans.Get_IsValidColumn(FarX, FarY))
                    { continue; }

                    const auto& SpanColumn = InSpans.Get_Column(HaloX, HaloY);
                    const auto& LayerColumn = InLayers.Get_Column(HaloX, HaloY);
                    const auto& ConnectionColumn = InConnections.Get_Column(HaloX, HaloY);

                    const auto& FarSpanColumn = InSpans.Get_Column(FarX, FarY);
                    const auto& FarLayerColumn = InLayers.Get_Column(FarX, FarY);

                    for (auto SpanIndex = 0; SpanIndex < SpanColumn.Num(); ++SpanIndex)
                    {
                        const auto Layer = LayerColumn[SpanIndex];

                        if (Layer == FCk_GroundNav_LayerField::kNoLayer)
                        { continue; }

                        const auto FarSpanIndex = ConnectionColumn[SpanIndex]._Neighbours[Direction];

                        if (FarSpanIndex == FCk_GroundNav_SpanConnections::kNoConnection)
                        { continue; }

                        const auto FarLayer = FarLayerColumn[FarSpanIndex];

                        if (FarLayer == FCk_GroundNav_LayerField::kNoLayer)
                        { continue; }

                        auto Site = FStubSite{};

                        Site._HaloX = HaloX;
                        Site._HaloY = HaloY;
                        Site._Layer = Layer;

                        Site._Stub._Direction = Direction;
                        Site._Stub._AlongIndex = Along;
                        Site._Stub._NearSurfaceZUu = SpanColumn[SpanIndex]._MaxZ;
                        Site._Stub._FarSurfaceZUu = FarSpanColumn[FarSpanIndex]._MaxZ;
                        Site._Stub._ClearanceUu = FMath::Min(
                            FMath::Min(
                                InClearance.Get_ClearanceAt(HaloX, HaloY, Layer),
                                InClearance.Get_ClearanceAt(FarX, FarY, FarLayer)),
                            InMaxClearanceUu);

                        OutSites.Emplace(Site);
                    }
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

        auto StubSites = TArray<FStubSite>{};
        Do_CollectSeamStubs(
            Spans, Layers, Connections, HaloClearance, HaloCells, TileCells,
            InParams._MaxClearanceUu, StubSites);

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

        OutTile._SeamStubs.Reserve(StubSites.Num());

        for (auto& Site : StubSites)
        {
            const auto PlateIndex = Plates.Get_PlateIndexAt(Site._HaloX, Site._HaloY, Site._Layer);

            if (PlateIndex == FCk_GroundNav_Plate::kNoPlate)
            { continue; }

            Site._Stub._PlateIndex = PlateIndex;

            OutTile._SeamStubs.Emplace(Site._Stub);
        }

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);
        Result.Set_ProbesSpent(ProbesSpent);
        Result.Set_DroppedInputCount(RasterResult.Get_DroppedInputCount());

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
