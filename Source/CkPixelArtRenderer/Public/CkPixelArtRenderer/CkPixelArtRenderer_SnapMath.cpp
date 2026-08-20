#include "CkPixelArtRenderer/CkPixelArtRenderer_SnapMath.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pixel_art
{
    auto
        Get_ViewBasis(
            const FMatrix& InViewRotationMatrix)
        -> FCk_PixelArt_ViewBasis
    {
        auto Basis = FCk_PixelArt_ViewBasis{};
        Basis.Right   = InViewRotationMatrix.GetColumn(0);
        Basis.Up      = InViewRotationMatrix.GetColumn(1);
        Basis.Forward = InViewRotationMatrix.GetColumn(2);

        return Basis;
    }

    auto
        Get_SnappedViewOrigin(
            const FVector& InOrigin,
            const FMatrix& InViewRotationMatrix,
            double InTexelWorldSize,
            FVector2f& OutRemainderTexels)
        -> FVector
    {
        OutRemainderTexels = FVector2f::ZeroVector;

        if (InTexelWorldSize <= 0.0)
        { return InOrigin; }

        const auto Basis = Get_ViewBasis(InViewRotationMatrix);

        const auto AlongRight = FVector::DotProduct(InOrigin, Basis.Right);
        const auto AlongUp    = FVector::DotProduct(InOrigin, Basis.Up);

        const auto SnappedAlongRight = FMath::RoundToDouble(AlongRight / InTexelWorldSize) * InTexelWorldSize;
        const auto SnappedAlongUp    = FMath::RoundToDouble(AlongUp    / InTexelWorldSize) * InTexelWorldSize;

        OutRemainderTexels = FVector2f{
            static_cast<float>((AlongRight - SnappedAlongRight) / InTexelWorldSize),
            static_cast<float>((AlongUp    - SnappedAlongUp)    / InTexelWorldSize)};

        // Correcting the origin by the two deltas rather than rebuilding it from the snapped components is what
        // leaves the view-forward component bit-for-bit untouched: a rebuild would round-trip the whole position
        // through the basis and move it along forward by the floating-point error of that round trip.
        return InOrigin
            + (SnappedAlongRight - AlongRight) * Basis.Right
            + (SnappedAlongUp    - AlongUp)    * Basis.Up;
    }

    auto
        Get_OrthoWidthFromProjection(
            const FMatrix& InProjection)
        -> double
    {
        const auto HorizontalScale = InProjection.M[0][0];

        if (FMath::IsNearlyZero(HorizontalScale))
        { return 0.0; }

        return 2.0 / HorizontalScale;
    }

    auto
        Apply_MarginFold(
            FMatrix& InOutProjection,
            const FIntPoint& InInnerSize,
            const FIntPoint& InRenderSize)
        -> void
    {
        if (InInnerSize.X <= 0 || InInnerSize.Y <= 0 || InRenderSize.X <= 0 || InRenderSize.Y <= 0)
        { return; }

        const auto HorizontalScale =
            static_cast<double>(InInnerSize.X) / static_cast<double>(InRenderSize.X);

        InOutProjection.M[0][0] *= HorizontalScale;
        InOutProjection.M[1][1] = InOutProjection.M[0][0] *
            (static_cast<double>(InRenderSize.X) / static_cast<double>(InRenderSize.Y));
    }

    auto
        Get_HorizontalMarginTexels(
            int32 InInnerWidth,
            int32 InInnerHeight,
            int32 InMarginTexels,
            double InViewportAspect)
        -> int32
    {
        if (InMarginTexels <= 0 || InInnerWidth <= 0 || InInnerHeight <= 0 || InViewportAspect <= 0.0)
        { return FMath::Max(0, InMarginTexels); }

        // The rendered height the engine will produce is CeilToInt(RenderedWidth * ViewportHeight / ViewportWidth),
        // and the exact-fraction rule costs half a pixel of that product, so the width that guarantees
        // InMarginTexels rows above AND below the displayed window is the one whose worst-case quotient still
        // reaches InnerHeight + 2 * Margin.
        const auto RequiredRenderedWidth = (InInnerHeight + 2.0 * InMarginTexels) * InViewportAspect + 0.5;
        const auto RequiredMargin = FMath::CeilToInt32((RequiredRenderedWidth - InInnerWidth) * 0.5);

        return FMath::Max(InMarginTexels, RequiredMargin);
    }
}
