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

    // Cells already held by THIS object (a re-validate / move-in-place) are not a collision.
    const auto SelfOccupant = FCk_Handle{InObject};

    auto FailedCells = TArray<FIntPoint>{};
    auto Reason = ECk_2dGridPlacement_Failure::None;

    const auto RecordFailure = [&](const FIntPoint& InCoord, ECk_2dGridPlacement_Failure InReason)
    {
        FailedCells.Add(InCoord);
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

        // Must be the pending-destroy-aware occupant query, NOT the raw Occupied tag: the tag lags
        // one reconcile tick and would falsely reject a same-tick remove+re-place.
        const auto OccupantThere = UCk_Utils_2dGridOccupancy_UE::Get_OccupantAt(InGrid, Coord);
        if (ck::IsValid(OccupantThere) && OccupantThere != SelfOccupant)
        {
            RecordFailure(Coord, ECk_2dGridPlacement_Failure::Occupied);
            continue;
        }

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

    // A multi-cell footprint must form one 4-connected component; disabled / out-of-bounds cells
    // are walls, occupied cells are passable (occupancy is not a topological cut).
    if (InConnectivity == ECk_GridConnectivity::RequireConnected && Cells.Num() > 1)
    {
        const auto IsWall = [&](const FIntPoint& InCoord) -> bool
        {
            const auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(InGrid, InCoord);
            return ck::Is_NOT_Valid(Cell) || UCk_Utils_2dGridCell_UE::Get_IsDisabled(Cell);
        };

        auto FootprintSet = TSet<FIntPoint>{};
        FootprintSet.Reserve(Cells.Num());
        for (const auto& Coord : Cells)
        { FootprintSet.Add(Coord); }

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
                    if (FootprintSet.Contains(Next) && NOT Reached.Contains(Next) && NOT IsWall(Next))
                    {
                        Reached.Add(Next);
                        Frontier.Add(Next);
                    }
                }
            }
        }

        // Wall cells already produced their own Disabled / OutOfBounds failure above — skipping
        // them here avoids a duplicate, mis-reasoned entry in _FailedCells.
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
        ECk_CardinalRotation InRotation,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_2dGridPlacement
{
    const auto IsGridValid = ck::IsValid(InGrid);
    CK_ENSURE_IF_NOT(IsGridValid,
        TEXT("Request_Place: grid handle is invalid"))
    {
        InDelegate.ExecuteIfBound(InGrid, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    const auto IsOccupantValid = ck::IsValid(InOccupant);
    CK_ENSURE_IF_NOT(IsOccupantValid,
        TEXT("Request_Place: occupant handle is invalid"))
    {
        InDelegate.ExecuteIfBound(InGrid, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    const auto Object = UCk_Utils_2dGridObject_UE::Cast(InOccupant);
    const auto IsObjectValid = ck::IsValid(Object);
    CK_ENSURE_IF_NOT(IsObjectValid,
        TEXT("Request_Place: occupant [{}] is not a 2dGridObject"), InOccupant)
    {
        InDelegate.ExecuteIfBound(InGrid, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    const auto Result = Get_CanPlace(InGrid, Object, InAnchor, InRotation, ECk_GridConnectivity::Ignore);
    if (NOT Result.Get_CanPlace())
    {
        InDelegate.ExecuteIfBound(InGrid, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    // Authority-gate the mutation: only the authority stamps occupancy. The validity query above
    // is side-effect free, so non-authority callers can still probe Get_CanPlace.
    if (NOT UCk_Utils_Net_UE::Get_HasAuthority(FCk_Handle{InGrid}))
    {
        InDelegate.ExecuteIfBound(InGrid, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    const auto Cells = UCk_Utils_2dGridObject_UE::Get_ResolvedCells(Object, InAnchor, InRotation);

    // Auto-replace: free this occupant's existing placement first so a re-place at a different
    // anchor/rotation reconciles cleanly instead of leaving the old footprint stamped.
    if (auto Existing = UCk_Utils_2dGridOccupancy_UE::Get_PlacementForOccupant(InOccupant);
        ck::IsValid(Existing))
    {
        UCk_Utils_2dGridOccupancy_UE::Request_RemovePlacement(Existing, {});
    }

    auto Placement = UCk_Utils_2dGridOccupancy_UE::Request_AddPlacement(
        InGrid, InOccupant, InAnchor, InRotation, Cells, {});

    ck::UUtils_Signal_2dGridPlacement_ObjectPlaced::Broadcast(
        InGrid, ck::MakePayload(InGrid, InOccupant, Cells));

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InGrid, ECk_Request_OperationResult::Succeeded);

    return Placement;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridPlacement_UE::
    Request_Remove(
        FCk_Handle_2dGridPlacement& InPlacement,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> bool
{
    if (ck::Is_NOT_Valid(InPlacement))
    {
        InDelegate.ExecuteIfBound(FCk_Handle{InPlacement}, ECk_Request_OperationResult::Failed_NotEnqueued);
        return false;
    }

    const auto& Params   = InPlacement.Get<ck::FFragment_2dGridPlacement_Params>();
    auto        Grid     = Params.Get_Grid();
    const auto  Occupant = Params.Get_Occupant();

    const auto Removed = UCk_Utils_2dGridOccupancy_UE::Request_RemovePlacement(InPlacement, {});
    if (NOT Removed)
    {
        InDelegate.ExecuteIfBound(FCk_Handle{InPlacement}, ECk_Request_OperationResult::Failed);
        return false;
    }

    if (ck::IsValid(Grid))
    {
        ck::UUtils_Signal_2dGridPlacement_ObjectRemoved::Broadcast(
            Grid, ck::MakePayload(Grid, Occupant));
    }

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(FCk_Handle{InPlacement}, ECk_Request_OperationResult::Succeeded);

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
