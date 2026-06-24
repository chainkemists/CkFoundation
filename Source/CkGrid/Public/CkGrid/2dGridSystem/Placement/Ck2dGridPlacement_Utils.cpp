#include "Ck2dGridPlacement_Utils.h"

#include "Ck2dGridPlacement_Fragment.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkGrid/CkGrid_Stats.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Object/Ck2dGridObject_Utils.h"
#include "CkGrid/2dGridSystem/Occupancy/Ck2dGridOccupancy_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Grid::FirstAvailable"), STAT_Grid_FirstAvailable, STATGROUP_CkGrid);
DECLARE_CYCLE_STAT(TEXT("Grid::CanPlace"),       STAT_Grid_CanPlace,       STATGROUP_CkGrid);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridPlacement_UE::
    Get_CanPlace(
        const FCk_Handle_2dGridSystem& InGrid,
        const FCk_Handle_2dGridObject& InObject,
        const FIntPoint& InAnchor,
        ECk_CardinalRotation InRotation,
        ECk_GridConnectivity InConnectivity)
    -> FCk_2dGridPlacement_Result
{
    SCOPE_CYCLE_COUNTER(STAT_Grid_CanPlace);

    CK_ENSURE_IF_NOT(ck::IsValid(InGrid),
        TEXT("Get_CanPlace: grid handle is invalid"))
    { return FCk_2dGridPlacement_Result::Failure(ECk_2dGridPlacement_Failure::OutOfBounds, {}); }

    CK_ENSURE_IF_NOT(ck::IsValid(InObject),
        TEXT("Get_CanPlace: object handle is invalid"))
    { return FCk_2dGridPlacement_Result::Failure(ECk_2dGridPlacement_Failure::OutOfBounds, {}); }

    const auto Cells = UCk_Utils_2dGridObject_UE::Get_ResolvedCells(InObject, InAnchor, InRotation);

    const auto& GridParams = InGrid.Get<ck::FFragment_2dGridSystem_Params>();
    const auto& DefaultCellTags = GridParams.Get_DefaultCellTags();

    const auto RequiredTags  = UCk_Utils_2dGridObject_UE::Get_RequiredCellTags(InObject);
    const auto ForbiddenTags = UCk_Utils_2dGridObject_UE::Get_ForbiddenCellTags(InObject);

    // The GridObject whose placement we are testing — cells already occupied by THIS object
    // (e.g. a re-validate / move-in-place) are not treated as a collision.
    const auto SelfOccupant = FCk_Handle{InObject};

    auto FailedCells = TArray<FIntPoint>{};
    auto Reason = ECk_2dGridPlacement_Failure::None;

    const auto RecordFailure = [&](const FIntPoint& InCoord, ECk_2dGridPlacement_Failure InReason)
    {
        FailedCells.Add(InCoord);
        // First failure wins as the headline reason (we still collect every failing cell).
        if (Reason == ECk_2dGridPlacement_Failure::None)
        { Reason = InReason; }
    };

    for (const auto& Coord : Cells)
    {
        const auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(InGrid, Coord);
        if (ck::Is_NOT_Valid(Cell))
        {
            RecordFailure(Coord, ECk_2dGridPlacement_Failure::OutOfBounds);
            continue;
        }

        if (UCk_Utils_2dGridCell_UE::Get_IsDisabled(Cell))
        {
            RecordFailure(Coord, ECk_2dGridPlacement_Failure::Disabled);
            continue;
        }

        // Occupied check via the pending-destroy-aware occupant query — NOT the raw Occupied tag,
        // which lags one reconcile tick and would falsely reject a same-tick remove+re-place (the
        // dying placement's stamp lingers a tick). A cell counts as occupied only if a VALID
        // occupant other than this object holds it.
        const auto OccupantThere = UCk_Utils_2dGridOccupancy_UE::Get_OccupantAt(InGrid, Coord);
        if (ck::IsValid(OccupantThere) && OccupantThere != SelfOccupant)
        {
            RecordFailure(Coord, ECk_2dGridPlacement_Failure::Occupied);
            continue;
        }

        // Tag-gating: the effective tag set for a cell is the grid's default tags unioned with
        // the cell's own tags.
        auto EffectiveTags = DefaultCellTags;
        EffectiveTags.AppendTags(UCk_Utils_2dGridCell_UE::Get_Tags(Cell));

        if (RequiredTags.Num() > 0 && NOT EffectiveTags.HasAll(RequiredTags))
        {
            RecordFailure(Coord, ECk_2dGridPlacement_Failure::TagMismatch);
            continue;
        }

        if (ForbiddenTags.Num() > 0 && EffectiveTags.HasAny(ForbiddenTags))
        {
            RecordFailure(Coord, ECk_2dGridPlacement_Failure::TagMismatch);
            continue;
        }
    }

    // Connectivity: a multi-cell footprint must form a single 4-connected component, treating
    // disabled / out-of-bounds footprint cells as walls (impassable). Occupied cells are passable
    // (occupancy is a soft constraint for connectivity, not a topological cut). A single-cell
    // footprint is trivially connected, so there is nothing to validate there.
    if (InConnectivity == ECk_GridConnectivity::RequireConnected && Cells.Num() > 1)
    {
        const auto IsWall = [&](const FIntPoint& InCoord) -> bool
        {
            const auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(InGrid, InCoord);
            // Out-of-bounds (invalid cell) and disabled cells are both walls.
            return ck::Is_NOT_Valid(Cell) || UCk_Utils_2dGridCell_UE::Get_IsDisabled(Cell);
        };

        auto FootprintSet = TSet<FIntPoint>{};
        FootprintSet.Reserve(Cells.Num());
        for (const auto& Coord : Cells)
        { FootprintSet.Add(Coord); }

        // Start the flood from the anchor cell if it is part of the footprint and not a wall;
        // otherwise from the first non-wall footprint cell.
        auto StartCoord = FIntPoint{};
        auto FoundStart = false;
        if (FootprintSet.Contains(InAnchor) && NOT IsWall(InAnchor))
        {
            StartCoord = InAnchor;
            FoundStart = true;
        }
        else
        {
            for (const auto& Coord : Cells)
            {
                if (NOT IsWall(Coord))
                {
                    StartCoord = Coord;
                    FoundStart = true;
                    break;
                }
            }
        }

        auto Reached = TSet<FIntPoint>{};
        if (FoundStart)
        {
            Reached.Reserve(Cells.Num());

            auto Frontier = TArray<FIntPoint>{};
            Frontier.Add(StartCoord);
            Reached.Add(StartCoord);

            const FIntPoint Neighbours[] =
            {
                FIntPoint{ 1,  0},
                FIntPoint{-1,  0},
                FIntPoint{ 0,  1},
                FIntPoint{ 0, -1}
            };

            while (NOT Frontier.IsEmpty())
            {
                const auto Current = Frontier.Pop(EAllowShrinking::No);
                for (const auto& Delta : Neighbours)
                {
                    const auto Next = Current + Delta;
                    // Only traverse to footprint cells that are not walls and not yet reached.
                    if (FootprintSet.Contains(Next) && NOT Reached.Contains(Next) && NOT IsWall(Next))
                    {
                        Reached.Add(Next);
                        Frontier.Add(Next);
                    }
                }
            }
        }

        // Any non-wall footprint cell the flood did not reach is disconnected from the start
        // component. Wall cells are unreachable by definition AND already produced their own
        // per-cell failure above (Disabled / OutOfBounds), so we skip them here to avoid a
        // duplicate / mis-reasoned entry in _FailedCells.
        for (const auto& Coord : Cells)
        {
            if (NOT Reached.Contains(Coord) && NOT IsWall(Coord))
            { RecordFailure(Coord, ECk_2dGridPlacement_Failure::Disconnected); }
        }
    }

    if (NOT FailedCells.IsEmpty())
    { return FCk_2dGridPlacement_Result::Failure(Reason, MoveTemp(FailedCells)); }

    return FCk_2dGridPlacement_Result::Success();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridPlacement_UE::
    Get_FirstAvailablePosition(
        const FCk_Handle_2dGridSystem& InGrid,
        const FCk_Handle_2dGridObject& InObject,
        ECk_CardinalRotation InRotation)
    -> FIntPoint
{
    SCOPE_CYCLE_COUNTER(STAT_Grid_FirstAvailable);

    if (ck::Is_NOT_Valid(InGrid) || ck::Is_NOT_Valid(InObject))
    { return FIntPoint(-1, -1); }

    const auto Dimensions = UCk_Utils_2dGridSystem_UE::Get_Dimensions(InGrid);

    for (auto Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < Dimensions.X; ++X)
        {
            const auto Anchor = FIntPoint(X, Y);
            const auto Result = Get_CanPlace(InGrid, InObject, Anchor, InRotation, ECk_GridConnectivity::Ignore);
            if (Result.Get_CanPlace())
            { return Anchor; }
        }
    }

    return FIntPoint(-1, -1);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridPlacement_UE::
    Request_Place(
        FCk_Handle_2dGridSystem& InGrid,
        FCk_Handle& InOccupant,
        const FIntPoint& InAnchor,
        ECk_CardinalRotation InRotation)
    -> FCk_Handle_2dGridPlacement
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGrid),
        TEXT("Request_Place: grid handle is invalid"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InOccupant),
        TEXT("Request_Place: occupant handle is invalid"))
    { return {}; }

    const auto Object = UCk_Utils_2dGridObject_UE::Cast(InOccupant);
    CK_ENSURE_IF_NOT(ck::IsValid(Object),
        TEXT("Request_Place: occupant [{}] is not a 2dGridObject"), InOccupant)
    { return {}; }

    const auto Result = Get_CanPlace(InGrid, Object, InAnchor, InRotation, ECk_GridConnectivity::Ignore);
    if (NOT Result.Get_CanPlace())
    { return {}; }

    // Authority-gate the mutation: only the authority stamps occupancy. The validity query above
    // is side-effect free, so non-authority callers can still probe Get_CanPlace.
    if (NOT UCk_Utils_Net_UE::Get_HasAuthority(FCk_Handle{InGrid}))
    { return {}; }

    const auto Cells = UCk_Utils_2dGridObject_UE::Get_ResolvedCells(Object, InAnchor, InRotation);

    // Auto-replace: if this occupant already holds a placement, remove it first so its old cells
    // free before we add the new one. Lets a re-place at a different anchor/rotation reconcile
    // cleanly instead of leaving the old footprint stamped. Get_CanPlace above already treats the
    // occupant's own cells as non-colliding (SelfOccupant), so an in-place move still validates.
    if (auto Existing = UCk_Utils_2dGridOccupancy_UE::Get_PlacementForOccupant(InOccupant);
        ck::IsValid(Existing))
    {
        UCk_Utils_2dGridOccupancy_UE::Request_RemovePlacement(Existing);
    }

    auto Placement = UCk_Utils_2dGridOccupancy_UE::Request_AddPlacement(
        InGrid, InOccupant, InAnchor, InRotation, Cells);

    ck::UUtils_Signal_2dGridPlacement_ObjectPlaced::Broadcast(
        InGrid, ck::MakePayload(InGrid, InOccupant, Cells));

    return Placement;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridPlacement_UE::
    Request_Remove(
        FCk_Handle_2dGridPlacement& InPlacement)
    -> bool
{
    if (ck::Is_NOT_Valid(InPlacement))
    { return false; }

    const auto& Params   = InPlacement.Get<ck::FFragment_2dGridPlacement_Params>();
    auto        Grid     = Params.Get_Grid();
    const auto  Occupant = Params.Get_Occupant();

    const auto Removed = UCk_Utils_2dGridOccupancy_UE::Request_RemovePlacement(InPlacement);
    if (NOT Removed)
    { return false; }

    if (ck::IsValid(Grid))
    {
        ck::UUtils_Signal_2dGridPlacement_ObjectRemoved::Broadcast(
            Grid, ck::MakePayload(Grid, Occupant));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridPlacement_UE::
    BindTo_OnObjectPlaced(
        FCk_Handle_2dGridSystem& InGrid,
        const FCk_Delegate_2dGridPlacement_ObjectPlaced& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_2dGridSystem
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_2dGridPlacement_ObjectPlaced, InGrid, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InGrid;
}

auto
    UCk_Utils_2dGridPlacement_UE::
    UnbindFrom_OnObjectPlaced(
        FCk_Handle_2dGridSystem& InGrid,
        const FCk_Delegate_2dGridPlacement_ObjectPlaced& InDelegate)
    -> FCk_Handle_2dGridSystem
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_2dGridPlacement_ObjectPlaced, InGrid, InDelegate);
    return InGrid;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridPlacement_UE::
    BindTo_OnObjectRemoved(
        FCk_Handle_2dGridSystem& InGrid,
        const FCk_Delegate_2dGridPlacement_ObjectRemoved& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_2dGridSystem
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_2dGridPlacement_ObjectRemoved, InGrid, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InGrid;
}

auto
    UCk_Utils_2dGridPlacement_UE::
    UnbindFrom_OnObjectRemoved(
        FCk_Handle_2dGridSystem& InGrid,
        const FCk_Delegate_2dGridPlacement_ObjectRemoved& InDelegate)
    -> FCk_Handle_2dGridSystem
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_2dGridPlacement_ObjectRemoved, InGrid, InDelegate);
    return InGrid;
}

// --------------------------------------------------------------------------------------------------------------------
