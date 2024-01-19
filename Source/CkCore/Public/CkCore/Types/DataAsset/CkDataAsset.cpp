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

    const auto SuperWorld = Get_WorldFromOuterRecursively(GetOuter());
    if (ck::IsValid(SuperWorld))
    {
        return SuperWorld;
    }

    // The following is needed for UObjects in Editor to have access to nodes that require a World.
    // By default, Unreal does not allow UObjects to have access to functions that require a WorldContext.
    // Of course, this means that if the inherited UObject does NOT have a valid world, those nodes would
    // fail to work
    if (NOT GIsPlayInEditorWorld)
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

    return nullptr;
}

auto
    UCk_DataAsset_PDA::
    Get_WorldFromOuterRecursively(UObject* InObject)
        -> UWorld*
{
    if (ck::Is_NOT_Valid(InObject))
    { return {}; }

    if (ck::IsValid(Cast<UWorld>(InObject)))
    {
        return Cast<UWorld>(InObject);
    }

    return Get_WorldFromOuterRecursively(InObject->GetOuter());
}

// --------------------------------------------------------------------------------------------------------------------
