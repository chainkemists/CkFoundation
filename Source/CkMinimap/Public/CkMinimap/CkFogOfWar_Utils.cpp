#include "CkFogOfWar_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkMinimap/CkMinimap_Log.h"

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

    // An unallocated grid is a LEGITIMATE transient (composed mid-frame, queried before its Setup pump), so
    // it gates nothing rather than ensuring — a PERMANENT one is already reported by Setup's own ensures.
    if (Current.Get_Explored().IsEmpty())
    { return true; }

    const auto& Params = InFogOfWar.Get<ck::FFragment_FogOfWar_Params>();

    const auto CellIndex = Get_CellIndex(
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
        const FCk_Handle& InRevealer,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    const auto Request = FCk_Request_FogOfWar_AddRevealer{InRevealer};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(Request);

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    Request_RemoveRevealer(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Handle& InRevealer,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    const auto Request = FCk_Request_FogOfWar_RemoveRevealer{InRevealer};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(Request);

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    Request_RevealLocation(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Request_FogOfWar_RevealLocation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(InRequest);

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    Request_RevealAll(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    const auto Request = FCk_Request_FogOfWar_RevealAll{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(Request);

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    Request_Reset(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    const auto Request = FCk_Request_FogOfWar_Reset{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InFogOfWar.AddOrGet<ck::FFragment_FogOfWar_Requests>()._Requests.Emplace(Request);

    return InFogOfWar;
}

auto
    UCk_Utils_FogOfWar_UE::
    Request_SetExplored(
        FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Request_FogOfWar_SetExplored& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_FogOfWar
{
    CK_CALLSTACK_RECORD(ck::FFragment_FogOfWar_Requests, InFogOfWar);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

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

auto
    UCk_Utils_FogOfWar_UE::
    Get_CellCounts(
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize)
    -> FIntPoint
{
    if (ck::Is_NOT_Valid(InBounds) || InCellSize <= 0.0f)
    { return FIntPoint{1, 1}; }

    const auto& HalfExtents = InBounds.Get_HalfExtents();

    return FIntPoint
    {
        FMath::Max(1, FMath::CeilToInt32(2.0 * HalfExtents.X / InCellSize)),
        FMath::Max(1, FMath::CeilToInt32(2.0 * HalfExtents.Y / InCellSize))
    };
}

auto
    UCk_Utils_FogOfWar_UE::
    Get_CellIndex(
        const FVector& InWorldPos,
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize,
        const FIntPoint& InCellCounts)
    -> int32
{
    if (ck::Is_NOT_Valid(InBounds) || InCellSize <= 0.0f || InCellCounts.X < 1 || InCellCounts.Y < 1)
    { return INDEX_NONE; }

    const auto& Center = InBounds.Get_Center();
    const auto& HalfExtents = InBounds.Get_HalfExtents();

    const auto MinX = Center.X - HalfExtents.X;
    const auto MinY = Center.Y - HalfExtents.Y;

    if (InWorldPos.X < MinX || InWorldPos.X > Center.X + HalfExtents.X ||
        InWorldPos.Y < MinY || InWorldPos.Y > Center.Y + HalfExtents.Y)
    { return INDEX_NONE; }

    const auto CellX = FMath::Clamp(FMath::FloorToInt32((InWorldPos.X - MinX) / InCellSize), 0, InCellCounts.X - 1);
    const auto CellY = FMath::Clamp(FMath::FloorToInt32((InWorldPos.Y - MinY) / InCellSize), 0, InCellCounts.Y - 1);

    return CellY * InCellCounts.X + CellX;
}

auto
    UCk_Utils_FogOfWar_UE::
    Get_CellCenter(
        int32 InCellX,
        int32 InCellY,
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize)
    -> FVector2D
{
    const auto& Center = InBounds.Get_Center();
    const auto& HalfExtents = InBounds.Get_HalfExtents();

    return FVector2D
    {
        Center.X - HalfExtents.X + (InCellX + 0.5) * InCellSize,
        Center.Y - HalfExtents.Y + (InCellY + 0.5) * InCellSize
    };
}

auto
    UCk_Utils_FogOfWar_UE::
    Get_CellUVRect(
        int32 InCellIndex,
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize)
    -> FBox2D
{
    const auto CellCounts = Get_CellCounts(InBounds, InCellSize);

    if (ck::Is_NOT_Valid(InBounds) || InCellSize <= 0.0f ||
        InCellIndex < 0 || InCellIndex >= CellCounts.X * CellCounts.Y)
    { return FBox2D{FVector2D::ZeroVector, FVector2D::ZeroVector}; }

    const auto CellX = InCellIndex % CellCounts.X;
    const auto CellY = InCellIndex / CellCounts.X;

    const auto& HalfExtents = InBounds.Get_HalfExtents();
    const auto ExtentX = 2.0 * HalfExtents.X;
    const auto ExtentY = 2.0 * HalfExtents.Y;

    const auto UMin = FMath::Clamp(CellY * InCellSize / ExtentY, 0.0, 1.0);
    const auto UMax = FMath::Clamp((CellY + 1) * InCellSize / ExtentY, 0.0, 1.0);
    const auto VMin = FMath::Clamp(1.0 - ((CellX + 1) * InCellSize / ExtentX), 0.0, 1.0);
    const auto VMax = FMath::Clamp(1.0 - (CellX * InCellSize / ExtentX), 0.0, 1.0);

    return FBox2D{FVector2D{UMin, VMin}, FVector2D{UMax, VMax}};
}

// --------------------------------------------------------------------------------------------------------------------
