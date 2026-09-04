#include "CkGroundNav_FieldMarkupCost.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkGroundNav/Bake/CkGroundNav_MarkupMask.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace fieldmarkupcost_private
    {
        auto Get_PlatePoliciesEqual(
            const FCk_GroundNav_PlateField& InLhs,
            const FCk_GroundNav_PlateField& InRhs) -> bool
        {
            if (InLhs._Plates.Num() != InRhs._Plates.Num() ||
                InLhs._AreaPolicies.Num() != InRhs._AreaPolicies.Num())
            { return false; }

            for (auto Index = 0; Index < InLhs._AreaPolicies.Num(); ++Index)
            {
                if (InLhs._AreaPolicies[Index] != InRhs._AreaPolicies[Index])
                { return false; }
            }

            for (auto Index = 0; Index < InLhs._Plates.Num(); ++Index)
            {
                if (InLhs._Plates[Index]._AreaPolicyIndex != InRhs._Plates[Index]._AreaPolicyIndex ||
                    InLhs._Plates[Index]._CostMultiplier != InRhs._Plates[Index]._CostMultiplier)
                { return false; }
            }

            return true;
        }

        auto Get_AnyUnobservedRecordReachesTile(
            const FCk_GroundNav_FieldParams&            InParams,
            const FCk_GroundNav_Tile&                   InTile,
            TConstArrayView<FCk_GroundNav_MarkupRecord> InMarkups) -> bool
        {
            const auto TileBounds = Get_TileWorldBounds(InParams, InTile);

            return ck::algo::AnyOf(InMarkups,
                [&](const FCk_GroundNav_MarkupRecord& InMarkup) -> bool
                {
                    if (InMarkup.Get_Enable() == ECk_EnableDisable::Disable)
                    { return false; }

                    if (InMarkup.Get_RequestedAtEpoch() < InTile._Epoch._Value)
                    { return false; }

                    const auto MarkupBounds = Get_MarkupWorldBounds(InMarkup);

                    return MarkupBounds.IsValid != 0 && TileBounds.Intersect(MarkupBounds);
                });
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_TileWorldBounds(
            const FCk_GroundNav_FieldParams& InParams,
            const FCk_GroundNav_Tile&        InTile)
        -> FBox
    {
        const auto CellSize = static_cast<double>(InTile._CellSizeUu);

        const auto SpanX = static_cast<double>(InTile._SizeX) * CellSize;
        const auto SpanY = static_cast<double>(InTile._SizeY) * CellSize;

        return FBox{
            FVector{InTile._Origin.X, InTile._Origin.Y, static_cast<double>(InParams._MinZUu)},
            FVector{InTile._Origin.X + SpanX, InTile._Origin.Y + SpanY,
                    static_cast<double>(InParams._MaxZUu)}};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ChangedTileBounds(
            const FCk_GroundNav_Field& InField,
            const FCk_GroundNav_Epoch& InEpoch)
        -> FBox
    {
        auto Bounds = FBox{ForceInit};

        for (const auto& Tile : InField._Tiles)
        {
            if (NOT Tile.Get_IsBuilt() || Tile._Epoch != InEpoch)
            { continue; }

            Bounds += Get_TileWorldBounds(InField._Params, Tile);
        }

        return Bounds;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_FieldWithMarkupCost(
            const FCk_GroundNav_Field&                  InField,
            TConstArrayView<FCk_GroundNav_MarkupRecord> InMarkups,
            const FCk_GroundNav_Epoch&                  InEpoch)
        -> TPair<FCk_GroundNav_FieldPtr, FCk_GroundNav_BakeStageResult>
    {
        using namespace fieldmarkupcost_private;

        auto Result = FCk_GroundNav_BakeStageResult{};
        auto Derived = MakeShared<FCk_GroundNav_Field>(InField);

        Derived->_Params._MarkupRecords = InMarkups;

        auto ChangedAnyTile = false;

        for (auto TileIndex = 0; TileIndex < Derived->_Tiles.Num(); ++TileIndex)
        {
            auto& Tile = Derived->_Tiles[TileIndex];

            if (NOT Tile.Get_IsBuilt())
            { continue; }

            auto Lattice = FCk_GroundNav_PlateLattice{};
            Lattice._OriginXY = FVector2D{Tile._Origin.X, Tile._Origin.Y};
            Lattice._CellSizeUu = Tile._CellSizeUu;
            Lattice._SizeX = Tile._SizeX;
            Lattice._SizeY = Tile._SizeY;
            Lattice._LayerCount = Tile._LayerCount;
            Lattice._SurfaceZ = Tile._SurfaceZ;

            Stamp_PlateCostPolicies(Lattice, InMarkups, Tile._Plates);

            const auto PoliciesChanged = NOT Get_PlatePoliciesEqual(
                InField._Tiles[TileIndex]._Plates, Tile._Plates);

            const auto AnswersAnUnobservedRecord = Get_AnyUnobservedRecordReachesTile(
                Derived->_Params, Tile, InMarkups);

            if (NOT PoliciesChanged && NOT AnswersAnUnobservedRecord)
            { continue; }

            Tile._Epoch = InEpoch;
            ChangedAnyTile = true;
        }

        if (ChangedAnyTile)
        { Derived->_Epoch = InEpoch; }

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);

        return TPair<FCk_GroundNav_FieldPtr, FCk_GroundNav_BakeStageResult>{Derived, Result};
    }
}

// --------------------------------------------------------------------------------------------------------------------
