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
        const FVector&     InWorldLocation,
        FVector2D&         OutViewportPosition)
    -> bool
{
    OutViewportPosition = FVector2D::ZeroVector;

    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid PlayerController"))
    { return false; }

    FVector outPixelLocation;
    constexpr bool playerViewportRelative = false;

    const auto& projectionSuccess = InPlayerController->ProjectWorldLocationToScreenWithDistance(InWorldLocation, outPixelLocation, playerViewportRelative);

    if (NOT projectionSuccess)
    { return false; }

    const auto& ScreenPosition = FVector2D(FMath::RoundToInt(outPixelLocation.X), FMath::RoundToInt(outPixelLocation.Y));

    FVector2D outViewportPosition2D;

    USlateBlueprintLibrary::ScreenToViewport(InPlayerController, ScreenPosition, outViewportPosition2D);

    OutViewportPosition.X = outViewportPosition2D.X;
    OutViewportPosition.Y = outViewportPosition2D.Y;

    return true;
}

auto
    UCk_Utils_Screen_UE::
    FindScreenEdgeLocationForWorldLocation(
        UObject* InWorldContextObject,
        const FVector& InLocation,
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

    const auto& world = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if(ck::Is_NOT_Valid(world))
    { return; }

    const auto& playerController = InWorldContextObject ? UGameplayStatics::GetPlayerController(InWorldContextObject, 0) : nullptr;
    if(ck::Is_NOT_Valid(playerController))
    { return; }

    const auto& viewportSize = DoGet_PlayerControllerViewportSize(InWorldContextObject, playerController);
    const auto& viewportCenter = FVector2D(viewportSize.X * InViewportCenterLoc.X, viewportSize.Y * InViewportCenterLoc.Y);

    FVector outCameraLoc;
    FRotator outCameraRot;

    playerController->GetPlayerViewPoint(outCameraLoc, outCameraRot);

    const auto& cameraToLoc = InLocation - outCameraLoc;
    const auto& forwardVec = outCameraRot.Vector();
    const auto& offset = cameraToLoc.GetSafeNormal();

    const auto& dotProduct = FVector::DotProduct(forwardVec, offset);
    const auto& isLocationBehindCamera = dotProduct < 0;

    FVector2D outTempScreenPosition;

    if (isLocationBehindCamera)
    {
        const auto& invertedCameraToLoc = cameraToLoc * -1.f;
        const auto& newWorldLocationToProject = outCameraLoc + invertedCameraToLoc;

        ProjectWorldLocationToWidgetPositionCoords(playerController, newWorldLocationToProject, outTempScreenPosition);

        outTempScreenPosition.X = viewportSize.X - outTempScreenPosition.X;
        outTempScreenPosition.Y = viewportSize.Y - outTempScreenPosition.Y;
    }
    else
    {
        ProjectWorldLocationToWidgetPositionCoords(playerController, InLocation, outTempScreenPosition);
    }

    // Check to see if it's on screen. If it is, ProjectWorldLocationToWidgetPositionCoords is all we need, return it.
    if (outTempScreenPosition.X >= 0.f && outTempScreenPosition.X <= viewportSize.X && outTempScreenPosition.Y >= 0.f && outTempScreenPosition.Y <= viewportSize.Y && NOT isLocationBehindCamera)
    {
        OutScreenPosition = outTempScreenPosition;
        OutIsOnScreen = true;
        return;
    }

    outTempScreenPosition -= viewportCenter;

    const auto& angleRadians = FMath::Atan2(outTempScreenPosition.Y, outTempScreenPosition.X) - FMath::DegreesToRadians(90.f);

    OutRotationAngleDegrees = FMath::RadiansToDegrees(angleRadians) + 180.f;

    const auto& cos = cosf(angleRadians);
    const auto& sin = -sinf(angleRadians);

    outTempScreenPosition = FVector2D(viewportCenter.X + (sin * 150.f), viewportCenter.Y + cos * 150.f);

    const auto& cotangent = cos / sin;

    const auto& clampedEdgePercent = FMath::Clamp(InEdgePercent, 0.0f, 1.0f);
    const auto& screenBounds = FVector2D(viewportCenter * clampedEdgePercent);

    if (cos > 0)
    {
        outTempScreenPosition = FVector2D(screenBounds.Y / cotangent, screenBounds.Y);
    }
    else
    {
        outTempScreenPosition = FVector2D(-screenBounds.Y / cotangent, -screenBounds.Y);
    }

    if (outTempScreenPosition.X > screenBounds.X)
    {
        outTempScreenPosition = FVector2D(screenBounds.X, screenBounds.X * cotangent);
    }
    else if (outTempScreenPosition.X < -screenBounds.X)
    {
        outTempScreenPosition = FVector2D(-screenBounds.X, -screenBounds.X * cotangent);
    }

    outTempScreenPosition += viewportCenter;

    OutScreenPosition = outTempScreenPosition;
}

auto
    UCk_Utils_Screen_UE::
    DoGet_PlayerControllerViewportSize(
        UObject* InWorldContextObject,
        APlayerController* InPlayerController)
    -> FVector2D
{
    if(ck::Is_NOT_Valid(InPlayerController))
    { return {}; }

    const auto& screenViewportSize = [InPlayerController]()
    {
        int32 outScreenSizeX;
        int32 outScreenSizeY;

        InPlayerController->GetViewportSize(outScreenSizeX, outScreenSizeY);

        return FVector2D(outScreenSizeX, outScreenSizeY);
    }();

    const auto& viewportSize = [InWorldContextObject, screenViewportSize]()
    {
        FVector2D outViewportSize;
        USlateBlueprintLibrary::ScreenToViewport(InWorldContextObject, screenViewportSize, outViewportSize);

        return outViewportSize;
    }();

    return viewportSize;
}

// --------------------------------------------------------------------------------------------------------------------
