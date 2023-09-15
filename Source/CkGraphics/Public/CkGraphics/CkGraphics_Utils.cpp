#include "CkGraphics_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Graphics_UE::
    Get_ModifiedColorIntensity(
        FColor InColor,
        float  InIntensity)
    -> FColor
{
    using ColorChannelType = decltype(InColor.R);

    const auto& IntensifyColor = [&](auto InChannel)
    {
        return static_cast<ColorChannelType>(static_cast<float>(InChannel) * InIntensity);
    };

    return FColor
    {
        IntensifyColor(InColor.R),
        IntensifyColor(InColor.G),
        IntensifyColor(InColor.B),
        InColor.A
    };
}

// --------------------------------------------------------------------------------------------------------------------
