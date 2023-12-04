#pragma once

#include "CkEcs/OwningActor/CkOwningActor_Fragment_Params.h"
#include <CkCore/Macros/CkMacros.h>

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
        FCk_Handle InHandle,
        AActor* InOwningActor);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "Has Entity Owning Actor")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor",
              DisplayName = "Ensure Has Entity Owning Actor")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor")
    static AActor*
    Get_EntityOwningActor(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor")
    static FCk_EntityOwningActor_BasicDetails
    Get_EntityOwningActorBasicDetails(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|OwningActor")
    static FCk_EntityOwningActor_BasicDetails
    Get_EntityOwningActorBasicDetails_FromActor(
        const AActor* InActor);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|OwningActor",
        meta=(CompactNodeTitle="ActorEntityHandle"))
    static FCk_Handle
    Get_ActorEntityHandle(
        const AActor* InActor);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|OwningActor",
        meta = (DefaultToSelf = "InActor"))
    static bool
    Get_IsActorEcsReady(
        AActor* InActor);

private:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|OwningActor",
        meta = (DefaultToSelf = "InActor", HidePin = "InActor",
            CompactNodeTitle="ActorEntityHandleSelf"))
    static FCk_Handle
    Get_ActorEntityHandleFromSelf(
        const AActor* InActor);
};

// --------------------------------------------------------------------------------------------------------------------
