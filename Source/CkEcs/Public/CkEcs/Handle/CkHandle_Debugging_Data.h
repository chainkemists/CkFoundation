#pragma once

#include "CkHandle_Debugging_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKECS_API UCk_Handle_FragmentsDebug : public UObject
{
    GENERATED_BODY()

    friend struct FCk_Handle;

private:
    UPROPERTY(BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    bool _IsHost = false;

    UPROPERTY(BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    FName _DebugName;

    UPROPERTY(BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    FName _LifetimeTag;

    UPROPERTY(BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    TArray<FName> _Tags;

    UPROPERTY(BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    TArray<FName> _Names;
};

// --------------------------------------------------------------------------------------------------------------------
