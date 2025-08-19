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

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Sphere",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugSphere(
        const UObject* InWorldContextObject,
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
        const FVector InLineStart,
        const FVector InLineEnd,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 5.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Circle",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugCircle(
        const UObject* InWorldContextObject,
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
        const FVector InLineStart,
        const FVector InLineEnd,
        float InArrowSize,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 5.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Box",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugBox(
        const UObject* InWorldContextObject,
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
        const FVector InOrigin,
        const FVector InDirection,
        float InLength,
        float InAngleWidth,
        float InAngleHeight,
        int32 InNumSides,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 5.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Capsule",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugCapsule(
        const UObject* InWorldContextObject,
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
        const FVector InTextLocation,
        const FString& InText,
        class AActor* InTestBaseActor = nullptr,
        FLinearColor InTextColor = FLinearColor::White,
        float InDuration = 0.f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Big Arrow",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugBigArrow(
        const UObject* InWorldContextObject,
        const FVector InLocation,
        const FRotator InDirection,
        const FLinearColor InArrowColor = FLinearColor::White,
        float InArrowLineHeight = 100.f,
        float InArrowLineWidth = 15.f,
        float InThickness = 1.5f,
        float InDuration = 0.f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Transform Gizmo",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugTransformGizmo(
        const UObject* InWorldContextObject,
        const FTransform& InTransform,
        float InAxisLength = 100.0f,
        float InAxisThickness = 3.0f,
        bool InDrawAxisCones = true,
        float InConeSize = 10.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Coordinate System",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugCoordinateSystem(
        const UObject* InWorldContextObject,
        const FVector InAxisLoc,
        const FRotator InAxisRot,
        float InScale = 1.0f,
        float InDuration = 0.0f,
        float InThickness = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Plane",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugPlane(
        const UObject* InWorldContextObject,
        const FPlane& InPlaneCoordinates,
        const FVector InLocation,
        float InSize,
        FLinearColor InPlaneColor = FLinearColor::White,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Frustum",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugFrustum(
        const UObject* InWorldContextObject,
        const FTransform& InFrustumTransform,
        FLinearColor InFrustumColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Float History Transform",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugFloatHistoryTransform(
        const UObject* InWorldContextObject,
        const FDebugFloatHistory& InFloatHistory,
        const FTransform& InDrawTransform,
        FVector2D InDrawSize,
        FLinearColor InDrawColor = FLinearColor::White,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Float History Location",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugFloatHistoryLocation(
        const UObject* InWorldContextObject,
        const FDebugFloatHistory& InFloatHistory,
        FVector InDrawLocation,
        FVector2D InDrawSize,
        FLinearColor InDrawColor = FLinearColor::White,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Flush Persistent Debug Lines",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    FlushPersistentDebugLines(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Flush Debug Strings",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    FlushDebugStrings(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Grid",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugGrid(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        const FVector InForward,
        const FVector InRight,
        float InGridSize = 1000.0f,
        int32 InGridLines = 20,
        FLinearColor InGridColor = FLinearColor::Gray,
        float InDuration = 0.0f,
        float InThickness = 1.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Cross",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugCross(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        float InSize = 50.0f,
        FLinearColor InColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 2.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Axis",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugAxis(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        const FVector InDirection,
        float InLength = 100.0f,
        FLinearColor InColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 2.0f,
        bool InDrawArrowHead = true,
        float InArrowSize = 10.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Wireframe Sphere",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugWireframeSphere(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        float InRadius = 100.0f,
        int32 InRings = 8,
        int32 InSegments = 16,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 1.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Triangle",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugTriangle(
        const UObject* InWorldContextObject,
        const FVector InVertex1,
        const FVector InVertex2,
        const FVector InVertex3,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 1.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Polygon",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugPolygon(
        const UObject* InWorldContextObject,
        const TArray<FVector>& InVertices,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 1.0f,
        bool InClosedLoop = true);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Wireframe Box",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugWireframeBox(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        const FVector InExtent,
        const FQuat InRotation,
        FLinearColor InLineColor = FLinearColor::White,
        float InDuration = 0.0f,
        float InThickness = 1.0f);
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
        const FBox2D& InRect,
        FLinearColor InRectColor = FLinearColor::White);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Hollow Rect (On Screen)",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugHollowRect_OnScreen(
        const UObject* InWorldContextObject,
        const FBox2D& InRect,
        FLinearColor InRectColor = FLinearColor::White,
        float InLineThickness = 1.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Line (On Screen)",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugLine_OnScreen(
        const UObject* InWorldContextObject,
        FVector2D InLineStart,
        FVector2D InLineEnd,
        FLinearColor InLineColor = FLinearColor::White,
        float InLineThickness = 1.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Draw Debug Text (On Screen)",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DrawDebugText_OnScreen(
        const UObject* InWorldContextObject,
        FVector2D InTextPosition,
        const FString& InText,
        FLinearColor InTextColor = FLinearColor::White,
        float InTextScale = 1.0f);
};

// --------------------------------------------------------------------------------------------------------------------