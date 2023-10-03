#pragma once

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkNet/CkNet_Common.h"
#include "CkNet/CkNet_Fragment_Data.h"

#include "CkNet_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNET_API UCk_Utils_Net_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Net_UE);

public:
    friend class UCk_EcsConstructionScript_ActorComponent_UE;

public:
    static auto Add(
        FCk_Handle InEntity,
        FCk_Net_ConnectionSettings InConnectionSettings) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Net",
              DisplayName="Copy Network Info")
    static void
    Copy(
        FCk_Handle InFrom,
        FCk_Handle InTo);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net",
              DisplayName="Has Network Info")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net",
              DisplayName="Ensure Has Network Info")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net")
    static ECk_Net_EntityNetRole
    Get_EntityNetRole(
        FCk_Handle InEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net")
    static ECk_Net_NetRoleType
    Get_EntityNetMode(
        FCk_Handle InEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net")
    static bool
    Get_HasAuthority(
        FCk_Handle InEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = InActor))
    static bool
    Get_IsActorLocallyOwned(AActor* InActor);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = InActor))
    static bool
    Get_IsRoleMatching(AActor* InActor, ECk_Net_ReplicationType InReplicationType);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static ECk_Net_NetRoleType
    Get_NetRole(const UObject* InContext = nullptr);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static ECk_Net_NetRoleType
    Get_NetMode(const UObject* InContext = nullptr);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net")
    static bool
    Get_IsEntityNetMode_Host(FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net")
    static bool
    Get_IsEntityNetMode_Client(FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net")
    static bool
    Get_IsEntityReplicated(FCk_Handle InHandle);

private:
    static auto Request_MarkEntityAs_DedicatedServer(FCk_Handle InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNET_API UCk_Utils_Ecs_Net_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Ecs_Net_UE);

public:
    template <typename T_ReplicatedFragment, typename T_UnaryUpdateFunc>
    static auto UpdateReplicatedFragment(FCk_Handle InHandle, T_UnaryUpdateFunc InUpdateFunc) -> void;

    template <typename T_ReplicatedFragment>
    static auto TryAddReplicatedFragment(FCk_Handle InHandle) -> ECk_AddedOrNot;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_ReplicatedFragment, typename T_UnaryUpdateFunc>
auto
    UCk_Utils_Ecs_Net_UE::
    UpdateReplicatedFragment(
        FCk_Handle InHandle,
        T_UnaryUpdateFunc InUpdateFunc)
    -> void
{
    static_assert(std::is_base_of_v<class UCk_Ecs_ReplicatedObject_UE, T_ReplicatedFragment>, "Replicated Fragment MUST derive from UCk_Ecs_ReplicatedObject_UE");

    if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InHandle))
    { return; }

    InHandle.Try_Transform<TObjectPtr<T_ReplicatedFragment>>([InUpdateFunc](TObjectPtr<T_ReplicatedFragment>& InRepComp)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InRepComp), TEXT("Invalid Replicated Fragment TObjectPtr"))
        { return; }

        InUpdateFunc(InRepComp.Get());
    });
}

template <typename T_ReplicatedFragment>
auto
    UCk_Utils_Ecs_Net_UE::
    TryAddReplicatedFragment(
        FCk_Handle InHandle)
    -> ECk_AddedOrNot
{
    static_assert(std::is_base_of_v<class UCk_Ecs_ReplicatedObject_UE, T_ReplicatedFragment>, "Replicated Fragment MUST derive from UCk_Ecs_ReplicatedObject_UE");

    if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InHandle))
    { return ECk_AddedOrNot::NotAdded; }

    if (InHandle.Has<TObjectPtr<T_ReplicatedFragment>>())
    { return ECk_AddedOrNot::AlreadyExists; }

    const auto EntityWithActor = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwnerIf(InHandle, [](FCk_Handle Handle)
    {
        return UCk_Utils_OwningActor_UE::Has(Handle);
    });

    if (ck::Is_NOT_Valid(EntityWithActor))
    { return ECk_AddedOrNot::NotAdded; }

    const auto& BasicDetails   = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(EntityWithActor);
    const auto& OwningActor    = BasicDetails.Get_Actor().Get();
    const auto& OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);

    CK_ENSURE_IF_NOT(ck::IsValid(OutermostActor),
        TEXT("OutermostActor for Actor [{}] with Entity [{}] is [{}]. Does this Entity require replication?"),
        OwningActor, EntityWithActor, OutermostActor)
    { return ECk_AddedOrNot::NotAdded; }

    auto ReplicatedFragment_Object = Cast<T_ReplicatedFragment>(UCk_Ecs_ReplicatedObject_UE::Create
    (
        T_ReplicatedFragment::StaticClass(),
        OutermostActor,
        NAME_None,
        InHandle
    ));

    CK_ENSURE_IF_NOT(ck::IsValid(ReplicatedFragment_Object),
        TEXT("Failed to create Replicated Fragment Object for Entity [{}]"), InHandle)
    { return ECk_AddedOrNot::NotAdded; }

    InHandle.Add<TObjectPtr<T_ReplicatedFragment>>(ReplicatedFragment_Object);

    UCk_Utils_ReplicatedObjects_UE::Request_AddReplicatedObject(InHandle, ReplicatedFragment_Object);

    return ECk_AddedOrNot::Added;
}

// --------------------------------------------------------------------------------------------------------------------
