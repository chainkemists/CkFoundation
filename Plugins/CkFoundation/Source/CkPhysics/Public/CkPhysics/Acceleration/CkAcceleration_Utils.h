#pragma once

#include "CkAcceleration_Fragment_Params.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkMacros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkAcceleration_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_Acceleration_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Acceleration_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Acceleration",
              DisplayName="Add Acceleration")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Acceleration_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration",
              DisplayName="Has Acceleration")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration",
              DisplayName="Ensure Has Acceleration")
    static bool
    Ensure(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration")
    static FVector
    Get_CurrentAcceleration(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
