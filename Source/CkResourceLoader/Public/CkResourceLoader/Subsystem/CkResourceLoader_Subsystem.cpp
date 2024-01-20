#include "CkResourceLoader_Subsystem.h"

#include "CkCore/Game/CkGame_Utils.h"

#include "CkResourceLoader/CkResourceLoader_Log.h"
#include "CkResourceLoader/Settings/CkResourceLoader_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ResourceLoader_Subsystem_UE::
    Request_TrackResource(
        const FCk_ResourceLoader_LoadedObject& InLoadedObject)
    -> void
{
    const auto& LoadedObjectSoftRef = InLoadedObject.Get_ObjectReference_Soft();
    auto& LoadedObjects = _TrackedResourcesLookup.FindOrAdd(LoadedObjectSoftRef);

    LoadedObjects.AddUnique(InLoadedObject);
    _TrackedResources.Add(InLoadedObject);

    const auto& MaxCacheNumber = UCk_Utils_ResourceLoader_Settings_UE::Get_MaxNumberOfCachedResourcesPerType();

    if (LoadedObjects.Num() > MaxCacheNumber)
    {
        ck::resource_loader::VeryVerbose
        (
            TEXT("Max Number of Cached Resources [{}] reached for [{}]! Popping oldest one [{}]"),
            MaxCacheNumber,
            LoadedObjectSoftRef,
            LoadedObjects[0].Get_ObjectReference_Hard()
        );

        _TrackedResources.Remove(LoadedObjects[0]);
        LoadedObjects.RemoveAt(0);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResourceLoader_Subsystem_UE::
    Get_Subsystem()
    -> SubsystemType*
{
    return GEngine->GetEngineSubsystem<UCk_ResourceLoader_Subsystem_UE>();
}

// --------------------------------------------------------------------------------------------------------------------
