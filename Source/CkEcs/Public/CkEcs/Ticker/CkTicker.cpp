#include "CkTicker.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FTicker::
        Tick(
            TimeType InDeltaTime)
        -> void
    {
        _ProcessorsRegistry.View<TickableType>().ForEach(
        [&](const EntityType InEntity, TickableType& InTickable)
        {
            InTickable->Tick(InDeltaTime);
        });
    }
}


// --------------------------------------------------------------------------------------------------------------------
