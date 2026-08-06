#include "Ck2dGridBlocker_Utils.h"

#include "Ck2dGridBlocker_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkLabel/Public/CkLabel/CkLabel_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridBlocker_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_2dGridBlocker_Spec& InParams)
    -> FCk_Handle_2dGridBlocker
{
    InHandle.Add<ck::FFragment_2dGridBlocker_Params>(InParams);
    InHandle.Add<ck::FFragment_2dGridBlocker_Current>();
    InHandle.Add<ck::FTag_2dGridBlocker_NeedsSetup>();

    auto DeathWatch = FCk_Delegate_OnBeginDestroy{};
    DeathWatch.BindUFunction(GetMutableDefault<UCk_Utils_2dGridBlocker_UE>(), GET_FUNCTION_NAME_CHECKED(UCk_Utils_2dGridBlocker_UE, OnBlockerBeginDestroy));
    UCk_Utils_EntityLifetime_UE::BindTo_OnBeginDestroy(InHandle, DeathWatch);

    auto Blocker = Cast(InHandle);

    // The GameplayLabel must be stamped BEFORE Request_Connect: the record indexes the entry under
    // the label it reads at connect time, and that index is what Get_BlockerWithTag queries.
    if (const auto Grid = InParams.Get_Grid();
        ck::IsValid(Grid))
    {
        auto GridBase = FCk_Handle{Grid};
        ck::RecordOf_GridBlockers_Utils::AddIfMissing(GridBase);

        if (const auto& Name = InParams.Get_Name();
            Name.IsValid())
        {
            auto BlockerBase = FCk_Handle{Blocker};
            UCk_Utils_GameplayLabel_UE::Add(BlockerBase, Name);
        }

        ck::RecordOf_GridBlockers_Utils::Request_Connect(
            GridBase, Blocker, ECk_Record_LabelRequirementPolicy::Optional);
    }

    return Blocker;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridBlocker_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_2dGridBlocker_Spec& InParams)
    -> FCk_Handle_2dGridBlocker
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InParams);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_2dGridBlocker_UE, FCk_Handle_2dGridBlocker,
    ck::FFragment_2dGridBlocker_Params, ck::FFragment_2dGridBlocker_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridBlocker_UE::
    Request_SetActive(
        FCk_Handle_2dGridBlocker& InBlocker,
        bool InActive,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_2dGridBlocker
{
    CK_CALLSTACK_RECORD(ck::FFragment_2dGridBlocker_Requests, InBlocker);

    const auto Request = FCk_Request_2dGridBlocker_SetActive{InActive};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InBlocker.AddOrGet<ck::FFragment_2dGridBlocker_Requests>()._Requests.Emplace(Request);

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

auto
    UCk_Utils_2dGridBlocker_UE::
    Get_Blockers(
        const FCk_Handle_2dGridSystem& InGrid)
    -> TArray<FCk_Handle_2dGridBlocker>
{
    if (ck::Is_NOT_Valid(InGrid))
    { return {}; }

    return ck::RecordOf_GridBlockers_Utils::Get_ValidEntries(FCk_Handle{InGrid});
}

auto
    UCk_Utils_2dGridBlocker_UE::
    Get_BlockerWithTag(
        const FCk_Handle_2dGridSystem& InGrid,
        FGameplayTag InTag)
    -> FCk_Handle_2dGridBlocker
{
    if (ck::Is_NOT_Valid(InGrid) || NOT InTag.IsValid())
    { return {}; }

    return ck::RecordOf_GridBlockers_Utils::Get_ValidEntry_ByTag(FCk_Handle{InGrid}, InTag);
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
