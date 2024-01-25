#pragma once

#include "CkEulerIntegrator_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkEulerIntegrator_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_EulerIntegrator_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EulerIntegrator_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EulerIntegrator",
              DisplayName="[Ck][EulerIntegration] Request Start")
    static void
    Request_Start(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EulerIntegrator",
              DisplayName="[Ck][EulerIntegration] Request Stop")
    static void
    Request_Stop(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
