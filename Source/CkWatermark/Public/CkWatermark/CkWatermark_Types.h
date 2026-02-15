#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkWatermark_Types.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Watermark_StatQuality : uint8
{
    VeryGood,
    Good,
    Okay,
    Bad,
    VeryBad
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Watermark_StatQuality);

// --------------------------------------------------------------------------------------------------------------------
// For "higher is better" stats: FPS, ServerFPS
// Evaluated as: value >= threshold → quality band

USTRUCT(BlueprintType)
struct CKWATERMARK_API FCk_Watermark_ColorBands_HigherIsBetter
{
    GENERATED_BODY()

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    float VeryGood_Threshold = 60.f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    FLinearColor VeryGood_Color = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    float Good_Threshold = 45.f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    FLinearColor Good_Color = FLinearColor(0.5f, 1.0f, 0.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    float Okay_Threshold = 30.f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    FLinearColor Okay_Color = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    float Bad_Threshold = 20.f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    FLinearColor Bad_Color = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);

    // Below Bad_Threshold
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    FLinearColor VeryBad_Color = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

    FLinearColor GetColorForValue(float InValue) const;
};

// --------------------------------------------------------------------------------------------------------------------
// For "lower is better" stats: Ping, EnsureCount
// Evaluated as: value <= threshold → quality band

USTRUCT(BlueprintType)
struct CKWATERMARK_API FCk_Watermark_ColorBands_LowerIsBetter
{
    GENERATED_BODY()

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    float VeryGood_Threshold = 30.f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    FLinearColor VeryGood_Color = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    float Good_Threshold = 60.f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    FLinearColor Good_Color = FLinearColor(0.5f, 1.0f, 0.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    float Okay_Threshold = 100.f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    FLinearColor Okay_Color = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    float Bad_Threshold = 150.f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    FLinearColor Bad_Color = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);

    // Above Bad_Threshold
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Watermark")
    FLinearColor VeryBad_Color = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

    FLinearColor GetColorForValue(float InValue) const;
};

// --------------------------------------------------------------------------------------------------------------------
