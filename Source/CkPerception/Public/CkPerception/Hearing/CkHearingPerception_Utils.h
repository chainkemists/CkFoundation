#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCore/Macros/CkMacros.h"
#include "CkPerception/Hearing/CkHearingPerception_Params.h"

#include "CkHearingPerception_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPERCEPTION_API UCk_Utils_HearingPerception_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_HearingPerception_UE);

public:
    UFUNCTION(BlueprintPure,
              meta = (CompactNodeTitle = "==", KeyWords = "==,equal"))
    static bool
    Get_NoiseEvent_IsEqual(
        const FCk_HearingPerception_NoiseEvent& InNoiseEventA,
        const FCk_HearingPerception_NoiseEvent& InNoiseEventB);

    UFUNCTION(BlueprintPure,
           meta = (CompactNodeTitle = "!=", KeyWords = "!=,not,equal"))
    static bool
    Get_NoiseEvent_IsNotEqual(
        const FCk_HearingPerception_NoiseEvent& InNoiseEventA,
        const FCk_HearingPerception_NoiseEvent& InNoiseEventB);
};

// --------------------------------------------------------------------------------------------------------------------
