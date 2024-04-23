#pragma once

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Engine/CkPlayerState.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Fragment_Data.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

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
    static auto
    Add(
        FCk_Handle& InEntity,
        const FCk_Net_ConnectionSettings& InConnectionSettings) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Net",
              DisplayName="[Ck] Copy Network Info")
    static void
    Copy(
        const FCk_Handle& InFrom,
        FCk_Handle& InTo);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net",
              DisplayName="[Ck] Has Network Info")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Net",
              DisplayName="[Ck] Ensure Has Network Info")
    static bool
    Ensure(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Entity Net Role",
              Category = "Ck|Utils|Net")
    static ECk_Net_EntityNetRole
    Get_EntityNetRole(
        const FCk_Handle& InEntity);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Entity Net Mode",
              Category = "Ck|Utils|Net")
    static ECk_Net_NetModeType
    Get_EntityNetMode(
        const FCk_Handle& InEntity);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get is Entity Role Matching",
              Category = "Ck|Utils|Net")
    static bool
    Get_IsEntityRoleMatching(
        const FCk_Handle& InEntity,
        ECk_Net_ReplicationType InReplicationType);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Has Entity Authority",
              Category = "Ck|Utils|Net")
    static bool
    Get_HasAuthority(
        const FCk_Handle& InEntity);

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Actor Locally Owned",
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = InActor))
    static bool
    Get_IsActorLocallyOwned(
        AActor* InActor);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Actor Role Matching",
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = InActor))
    static bool
    Get_IsRoleMatching(
        AActor* InActor,
        ECk_Net_ReplicationType InReplicationType);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Net Role",
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static ECk_Net_NetModeType
    Get_NetRole(
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Object Net Mode",
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static ECk_Net_NetModeType
    Get_NetMode(
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Entity Net Mode Host",
              Category = "Ck|Utils|Net")
    static bool
    Get_IsEntityNetMode_Host(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Entity Net Mode Client",
              Category = "Ck|Utils|Net")
    static bool
    Get_IsEntityNetMode_Client(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Entity Replicated",
              Category = "Ck|Utils|Net")
    static ECk_Replication
    Get_EntityReplication(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Min Ping",
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static FCk_Time
    Get_MinPing(
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Max Ping",
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static FCk_Time
    Get_MaxPing(
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Average Ping",
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static FCk_Time
    Get_AveragePing(
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Average Latency",
              Category = "Ck|Utils|Net", meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static FCk_Time
    Get_AverageLatency(
        const UObject* InContext = nullptr);

#if CK_BUILD_TEST
public:
    static auto
    Get_PingRangeHistoryEntries() -> TArray<FCk_PlayerState_PingRange_History_Entry>;
#endif
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
    requires(std::is_base_of_v<class UCk_Ecs_ReplicatedObject_UE, T_ReplicatedFragment>)
    static auto
    TryUpdateReplicatedFragment(
        FCk_Handle& InHandle,
        T_UnaryUpdateFunc InUpdateFunc) -> void;

    template <typename T_ReplicatedFragment>
    requires(std::is_base_of_v<class UCk_Ecs_ReplicatedObject_UE, T_ReplicatedFragment>)
    static auto
    Get_HasReplicatedFragment(
        const FCk_Handle& InHandle) -> bool;

    template <typename T_ReplicatedFragment>
    requires(std::is_base_of_v<class UCk_Ecs_ReplicatedObject_UE, T_ReplicatedFragment>)
    static auto
    TryAddReplicatedFragment(
        FCk_Handle& InHandle,
        UCk_Ecs_ReplicatedObject_UE* InExistingObject = nullptr) -> ECk_AddedOrNot;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_ReplicatedFragment, typename T_UnaryUpdateFunc>
requires(std::is_base_of_v<class UCk_Ecs_ReplicatedObject_UE, T_ReplicatedFragment>)
auto
    UCk_Utils_Ecs_Net_UE::
    TryUpdateReplicatedFragment(
        FCk_Handle& InHandle,
        T_UnaryUpdateFunc InUpdateFunc)
    -> void
{
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
requires(std::is_base_of_v<class UCk_Ecs_ReplicatedObject_UE, T_ReplicatedFragment>)
auto
    UCk_Utils_Ecs_Net_UE::
    Get_HasReplicatedFragment(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<TObjectPtr<T_ReplicatedFragment>>();
}

template <typename T_ReplicatedFragment>
requires(std::is_base_of_v<class UCk_Ecs_ReplicatedObject_UE, T_ReplicatedFragment>)
auto
    UCk_Utils_Ecs_Net_UE::
    TryAddReplicatedFragment(
        FCk_Handle& InHandle,
        UCk_Ecs_ReplicatedObject_UE* InExistingObject)
    -> ECk_AddedOrNot
{
    if (UCk_Utils_Net_UE::Get_EntityReplication(InHandle) == ECk_Replication::DoesNotReplicate)
    { return ECk_AddedOrNot::NotAdded; }

    if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InHandle))
    { return ECk_AddedOrNot::NotAdded; }

    if (InHandle.Has<TObjectPtr<T_ReplicatedFragment>>())
    { return ECk_AddedOrNot::AlreadyExists; }

    const auto EntityWithActor = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwnerIf(InHandle,
    [](const FCk_Handle& Handle)
    {
        return UCk_Utils_OwningActor_UE::Has(Handle);
    });

    if (ck::Is_NOT_Valid(EntityWithActor))
    { return ECk_AddedOrNot::NotAdded; }

    const auto& BasicDetails   = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(EntityWithActor);
    const auto& OwningActor    = BasicDetails.Get_Actor().Get();
    const auto& OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);

    CK_ENSURE_IF_NOT(ck::IsValid(OutermostActor),
        TEXT("Unable to add ReplicatedFragment [{}] for Entity [{}]. We require an Entity with an Actor in the Outer chain that has remote "
            "authority (is Replicated) to be able to add replicated fragments. During this search, we found the Entity [{}] with OwningActor [{}] "
            "that does NOT have remote authority, nor does any owning Actor in the ownership chain. Does this Entity even require replication?"),
        ck::Get_RuntimeTypeToString<T_ReplicatedFragment>(), InHandle,  EntityWithActor, OwningActor)
    { return ECk_AddedOrNot::NotAdded; }

    auto ReplicatedFragment_Object = [&]()
    {
        if (ck::IsValid(InExistingObject))
        {
            UCk_Ecs_ReplicatedObject_UE::Setup(InExistingObject, OutermostActor, InHandle);
            return Cast<T_ReplicatedFragment>(InExistingObject);
        }

        return Cast<T_ReplicatedFragment>(UCk_Ecs_ReplicatedObject_UE::Create
        (
            T_ReplicatedFragment::StaticClass(),
            OutermostActor,
            NAME_None,
            InHandle
        ));
    }();

    CK_ENSURE_IF_NOT(ck::IsValid(ReplicatedFragment_Object),
        TEXT("Failed to create (or convert from [{}]) Replicated Fragment Object for Entity [{}]"), InExistingObject, InHandle)
    { return ECk_AddedOrNot::NotAdded; }

    InHandle.Add<TObjectPtr<T_ReplicatedFragment>>(ReplicatedFragment_Object);

    UCk_Utils_ReplicatedObjects_UE::Request_AddReplicatedObject(InHandle, ReplicatedFragment_Object);

    return ECk_AddedOrNot::Added;
}

// --------------------------------------------------------------------------------------------------------------------
