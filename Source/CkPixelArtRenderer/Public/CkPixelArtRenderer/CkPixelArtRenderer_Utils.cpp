#include "CkPixelArtRenderer/CkPixelArtRenderer_Utils.h"

#include "CkPixelArtRenderer/CkPixelArtRenderer_SnapMath.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PixelArtRenderer_UE::
    Get_ExactFraction(
        int32 InTargetWidth,
        int32 InViewportWidth)
    -> float
{
    const auto InputDescribesADownscale =
        InViewportWidth > 0 && InTargetWidth > 0 && InTargetWidth <= InViewportWidth;

    if (!InputDescribesADownscale)
    { return 1.0f; }

    return (InTargetWidth - 0.5f) / InViewportWidth;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PixelArtRenderer_UE::
    Get_SnappedViewOrigin(
        const FVector& InOrigin,
        const FMatrix& InViewRotationMatrix,
        float InTexelWorldSize,
        FVector2D& OutRemainderTexels)
    -> FVector
{
    auto RemainderTexels = FVector2f::ZeroVector;

    const auto Snapped = ck::pixel_art::Get_SnappedViewOrigin(
        InOrigin, InViewRotationMatrix, InTexelWorldSize, RemainderTexels);

    OutRemainderTexels = FVector2D{RemainderTexels};

    return Snapped;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PixelArtRenderer_UE::
    Get_OrthoWidthFromProjection(
        const FMatrix& InProjection)
    -> float
{
    return static_cast<float>(ck::pixel_art::Get_OrthoWidthFromProjection(InProjection));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PixelArtRenderer_UE::
    Get_HorizontalMarginTexels(
        int32 InInnerWidth,
        int32 InInnerHeight,
        int32 InMarginTexels,
        float InViewportAspect)
    -> int32
{
    return ck::pixel_art::Get_HorizontalMarginTexels(
        InInnerWidth, InInnerHeight, InMarginTexels, InViewportAspect);
}
