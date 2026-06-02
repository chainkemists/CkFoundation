#include "Ck2dGridBlocker_Utils.h"

#include "Ck2dGridBlocker_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridBlocker_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_2dGridBlocker_ParamsData& InParams)
    -> FCk_Handle_2dGridBlocker
{
    InHandle.Add<ck::FFragment_2dGridBlocker_Params>(InParams);
    InHandle.Add<ck::FFragment_2dGridBlocker_Current>();
    InHandle.Add<ck::FTag_2dGridBlocker_NeedsSetup>();

    // Death-watch: decrement the counted Disabled tag on stamped cells when the
    // blocker entity is destroyed (so a bare-spawned blocker releases its block).
    auto DeathWatch = FCk_Delegate_OnBeginDestroy{};
    DeathWatch.BindUFunction(GetMutableDefault<UCk_Utils_2dGridBlocker_UE>(), GET_FUNCTION_NAME_CHECKED(UCk_Utils_2dGridBlocker_UE, OnBlockerBeginDestroy));
    UCk_Utils_EntityLifetime_UE::BindTo_OnBeginDestroy(InHandle, DeathWatch);

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_2dGridBlocker_UE, FCk_Handle_2dGridBlocker,
    ck::FFragment_2dGridBlocker_Params, ck::FFragment_2dGridBlocker_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridBlocker_UE::
    Request_SetActive(
        FCk_Handle_2dGridBlocker& InBlocker,
        bool InActive)
    -> FCk_Handle_2dGridBlocker
{
    CK_CALLSTACK_RECORD(ck::FFragment_2dGridBlocker_Requests, InBlocker);

    InBlocker.AddOrGet<ck::FFragment_2dGridBlocker_Requests>()._Requests.Emplace(
        FCk_Request_2dGridBlocker_SetActive{InActive});

    return InBlocker;
}

auto
    UCk_Utils_2dGridBlocker_UE::
    Get_Grid(
        const FCk_Handle_2dGridBlocker& InBlocker)
    -> FCk_Handle_2dGridSystem
{
    return InBlocker.Get<ck::FFragment_2dGridBlocker_Params>().Get_Grid();
}

auto
    UCk_Utils_2dGridBlocker_UE::
    Get_BlockedCells(
        const FCk_Handle_2dGridBlocker& InBlocker)
    -> TArray<FIntPoint>
{
    return InBlocker.Get<ck::FFragment_2dGridBlocker_Current>().Get_StampedCells();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridBlocker_UE::
    OnBlockerBeginDestroy(
        FCk_Handle InHandle)
    -> void
{
    if (NOT InHandle.Has<ck::FFragment_2dGridBlocker_Current>())
    { return; }

    auto& Current = InHandle.Get<ck::FFragment_2dGridBlocker_Current>();

    // Only release the block if it's currently active; an inactive blocker has
    // already decremented its cells and must not double-decrement on destroy.
    if (NOT Current.Get_IsActive())
    { return; }

    if (NOT InHandle.Has<ck::FFragment_2dGridBlocker_Params>())
    { return; }

    const auto Grid = InHandle.Get<ck::FFragment_2dGridBlocker_Params>().Get_Grid();
    if (ck::Is_NOT_Valid(Grid))
    { return; }

    for (const auto& Coord : Current.Get_StampedCells())
    {
        auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(Grid, Coord);
        if (ck::IsValid(Cell))
        { Cell.Remove<ck::FTag_2dGridCell_Disabled>(); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
