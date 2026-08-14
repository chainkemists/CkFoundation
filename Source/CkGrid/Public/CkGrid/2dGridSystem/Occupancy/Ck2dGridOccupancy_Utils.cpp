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
        const TArray<FIntPoint>& InCells,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_2dGridPlacement
{
    const auto IsGridValid = ck::IsValid(InGrid);
    CK_ENSURE_IF_NOT(IsGridValid,
        TEXT("Request_AddPlacement: grid handle is invalid"))
    {
        InDelegate.ExecuteIfBound(FCk_Handle{InGrid}, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    auto GridBase = FCk_Handle{InGrid};

    ck::RecordOf_GridPlacements_Utils::AddIfMissing(GridBase);
    InGrid.AddOrGet<ck::FFragment_2dGridOccupancy_Current>();

    // Safe on clients too — the client sync path reuses this helper, and TryAddContainerFragment
    // plus the Replicate processor are themselves host/authority gated.
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_2dGridPlacements>(GridBase);
    InGrid.AddOrGet<ck::FTag_2dGridOccupancy_MayRequireReplication>();

    // The GRID is the placement's lifetime owner, so the placement dies with the grid.
    auto Params = ck::FFragment_2dGridPlacement_Params{InOccupant, InGrid, InAnchor, InCells};
    Params.Set_Rotation(InRotation);

    auto PlacementBase = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(GridBase);
    PlacementBase.Add<ck::FFragment_2dGridPlacement_Params>(Params);

    auto Placement = ck::StaticCast<FCk_Handle_2dGridPlacement>(PlacementBase);

    ck::RecordOf_GridPlacements_Utils::Request_Connect(
        GridBase, Placement, ECk_Record_LabelRequirementPolicy::Optional);

    // Bind the occupant death-watch at most ONCE: re-binding the same delegate signature trips
    // CkEnsure (ContainsDelegateWithSignature). The handler always reads the CURRENT back-ref, so
    // a re-place just overwrites _Placement; the ref fragment is added in lockstep with the watch.
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

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(GridBase, ECk_Request_OperationResult::Succeeded);

    return Placement;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridOccupancy_UE::
    Request_RemovePlacement(
        FCk_Handle_2dGridPlacement& InPlacement,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> bool
{
    if (ck::Is_NOT_Valid(InPlacement))
    {
        InDelegate.ExecuteIfBound(FCk_Handle{InPlacement}, ECk_Request_OperationResult::Failed_NotEnqueued);
        return false;
    }

    // Read the grid BEFORE destroying the placement — its params die with the entity.
    if (auto Grid = InPlacement.Get<ck::FFragment_2dGridPlacement_Params>().Get_Grid();
        ck::IsValid(Grid))
    {
        Grid.AddOrGet<ck::FTag_2dGridOccupancy_MayRequireReplication>();
    }

    auto PlacementBase = FCk_Handle{InPlacement};
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PlacementBase);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(PlacementBase, ECk_Request_OperationResult::Succeeded);

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

    // A cell's stamp still points at a dying placement until the next reconcile tick; treat a
    // pending-destroy placement as gone so a same-tick remove+re-place reads the cell as free
    // (see "Occupancy reconcile" in CkGrid/Claude.md).
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
