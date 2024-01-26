#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Time/CkTime.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCurve_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Curve_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Curve_UE);

public:
    static auto
    Get_ValueAtTime(
        const FRichCurve& InRichCurve,
        const FCk_Time& InTime) -> float;

    static auto
    Get_TimeRange(
        const FRichCurve& InRichCurve) -> FCk_FloatRange;

    static auto
    Get_ValueRange(
        const FRichCurve& InRichCurve) -> FCk_FloatRange;
};

// --------------------------------------------------------------------------------------------------------------------
