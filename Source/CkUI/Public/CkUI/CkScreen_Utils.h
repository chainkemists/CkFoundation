#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCore/Macros/CkMacros.h"

#include "CkScreen_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKUI_API UCk_Utils_Screen_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Screen_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|UI")
    static bool
    ProjectWorldLocationToWidgetPositionCoords(
        APlayerController* InPlayerController,
        const FVector& InWorldLocation, 
        FVector2D& OutViewportPosition);

    /**
    * Converts a world location to screen position for HUD drawing. This differs from the results of FSceneView::WorldToScreen in that it returns a position along the edge of the screen for offscreen locations
    *
    * @param InLocation - The world space location to be converted to screen space
    * @param InEdgePercent - How close to the edge of the screen, 1.0 = at edge, 0.0 = at center of screen. .9 or .95 is usually desirable
    * @param InViewportCenterLoc - for offsetting center of the screen, leave at (0.5, 0.5) for no offset
    * @param OutScreenPosition - the screen coordinates for HUD drawing
    * @param OutRotationAngleDegrees - The angle to rotate a hud element if you want it pointing toward the offscreen indicator, 0° if onscreen
    * @param OutIsOnScreen - True if the specified location is in the camera view (may be obstructed)
    */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|UI",
              meta=(WorldContext = "InWorldContextObject", CallableWithoutWorldContext))
    static void
    FindScreenEdgeLocationForWorldLocation(
        UObject* InWorldContextObject,
        const FVector& InLocation,
        float InEdgePercent,
        FVector2D InViewportCenterLoc,
        FVector2D& OutScreenPosition,
        float& OutRotationAngleDegrees,
        bool &OutIsOnScreen);

private:
    static auto DoGet_PlayerControllerViewportSize(
        UObject* InWorldContextObject,
        APlayerController* InPlayerController) -> FVector2D;
};

// --------------------------------------------------------------------------------------------------------------------
