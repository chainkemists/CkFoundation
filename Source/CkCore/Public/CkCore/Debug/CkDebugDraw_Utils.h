#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkDebugDraw_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ASCII_ProgressBar_Style : uint8
{
    Equal_Symbol UMETA(DisplayName = "Equal Symbol: ="),
    HashTag_Symbol UMETA(DisplayName = "HashTag Symbol: #"),
    FilledBlock_Symbol UMETA(DisplayName = "Filled Block Symbol: █"),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ASCII_ProgressBar_Style);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_DebugDraw_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DebugDraw_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Create ASCII Progress Bar")
    static FString
    Create_ASCII_ProgressBar(
        const FCk_FloatRange_0to1& InProgressValue,
        int32 InProgressBarCharacterLength,
        ECk_ForwardReverse InForwardOrReverse = ECk_ForwardReverse::Forward,
        ECk_ASCII_ProgressBar_Style InStyle = ECk_ASCII_ProgressBar_Style::FilledBlock_Symbol);
};

// --------------------------------------------------------------------------------------------------------------------
