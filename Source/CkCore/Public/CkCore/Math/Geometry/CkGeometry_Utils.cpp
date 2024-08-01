#include "CkGeometry_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "GameFramework/HUD.h"

#include <Kismet/KismetMathLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Geometry_UE::
    ProjectBox3D_ToScreen(
        APlayerController* InPlayerController,
        const FBox& InBox3D)
    -> FBox2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Cannot Project Box3D [{}] to screen because the supplied PlayerController is INVALID"), InBox3D)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InBox3D), TEXT("Cannot Project Box3D [{}] to screen because it is INVALID"), InBox3D)
    { return {}; }

    auto BoundingBoxScreenSpace_Min = FVector2D{ FLT_MAX, FLT_MAX };
    auto BoundingBoxScreenSpace_Max = FVector2D{ -FLT_MAX, -FLT_MAX };

    constexpr auto BoxVerticesCount = 8;

    FVector BoxVertices[BoxVerticesCount];
    InBox3D.GetVertices(BoxVertices);

    for (const auto& BoxVertex : BoxVertices)
    {
        constexpr auto PlayerViewportRelative = false;
        FVector2D ScreenLocation;

        if (NOT InPlayerController->ProjectWorldLocationToScreen(BoxVertex, ScreenLocation, PlayerViewportRelative))
        { FBox2D{}; }

        BoundingBoxScreenSpace_Min.X = FMath::Min(ScreenLocation.X, BoundingBoxScreenSpace_Min.X);
        BoundingBoxScreenSpace_Min.Y = FMath::Min(ScreenLocation.Y, BoundingBoxScreenSpace_Min.Y);
        BoundingBoxScreenSpace_Max.X = FMath::Max(ScreenLocation.X, BoundingBoxScreenSpace_Max.X);
        BoundingBoxScreenSpace_Max.Y = FMath::Max(ScreenLocation.Y, BoundingBoxScreenSpace_Max.Y);
    }

    const auto& ViewportSize = [&]()
    {
        int32 OutScreenSizeX;
        int32 OutScreenSizeY;

        InPlayerController->GetViewportSize(OutScreenSizeX, OutScreenSizeY);

        return FVector2D(OutScreenSizeX, OutScreenSizeY);
    }();

    //const auto& ScreenViewportSize = [&]()
    //{
    //    FVector2D outViewportSize;
    //    USlateBlueprintLibrary::ScreenToViewport(InPlayerController, ViewportSize, outViewportSize);

    //    return outViewportSize;
    //}();

    // Prevent getting large values when the camera get close to the target
    BoundingBoxScreenSpace_Min.X = FMath::Max(-ViewportSize.X, BoundingBoxScreenSpace_Min.X);
    BoundingBoxScreenSpace_Min.Y = FMath::Max(-ViewportSize.Y, BoundingBoxScreenSpace_Min.Y);
    BoundingBoxScreenSpace_Max.X = FMath::Min(ViewportSize.X * 2.0f, BoundingBoxScreenSpace_Max.X);
    BoundingBoxScreenSpace_Max.Y = FMath::Min(ViewportSize.Y * 2.0f, BoundingBoxScreenSpace_Max.Y);

    return FBox2D{ BoundingBoxScreenSpace_Min, BoundingBoxScreenSpace_Max };
}

auto
    UCk_Utils_Geometry_UE::
    Break_Box_WithCenterAndExtents(
        const FBox& InBox,
        FVector& OutMin,
        FVector& OutMax,
        FVector& OutCenter,
        FVector& InExtents,
        bool& OutIsValidBox)
    -> void
{
    OutMin = InBox.Min,
    OutMax = InBox.Max,
    InBox.GetCenterAndExtents(OutCenter, InExtents);
    OutIsValidBox = static_cast<bool>(InBox.IsValid);
}

auto
    UCk_Utils_Geometry_UE::
    Break_Box2D_WithCenterAndExtents(
        const FBox2D& InBox,
        FVector2D& OutMin,
        FVector2D& OutMax,
        FVector2D& OutCenter,
        FVector2D& InExtents,
        bool& OutIsValidBox)
    -> void
{
    OutMin = InBox.Min,
    OutMax = InBox.Max,
    InBox.GetCenterAndExtents(OutCenter, InExtents);
    OutIsValidBox = static_cast<bool>(InBox.bIsValid);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Geometry_Actor_UE::
    Get_ActorBounds_ByComponentClass(
        AActor* InActor,
        TSubclassOf<USceneComponent> InComponentToAllow,
        bool InIncludeFromChildActors)
    -> FBox
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Cannot calculate Actor Bounds because the supplied Actor is INVALID"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InComponentToAllow), TEXT("Cannot calculate Bounds for Actor [{}] because the component class to use is INVALID"), InActor)
    { return {}; }

    auto Bounds = FBox{};

    if (const auto RootComponent = InActor->GetRootComponent(); RootComponent->IsA(InComponentToAllow))
    {
        Bounds = RootComponent->Bounds.GetBox();
    }
    else
    {
        InActor->ForEachComponent<USceneComponent>(InIncludeFromChildActors, [&](const USceneComponent* InSceneComponent)
        {
            if (NOT InSceneComponent->IsA(InComponentToAllow))
            { return; }

            if (NOT InSceneComponent->IsRegistered())
            { return; }

            if (NOT InSceneComponent->IsCollisionEnabled())
            { return; }

            Bounds += InSceneComponent->Bounds.GetBox();
        });
    }

    return Bounds;
}

// --------------------------------------------------------------------------------------------------------------------
