#include "CkTicker.h"

// --------------------------------------------------------------------------------------------------------------------

auto ck::FTicker::
Tick(FTimeType InDeltaTime) -> void
{
    _ProcessorsRegistry.View<FTickableType>().ForEach(
    [&](const FEntityType InEntity, FTickableType& InTickable)
    {
        InTickable->Tick(InDeltaTime);
    });
}

// --------------------------------------------------------------------------------------------------------------------
