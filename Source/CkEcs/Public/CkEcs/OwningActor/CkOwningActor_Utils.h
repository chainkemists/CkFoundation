#pragma once

#include "CkOwningActor_Fragment_Data.h"
#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkOwningActor_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
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

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Ensure Has Feature")
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
              DisplayName = "[Ck][OwningActor] Get Is Actor Ecs Ready",
              Category = "Ck|Utils|OwningActor",
              meta = (DefaultToSelf = "InActor"))
    static bool
    Get_IsActorEcsReady(
        const AActor* InActor);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "[Ck][OwningActor] Get Actor To Entity (From Self)",
              meta = (DefaultToSelf = "InActor", HidePin = "InActor", CompactNodeTitle="ActorToEntity_FromSelf"))
    static FCk_Handle
    Get_ActorEntityHandleFromSelf(
        const AActor* InActor);
};

// --------------------------------------------------------------------------------------------------------------------