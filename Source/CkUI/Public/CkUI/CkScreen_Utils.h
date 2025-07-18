#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include "CkScreen_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_LinePlaneIntersectionStatus : uint8
{
    Success,
    InvalidPlayerController,
    NoIntersectionFound
};

USTRUCT(BlueprintType)
struct CKUI_API FCk_LinePlaneIntersectionResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LinePlaneIntersectionResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_LinePlaneIntersectionStatus _Status;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _IntersectionPoint;

public:
    CK_PROPERTY(_Status);
    CK_PROPERTY(_IntersectionPoint);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_LinePlaneIntersectionResult, _Status, _IntersectionPoint);
};

USTRUCT(BlueprintType)
struct CKUI_API FCk_ScreenEdgeLocationResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ScreenEdgeLocationResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector2D _ScreenPosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _RotationAngleDegrees;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    bool _IsOnScreen;

public:
    CK_PROPERTY(_ScreenPosition);
    CK_PROPERTY(_RotationAngleDegrees);
    CK_PROPERTY(_IsOnScreen);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ScreenEdgeLocationResult, _ScreenPosition, _RotationAngleDegrees, _IsOnScreen);
};

UCLASS(NotBlueprintable)
class CKUI_API UCk_Utils_Screen_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Screen_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Project World Location To Widget Position Coords",
              Category = "Ck|Utils|UI")
    static bool
    ProjectWorldLocationToWidgetPositionCoords(
        APlayerController* InPlayerController,
        FVector InWorldLocation,
        FVector2D& OutViewportPosition);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Find Screen Edge Location For World Location",
              Category = "Ck|Utils|UI",
              meta=(WorldContext = "InWorldContextObject", CallableWithoutWorldContext))
    static FCk_ScreenEdgeLocationResult
    Request_ScreenEdgeLocationForWorldLocation(
        UObject* InWorldContextObject,
        FVector InWorldLocation,
        FCk_FloatRange_0to1 InEdgeDistanceFromCenter,
        FVector2D InViewportCenterOffset);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Line Plane Intersection From Mouse (Player Controller)",
              Category = "Ck|Utils|UI")
    static FCk_LinePlaneIntersectionResult
    Request_LinePlaneIntersectionFromMouse_WithPlayerController(
        APlayerController* InPlayerController,
        FVector InPlaneOrigin,
        FVector InPlaneNormal);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Line Plane Intersection From Mouse (World Context)",
              Category = "Ck|Utils|UI",
              meta=(WorldContext = "InWorldContextObject"))
    static FCk_LinePlaneIntersectionResult
    Request_LinePlaneIntersectionFromMouse_WithWorldContext(
        UObject* InWorldContextObject,
        FVector InPlaneOrigin,
        FVector InPlaneNormal);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Line Plane Intersection From Mouse (Entity)",
              Category = "Ck|Utils|UI")
    static FCk_LinePlaneIntersectionResult
    Request_LinePlaneIntersectionFromMouse_WithEntity(
        const FCk_Handle& InHandle,
        FVector InPlaneOrigin,
        FVector InPlaneNormal);

private:
    static auto DoGet_PlayerControllerViewportSize(
        APlayerController* InPlayerController) -> FVector2D;

    static auto DoRequest_LinePlaneIntersectionFromMouse_Internal(
        APlayerController* InPlayerController,
        FVector InPlaneOrigin,
        FVector InPlaneNormal) -> FCk_LinePlaneIntersectionResult;
};

// --------------------------------------------------------------------------------------------------------------------
