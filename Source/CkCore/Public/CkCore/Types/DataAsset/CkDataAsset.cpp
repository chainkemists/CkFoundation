#include "CkDataAsset.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Engine/Engine.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DataAsset_PDA::
    GetWorld() const
    -> UWorld*
{
    const auto& World = [&]() -> UWorld*
    {
        if (ck::Is_NOT_Valid(GEngine))
        { return {}; }

        for (auto& Context : GEngine->GetWorldContexts())
        {
            const auto& ContextWorld = Context.World();

            if (ck::Is_NOT_Valid(ContextWorld))
            { continue; }

            if (const auto& GameInstance = ContextWorld->GetGameInstance(); ck::IsValid(GameInstance))
            {
                return GameInstance->GetWorld();
            }
        }

        return {};
    }();

    return World;
}

// --------------------------------------------------------------------------------------------------------------------
