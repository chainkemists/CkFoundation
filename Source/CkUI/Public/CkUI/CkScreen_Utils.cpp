#include "CkScreen_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include <Blueprint/SlateBlueprintLibrary.h>
#include <Engine/Engine.h>
#include <GameFramework/GameUserSettings.h>
#include <GameFramework/PlayerController.h>
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetMathLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Screen_UE::
    ProjectWorldLocationToWidgetPositionCoords(
        APlayerController* InPlayerController,
        FVector InWorldLocation,
        FVector2D& OutViewportPosition)
    -> bool
{
    OutViewportPosition = FVector2D::ZeroVector;

    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid PlayerController"))
    { return false; }

    auto PixelLocation = FVector{};
    constexpr auto PlayerViewportRelative = false;

    if (const auto& ProjectionSuccess = InPlayerController->ProjectWorldLocationToScreenWithDistance(InWorldLocation, PixelLocation, PlayerViewportRelative);
        NOT ProjectionSuccess)
    { return false; }

    const auto& ScreenPosition = FVector2D(FMath::RoundToInt(PixelLocation.X), FMath::RoundToInt(PixelLocation.Y));

    auto ViewportPosition2D = FVector2D{};

    USlateBlueprintLibrary::ScreenToViewport(InPlayerController, ScreenPosition, ViewportPosition2D);

    OutViewportPosition.X = ViewportPosition2D.X;
    OutViewportPosition.Y = ViewportPosition2D.Y;

    return true;
}

auto
    UCk_Utils_Screen_UE::
    Request_ScreenEdgeLocationForWorldLocation(
        UObject* InWorldContextObject,
        FVector InWorldLocation,
        FCk_FloatRange_0to1 InEdgeDistanceFromCenter,
        FVector2D InViewportCenterOffset)
    -> FCk_ScreenEdgeLocationResult
{
    auto Result = FCk_ScreenEdgeLocationResult{};

    if(ck::Is_NOT_Valid(GEngine))
    { return Result; }

    if(const auto& World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
        ck::Is_NOT_Valid(World))
    { return Result; }

    const auto& PlayerController = InWorldContextObject ? UGameplayStatics::GetPlayerController(InWorldContextObject, 0) : nullptr;
    if(ck::Is_NOT_Valid(PlayerController))
    { return Result; }

    const auto& ViewportSize = DoGet_PlayerControllerViewportSize(PlayerController);
    const auto& ViewportCenter = FVector2D(ViewportSize.X * InViewportCenterOffset.X, ViewportSize.Y * InViewportCenterOffset.Y);

    auto CameraLoc = FVector{};
    auto CameraRot = FRotator{};

    PlayerController->GetPlayerViewPoint(CameraLoc, CameraRot);

    const auto& CameraToLoc = InWorldLocation - CameraLoc;
    const auto& ForwardVec = CameraRot.Vector();
    const auto& Offset = CameraToLoc.GetSafeNormal();

    const auto& DotProduct = FVector::DotProduct(ForwardVec, Offset);
    const auto& IsLocationBehindCamera = DotProduct < 0;

    auto TempScreenPosition = FVector2D{};

    if (IsLocationBehindCamera)
    {
        const auto& InvertedCameraToLoc = CameraToLoc * -1.f;
        const auto& NewWorldLocationToProject = CameraLoc + InvertedCameraToLoc;

        ProjectWorldLocationToWidgetPositionCoords(PlayerController, NewWorldLocationToProject, TempScreenPosition);

        TempScreenPosition.X = ViewportSize.X - TempScreenPosition.X;
        TempScreenPosition.Y = ViewportSize.Y - TempScreenPosition.Y;
    }
    else
    {
        ProjectWorldLocationToWidgetPositionCoords(PlayerController, InWorldLocation, TempScreenPosition);
    }

    if (TempScreenPosition.X >= 0.f && TempScreenPosition.X <= ViewportSize.X && TempScreenPosition.Y >= 0.f && TempScreenPosition.Y <= ViewportSize.Y && NOT IsLocationBehindCamera)
    {
        Result.Set_ScreenPosition(TempScreenPosition);
        Result.Set_IsOnScreen(true);
        return Result;
    }

    TempScreenPosition -= ViewportCenter;

    const auto& AngleRadians = FMath::Atan2(TempScreenPosition.Y, TempScreenPosition.X) - FMath::DegreesToRadians(90.f);

    Result.Set_RotationAngleDegrees(FMath::RadiansToDegrees(AngleRadians) + 180.f);

    const auto& Cos = cosf(AngleRadians);
    const auto& Sin = -sinf(AngleRadians);

    TempScreenPosition = FVector2D(ViewportCenter.X + (Sin * 150.f), ViewportCenter.Y + Cos * 150.f);

    const auto& Cotangent = Cos / Sin;

    const auto& ClampedEdgePercent = InEdgeDistanceFromCenter.Get_ClampedValue(InEdgeDistanceFromCenter.Get_Value());
    const auto& ScreenBounds = FVector2D(ViewportCenter * ClampedEdgePercent);

    if (Cos > 0)
    {
        TempScreenPosition = FVector2D(ScreenBounds.Y / Cotangent, ScreenBounds.Y);
    }
    else
    {
        TempScreenPosition = FVector2D(-ScreenBounds.Y / Cotangent, -ScreenBounds.Y);
    }

    if (TempScreenPosition.X > ScreenBounds.X)
    {
        TempScreenPosition = FVector2D(ScreenBounds.X, ScreenBounds.X * Cotangent);
    }
    else if (TempScreenPosition.X < -ScreenBounds.X)
    {
        TempScreenPosition = FVector2D(-ScreenBounds.X, -ScreenBounds.X * Cotangent);
    }

    TempScreenPosition += ViewportCenter;

    Result.Set_ScreenPosition(TempScreenPosition);

    return Result;
}

auto
    UCk_Utils_Screen_UE::
    Request_LinePlaneIntersectionFromMouse_WithPlayerController(
        APlayerController* InPlayerController,
        FVector InPlaneOrigin,
        FVector InPlaneNormal)
    -> FCk_LinePlaneIntersectionResult
{
    return DoRequest_LinePlaneIntersectionFromMouse_Internal(InPlayerController, InPlaneOrigin, InPlaneNormal);
}

auto
    UCk_Utils_Screen_UE::
    Request_LinePlaneIntersectionFromMouse_WithWorldContext(
        UObject* InWorldContextObject,
        FVector InPlaneOrigin,
        FVector InPlaneNormal)
    -> FCk_LinePlaneIntersectionResult
{
    if(ck::Is_NOT_Valid(InWorldContextObject))
    { return {}; }

    const auto& PlayerController = UGameplayStatics::GetPlayerController(InWorldContextObject, 0);

    return DoRequest_LinePlaneIntersectionFromMouse_Internal(PlayerController, InPlaneOrigin, InPlaneNormal);
}

auto
    UCk_Utils_Screen_UE::
    Request_LinePlaneIntersectionFromMouse_WithEntity(
        const FCk_Handle& InHandle,
        FVector InPlaneOrigin,
        FVector InPlaneNormal)
    -> FCk_LinePlaneIntersectionResult
{
    const auto& World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
    if(ck::Is_NOT_Valid(World))
    { return {}; }

    const auto& PlayerController = UGameplayStatics::GetPlayerController(World, 0);

    return DoRequest_LinePlaneIntersectionFromMouse_Internal(PlayerController, InPlaneOrigin, InPlaneNormal);
}

auto
    UCk_Utils_Screen_UE::
    DoGet_PlayerControllerViewportSize(
        APlayerController* InPlayerController)
    -> FVector2D
{
    if(ck::Is_NOT_Valid(InPlayerController))
    { return {}; }

    const auto& ScreenViewportSize = [&]()
    {
        auto ScreenSizeX = 0;
        auto ScreenSizeY = 0;

        InPlayerController->GetViewportSize(ScreenSizeX, ScreenSizeY);

        return FVector2D(ScreenSizeX, ScreenSizeY);
    }();

    const auto& ViewportSize = [&]()
    {
        auto Size = FVector2D{};
        USlateBlueprintLibrary::ScreenToViewport(InPlayerController, ScreenViewportSize, Size);

        return Size;
    }();

    return ViewportSize;
}

auto
    UCk_Utils_Screen_UE::
    DoRequest_LinePlaneIntersectionFromMouse_Internal(
        APlayerController* InPlayerController,
        FVector InPlaneOrigin,
        FVector InPlaneNormal)
    -> FCk_LinePlaneIntersectionResult
{
    auto Result = FCk_LinePlaneIntersectionResult{};

    if(ck::Is_NOT_Valid(InPlayerController))
    {
        Result.Set_Status(ECk_LinePlaneIntersectionStatus::InvalidPlayerController);
        return Result;
    }

    // Convert mouse position to world space
    auto WorldLocation = FVector{};
    auto WorldDirection = FVector{};

    if(NOT InPlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        Result.Set_Status(ECk_LinePlaneIntersectionStatus::InvalidPlayerController);
        return Result;
    }

    // Perform line-plane intersection
    auto IntersectionT = 0.0f;
    auto IntersectionPoint = FVector{};
    const auto& IntersectionFound = UKismetMathLibrary::LinePlaneIntersection_OriginNormal(
        WorldLocation,
        WorldLocation + (WorldDirection * 100000.0f),
        InPlaneOrigin,
        InPlaneNormal,
        IntersectionT,
        IntersectionPoint
    );

    if(NOT IntersectionFound)
    {
        Result.Set_Status(ECk_LinePlaneIntersectionStatus::NoIntersectionFound);
        return Result;
    }

    Result.Set_Status(ECk_LinePlaneIntersectionStatus::Success);
    Result.Set_IntersectionPoint(IntersectionPoint);

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
