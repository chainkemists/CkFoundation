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
    Request_GatherAllRenderedActors(
        UObject* InWorldContextObject,
        FCk_Time InTimeTolerance,
        FCk_Delegate_OnAllRenderedActorsGathered InDelegate)
    -> void
{
    TArray<AActor*> AllRenderedActors;

    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject), TEXT("Invalid WorldContextObject"))
    { return; }

    // TODO: Modify this to return a cached result

    for (auto ActorItr = TActorIterator<AActor>{InWorldContextObject->GetWorld()}; ActorItr; ++ActorItr)
    {
        if (ActorItr->WasRecentlyRendered(InTimeTolerance.Get_Seconds()))
        {
            AllRenderedActors.Add(*ActorItr);
        }
    }

    std::ignore = InDelegate.ExecuteIfBound(AllRenderedActors);
}

// --------------------------------------------------------------------------------------------------------------------
