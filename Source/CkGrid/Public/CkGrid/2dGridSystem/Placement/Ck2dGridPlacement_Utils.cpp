#include "Ck2dGridPlacement_Utils.h"

#include "Ck2dGridPlacement_Fragment.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Object/Ck2dGridObject_Utils.h"
#include "CkGrid/2dGridSystem/Occupancy/Ck2dGridOccupancy_Utils.h"

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

        if (UCk_Utils_2dGridOccupancy_UE::Get_IsOccupied(InGrid, Coord))
        {
            const auto OccupantThere = UCk_Utils_2dGridOccupancy_UE::Get_OccupantAt(InGrid, Coord);
            if (OccupantThere != SelfOccupant)
            {
                RecordFailure(Coord, ECk_2dGridPlacement_Failure::Occupied);
                continue;
            }
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

    // TODO(Task: GridConnectivity): RequireConnected adjacency is implemented in a later task.
    // For now the connectivity parameter is accepted but ignored.
    (void)InConnectivity;

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
    Get_OccupantAt(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InCoordinate)
    -> FCk_Handle
{
    return UCk_Utils_2dGridOccupancy_UE::Get_OccupantAt(InGrid, InCoordinate);
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
