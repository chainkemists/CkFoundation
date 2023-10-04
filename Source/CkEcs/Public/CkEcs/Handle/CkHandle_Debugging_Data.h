#pragma once

#include "CkHandle_Debugging_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class CKECS_API UCk_Handle_FragmentsDebug : public UObject
{
    GENERATED_BODY()

    friend struct FCk_Handle;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TArray<FName> _Names;
};

// --------------------------------------------------------------------------------------------------------------------
