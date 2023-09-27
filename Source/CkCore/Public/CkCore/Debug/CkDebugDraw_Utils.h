#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkDebugDraw_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_DebugDraw_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DebugDraw_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug")
    static FString
    Create_ASCII_ProgressBar(
        const FCk_FloatRange_0to1& InProgressValue,
        int32 InProgressBarCharacterLength);
};

// --------------------------------------------------------------------------------------------------------------------
