#include "CkDataAsset.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Engine/Engine.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DataAsset_PDA::
    GetWorld() const
    -> UWorld*
{
    const auto& world = [&]() -> UWorld*
    {
        if (ck::Is_NOT_Valid(GEngine))
        { return {}; }

        for (auto& context : GEngine->GetWorldContexts())
        {
            const auto& contextWorld = context.World();

            if (ck::Is_NOT_Valid(contextWorld))
            { continue; }

            const auto& gameInstance = contextWorld->GetGameInstance();

            if (ck::IsValid(gameInstance))
            {
                return gameInstance->GetWorld();
            }
        }

        return {};
    }();

    return world;
}

// --------------------------------------------------------------------------------------------------------------------
