#include "Ck2dGridOccupancy_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Placement/Ck2dGridPlacement_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridOccupancy_UE::
    Request_AddPlacement(
        FCk_Handle_2dGridSystem& InGrid,
        const FCk_Handle& InOccupant,
        const FIntPoint& InAnchor,
        ECk_CardinalRotation InRotation,
        const TArray<FIntPoint>& InCells)
    -> FCk_Handle_2dGridPlacement
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid),
        TEXT("Request_AddPlacement: grid handle is invalid"))
    { return {}; }

    auto GridBase = FCk_Handle{InGrid};

    // Ensure the grid carries the occupancy record + authoritative stamped-state. The
    // StampCells processor runs un-gated and reconciles from the record every tick.
    ck::RecordOf_GridPlacements_Utils::AddIfMissing(GridBase);
    InGrid.AddOrGet<ck::FFragment_2dGridOccupancy_Current>();

    // Create the placement entity with the GRID as its lifetime owner so it dies with the grid.
    // The generated constructor covers (Occupant, Grid, Anchor, Cells); set Rotation explicitly.
    auto Params = ck::FFragment_2dGridPlacement_Params{InOccupant, InGrid, InAnchor, InCells};
    Params.Set_Rotation(InRotation);

    auto PlacementBase = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(GridBase);
    PlacementBase.Add<ck::FFragment_2dGridPlacement_Params>(Params);

    auto Placement = ck::StaticCast<FCk_Handle_2dGridPlacement>(PlacementBase);

    // Connect the placement into the grid's record. Optional label requirement (no GameplayLabel).
    ck::RecordOf_GridPlacements_Utils::Request_Connect(
        GridBase, Placement, ECk_Record_LabelRequirementPolicy::Optional);

    // Death-watch the OCCUPANT: store a back-ref so the handler can find + destroy the placement.
    if (ck::IsValid(InOccupant))
    {
        auto Occupant = InOccupant;
        Occupant.AddOrGet<ck::FFragment_2dGridOccupant_PlacementRef>()._Placement = Placement;

        auto OccupantWatch = FCk_Delegate_OnBeginDestroy{};
        OccupantWatch.BindUFunction(GetMutableDefault<UCk_Utils_2dGridOccupancy_UE>(),
            GET_FUNCTION_NAME_CHECKED(UCk_Utils_2dGridOccupancy_UE, OnOccupantBeginDestroy));
        UCk_Utils_EntityLifetime_UE::BindTo_OnBeginDestroy(Occupant, OccupantWatch);
    }

    return Placement;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridOccupancy_UE::
    Request_RemovePlacement(
        FCk_Handle_2dGridPlacement& InPlacement)
    -> bool
{
    if (ck::Is_NOT_Valid(InPlacement))
    { return false; }

    // CkRecord's reverse-link prunes the now-dead record entry; the un-gated StampCells pass
    // then un-stamps its cells on the next tick.
    auto PlacementBase = FCk_Handle{InPlacement};
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PlacementBase);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridOccupancy_UE::
    Get_Placements(
        const FCk_Handle_2dGridSystem& InGrid)
    -> TArray<FCk_Handle_2dGridPlacement>
{
    if (ck::Is_NOT_Valid(InGrid))
    { return {}; }

    return ck::RecordOf_GridPlacements_Utils::Get_ValidEntries(FCk_Handle{InGrid});
}

auto
    UCk_Utils_2dGridOccupancy_UE::
    Get_IsOccupied(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InCoordinate)
    -> bool
{
    if (ck::Is_NOT_Valid(InGrid))
    { return false; }

    const auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(InGrid, InCoordinate);
    if (ck::Is_NOT_Valid(Cell))
    { return false; }

    return Cell.Has<ck::FTag_2dGridCell_Occupied>();
}

auto
    UCk_Utils_2dGridOccupancy_UE::
    Get_PlacementAt(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InCoordinate)
    -> FCk_Handle_2dGridPlacement
{
    if (ck::Is_NOT_Valid(InGrid))
    { return {}; }

    const auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(InGrid, InCoordinate);
    if (ck::Is_NOT_Valid(Cell))
    { return {}; }

    if (NOT Cell.Has<ck::FFragment_2dGridCell_Occupancy>())
    { return {}; }

    return Cell.Get<ck::FFragment_2dGridCell_Occupancy>().Get_Placement();
}

auto
    UCk_Utils_2dGridOccupancy_UE::
    Get_OccupantAt(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InCoordinate)
    -> FCk_Handle
{
    const auto Placement = Get_PlacementAt(InGrid, InCoordinate);
    if (ck::Is_NOT_Valid(Placement))
    { return {}; }

    return Placement.Get<ck::FFragment_2dGridPlacement_Params>().Get_Occupant();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridOccupancy_UE::
    OnOccupantBeginDestroy(
        FCk_Handle InHandle)
    -> void
{
    if (NOT InHandle.Has<ck::FFragment_2dGridOccupant_PlacementRef>())
    { return; }

    auto Placement = InHandle.Get<ck::FFragment_2dGridOccupant_PlacementRef>().Get_Placement();
    if (ck::Is_NOT_Valid(Placement))
    { return; }

    auto PlacementBase = FCk_Handle{Placement};
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PlacementBase);
}

// --------------------------------------------------------------------------------------------------------------------
