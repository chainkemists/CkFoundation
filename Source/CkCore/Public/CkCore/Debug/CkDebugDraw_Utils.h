#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkLog/CkLog_Category.h"
#include "CkLog/CkLog_Utils.h"

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

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Sphere",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugSphere(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InCenter,
        float InRadius = 100.0f,
        int32 InSegments = 12,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 5.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Line",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugLine(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InLineStart,
        const FVector InLineEnd,
        FLinearColor InLineColor,
        float InDuration = 0.0f,
        float InThickness = 5.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Circle",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugCircle(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        FVector InCenter,
        float InRadius,
        int32 InNumSegments = 12,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 5.0f,
        FVector InYAxis = FVector(0.f,1.f,0.f),
        FVector InZAxis = FVector(0.f,0.f,1.f),
        bool InDrawAxis = false);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Point",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugPoint(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InPosition,
        float InSize,
        FLinearColor InPointColor,
        float InDuration = 0.f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Arrow",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugArrow(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InLineStart,
        const FVector InLineEnd,
        float InArrowSize,
        FLinearColor InLineColor,
        float InDuration = 0.0f,
        float InThickness = 5.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Box",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugBox(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InCenter,
        FVector InExtent,
        FLinearColor InLineColor,
        const FRotator InRotation = FRotator::ZeroRotator,
        float InDuration = 0.0f,
        float InThickness = 5.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Cylinder",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugCylinder(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InStart,
        const FVector InEnd,
        float InRadius = 100.f,
        int32 InSegments = 12,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 5.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Cone",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugCone(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InOrigin,
        const FVector InDirection,
        float InLength,
        float InAngleWidth,
        float InAngleHeight,
        int32 InNumSides,
        FLinearColor InLineColor,
        float InDuration = 0.0f,
        float InThickness = 5.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Capsule",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugCapsule(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InCenter,
        float InHalfHeight,
        float InRadius,
        const FRotator InRotation,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 5.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug String",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugString(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InTextLocation,
        const FString& InText,
        class AActor* InTestBaseActor = nullptr,
        FLinearColor InTextColor = FLinearColor::White,
        float InDuration = 0.f);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_DebugDraw_Screen_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DebugDraw_Screen_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Rect (On Screen)",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugRect_OnScreen(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FBox2D& InRect,
        FLinearColor InRectColor = FLinearColor::White);
};

// --------------------------------------------------------------------------------------------------------------------
