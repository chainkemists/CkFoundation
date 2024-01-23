#include "CkGraphics_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <Engine/World.h>
#include <EngineUtils.h>

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

auto
    UCk_Utils_Graphics_UE::
    Get_WasActorRecentlyRendered(
        AActor*  InActor,
        FCk_Time InTimeTolerance)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to Get_WasActorRecentlyRendered"))
    { return {}; }

    return InActor->WasRecentlyRendered(InTimeTolerance.Get_Seconds());
}

// --------------------------------------------------------------------------------------------------------------------
