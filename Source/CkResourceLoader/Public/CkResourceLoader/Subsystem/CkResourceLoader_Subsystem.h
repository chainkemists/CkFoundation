#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Entity/CkEntity.h"

#include "CkResourceLoader/CkResourceLoader_Fragment_Data.h"

#include "CkResourceLoader_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable,NotBlueprintType, DisplayName = "CkSubsystem_ResourceLoader")
class CKRESOURCELOADER_API UCk_ResourceLoader_Subsystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ResourceLoader_Subsystem_UE);

public:
    auto
    Request_TrackResource(
        const FCk_ResourceLoader_LoadedObject& InLoadedObject) -> void;

private:
    UPROPERTY(Transient)
    TSet<FCk_ResourceLoader_LoadedObject> _TrackedResources;

private:
    TMultiMap<FCk_ResourceLoader_ObjectReference_Soft, TArray<FCk_ResourceLoader_LoadedObject>> _TrackedResourcesLookup;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKRESOURCELOADER_API UCk_Utils_ResourceLoader_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_ResourceLoader_Subsystem_UE;

public:
    static auto
    Get_Subsystem() -> SubsystemType*;
};

// --------------------------------------------------------------------------------------------------------------------
