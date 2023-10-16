#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
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
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityLifetime")
    static FCk_Handle
    Get_LifetimeOwner(FCk_Handle InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityLifetime")
    static TArray<FCk_Handle>
    Get_LifetimeDependents(FCk_Handle InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityLifetime")
    static bool
    Get_IsPendingDestroy(FCk_Handle InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityLifetime")
    static FCk_Handle
    Get_TransientEntity(FCk_Handle InHandle);

public:
    template <typename T_Predicate>
    static auto
    Get_LifetimeOwnerIf(
        FCk_Handle  InHandle,
        T_Predicate T_Func) -> FCk_Handle
    ;

public:
    static auto Request_CreateEntity(RegistryType& InRegistry) -> HandleType;
    static auto Request_CreateEntity(RegistryType& InRegistry,
        const EntityIdHint& InEntityHint) -> HandleType;

public:
    static auto Get_TransientEntity(const RegistryType& InRegistry) -> HandleType;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Predicate>
auto
    UCk_Utils_EntityLifetime_UE::
    Get_LifetimeOwnerIf(
        FCk_Handle InHandle,
        T_Predicate T_Func)
    -> FCk_Handle
{
    auto CurrentHandle = InHandle;
    while (CurrentHandle.Has<ck::FFragment_LifetimeOwner>())
    {
        if (T_Func(CurrentHandle))
        {
            return CurrentHandle;
        }

        CurrentHandle = Get_LifetimeOwner(CurrentHandle);
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
