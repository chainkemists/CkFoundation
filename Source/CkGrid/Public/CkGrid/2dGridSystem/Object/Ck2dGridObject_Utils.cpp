#include "Ck2dGridObject_Utils.h"

#include "Ck2dGridObject_Fragment.h"

#include "CkGrid/CkGrid_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_2dGridObject_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_2dGridObject_ParamsData& InParams)
    -> FCk_Handle_2dGridObject
{
    InHandle.Add<ck::FFragment_2dGridObject_Params>(InParams);
    return Cast(InHandle);
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

    const auto CenterOffset = Params.Get_Centering() == ECk_GridObject_Centering::Center
        ? FIntPoint(W / 2, H / 2)
        : FIntPoint::ZeroValue;

    const auto AnchorOffset = Params.Get_AnchorOffset();

    auto Offsets = TArray<FIntPoint>{};
    Offsets.Reserve(W * H);

    for (auto Y = 0; Y < H; ++Y)
    {
        for (auto X = 0; X < W; ++X)
        {
            Offsets.Add(FIntPoint(X, Y) - CenterOffset + AnchorOffset);
        }
    }

    const auto RotatedOffsets = UCk_Utils_Grid2D_UE::Get_RotatedShape(Offsets, InRotation);

    auto ResolvedCells = TArray<FIntPoint>{};
    ResolvedCells.Reserve(RotatedOffsets.Num());

    for (const auto& Offset : RotatedOffsets)
    {
        ResolvedCells.Add(InAnchor + Offset);
    }

    return ResolvedCells;
}

// --------------------------------------------------------------------------------------------------------------------
