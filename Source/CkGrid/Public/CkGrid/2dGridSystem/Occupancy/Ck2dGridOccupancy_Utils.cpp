#include "Ck2dGridOccupancy_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

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

    // Ensure the replicated container exists on the grid (no-op off-host / non-replicating /
    // already-present) and flag the grid for the authority-only Replicate pass to rebuild + push
    // the RepData. Both are safe to call on clients too (the client-apply processor reuses this
    // helper): TryAddContainerFragment / the Replicate processor are host/authority gated.
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_2dGridPlacements>(GridBase);
    InGrid.AddOrGet<ck::FTag_2dGridOccupancy_MayRequireReplication>();

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
    // The watch is bound at most ONCE per occupant — the handler always reads the CURRENT back-ref,
    // so an auto-replace re-place just overwrites _Placement; re-binding would double-bind the same
    // delegate signature and trip CkEnsure (ContainsDelegateWithSignature). The PlacementRef fragment
    // is added in lockstep with the watch, so its prior presence means the watch is already bound.
    if (ck::IsValid(InOccupant))
    {
        auto Occupant = InOccupant;
        const auto AlreadyWatched = Occupant.Has<ck::FFragment_2dGridOccupant_PlacementRef>();
        Occupant.AddOrGet<ck::FFragment_2dGridOccupant_PlacementRef>()._Placement = Placement;

        if (NOT AlreadyWatched)
        {
            auto OccupantWatch = FCk_Delegate_OnBeginDestroy{};
            OccupantWatch.BindUFunction(GetMutableDefault<UCk_Utils_2dGridOccupancy_UE>(),
                GET_FUNCTION_NAME_CHECKED(UCk_Utils_2dGridOccupancy_UE, OnOccupantBeginDestroy));
            UCk_Utils_EntityLifetime_UE::BindTo_OnBeginDestroy(Occupant, OccupantWatch);
        }
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

    // Flag the grid so the authority Replicate pass re-pushes the (now-shorter) placement set.
    // Read the grid BEFORE destroying the placement entity (its params go away with it).
    if (auto Grid = InPlacement.Get<ck::FFragment_2dGridPlacement_Params>().Get_Grid();
        ck::IsValid(Grid))
    {
        Grid.AddOrGet<ck::FTag_2dGridOccupancy_MayRequireReplication>();
    }

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

    auto Placement = Cell.Get<ck::FFragment_2dGridCell_Occupancy>().Get_Placement();
    if (ck::Is_NOT_Valid(Placement))
    { return {}; }

    // The Occupied tag + back-ref un-stamp one reconcile tick AFTER the placement entity is
    // destroyed. In the window between Request_RemovePlacement (which tags the placement
    // pending-destroy) and the next reconcile, the stale stamp still points at a dying placement.
    // Treat a pending-destroy placement as already gone so the cell reads free synchronously —
    // otherwise a same-tick remove+re-place sees the cell as still occupied and rejects the new
    // placement until the reconcile catches up.
    if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(
            FCk_Handle{Placement}, ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    { return {}; }

    return Placement;
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

auto
    UCk_Utils_2dGridOccupancy_UE::
    Get_PlacementForOccupant(
        const FCk_Handle& InOccupant)
    -> FCk_Handle_2dGridPlacement
{
    if (ck::Is_NOT_Valid(InOccupant))
    { return {}; }

    if (NOT InOccupant.Has<ck::FFragment_2dGridOccupant_PlacementRef>())
    { return {}; }

    return InOccupant.Get<ck::FFragment_2dGridOccupant_PlacementRef>().Get_Placement();
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
