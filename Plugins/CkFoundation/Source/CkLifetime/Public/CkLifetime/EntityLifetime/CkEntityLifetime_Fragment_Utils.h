#pragma once

#include "CkMacros/CkMacros.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Handle/CkHandle.h"

#include <CoreMinimal.h>

#include "CkEntityLifetime_Fragment_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKLIFETIME_API UCk_Utils_EntityLifetime_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityLifetime_UE);

public:
    using RegistryType = FCk_Registry;
    using EntityType   = FCk_Entity;
    using HandleType   = FCk_Handle;

public:
    struct EntityIdHint { EntityType Entity; };

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|EntityLifetime|Requests")
    static void
    Request_DestroyEntity(FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|EntityLifetime|Requests")
    static FCk_Handle
    Request_CreateEntity(FCk_Handle InHandle);

public:
    static auto Request_CreateEntity(RegistryType& InRegistry) -> HandleType;
    static auto Request_CreateEntity(RegistryType& InRegistry,
        const EntityIdHint& InEntityHint) -> HandleType;
};

// --------------------------------------------------------------------------------------------------------------------
