#pragma once

#include "CkEulerIntegrator_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
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
              DisplayName="[Ck][EulerIntegration] Request Start",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_Start(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EulerIntegrator",
              DisplayName="[Ck][EulerIntegration] Request Stop",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_Stop(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
