#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Handle/CkHandle.h"

#include <CoreMinimal.h>

#include "CkEntityLifetime_Fragment_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_EntityLifetime_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityLifetime_UE);

public:
    using RegistryType = FCk_Registry;
    using EntityType   = FCk_Entity;
    using HandleType   = FCk_Handle;

public:
    struct EntityIdHint
    {
        CK_GENERATED_BODY(EntityIdHint);

    private:
        EntityType _Entity;

    public:
        CK_PROPERTY_GET(_Entity);
        CK_DEFINE_CONSTRUCTORS(EntityIdHint, _Entity);
    };

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|EntityLifetime")
    static void
    Request_DestroyEntity(FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|EntityLifetime")
    static FCk_Handle
    Request_CreateEntity(FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|EntityLifetime")
    static bool
    Get_IsPendingDestroy(FCk_Handle InHandle);

public:
    static auto Request_CreateEntity(RegistryType& InRegistry) -> HandleType;
    static auto Request_CreateEntity(RegistryType& InRegistry,
        const EntityIdHint& InEntityHint) -> HandleType;

public:
    static auto Get_TransientEntity(const RegistryType& InRegistry) -> HandleType;
};

// --------------------------------------------------------------------------------------------------------------------
