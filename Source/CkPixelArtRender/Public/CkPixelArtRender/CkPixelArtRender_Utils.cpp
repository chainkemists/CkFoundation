#include "CkPixelArtRender/CkPixelArtRender_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PixelArtRender_UE::
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
