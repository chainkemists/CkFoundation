#pragma once

#include "CkOwningActor_Fragment_Data.h"
#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkOwningActor_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "AActor FCk_Handle"))
class CKECS_API UCk_Utils_OwningActor_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_OwningActor_UE);

public:
    static void
    Add(
        FCk_Handle& InHandle,
        AActor* InOwningActor);

    static void
    SetupActorEntityLink(
        FCk_Handle& InHandle,
        AActor* InActor);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Has Feature",
              meta = (ScriptName = "Has_OwningActor"))
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Ensure Has Feature",
              meta = (ScriptName = "Ensure_OwningActor"))
    static bool
    Ensure(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName="[Ck][OwningActor] Try Get Entity With OwningActor In Ownership Chain")
    static FCk_Handle
    TryGet_Entity_OwningActor_InOwnershipChain(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Get Entity To Actor",
              meta = (CompactNodeTitle="EntityToActor"))
    static AActor*
    Get_EntityOwningActor(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Try Get Entity To Actor",
              meta = (CompactNodeTitle="TryEntityToActor"))
    static AActor*
    TryGet_EntityOwningActor(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Get Entity To Actor (Recursive)",
              meta = (CompactNodeTitle="EntityToActor (Recursive)"))
    static AActor*
    TryGet_EntityOwningActor_Recursive(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Get Basic Details (From Entity)",
              meta = (CompactNodeTitle="EntityBasicDetails"))
    static FCk_EntityOwningActor_BasicDetails
    Get_EntityOwningActorBasicDetails(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Get Basic Details (From Actor)",
              meta = (CompactNodeTitle="ActorBasicDetails"))
    static FCk_EntityOwningActor_BasicDetails
    Get_EntityOwningActorBasicDetails_FromActor(
        const AActor* InActor);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Get Actor To Entity",
              meta=(CompactNodeTitle="ActorToEntity"))
    static FCk_Handle
    Get_ActorEntityHandle(
        const AActor* InActor);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Try Get Actor To Entity",
              meta=(CompactNodeTitle="TryActorToEntity"))
    static FCk_Handle
    TryGet_ActorEntityHandle(
        const AActor* InActor);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][OwningActor] Get Is Actor Ecs Ready",
              Category = "Ck|Utils|OwningActor",
              meta = (DefaultToSelf = "InActor"))
    static bool
    Get_IsActorEcsReady(
        const AActor* InActor);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][OwningActor] Promise/Future On Actor Ecs Ready",
              Category = "Ck|Utils|OwningActor",
              meta = (DefaultToSelf = "InActor", Keywords = "bind, wait, when, ready, ecs, future, replicated"))
    static void
    Promise_OnActorEcsReady(
        AActor* InActor,
        const FCk_Delegate_OwningActor_OnEcsReady& InDelegate,
        ECk_ActorEcsReady_Policy InPolicy = ECk_ActorEcsReady_Policy::ValuesReplicated);

    static void
    Promise_OnActorEcsReady(
        AActor* InActor,
        TFunction<void(AActor*, FCk_Handle)> InCallback,
        ECk_ActorEcsReady_Policy InPolicy = ECk_ActorEcsReady_Policy::ValuesReplicated);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][OwningActor] OwningActorBasicDetails == OwningActorBasicDetails",
              Category = "Ck|Utils|OwningActor",
              meta = (CompactNodeTitle = "==", KeyWords = "==,equal"))
    static bool
    IsEqual(
        const FCk_EntityOwningActor_BasicDetails& InBasicDetailsA,
        const FCk_EntityOwningActor_BasicDetails& InBasicDetailsB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][OwningActor] OwningActorBasicDetails != OwningActorBasicDetails",
              Category = "Ck|Utils|OwningActor",
              meta = (CompactNodeTitle = "!=", KeyWords = "!=,not,equal"))
    static bool
    IsNotEqual(
        const FCk_EntityOwningActor_BasicDetails& InBasicDetailsA,
        const FCk_EntityOwningActor_BasicDetails& InBasicDetailsB);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Get Actor To Entity (From Self)",
              meta = (DefaultToSelf = "InActor", HidePin = "InActor", CompactNodeTitle="ActorToEntity_FromSelf"))
    static FCk_Handle
    Get_ActorEntityHandleFromSelf(
        const AActor* InActor);

private:
    friend class UCk_EntityOwningActor_ActorComponent_UE;

    static auto
    DoGetOrAdd_EntityOwningActorComponent(
        AActor* InActor) -> UCk_EntityOwningActor_ActorComponent_UE*;

    static auto
    DoFlush_PendingEcsReady(
        UCk_EntityOwningActor_ActorComponent_UE* InComp,
        AActor* InActor,
        const FCk_Handle& InEntity) -> void;

    static auto
    DoFlush_PendingEcsReady_LinkEstablished(
        UCk_EntityOwningActor_ActorComponent_UE* InComp,
        AActor* InActor,
        const FCk_Handle& InEntity) -> void;

    static auto
    DoFlush_PendingEcsReady_ValuesReplicated(
        UCk_EntityOwningActor_ActorComponent_UE* InComp,
        AActor* InActor,
        const FCk_Handle& InEntity) -> void;

    static auto
    DoGet_ShouldDeferUntilReplicationComplete(
        const FCk_Handle& InEntity) -> bool;

    static auto
    DoBind_ReplicationCompleteTrampoline(
        UCk_EntityOwningActor_ActorComponent_UE* InComp,
        const FCk_Handle& InEntity) -> void;
};

// --------------------------------------------------------------------------------------------------------------------