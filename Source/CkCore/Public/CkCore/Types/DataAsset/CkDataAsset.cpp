#include "CkDataAsset.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Engine/Engine.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DataAsset_PDA::
    GetWorld() const
    -> UWorld*
{
    if (ck::IsValid(Get_CurrentWorld()))
    {
        return Get_CurrentWorld().Get();
    }

    const auto SuperWorld = GetWorldFromOuterRecursively(GetOuter());
    if (ck::IsValid(SuperWorld))
    {
        return SuperWorld;
    }

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

auto
    UCk_DataAsset_PDA::
    GetWorldFromOuterRecursively(UObject* InObject)
        -> UWorld*
{
    if (ck::Is_NOT_Valid(InObject))
    { return {}; }

    if (ck::IsValid(Cast<UWorld>(InObject)))
    {
        return Cast<UWorld>(InObject);
    }

    return GetWorldFromOuterRecursively(InObject->GetOuter());
}

// --------------------------------------------------------------------------------------------------------------------
