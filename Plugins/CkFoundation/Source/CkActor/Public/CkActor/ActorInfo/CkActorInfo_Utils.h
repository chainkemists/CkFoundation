#pragma once

#include "CkMacros/CkMacros.h"

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkActorInfo_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKACTOR_API UCk_Utils_ActorInfo_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ActorInfo_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ActorInfo",
              DisplayName = "Add Entity Actor")
    static void
    Add(
        FCk_Handle                                InHandle,
        const FCk_Fragment_ActorInfo_ParamsData& InParamsData);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ActorInfo",
              DisplayName = "Has Entity Actor")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ActorInfo",
              DisplayName = "Ensure Has Entity Actor")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ActorInfo")
    static FCk_ActorInfo_BasicDetails
    Get_ActorInfoBasicDetails(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ActorInfo")
    static FCk_ActorInfo_BasicDetails
    Get_ActorInfoBasicDetails_FromActor(
        const AActor* InActor);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ActorInfo",
        meta = (DefaultToSelf = "InActor"))
    static FCk_Handle
    Get_ActorEcsHandle(
        const AActor* InActor);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ActorInfo",
        meta = (DefaultToSelf = "InActor"))
    static bool
    Get_IsActorEcsReady(
        AActor* InActor);
};

// --------------------------------------------------------------------------------------------------------------------
