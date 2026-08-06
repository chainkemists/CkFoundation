#include "Ck2dGridObject_Utils.h"

#include "Ck2dGridObject_Fragment.h"

#include "CkGrid/CkGrid_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridObject_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_2dGridObject_Spec& InParams)
    -> FCk_Handle_2dGridObject
{
    InHandle.Add<ck::FFragment_2dGridObject_Params>(InParams);
    return Cast(InHandle);
}

auto
    UCk_Utils_2dGridObject_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_2dGridObject_Spec& InParams)
    -> FCk_Handle_2dGridObject
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InParams);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_2dGridObject_UE, FCk_Handle_2dGridObject, ck::FFragment_2dGridObject_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridObject_UE::
    Get_FootprintExtent(
        const FCk_Handle_2dGridObject& InObject)
    -> FIntPoint
{
    return InObject.Get<ck::FFragment_2dGridObject_Params>().Get_FootprintExtent();
}

auto
    UCk_Utils_2dGridObject_UE::
    Get_RequiredCellTags(
        const FCk_Handle_2dGridObject& InObject)
    -> FGameplayTagContainer
{
    return InObject.Get<ck::FFragment_2dGridObject_Params>().Get_RequiredCellTags();
}

auto
    UCk_Utils_2dGridObject_UE::
    Get_ForbiddenCellTags(
        const FCk_Handle_2dGridObject& InObject)
    -> FGameplayTagContainer
{
    return InObject.Get<ck::FFragment_2dGridObject_Params>().Get_ForbiddenCellTags();
}

auto
    UCk_Utils_2dGridObject_UE::
    Get_ResolvedCells(
        const FCk_Handle_2dGridObject& InObject,
        const FIntPoint& InAnchor,
        ECk_CardinalRotation InRotation)
    -> TArray<FIntPoint>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("GridObject is invalid"))
    { return {}; }

    const auto& Params = InObject.Get<ck::FFragment_2dGridObject_Params>();
    const auto Extent = Params.Get_FootprintExtent();
    const auto W = Extent.X;
    const auto H = Extent.Y;

    CK_ENSURE_IF_NOT(W > 0 && H > 0,
        TEXT("GridObject footprint extent [{}] must be positive on both axes"), Extent)
    { return {}; }

    auto RawOffsets = TArray<FIntPoint>{};
    RawOffsets.Reserve(W * H);

    for (auto Y = 0; Y < H; ++Y)
    {
        for (auto X = 0; X < W; ++X)
        {
            RawOffsets.Add(FIntPoint(X, Y));
        }
    }

    // Get_RotatedShape rotates AND re-normalizes to a non-negative bounding box, so centering and
    // anchor-offset are applied AFTER rotation (against the rotated extent) — otherwise the
    // normalization would erase them and a centered object would only stay centered at 0 rotation.
    const auto RotatedOffsets = UCk_Utils_Grid2D_UE::Get_RotatedShape(RawOffsets, InRotation);

    auto RotatedExtent = FIntPoint::ZeroValue;
    for (const auto& Offset : RotatedOffsets)
    {
        RotatedExtent.X = FMath::Max(RotatedExtent.X, Offset.X + 1);
        RotatedExtent.Y = FMath::Max(RotatedExtent.Y, Offset.Y + 1);
    }

    const auto CenterOffset = Params.Get_Centering() == ECk_GridObject_Centering::Center
        ? FIntPoint(RotatedExtent.X / 2, RotatedExtent.Y / 2)
        : FIntPoint::ZeroValue;

    const auto AnchorOffset = Params.Get_AnchorOffset();

    auto ResolvedCells = TArray<FIntPoint>{};
    ResolvedCells.Reserve(RotatedOffsets.Num());

    for (const auto& Offset : RotatedOffsets)
    {
        ResolvedCells.Add(InAnchor + Offset - CenterOffset + AnchorOffset);
    }

    return ResolvedCells;
}

// --------------------------------------------------------------------------------------------------------------------
