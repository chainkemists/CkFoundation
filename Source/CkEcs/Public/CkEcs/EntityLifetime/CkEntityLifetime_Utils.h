#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Params.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Handle/CkHandle.h"

#include <CoreMinimal.h>

#include "CkEntityLifetime_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_EntityLifetime_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityLifetime_UE);

public:
    using RegistryType          = FCk_Registry;
    using EntityType            = FCk_Entity;
    using HandleType            = FCk_Handle;
    using PostEntityCreatedFunc = TFunction<void(FCk_Handle&)>;

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
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Lifetime] Request Destroy Entity",
              Category = "Ck|Utils|Lifetime")
    static void
    Request_DestroyEntity(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_EntityLifetime_DestructionBehavior InDestructionBehavior = ECk_EntityLifetime_DestructionBehavior::ForceDestroy);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Lifetime] Request Create New Entity",
              Category = "Ck|Utils|Lifetime")
    static FCk_Handle
    Request_CreateEntity(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Lifetime] Get Entity Lifetime Owner",
              Category = "Ck|Utils|Lifetime")
    static FCk_Handle
    Get_LifetimeOwner(
        const FCk_Handle& InHandle,
        ECk_PendingKill_Policy InPendingKillPolicy = ECk_PendingKill_Policy::ExcludePendingKill);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Lifetime] Get Entity Lifetime Dependents",
              Category = "Ck|Utils|Lifetime")
    static TArray<FCk_Handle>
    Get_LifetimeDependents(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Lifetime] Get Is Entity Pending Destroy",
              Category = "Ck|Utils|Lifetime")
    static bool
    Get_IsPendingDestroy(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Lifetime] Get Transient Entity",
              Category = "Ck|Utils|Lifetime")
    static FCk_Handle
    Get_TransientEntity(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Lifetime] Get World For Entity",
              Category = "Ck|Utils|Lifetime")
    static UWorld*
    Get_WorldForEntity(
        const FCk_Handle& InHandle);

public:
    template <typename T_Predicate>
    [[nodiscard]]
    static auto
    Get_LifetimeOwnerIf(
        const FCk_Handle& InHandle,
        T_Predicate T_Func,
        ECk_PendingKill_Policy InPendingKillPolicy = ECk_PendingKill_Policy::ExcludePendingKill) -> FCk_Handle;

public:
    [[nodiscard]]
    static auto
    Request_CreateEntity(
        const FCk_Handle& InHandle,
        PostEntityCreatedFunc InFunc) -> HandleType;

    [[nodiscard]]
    static auto
    Request_CreateEntity(
        RegistryType& InRegistry,
        PostEntityCreatedFunc InFunc = PostEntityCreatedFunc{}) -> HandleType;

    [[nodiscard]]
    static auto
    Request_CreateEntity(
        RegistryType& InRegistry,
        const EntityIdHint& InEntityHint,
        PostEntityCreatedFunc InFunc = PostEntityCreatedFunc{}) -> HandleType;

public:
    [[nodiscard]]
    static auto
    Get_TransientEntity(
        const RegistryType& InRegistry) -> HandleType;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Predicate>
auto
    UCk_Utils_EntityLifetime_UE::
    Get_LifetimeOwnerIf(
        const FCk_Handle& InHandle,
        T_Predicate T_Func,
        ECk_PendingKill_Policy InPendingKillPolicy)
    -> FCk_Handle
{
    auto CurrentHandle = InHandle;
    while (CurrentHandle.Has<ck::FFragment_LifetimeOwner>())
    {
        if (T_Func(CurrentHandle))
        {
            return CurrentHandle;
        }

        CurrentHandle = Get_LifetimeOwner(CurrentHandle, InPendingKillPolicy);
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
