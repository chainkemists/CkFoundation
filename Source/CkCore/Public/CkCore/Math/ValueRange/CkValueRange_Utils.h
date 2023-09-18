#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkValueRange_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_IntRange_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IntRange_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|IntRange")
    static bool
    Get_IsWithinRange(
        int32 InValue,
        const FCk_IntRange& InRange,
        ECk_Inclusiveness InInclusiveness);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|IntRange")
    static int32
    Get_RandomValueInRange(
        const FCk_IntRange& InRange);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|IntRange",
              meta = (DisplayName = "ToVector2D", BlueprintAutocast,  CompactNodeTitle = "->"))
    static FVector2D
    Conv_IntRangeToVector2D(
        const FCk_IntRange& InRange);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_FloatRange_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatRange_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|FloatRange")
    static bool
    Get_IsWithinRange(
        float InValue,
        const FCk_FloatRange& InRange,
        ECk_Inclusiveness InInclusiveness);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|FloatRange")
    static int32
    Get_RandomValueInRange(
        const FCk_FloatRange& InRange);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|FloatRange",
              meta = (DisplayName = "ToVector2D", BlueprintAutocast,  CompactNodeTitle = "->"))
    static FVector2D
    Conv_FloatRangeToVector2D(
        const FCk_FloatRange& InRange);
};

// --------------------------------------------------------------------------------------------------------------------
