#include "CkFogOfWar_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkMinimap/CkMinimap_Log.h"
#include "CkMinimap/CkMinimap_Math.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FogOfWar_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_FogOfWar_ParamsData& InParams)
    -> FCk_Handle_FogOfWar
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle. Unable to Add the FogOfWar feature"))
    { return {}; }

    CK_ENSURE_IF_NOT(NOT Has(InHandle),
        TEXT("Handle [{}] already has the FogOfWar feature. An entity hosts at most ONE exploration grid — "
             "cover multiple map areas with one fog entity each"),
        InHandle)
    { return Cast(InHandle); }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Bounds()),
        TEXT("FogOfWar Add on Entity [{}] requires valid Bounds (both half-extents > 0)"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_FogOfWar_Params>(InParams);
    InHandle.Add<ck::FFragment_FogOfWar_Current>();
    InHandle.Add<ck::FTag_FogOfWar_NeedsSetup>();

    return ck::StaticCast<FCk_Handle_FogOfWar>(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_FogOfWar_UE, FCk_Handle_FogOfWar, ck::FFragment_FogOfWar_Current, ck::FFragment_FogOfWar_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FogOfWar_UE::
    Get_IsLocationExplored(
        const FCk_Handle_FogOfWar& InFogOfWar,
        FVector InWorldLocation)
    -> bool
{
    const auto& Current = InFogOfWar.Get<ck::FFragment_FogOfWar_Current>();

    // An unallocated grid is a LEGITIMATE transient, not an error: a fog composed mid-frame is queried by
    // same-frame projections (e.g. the minimap's PostTransform update) before its Setup pump runs. Until the
    // grid exists the fog gates nothing — everything reads explored (unfogged). A PERMANENTLY unallocated
    // grid (invalid bounds / blown cell budget) is already reported loudly by the Setup processor's ensures.
    if (Current.Get_Explored().IsEmpty())
    { return true; }

    const auto& Params = InFogOfWar.Get<ck::FFragment_FogOfWar_Params>();

    const auto CellIndex = ck::minimap::Get_CellIndex(
        InWorldLocation, Params.Get_Bounds(), Params.Get_CellSize(), Current.Get_CellCounts());

    if (CellIndex == INDEX_NONE)
    { return true; }

    return Current.Get_Explored()[CellIndex];
}

auto
    UCk_Utils_FogOfWar_UE::
    Get_ExploredFraction(
        const FCk_Handle_FogOfWar& InFogOfWar)
    -> float
{
    const auto& Explored = InFogOfWar.Get<ck::FFragment_FogOfWar_Current>().Get_Explored();

    if (Explored.IsEmpty())
    { return 0.0f; }

    return static_cast<float>(Explored.CountSetBits()) / static_cast<float>(Explored.Num());
}

auto
    UCk_Utils_FogOfWar_UE::
    Get_CellCounts(
        const FCk_Handle_FogOfWar& InFogOfWar)
    -> FIntPoint
{
    return InFogOfWar.Get<ck::FFragment_FogOfWar_Current>().Get_CellCounts();
}

auto
    UCk_Utils_FogOfWar_UE::
    Get_ExploredData(
        const FCk_Handle_FogOfWar& InFogOfWar)
    -> FCk_RepData_FogOfWar
{
    const auto& Current = InFogOfWar.Get<ck::FFragment_FogOfWar_Current>();
    const auto& Explored = Current.Get_Explored();

    auto Payload = FCk_RepData_FogOfWar{};
    Payload.Set_CellCountX(Current.Get_CellCounts().X);
    Payload.Set_CellCountY(Current.Get_CellCounts().Y);

    auto PackedCells = TArray<uint8>{};
    PackedCells.SetNumZeroed((Explored.Num() + 7) / 8);

    for (auto CellIndex = 0; CellIndex < Explored.Num(); ++CellIndex)
    {
        if (NOT Explored[CellIndex])
        { continue; }

        PackedCells[CellIndex / 8] |= static_cast<uint8>(1 << (CellIndex % 8));
    }

    Payload.Set_PackedCells(MoveTemp(PackedCells));

    return Payload;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FogOfWar_UE::
    Request_AddRevealer(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Handle& InRevealer)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(
        FCk_Request_FogOfWar_AddRevealer{InRevealer});

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    Request_RemoveRevealer(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Handle& InRevealer)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(
        FCk_Request_FogOfWar_RemoveRevealer{InRevealer});

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    Request_RevealLocation(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Request_FogOfWar_RevealLocation& InRequest)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(InRequest);

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    Request_RevealAll(
        FCk_Handle_FogOfWar& InFogOfWar)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(
        FCk_Request_FogOfWar_RevealAll{});

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    Request_Reset(
        FCk_Handle_FogOfWar& InFogOfWar)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(
        FCk_Request_FogOfWar_Reset{});

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    Request_SetExplored(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Request_FogOfWar_SetExplored& InRequest)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(InRequest);

    return InFogOfWar;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FogOfWar_UE::
    BindTo_OnCellsRevealed(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Delegate_FogOfWar_CellsRevealed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_FogOfWar
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnFogOfWarCellsRevealed, InFogOfWar, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    BindTo_OnReset(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Delegate_FogOfWar_Reset& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_FogOfWar
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnFogOfWarReset, InFogOfWar, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    UnbindFrom_OnCellsRevealed(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Delegate_FogOfWar_CellsRevealed& InDelegate)
    -> FCk_Handle_FogOfWar
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnFogOfWarCellsRevealed, InFogOfWar, InDelegate);

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    UnbindFrom_OnReset(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Delegate_FogOfWar_Reset& InDelegate)
    -> FCk_Handle_FogOfWar
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnFogOfWarReset, InFogOfWar, InDelegate);

    return InFogOfWar;
}

// --------------------------------------------------------------------------------------------------------------------
