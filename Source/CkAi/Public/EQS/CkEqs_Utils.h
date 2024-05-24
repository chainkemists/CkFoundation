#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>
#include <CoreMinimal.h>

#include "CkMacros.h"
#include "CkEqs_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UEnvQueryInstanceBlueprintWrapper;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKAI_API UCk_Utils_Eqs_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Eqs_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "AI|EQS",
              DisplayName = "Set EQS Named Integer Param")
    static void
    SetEqsNamedIntParam(
        UEnvQueryInstanceBlueprintWrapper* EnvQueryInstance, 
        FName ParamName,
        int Value);
};

// --------------------------------------------------------------------------------------------------------------------
