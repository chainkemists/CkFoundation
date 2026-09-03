#include "CkGroundNav_Query_Attributes.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        Get_SurfaceAttributes(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_SurfaceRef& InSurface)
        -> FCk_GroundNav_SurfaceAttributes
    {
        auto Attributes = FCk_GroundNav_SurfaceAttributes{};

        if (NOT InSurface.Get_IsValid() || NOT InField._Tiles.IsValidIndex(InSurface._TileIndex))
        { return Attributes; }

        if (Get_TileStatus(InField, InSurface._TileIndex) != ECk_GroundNav_BuildStatus::Built)
        {
            Attributes._Status = ECk_NavSurface_QueryStatus::Unbuilt;
            return Attributes;
        }

        const auto Address = FCk_GroundNav_CellAddress{
            InSurface._TileIndex, InSurface._CellX, InSurface._CellY};

        auto ResolvedSurface = FCk_GroundNav_SurfaceRef{};
        auto SurfaceZUu = 0.0f;
        auto ClearanceUu = 0.0f;

        if (NOT Get_SurfaceAt(InField, Address, InSurface._LayerIndex, ResolvedSurface, SurfaceZUu, ClearanceUu))
        { return Attributes; }

        if (ResolvedSurface._PlateIndex != InSurface._PlateIndex)
        { return Attributes; }

        const auto& PlateField = InField._Tiles[InSurface._TileIndex]._Plates;

        if (NOT PlateField._Plates.IsValidIndex(ResolvedSurface._PlateIndex))
        { return Attributes; }

        const auto& Plate = PlateField._Plates[ResolvedSurface._PlateIndex];

        Attributes._Status = ECk_NavSurface_QueryStatus::Success;
        Attributes._Surface = ResolvedSurface;
        Attributes._SurfaceNormal = Get_SurfaceNormal(InField, ResolvedSurface);
        Attributes._ClearanceUu = ClearanceUu;

// Policy is a PLATE label, so the answer is the plate's own and never a per-cell lookup. An

// unpriced plate carries INDEX_NONE and the identity multiplier.
        Attributes._CostMultiplier = Plate._CostMultiplier;
        Attributes._AreaTags = PlateField.Get_AreaPolicy(Plate._AreaPolicyIndex);

        return Attributes;
    }

    auto
        Get_SurfaceAttributesAt(
            const FCk_GroundNav_Field&            InField,
            const FCk_GroundNav_IsNavigableQuery& InQuery)
        -> FCk_GroundNav_SurfaceAttributes
    {
        const auto Found = Get_IsNavigable(InField, InQuery);

        if (NOT Found.Get_IsSuccess())
        {
            auto Attributes = FCk_GroundNav_SurfaceAttributes{};
            Attributes._Status = Found._Status;

            return Attributes;
        }

        return Get_SurfaceAttributes(InField, Found._Surface);
    }

    auto
        Get_SurfaceAttributesAt_Batch(
            const FCk_GroundNav_Field&                      InField,
            TConstArrayView<FCk_GroundNav_IsNavigableQuery> InQueries,
            TArrayView<FCk_GroundNav_SurfaceAttributes>     OutResults)
        -> void
    {
        const auto StorageIsLargeEnough = OutResults.Num() >= InQueries.Num();

        CK_ENSURE_IF_NOT(StorageIsLargeEnough,
            TEXT("Attribute batch was given [{}] result slots for [{}] queries"),
            OutResults.Num(), InQueries.Num())
        { return; }

        for (auto Index = 0; Index < InQueries.Num(); ++Index)
        { OutResults[Index] = Get_SurfaceAttributesAt(InField, InQueries[Index]); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
