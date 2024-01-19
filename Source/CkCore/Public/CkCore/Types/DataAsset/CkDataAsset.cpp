#include "CkDataAsset.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/World/CkWorld_Utils.h"

#include <Engine/Engine.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DataAsset_PDA::
    GetWorld() const
    -> UWorld*
{
    const auto& MaybeWorld = UCk_Utils_World_UE::TryGet_MutableFirstValidWorld(this, [](UWorld*) { return true; });

    if (ck::IsValid(MaybeWorld))
    { return *MaybeWorld; }

    return {};
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
