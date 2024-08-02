#include "CkScreen_Utils.h"

#include <GameFramework/PlayerController.h>
#include <Blueprint/SlateBlueprintLibrary.h>
#include <GameFramework/GameUserSettings.h>
#include "Engine/Engine.h"
#include <Components/Viewport.h>

#include "CkCore/Ensure/CkEnsure.h"
#include "Kismet/GameplayStatics.h"

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

    auto OutPixelLocation = FVector{};
    constexpr auto bPlayerViewportRelative = false;

    if (const auto& ProjectionSuccess = InPlayerController->ProjectWorldLocationToScreenWithDistance(InWorldLocation, OutPixelLocation, bPlayerViewportRelative);
        NOT ProjectionSuccess)
    { return false; }

    const auto& ScreenPosition = FVector2D(FMath::RoundToInt(OutPixelLocation.X), FMath::RoundToInt(OutPixelLocation.Y));

    auto OutViewportPosition2D = FVector2D{};

    USlateBlueprintLibrary::ScreenToViewport(InPlayerController, ScreenPosition, OutViewportPosition2D);

    OutViewportPosition.X = OutViewportPosition2D.X;
    OutViewportPosition.Y = OutViewportPosition2D.Y;

    return true;
}

auto
    UCk_Utils_Screen_UE::
    FindScreenEdgeLocationForWorldLocation(
        UObject* InWorldContextObject,
        FVector InLocation,
        float InEdgePercent,
        FVector2D InViewportCenterLoc,
        FVector2D& OutScreenPosition,
        float& OutRotationAngleDegrees,
        bool &OutIsOnScreen)
    -> void
{
    OutIsOnScreen = false;
    OutRotationAngleDegrees = 0.f;

    if(ck::Is_NOT_Valid(GEngine))
    { return; }

    if(const auto& World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
        ck::Is_NOT_Valid(World))
    { return; }

    const auto& PlayerController = InWorldContextObject ? UGameplayStatics::GetPlayerController(InWorldContextObject, 0) : nullptr;
    if(ck::Is_NOT_Valid(PlayerController))
    { return; }

    const auto& ViewportSize = DoGet_PlayerControllerViewportSize(PlayerController);
    const auto& ViewportCenter = FVector2D(ViewportSize.X * InViewportCenterLoc.X, ViewportSize.Y * InViewportCenterLoc.Y);

    auto OutCameraLoc = FVector{};
    auto OutCameraRot = FRotator{};

    PlayerController->GetPlayerViewPoint(OutCameraLoc, OutCameraRot);

    const auto& CameraToLoc = InLocation - OutCameraLoc;
    const auto& ForwardVec = OutCameraRot.Vector();
    const auto& Offset = CameraToLoc.GetSafeNormal();

    const auto& DotProduct = FVector::DotProduct(ForwardVec, Offset);
    const auto& IsLocationBehindCamera = DotProduct < 0;

    auto OutTempScreenPosition = FVector2D{};

    if (IsLocationBehindCamera)
    {
        const auto& invertedCameraToLoc = CameraToLoc * -1.f;
        const auto& newWorldLocationToProject = OutCameraLoc + invertedCameraToLoc;

        ProjectWorldLocationToWidgetPositionCoords(PlayerController, newWorldLocationToProject, OutTempScreenPosition);

        OutTempScreenPosition.X = ViewportSize.X - OutTempScreenPosition.X;
        OutTempScreenPosition.Y = ViewportSize.Y - OutTempScreenPosition.Y;
    }
    else
    {
        ProjectWorldLocationToWidgetPositionCoords(PlayerController, InLocation, OutTempScreenPosition);
    }

    // Check to see if it's on screen. If it is, ProjectWorldLocationToWidgetPositionCoords is all we need, return it.
    if (OutTempScreenPosition.X >= 0.f && OutTempScreenPosition.X <= ViewportSize.X && OutTempScreenPosition.Y >= 0.f && OutTempScreenPosition.Y <= ViewportSize.Y && NOT IsLocationBehindCamera)
    {
        OutScreenPosition = OutTempScreenPosition;
        OutIsOnScreen = true;
        return;
    }

    OutTempScreenPosition -= ViewportCenter;

    const auto& AngleRadians = FMath::Atan2(OutTempScreenPosition.Y, OutTempScreenPosition.X) - FMath::DegreesToRadians(90.f);

    OutRotationAngleDegrees = FMath::RadiansToDegrees(AngleRadians) + 180.f;

    const auto& Cos = cosf(AngleRadians);
    const auto& Sin = -sinf(AngleRadians);

    OutTempScreenPosition = FVector2D(ViewportCenter.X + (Sin * 150.f), ViewportCenter.Y + Cos * 150.f);

    const auto& Cotangent = Cos / Sin;

    const auto& ClampedEdgePercent = FMath::Clamp(InEdgePercent, 0.0f, 1.0f);
    const auto& ScreenBounds = FVector2D(ViewportCenter * ClampedEdgePercent);

    if (Cos > 0)
    {
        OutTempScreenPosition = FVector2D(ScreenBounds.Y / Cotangent, ScreenBounds.Y);
    }
    else
    {
        OutTempScreenPosition = FVector2D(-ScreenBounds.Y / Cotangent, -ScreenBounds.Y);
    }

    if (OutTempScreenPosition.X > ScreenBounds.X)
    {
        OutTempScreenPosition = FVector2D(ScreenBounds.X, ScreenBounds.X * Cotangent);
    }
    else if (OutTempScreenPosition.X < -ScreenBounds.X)
    {
        OutTempScreenPosition = FVector2D(-ScreenBounds.X, -ScreenBounds.X * Cotangent);
    }

    OutTempScreenPosition += ViewportCenter;

    OutScreenPosition = OutTempScreenPosition;
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
        auto OutScreenSizeX = 0;
        auto OutScreenSizeY = 0;

        InPlayerController->GetViewportSize(OutScreenSizeX, OutScreenSizeY);

        return FVector2D(OutScreenSizeX, OutScreenSizeY);
    }();

    const auto& ViewportSize = [&]()
    {
        auto OutViewportSize = FVector2D{};
        USlateBlueprintLibrary::ScreenToViewport(InPlayerController, ScreenViewportSize, OutViewportSize);

        return OutViewportSize;
    }();

    return ViewportSize;
}

// --------------------------------------------------------------------------------------------------------------------
