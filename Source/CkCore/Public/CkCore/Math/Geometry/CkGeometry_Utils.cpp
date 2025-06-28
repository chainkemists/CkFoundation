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
        const FBox& InBox)
    -> FBox2D
{
    // Inspired from (plugin) Cog's FCogWindowHelper::ComputeBoundingBoxScreenPosition function (CogWindowHelper.cpp)

    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Cannot project Box3D [{}] to screen because the supplied PlayerController is INVALID"), InBox)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot project Box3D [{}] to screen because it is INVALID"), InBox)
    { return {}; }

    auto BoundingBoxScreenSpace_Min = FVector2D{ FLT_MAX, FLT_MAX };
    auto BoundingBoxScreenSpace_Max = FVector2D{ -FLT_MAX, -FLT_MAX };

    ForEach_BoxVertices(InBox, [&](const FVector& InBoxVertex)
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
        const FBox& InBox)
    -> TArray<FVector>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot calculate Box3D Vertices because it is INVALID"))
    { return {}; }

    const auto& Result = [&]()
    {
        constexpr auto BoxVerticesCount = 8;

        TArray<FVector> ToRet;
        ToRet.Reserve(BoxVerticesCount);


        FVector BoxVertices[BoxVerticesCount];
        InBox.GetVertices(BoxVertices);

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
        const FBox& InBox)
    -> TArray<FCk_LineSegment>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot calculate Box3D Edges because it is INVALID"))
    { return {}; }

    const auto BoxEdges = TArray
    {
        FIntVector2{0, 1}, FIntVector2{0, 2}, FIntVector2{0, 4},
        FIntVector2{1, 3}, FIntVector2{1, 5}, FIntVector2{2, 3},
        FIntVector2{2, 6}, FIntVector2{3, 7}, FIntVector2{4, 5},
        FIntVector2{4, 6}, FIntVector2{5, 7}, FIntVector2{6, 7}
    };

    const auto& BoxVertices = Get_Box_Vertices(InBox);

    return ck::algo::Transform<TArray<FCk_LineSegment>>(BoxEdges, [&](const FIntVector2& InBoxEdge)
    {
        const auto& EdgeStart = BoxVertices[InBoxEdge.X];
        const auto& EdgeEnd = BoxVertices[InBoxEdge.Y];

        return FCk_LineSegment{ EdgeStart, EdgeEnd };
    });
}

auto
    UCk_Utils_Geometry_UE::
    Get_Box_Volume(
        const FBox& InBox)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot calculate Box3D Volume because it is INVALID"))
    { return {}; }

    return InBox.GetVolume();
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
    Get_IsValid_Box(
        const FBox& InBox)
    -> bool
{
    return ck::IsValid(InBox);
}

auto
    UCk_Utils_Geometry_UE::
    ForEach_BoxEdges(
        const FBox& InBox,
        TFunction<void(const FCk_LineSegment&)> InFunc)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot iteratate over the Box3D Edges because it is INVALID"))
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
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot iteratate over the Box3D Vertices because it is INVALID"))
    { return ECk_SucceededFailed::Failed; }

    for (const auto& Vertex : Get_Box_Vertices(InBox))
    {
        InFunc(Vertex);
    }

    return ECk_SucceededFailed::Succeeded;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Vertices(
        const FBox2D& InBox)
    -> TArray<FVector2D>
{
    if (ck::Is_NOT_Valid(InBox))
    { return {}; }

    const auto BoxVertices = TArray
    {
        InBox.Min,
        FVector2D(InBox.Max.X, InBox.Min.Y),
        FVector2D(InBox.Min.X, InBox.Max.Y),
        InBox.Max
    };

    return BoxVertices;
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Edges(
        const FBox2D& InBox)
    -> TArray<FCk_LineSegment2D>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot retrieve the Box2D Vertices because it is INVALID"))
    { return {}; }

    const auto BoxEdges = TArray
    {
        FIntVector2{0, 1}, FIntVector2{1, 3},
        FIntVector2{3, 2}, FIntVector2{2, 0}
    };

    const auto& BoxVertices = Get_Box_Vertices(InBox);

    return ck::algo::Transform<TArray<FCk_LineSegment2D>>(BoxEdges, [&](const FIntVector2& InBoxEdge)
    {
        const auto& EdgeStart = BoxVertices[InBoxEdge.X];
        const auto& EdgeEnd = BoxVertices[InBoxEdge.Y];

        return FCk_LineSegment2D{ EdgeStart, EdgeEnd };
    });
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Area(
        const FBox2D& InBox)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot calculate the Box2D Area because it is INVALID"), InBox)
    { return {}; }

    return InBox.GetArea();
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Overlap(
        const FBox2D& InBoxA,
        const FBox2D& InBoxB)
    -> FBox2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBoxA), TEXT("Cannot calculate the Box2D Overlap because Box A is INVALID"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InBoxB), TEXT("Cannot calculate the Box2D Overlap because Box B is INVALID"))
    { return {}; }

    return InBoxA.Overlap(InBoxB);
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Intersects(
        const FBox2D& InBoxA,
        const FBox2D& InBoxB)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBoxA), TEXT("Cannot calculate Box2D Intersects because Box A is INVALID"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InBoxB), TEXT("Cannot calculate Box2D Intersects because Box B is INVALID"))
    { return {}; }

    return InBoxA.Intersect(InBoxB);
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_IsPointInBox(
        const FBox2D& InBox,
        const FVector2D& InPoint)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot calculate if Point [{}] is inside Box2D because the Box is INVALID"), InPoint)
    { return {}; }

    return InPoint.X >= InBox.Min.X && InPoint.X <= InBox.Max.X && InPoint.Y >= InBox.Min.Y && InPoint.Y <= InBox.Max.Y;
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Center(
        const FBox2D& InBox)
    -> FVector2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot get Box2D Center because it is INVALID"))
    { return {}; }

    return InBox.GetCenter();
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Extents(
        const FBox2D& InBox)
    -> FVector2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot get Box2D Extents because it is INVALID"))
    { return {}; }

    return InBox.GetExtent();
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Size(
        const FBox2D& InBox)
    -> FVector2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot get Box2D Size because it is INVALID"))
    { return {}; }

    return InBox.GetSize();
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_CenterAndExtents(
        const FBox2D& InBox,
        FVector2D& OutCenter,
        FVector2D& OutExtents)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot get Box2D Center and Extents because it is INVALID"))
    {
        OutCenter = FVector2D::ZeroVector;
        OutExtents = FVector2D::ZeroVector;
        return;
    }

    InBox.GetCenterAndExtents(OutCenter, OutExtents);
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_ClosestPointTo(
        const FBox2D& InBox,
        const FVector2D& InPoint)
    -> FVector2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot get closest point to Box2D because it is INVALID"))
    { return {}; }

    return InBox.GetClosestPointTo(InPoint);
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_DistanceToPoint(
        const FBox2D& InBox,
        const FVector2D& InPoint)
    -> float
{
    return FMath::Sqrt(Get_Box_SquaredDistanceToPoint(InBox, InPoint));
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_SquaredDistanceToPoint(
        const FBox2D& InBox,
        const FVector2D& InPoint)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot get distance from Box2D to point because it is INVALID"))
    { return 0.0f; }

    return InBox.ComputeSquaredDistanceToPoint(InPoint);
}

auto
    UCk_Utils_Geometry2D_UE::
    Expand_Box_ByScalar(
        const FBox2D& InBox,
        float InExpandBy)
    -> FBox2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot expand Box2D because it is INVALID"))
    { return {}; }

    return InBox.ExpandBy(InExpandBy);
}

auto
    UCk_Utils_Geometry2D_UE::
    Expand_Box_ByVector(
        const FBox2D& InBox,
        const FVector2D& InExpandBy)
    -> FBox2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot expand Box2D because it is INVALID"))
    { return {}; }

    return InBox.ExpandBy(InExpandBy);
}

auto
    UCk_Utils_Geometry2D_UE::
    Shift_Box(
        const FBox2D& InBox,
        const FVector2D& InOffset)
    -> FBox2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot shift Box2D because it is INVALID"))
    { return {}; }

    return InBox.ShiftBy(InOffset);
}

auto
    UCk_Utils_Geometry2D_UE::
    Move_Box_To(
        const FBox2D& InBox,
        const FVector2D& InDestination)
    -> FBox2D
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot move Box2D because it is INVALID"))
    { return {}; }

    return InBox.MoveTo(InDestination);
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_IsPointInsideBox(
        const FBox2D& InBox,
        const FVector2D& InPoint)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot check if point is inside Box2D because it is INVALID"))
    { return false; }

    return InBox.IsInside(InPoint);
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_IsPointInsideOrOnBox(
        const FBox2D& InBox,
        const FVector2D& InPoint)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot check if point is inside or on Box2D because it is INVALID"))
    { return false; }

    return InBox.IsInsideOrOn(InPoint);
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_IsBoxInsideBox(
        const FBox2D& InOuterBox,
        const FBox2D& InInnerBox)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOuterBox), TEXT("Cannot check box containment because outer Box2D is INVALID"))
    { return false; }

    CK_ENSURE_IF_NOT(ck::IsValid(InInnerBox), TEXT("Cannot check box containment because inner Box2D is INVALID"))
    { return false; }

    return InOuterBox.IsInside(InInnerBox);
}

auto
    UCk_Utils_Geometry2D_UE::
    Make_Box_FromCenterAndExtents(
        const FVector2D& InCenter,
        const FVector2D& InExtents)
    -> FBox2D
{
    return FBox2D::BuildAABB(InCenter, InExtents);
}

auto
    UCk_Utils_Geometry2D_UE::
    Make_Box_FromPoints(
        const TArray<FVector2D>& InPoints)
    -> FBox2D
{
    CK_ENSURE_IF_NOT(NOT InPoints.IsEmpty(), TEXT("Cannot create Box2D from empty points array"))
    { return {}; }

    return FBox2D(InPoints);
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_Perimeter(
        const FBox2D& InBox)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot calculate Box2D perimeter because it is INVALID"))
    { return 0.0f; }

    const auto Size = InBox.GetSize();
    return 2.0f * (Size.X + Size.Y);
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_AspectRatio(
        const FBox2D& InBox)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot calculate Box2D aspect ratio because it is INVALID"))
    { return 1.0f; }

    const auto Size = InBox.GetSize();

    CK_ENSURE_IF_NOT(Size.Y > 0.0f, TEXT("Cannot calculate aspect ratio because Box2D height is zero"))
    { return 1.0f; }

    return Size.X / Size.Y;
}

auto
    UCk_Utils_Geometry2D_UE::
    Get_Box_OverlapArea(
        const FBox2D& InBoxA,
        const FBox2D& InBoxB)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBoxA), TEXT("Cannot calculate overlap area because Box A is INVALID"))
    { return 0.0f; }

    CK_ENSURE_IF_NOT(ck::IsValid(InBoxB), TEXT("Cannot calculate overlap area because Box B is INVALID"))
    { return 0.0f; }

    const auto Overlap = Get_Box_Overlap(InBoxA, InBoxB);
    return ck::IsValid(Overlap) ? Get_Box_Area(Overlap) : 0.0f;
}

auto
    UCk_Utils_Geometry2D_UE::
    Calculate_OverlapPercent(
        const FBox2D& InBoundsA,
        const FBox2D& InBoundsB)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBoundsA), TEXT("Cannot calculate overlap percent because Box A is INVALID"))
    { return 0.0f; }

    CK_ENSURE_IF_NOT(ck::IsValid(InBoundsB), TEXT("Cannot calculate overlap percent because Box B is INVALID"))
    { return 0.0f; }

    if (NOT Get_Box_Intersects(InBoundsA, InBoundsB))
    {
        return 0.0f;
    }

    const auto IntersectionBounds = Get_Box_Overlap(InBoundsA, InBoundsB);
    const auto IntersectionArea = Get_Box_Area(IntersectionBounds);

    // Use the smaller box's area as the reference for overlap percentage
    const auto AreaA = Get_Box_Area(InBoundsA);
    const auto AreaB = Get_Box_Area(InBoundsB);
    const auto ReferenceArea = FMath::Min(AreaA, AreaB);

    return (ReferenceArea > 0.0f) ? (IntersectionArea / ReferenceArea) : 0.0f;
}

auto
    UCk_Utils_Geometry2D_UE::
    Make_Box_WithOrigin(
        FVector2D InOrigin,
        FVector2D InExtents)
    -> FBox2D
{
    return FBox2D{InOrigin - InExtents, InOrigin + InExtents};
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
    Get_IsValid_Box(
        const FBox2D& InBox)
    -> bool
{
    return ck::IsValid(InBox);
}

auto
    UCk_Utils_Geometry2D_UE::
    ForEach_BoxVertices(
        const FBox2D& InBox,
        TFunction<void(const FVector2D&)> InFunc)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot iterate over Box2D Vertices because it is INVALID"))
    { return ECk_SucceededFailed::Failed; }

    for (const auto& Vertex : Get_Box_Vertices(InBox))
    {
        InFunc(Vertex);
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
    CK_ENSURE_IF_NOT(ck::IsValid(InBox), TEXT("Cannot iterate over Box2D Edges because it is INVALID"))
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
        bool InOnlyCollidingComponents,
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

            if (InOnlyCollidingComponents && NOT InSceneComponent->IsCollisionEnabled())
            { return; }

            Bounds += InSceneComponent->Bounds.GetBox();
        });
    }

    return Bounds;
}

// --------------------------------------------------------------------------------------------------------------------