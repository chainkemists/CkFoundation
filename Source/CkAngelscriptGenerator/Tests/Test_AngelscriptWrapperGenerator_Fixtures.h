#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "Test_AngelscriptWrapperGenerator_Fixtures.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract)
class UCkTest_AngelscriptWrapperGenerator_Library : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Tests",
              meta = (NotInAngelscript))
    static int32
    HiddenFromAngelscript();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Tests")
    static int32
    CallableFromAngelscript();
};

// --------------------------------------------------------------------------------------------------------------------
