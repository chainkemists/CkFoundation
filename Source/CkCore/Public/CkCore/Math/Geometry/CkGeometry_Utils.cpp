#include "CkGeometry_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "GameFramework/HUD.h"

#include <Kismet/KismetMathLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Geometry_UE::
    Project_Box_ToScreen(
        APlayerController* InPlayerController,
        const FBox& InBox3D)
    -> FBox2D
{
    // Inspired from (plugin) Cog's FCogWindowHelper::ComputeBoundingBoxScreenPosition function (CogWindowHelper.cpp)

    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Cannot Project Box3D [{}] to screen because the supplied PlayerController is INVALID"), InBox3D)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InBox3D), TEXT("Cannot Project Box3D [{}] to screen because it is INVALID"), InBox3D)
    { return {}; }

    auto BoundingBoxScreenSpace_Min = FVector2D{ FLT_MAX, FLT_MAX };
    auto BoundingBoxScreenSpace_Max = FVector2D{ -FLT_MAX, -FLT_MAX };

    ForEach_BoxVertices(InBox3D, [&](const FVector& InBoxVertex)
    {
        constexpr auto PlayerViewportRelative = false;
        auto ScreenLocation = FVector2D{};

        if (NOT InPlayerController->ProjectWorldLocationToScreen(InBoxVertex, ScreenLocation, PlayerViewportRelative))
        { FBox2D{}; }

        BoundingBoxScreenSpace_Min.X = FMath::Min(ScreenLocation.X, BoundingBoxScreenSpace_Min.X);
        BoundingBoxScreenSpace_Min.Y = FMath::Min(ScreenLocation.Y, BoundingBoxScreenSpace_Min.Y);
        BoundingBoxScreenSpace_Max.X = FMath::Max(ScreenLocation.X, BoundingBoxScreenSpace_Max.X);
        BoundingBoxScreenSpace_Max.Y = FMath::Max(ScreenLocation.Y, BoundingBoxScreenSpace_Max.Y);
    });

    const auto& ViewportSize = [&]()
    {
        auto OutScreenSizeX = 0;
        auto OutScreenSizeY = 0;

        InPlayerController->GetViewportSize(OutScreenSizeX, OutScreenSizeY);

        return FVector2D(OutScreenSizeX, OutScreenSizeY);
    }();

    // Prevent getting large values when the camera get close to the target
    BoundingBoxScreenSpace_Min.X = FMath::Max(-ViewportSize.X, BoundingBoxScreenSpace_Min.X);
    BoundingBoxScreenSpace_Min.Y = FMath::Max(-ViewportSize.Y, BoundingBoxScreenSpace_Min.Y);
    BoundingBoxScreenSpace_Max.X = FMath::Min(ViewportSize.X * 2.0f, BoundingBoxScreenSpace_Max.X);
    BoundingBoxScreenSpace_Max.Y = FMath::Min(ViewportSize.Y * 2.0f, BoundingBoxScreenSpace_Max.Y);

    return FBox2D{ BoundingBoxScreenSpace_Min, BoundingBoxScreenSpace_Max };
}

auto
    UCk_Utils_Geometry_UE::
    Get_Box_Vertices(
        const FBox& InBox3D)
    -> TArray<FVector>
{
    if (ck::Is_NOT_Valid(InBox3D))
    { return {}; }

    const auto& Result = [&]()
    {
        constexpr auto BoxVerticesCount = 8;

        TArray<FVector> ToRet;
        ToRet.Reserve(BoxVerticesCount);


        FVector BoxVertices[BoxVerticesCount];
        InBox3D.GetVertices(BoxVertices);

        for (const auto& Vertex : BoxVertices)
        {
            ToRet.Add(Vertex);
        }

        return ToRet;
    }();

    return Result;
}

auto
    UCk_Utils_Geometry_UE::
    Get_Box_Edges(
        const FBox& InBox3D)
    -> TArray<FCk_LineSegment>
{
    if (ck::Is_NOT_Valid(InBox3D))
    { return {}; }

    const auto BoxEdges = TArray
    {
        FIntVector2{0, 1}, FIntVector2{0, 2}, FIntVector2{0, 4},
        FIntVector2{1, 3}, FIntVector2{1, 5}, FIntVector2{2, 3},
        FIntVector2{2, 6}, FIntVector2{3, 7}, FIntVector2{4, 5},
        FIntVector2{4, 6}, FIntVector2{5, 7}, FIntVector2{6, 7}
    };

    const auto& BoxVertices = Get_Box_Vertices(InBox3D);

    return ck::algo::Transform<TArray<FCk_LineSegment>>(BoxEdges, [&](const FIntVector2& InBoxEdge)
    {
        const auto& EdgeStart = BoxVertices[InBoxEdge.X];
        const auto& EdgeEnd = BoxVertices[InBoxEdge.Y];

        return FCk_LineSegment{ EdgeStart, EdgeEnd };
    });
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
    ForEach_BoxEdges(
        const FBox& InBox,
        TFunction<void(const FCk_LineSegment&)> InFunc)
    -> ECk_SucceededFailed
{
    if (ck::Is_NOT_Valid(InBox))
    { return ECk_SucceededFailed::Failed; }

    for (const auto& Edge : Get_Box_Edges(InBox))
    {
        InFunc(Edge);
    }

    return ECk_SucceededFailed::Succeeded;
}

auto
    UCk_Utils_Geometry_UE::
    ForEach_BoxVertices(
        const FBox& InBox,
        TFunction<void(const FVector&)> InFunc)
    -> ECk_SucceededFailed
{
    if (ck::Is_NOT_Valid(InBox))
    { return ECk_SucceededFailed::Failed; }

    for (const auto& BoxVertex : Get_Box_Vertices(InBox))
    {
        InFunc(BoxVertex);
    }

    return ECk_SucceededFailed::Succeeded;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Vertices(
        const FBox2D& InBox2D)
    -> TArray<FVector2D>
{
    if (ck::Is_NOT_Valid(InBox2D))
    { return {}; }

    const auto BoxVertices = TArray
    {
        InBox2D.Min,
        FVector2D(InBox2D.Max.X, InBox2D.Min.Y),
        FVector2D(InBox2D.Min.X, InBox2D.Max.Y),
        InBox2D.Max
    };

    return BoxVertices;
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Edges(
        const FBox2D& InBox2D)
    -> TArray<FCk_LineSegment2D>
{
    if (ck::Is_NOT_Valid(InBox2D))
    { return {}; }

    const auto BoxEdges = TArray
    {
        FIntVector2{0, 1}, FIntVector2{1, 3},
        FIntVector2{3, 2}, FIntVector2{2, 0}
    };

    const auto& BoxVertices = Get_Box_Vertices(InBox2D);

    return ck::algo::Transform<TArray<FCk_LineSegment2D>>(BoxEdges, [&](const FIntVector2& InBoxEdge)
    {
        const auto& EdgeStart = BoxVertices[InBoxEdge.X];
        const auto& EdgeEnd = BoxVertices[InBoxEdge.Y];

        return FCk_LineSegment2D{ EdgeStart, EdgeEnd };
    });
}

auto
    UCk_Utils_Geometry2D_UE::
    Break_Box_WithCenterAndExtents(
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

auto
    UCk_Utils_Geometry2D_UE::
    ForEach_BoxVertices(
        const FBox2D& InBox,
        TFunction<void(const FVector2D&)> InFunc)
    -> ECk_SucceededFailed
{
    if (ck::Is_NOT_Valid(InBox))
    { return ECk_SucceededFailed::Failed; }

    for (const auto& BoxVertex : Get_Box_Vertices(InBox))
    {
        InFunc(BoxVertex);
    }

    return ECk_SucceededFailed::Succeeded;
}

auto
    UCk_Utils_Geometry2D_UE::
    ForEach_BoxEdges(
        const FBox2D& InBox,
        TFunction<void(const FCk_LineSegment2D&)> InFunc)
    -> ECk_SucceededFailed
{
    if (ck::Is_NOT_Valid(InBox))
    { return ECk_SucceededFailed::Failed; }

    for (const auto& Edge : Get_Box_Edges(InBox))
    {
        InFunc(Edge);
    }

    return ECk_SucceededFailed::Succeeded;
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
