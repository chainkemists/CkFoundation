#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Params.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Common.h"
#include "CkEcs/Net/CkNet_Fragment_Data.h"
#include "CkEcs/Delegates/CkDelegates.h"

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
    /*
    * NOT added by design. Every Entity, except the top-level Transient Entity, should have a lifetime owner.
    * To check whether we 'are' the top-level Entity, get the transient entity from the Subsystem and compare
    * OR use the `Get_TransientEntity(...)` found in this utils
    *
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Lifetime] Has Entity Lifetime Owner",
              Category = "Ck|Utils|Lifetime")
    static FCk_Handle
    Get_HasLifetimeOwner(
        const FCk_Handle& InHandle,
        ECk_PendingKill_Policy InPendingKillPolicy = ECk_PendingKill_Policy::ExcludePendingKill);
    */

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
        const FCk_Handle& InHandle,
        ECk_EntityLifetime_DestructionPhase InDestructionPhase = ECk_EntityLifetime_DestructionPhase::Confirmed);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Lifetime] Is Transient Entity?",
              Category = "Ck|Utils|Lifetime")
    static bool
    Get_IsTransientEntity(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Lifetime] Get World For Entity",
              Category = "Ck|Utils|Lifetime")
    static UWorld*
    Get_WorldForEntity(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Lifetime",
              DisplayName = "[Ck][Lifetime] Get Entity In Ownership Chain If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static FCk_Handle
    Get_EntityInOwnershipChain_If(
        UPARAM(ref) FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Predicate_InHandle_OutResult& InPredicate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Lifetime",
              DisplayName = "[Ck][Lifetime] Bind To OnDestroy")
    static void
    BindTo_OnUpdate(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Lifetime_OnDestroy& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Lifetime",
              DisplayName = "[Ck][Lifetime] Unbind From OnDestroy")
    static void
    UnbindFrom_OnUpdate(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Lifetime_OnDestroy& InDelegate);

public:
    // These functions already exist on Net_Utils (which are now a pass-through). The reason they exist on this
    // utils as well is because CkEcs cannot depend on CkNet
    [[nodiscard]]
    static auto
    Get_EntityNetRole(
        const FCk_Handle& InEntity) -> ECk_Net_EntityNetRole;

    [[nodiscard]]
    static auto
    Get_EntityNetMode(
        const FCk_Handle& InEntity) -> ECk_Net_NetModeType;

public:
    template <typename T_Predicate>
    [[nodiscard]]
    static auto
    Get_EntityInOwnershipChain_If(
        const FCk_Handle& InHandle,
        T_Predicate T_Func,
        ECk_PendingKill_Policy InPendingKillPolicy = ECk_PendingKill_Policy::ExcludePendingKill) -> FCk_Handle;

    template <typename T_TypeSafeHandle>
    [[nodiscard]]
    static auto
    Get_LifetimeOwner_AsTypeSafe(
        const FCk_Handle& InHandle) -> T_TypeSafeHandle;

    template <typename T_TypeSafeHandle>
    [[nodiscard]]
    static auto
    Request_CreateEntity_AsTypeSafe(
        const FCk_Handle& InHandle) -> T_TypeSafeHandle;

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

    static auto
    Request_SetupEntityWithLifetimeOwner(
        FCk_Handle& InNewEntity,
        const FCk_Handle& InLifetimeOwner) -> void;

    static auto
    Request_TransferLifetimeOwner(
        FCk_Handle& InEntity,
        const FCk_Handle& InNewLifetimeOwner) -> void;

public:
    [[nodiscard]]
    static auto
    Get_TransientEntity(
        const RegistryType& InRegistry) -> HandleType;

    [[nodiscard]]
    static auto
    Get_TransientEntity(
        const HandleType& InHandle) -> HandleType;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Predicate>
auto
    UCk_Utils_EntityLifetime_UE::
    Get_EntityInOwnershipChain_If(
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

template <typename T_TypeSafeHandle>
auto
    UCk_Utils_EntityLifetime_UE::
    Get_LifetimeOwner_AsTypeSafe(
        const FCk_Handle& InHandle)
    -> T_TypeSafeHandle
{
    auto LifetimeOwner = Get_LifetimeOwner(InHandle);
    return ck::StaticCast<T_TypeSafeHandle>(LifetimeOwner);
}

template <typename T_TypeSafeHandle>
auto
    UCk_Utils_EntityLifetime_UE::
    Request_CreateEntity_AsTypeSafe(
        const FCk_Handle& InHandle)
    -> T_TypeSafeHandle
{
    QUICK_SCOPE_CYCLE_COUNTER(Request_Create_Entity)

    auto NewEntity = Request_CreateEntity(InHandle);
    return ck::StaticCast<T_TypeSafeHandle>(NewEntity);
}

// --------------------------------------------------------------------------------------------------------------------
