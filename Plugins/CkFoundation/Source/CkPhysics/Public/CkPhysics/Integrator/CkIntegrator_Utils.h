#pragma once

#include "CkIntegrator_Fragment_Params.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkMacros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkIntegrator_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_Integrator_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Integrator_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Integrator",
              DisplayName="Request Start Integration")
    static void
    Request_Start(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Integrator",
              DisplayName="Request Stop Integration")
    static void
    Request_Stop(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
