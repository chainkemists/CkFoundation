#pragma once
#include "CkActorModifier_Fragment_Params.h"

#include "CkMacros/CkMacros.h"

#include "CkActorModifier_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKACTOR_API UCk_Utils_ActorModifier_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ActorModifier_UE);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier|Requests",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetLocation(
        FCk_Handle                                   InHandle,
        const FCk_Request_ActorModifier_SetLocation& InRequest,
        const FCk_Delegate_Transform_OnUpdate&       InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier|Requests",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_AddLocationOffset(
        FCk_Handle                                         InHandle,
        const FCk_Request_ActorModifier_AddLocationOffset& InRequest,
        const FCk_Delegate_Transform_OnUpdate&             InDelegate);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier|Requests",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetRotation(
        FCk_Handle                                   InHandle,
        const FCk_Request_ActorModifier_SetRotation& InRequest,
        const FCk_Delegate_Transform_OnUpdate&       InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier|Requests",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_AddRotationOffset(
        FCk_Handle                                         InHandle,
        const FCk_Request_ActorModifier_AddRotationOffset& InRequest,
        const FCk_Delegate_Transform_OnUpdate&             InDelegate);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier|Requests",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetScale(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SetScale& InRequest,
        const FCk_Delegate_Transform_OnUpdate& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier|Requests",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SetTransform(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SetTransform& InRequest,
        const FCk_Delegate_Transform_OnUpdate&        InDelegate);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier|Requests",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SpawnActor(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SpawnActor& InRequest,
        const FCk_Delegate_ActorModifier_OnActorSpawned& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier|Requests",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_AddActorComponent(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_AddActorComponent& InRequest,
        const FCk_Delegate_ActorModifier_OnActorComponentAdded& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
