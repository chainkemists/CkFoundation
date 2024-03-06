#pragma once
#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsEnsure.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKECS_API UCk_Utils_EcsEnsure_UE
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ensure",
              DisplayName = "[Ck] Ensure IsValid Entity",
              meta     = (DevelopmentOnly, ExpandEnumAsExecs = "OutHitStatus", DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    EnsureMsgf_IsValid(
        const FCk_Handle& InHandle,
        FText InMsg,
        ECk_ValidInvalid& OutHitStatus,
        const UObject* InContext = nullptr);
};

// --------------------------------------------------------------------------------------------------------------------
