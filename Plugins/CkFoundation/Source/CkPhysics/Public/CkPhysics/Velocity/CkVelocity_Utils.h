#pragma once

#include "CkVelocity_Fragment_Params.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkMacros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkVelocity_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_Velocity_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Velocity_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Velocity",
              DisplayName="Add Velocity")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Velocity_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity",
              DisplayName="Has Velocity")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity",
              DisplayName="Ensure Has Velocity")
    static bool
    Ensure(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity")
    static FVector
    Get_CurrentVelocity(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
