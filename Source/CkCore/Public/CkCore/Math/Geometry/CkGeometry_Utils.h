#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/Geometry/CkGeometry_Types.h"

#include <CoreMinimal.h>
#include <FrameTypes.h>
#include <OrientedBoxTypes.h>

#include <Kismet/BlueprintFunctionLibrary.h>

#include <Math/Box.h>
#include <Math/Box2D.h>

#include "CkGeometry_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Geometry_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Geometry_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Project Box3D To Screen",
              Category = "Ck|Utils|Math|Geometry")
    static FBox2D
    Project_Box_ToScreen(
        APlayerController* InPlayerController,
        const FBox& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box3D Vertices",
              Category = "Ck|Utils|Math|Geometry")
    static TArray<FVector>
    Get_Box_Vertices(
        const FBox& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box3D Edges",
              Category = "Ck|Utils|Math|Geometry")
    static TArray<FCk_LineSegment>
    Get_Box_Edges(
        const FBox& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box3D Volume",
              Category = "Ck|Utils|Math|Geometry")
    static float
    Get_Box_Volume(
        const FBox& InBox);

private:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Break Box (With Center And Extents)",
              Category = "Ck|Utils|Math|Geometry")
    static void
    Break_Box_WithCenterAndExtents(
        const FBox& InBox,
        FVector& OutMin,
        FVector& OutMax,
        FVector& OutCenter,
        FVector& InExtents,
        bool& OutIsValidBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Valid (Box3D)",
              Category = "Ck|Utils|Math|Geometry",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    Get_IsValid_Box(
        const FBox& InBox);

public:
    static auto
    ForEach_BoxEdges(
        const FBox& InBox,
        const TFunction<void(const FCk_LineSegment&)>& InFunc) -> ECk_SucceededFailed;

    static auto
    ForEach_BoxVertices(
        const FBox& InBox,
        const TFunction<void(const FVector&)>& InFunc) -> ECk_SucceededFailed;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Geometry2D_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Geometry2D_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Vertices",
              Category = "Ck|Utils|Math|Geometry2D")
    static TArray<FVector2D>
    Get_Box_Vertices(
        const FBox2D& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Edges",
              Category = "Ck|Utils|Math|Geometry2D")
    static TArray<FCk_LineSegment2D>
    Get_Box_Edges(
        const FBox2D& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Area",
              Category = "Ck|Utils|Math|Geometry2D")
    static float
    Get_Box_Area(
        const FBox2D& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Overlap",
              Category = "Ck|Utils|Math|Geometry2D")
    static FBox2D
    Get_Box_Overlap(
        const FBox2D& InBoxA,
        const FBox2D& InBoxB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Intersects",
              Category = "Ck|Utils|Math|Geometry2D")
    static bool
    Get_Box_Intersects(
        const FBox2D& InBoxA,
        const FBox2D& InBoxB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Point In Box2D",
              Category = "Ck|Utils|Math|Geometry2D")
    static bool
    Get_IsPointInBox(
        const FBox2D& InBox,
        const FVector2D& InPoint);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Center",
              Category = "Ck|Utils|Math|Geometry2D")
    static FVector2D
    Get_Box_Center(
        const FBox2D& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Extents",
              Category = "Ck|Utils|Math|Geometry2D")
    static FVector2D
    Get_Box_Extents(
        const FBox2D& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Size",
              Category = "Ck|Utils|Math|Geometry2D")
    static FVector2D
    Get_Box_Size(
        const FBox2D& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Center And Extents",
              Category = "Ck|Utils|Math|Geometry2D")
    static void
    Get_Box_CenterAndExtents(
        const FBox2D& InBox,
        FVector2D& OutCenter,
        FVector2D& OutExtents);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Closest Point",
              Category = "Ck|Utils|Math|Geometry2D")
    static FVector2D
    Get_Box_ClosestPointTo(
        const FBox2D& InBox,
        const FVector2D& InPoint);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Distance To Point",
              Category = "Ck|Utils|Math|Geometry2D")
    static float
    Get_Box_DistanceToPoint(
        const FBox2D& InBox,
        const FVector2D& InPoint);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Squared Distance To Point",
              Category = "Ck|Utils|Math|Geometry2D")
    static float
    Get_Box_SquaredDistanceToPoint(
        const FBox2D& InBox,
        const FVector2D& InPoint);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Expand Box2D By Scalar",
              Category = "Ck|Utils|Math|Geometry2D")
    static FBox2D
    Expand_Box_ByScalar(
        const FBox2D& InBox,
        float InExpandBy);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Expand Box2D By Vector",
              Category = "Ck|Utils|Math|Geometry2D")
    static FBox2D
    Expand_Box_ByVector(
        const FBox2D& InBox,
        const FVector2D& InExpandBy);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Shift Box2D",
              Category = "Ck|Utils|Math|Geometry2D")
    static FBox2D
    Shift_Box(
        const FBox2D& InBox,
        const FVector2D& InOffset);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Move Box2D To",
              Category = "Ck|Utils|Math|Geometry2D")
    static FBox2D
    Move_Box_To(
        const FBox2D& InBox,
        const FVector2D& InDestination);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Point Inside Box2D",
              Category = "Ck|Utils|Math|Geometry2D")
    static bool
    Get_IsPointInsideBox(
        const FBox2D& InBox,
        const FVector2D& InPoint);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Point Inside Or On Box2D",
              Category = "Ck|Utils|Math|Geometry2D")
    static bool
    Get_IsPointInsideOrOnBox(
        const FBox2D& InBox,
        const FVector2D& InPoint);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Box Inside Box2D",
              Category = "Ck|Utils|Math|Geometry2D")
    static bool
    Get_IsBoxInsideBox(
        const FBox2D& InOuterBox,
        const FBox2D& InInnerBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Make Box2D From Center And Extents",
              Category = "Ck|Utils|Math|Geometry2D")
    static FBox2D
    Make_Box_FromCenterAndExtents(
        const FVector2D& InCenter,
        const FVector2D& InExtents);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Make Box2D From Points",
              Category = "Ck|Utils|Math|Geometry2D")
    static FBox2D
    Make_Box_FromPoints(
        const TArray<FVector2D>& InPoints);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Perimeter",
              Category = "Ck|Utils|Math|Geometry2D")
    static float
    Get_Box_Perimeter(
        const FBox2D& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Aspect Ratio",
              Category = "Ck|Utils|Math|Geometry2D")
    static float
    Get_Box_AspectRatio(
        const FBox2D& InBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Overlap Area",
              Category = "Ck|Utils|Math|Geometry2D")
    static float
    Get_Box_OverlapArea(
        const FBox2D& InBoxA,
        const FBox2D& InBoxB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Calculate Box2D Overlap Percent",
              Category = "Ck|Utils|Math|Geometry2D")
    static float
    Calculate_OverlapPercent(
        const FBox2D& InBoundsA,
        const FBox2D& InBoundsB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Make Box2D (With Origin)",
              Category = "Ck|Utils|Math|Geometry2D")
    static FBox2D
    Make_Box_WithOrigin(
        FVector2D InOrigin,
        FVector2D InExtents);
private:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Break Box2D (With Center And Extents)",
              Category = "Ck|Utils|Math|Geometry2D")
    static void
    Break_Box_WithCenterAndExtents(
        const FBox2D& InBox,
        FVector2D& OutMin,
        FVector2D& OutMax,
        FVector2D& OutCenter,
        FVector2D& InExtents,
        bool& OutIsValidBox);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Valid (Box2D)",
              Category = "Ck|Utils|Math|Geometry2D",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    Get_IsValid_Box(
        const FBox2D& InBox);

public:
    static auto
    ForEach_BoxEdges(
        const FBox2D& InBox,
        const TFunction<void(const FCk_LineSegment2D&)>& InFunc) -> ECk_SucceededFailed;

    static auto
    ForEach_BoxVertices(
        const FBox2D& InBox,
        const TFunction<void(const FVector2D&)>& InFunc) -> ECk_SucceededFailed;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Geometry_Actor_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Geometry_Actor_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Actor Bounds (By Component Class)",
              Category = "Ck|Utils|Math|Geometry|Actor")
    static FBox
    Get_ActorBounds_ByComponentClass(
        AActor* InActor,
        TSubclassOf<USceneComponent> InComponentToAllow,
        bool InOnlyCollidingComponents = true,
        bool InIncludeFromChildActors = true);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_Utils_Frame3D_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Frame3D_UE);

public:
    // Creation functions
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck] Make Frame3D")
    static FCk_Frame3D
    Request_Create(
        FVector InOrigin,
        FQuat InRotation);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck] Make Frame3D From Transform")
    static FCk_Frame3D
    Request_CreateFromTransform(
        const FTransform& InTransform);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck] Make Frame3D With Z Aligned")
    static FCk_Frame3D
    Request_CreateWithZAligned(
        const FVector& InOrigin,
        const FVector& InZDirection);

    // Basic properties
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Get Origin")
    static FVector
    Get_Origin(
        const FCk_Frame3D& InFrame);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Get Rotation")
    static FQuat
    Get_Rotation(
        const FCk_Frame3D& InFrame);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Get X Axis")
    static FVector
    Get_XAxis(
        const FCk_Frame3D& InFrame);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Get Y Axis")
    static FVector
    Get_YAxis(
        const FCk_Frame3D& InFrame);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Get Z Axis")
    static FVector
    Get_ZAxis(
        const FCk_Frame3D& InFrame);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Get Axis")
    static FVector
    Get_Axis(
        const FCk_Frame3D& InFrame,
        int32 InAxisIndex);

    // Transformations
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] To Transform")
    static FTransform
    Get_Transform(
        const FCk_Frame3D& InFrame);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] To Inverse Transform")
    static FTransform
    Get_InverseTransform(
        const FCk_Frame3D& InFrame);

    // Point/Vector operations
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Point At")
    static FVector
    Get_PointAt(
        const FCk_Frame3D& InFrame,
        float InX,
        float InY,
        float InZ);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] To Frame Point")
    static FVector
    Get_ToFramePoint(
        const FCk_Frame3D& InFrame,
        const FVector& InPoint);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] From Frame Point")
    static FVector
    Get_FromFramePoint(
        const FCk_Frame3D& InFrame,
        const FVector& InPoint);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] To Frame Vector")
    static FVector
    Get_ToFrameVector(
        const FCk_Frame3D& InFrame,
        const FVector& InVector);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] From Frame Vector")
    static FVector
    Get_FromFrameVector(
        const FCk_Frame3D& InFrame,
        const FVector& InVector);

    // Mutation functions
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Request Set Origin")
    static void
    Request_SetOrigin(
        UPARAM(ref) FCk_Frame3D& InFrame,
        const FVector& InOrigin);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Request Set Rotation")
    static void
    Request_SetRotation(
        UPARAM(ref) FCk_Frame3D& InFrame,
        const FQuat& InRotation);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Request Rotate")
    static void
    Request_Rotate(
        UPARAM(ref) FCk_Frame3D& InFrame,
        const FQuat& InRotation);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Request Transform")
    static void
    Request_Transform(
        UPARAM(ref) FCk_Frame3D& InFrame,
        const FTransform& InTransform);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Request Align Axis")
    static void
    Request_AlignAxis(
        UPARAM(ref) FCk_Frame3D& InFrame,
        int32 InAxisIndex,
        const FVector& InToDirection);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] Request Constrained Align Axis")
    static void
    Request_ConstrainedAlignAxis(
        UPARAM(ref) FCk_Frame3D& InFrame,
        int32 InAxisIndex,
        const FVector& InToDirection,
        const FVector& InAroundVector);

    // Plane operations
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] To Plane UV")
    static FVector2D
    Get_ToPlaneUV(
        const FCk_Frame3D& InFrame,
        const FVector& InPos,
        int32 InPlaneNormalAxis = 2);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] From Plane UV")
    static FVector
    Get_FromPlaneUV(
        const FCk_Frame3D& InFrame,
        const FVector2D& InPosUV,
        int32 InPlaneNormalAxis = 2);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Frame3D",
        DisplayName="[Ck][Frame3D] To Plane")
    static FVector
    Get_ToPlane(
        const FCk_Frame3D& InFrame,
        const FVector& InPos,
        int32 InPlaneNormalAxis = 2);
};

// --------------------------------------------------------------------------------------------------------------------
// ORIENTED BOX 2D UTILS
// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_Utils_OrientedBox2D_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_OrientedBox2D_UE);

public:
    // Creation functions
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck] Make Oriented Box 2D")
    static FCk_OrientedBox2D
    Request_Create(
        const FVector2D& InOrigin,
        const FVector2D& InExtents);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck] Make Oriented Box 2D With Rotation")
    static FCk_OrientedBox2D
    Request_CreateWithRotation(
        const FVector2D& InOrigin,
        float InAngleRadians,
        const FVector2D& InExtents);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck] Make Oriented Box 2D From AABB")
    static FCk_OrientedBox2D
    Request_CreateFromAABB(
        const FBox2D& InBox);

    // Basic properties
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Get Origin")
    static FVector2D
    Get_Origin(
        const FCk_OrientedBox2D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Get Extents")
    static FVector2D
    Get_Extents(
        const FCk_OrientedBox2D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Get Center")
    static FVector2D
    Get_Center(
        const FCk_OrientedBox2D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Get X Axis")
    static FVector2D
    Get_XAxis(
        const FCk_OrientedBox2D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Get Y Axis")
    static FVector2D
    Get_YAxis(
        const FCk_OrientedBox2D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Get Area")
    static float
    Get_Area(
        const FCk_OrientedBox2D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Get Perimeter")
    static float
    Get_Perimeter(
        const FCk_OrientedBox2D& InBox);

    // Spatial queries
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Contains Point")
    static bool
    Get_Contains(
        const FCk_OrientedBox2D& InBox,
        const FVector2D& InPoint);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Distance Squared")
    static float
    Get_DistanceSquared(
        const FCk_OrientedBox2D& InBox,
        const FVector2D& InPoint);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Closest Point")
    static FVector2D
    Get_ClosestPoint(
        const FCk_OrientedBox2D& InBox,
        const FVector2D& InPoint);

    // Corner access
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Get Corner")
    static FVector2D
    Get_Corner(
        const FCk_OrientedBox2D& InBox,
        int32 InIndex);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Get All Corners")
    static TArray<FVector2D>
    Get_AllCorners(
        const FCk_OrientedBox2D& InBox);

    // // Delegate-based corner enumeration
    // UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox2D",
    //     DisplayName="[Ck][OrientedBox2D] Enumerate Corners")
    // static void
    // Request_EnumerateCorners(
    //     const FCk_OrientedBox2D& InBox,
    //     const FCk_Delegate_Vector2D& InCornerDelegate);
    //
    // UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox2D",
    //     DisplayName="[Ck][OrientedBox2D] Test Corners")
    // static bool
    // Get_TestCorners(
    //     const FCk_OrientedBox2D& InBox,
    //     const FCk_Delegate_Vector2D_Predicate& InCornerPredicate);

    // Coordinate transformations
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] To Local Space")
    static FVector2D
    Get_ToLocalSpace(
        const FCk_OrientedBox2D& InBox,
        const FVector2D& InPoint);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] From Local Space")
    static FVector2D
    Get_FromLocalSpace(
        const FCk_OrientedBox2D& InBox,
        const FVector2D& InPoint);

    // Mutation functions
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Request Set Origin")
    static void
    Request_SetOrigin(
        UPARAM(ref) FCk_OrientedBox2D& InBox,
        const FVector2D& InOrigin);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Request Set Extents")
    static void
    Request_SetExtents(
        UPARAM(ref) FCk_OrientedBox2D& InBox,
        const FVector2D& InExtents);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Request Set Angle Radians")
    static void
    Request_SetAngleRadians(
        UPARAM(ref) FCk_OrientedBox2D& InBox,
        float InAngleRadians);

    // Static utilities
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Unit Zero Centered")
    static FCk_OrientedBox2D
    Get_UnitZeroCentered();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox2D",
        DisplayName="[Ck][OrientedBox2D] Unit Positive")
    static FCk_OrientedBox2D
    Get_UnitPositive();

    // Debug drawing
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox2D|Debug",
        DisplayName="[Ck][OrientedBox2D] Debug Draw",
        meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DebugDraw_OrientedBox2D(
        const UObject* InWorldContextObject,
        const FCk_OrientedBox2D& InBox,
        const FLinearColor& InColor = FLinearColor::Red,
        float InDuration = 0.0f,
        float InThickness = 2.0f,
        float InZHeight = 0.0f);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_Utils_OrientedBox3D_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_OrientedBox3D_UE);

public:
    // Creation functions
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck] Make Oriented Box 3D")
    static FCk_OrientedBox3D
    Request_Create(
        const FVector& InOrigin,
        const FVector& InExtents);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck] Make Oriented Box 3D With Frame")
    static FCk_OrientedBox3D
    Request_CreateWithFrame(
        const FCk_Frame3D& InFrame,
        const FVector& InExtents);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck] Make Oriented Box 3D From AABB")
    static FCk_OrientedBox3D
    Request_CreateFromAABB(
        const FBox& InBox);

    // Basic properties
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Get Frame")
    static FCk_Frame3D
    Get_Frame(
        const FCk_OrientedBox3D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Get Extents")
    static FVector
    Get_Extents(
        const FCk_OrientedBox3D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Get Center")
    static FVector
    Get_Center(
        const FCk_OrientedBox3D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Get X Axis")
    static FVector
    Get_XAxis(
        const FCk_OrientedBox3D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Get Y Axis")
    static FVector
    Get_YAxis(
        const FCk_OrientedBox3D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Get Z Axis")
    static FVector
    Get_ZAxis(
        const FCk_OrientedBox3D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Get Volume")
    static float
    Get_Volume(
        const FCk_OrientedBox3D& InBox);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Get Surface Area")
    static float
    Get_SurfaceArea(
        const FCk_OrientedBox3D& InBox);

    // Spatial queries
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Contains Point")
    static bool
    Get_Contains(
        const FCk_OrientedBox3D& InBox,
        const FVector& InPoint);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Distance Squared")
    static float
    Get_DistanceSquared(
        const FCk_OrientedBox3D& InBox,
        const FVector& InPoint);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Signed Distance")
    static float
    Get_SignedDistance(
        const FCk_OrientedBox3D& InBox,
        const FVector& InPoint);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Closest Point")
    static FVector
    Get_ClosestPoint(
        const FCk_OrientedBox3D& InBox,
        const FVector& InPoint);

    // Corner access
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Get Corner")
    static FVector
    Get_Corner(
        const FCk_OrientedBox3D& InBox,
        int32 InIndex);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Get All Corners")
    static TArray<FVector>
    Get_AllCorners(
        const FCk_OrientedBox3D& InBox);

    // Delegate-based corner enumeration
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Enumerate Corners")
    static void
    Request_EnumerateCorners(
        const FCk_OrientedBox3D& InBox,
        const FCk_Delegate_Vector& InCornerDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Test Corners")
    static bool
    Get_TestCorners(
        const FCk_OrientedBox3D& InBox,
        const FCk_Delegate_Vector_Predicate& InCornerPredicate);

    // Mutation functions
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Request Set Frame")
    static void
    Request_SetFrame(
        UPARAM(ref) FCk_OrientedBox3D& InBox,
        const FCk_Frame3D& InFrame);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Request Set Extents")
    static void
    Request_SetExtents(
        UPARAM(ref) FCk_OrientedBox3D& InBox,
        const FVector& InExtents);

    // Static utilities
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Unit Zero Centered")
    static FCk_OrientedBox3D
    Get_UnitZeroCentered();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|OrientedBox3D",
        DisplayName="[Ck][OrientedBox3D] Unit Positive")
    static FCk_OrientedBox3D
    Get_UnitPositive();

    // Debug drawing
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|OrientedBox3D|Debug",
              DisplayName="[Ck][OrientedBox3D] Debug Draw",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DebugDraw_OrientedBox3D(
        const UObject* InWorldContextObject,
        const FCk_OrientedBox3D& InBox,
        const FLinearColor& InColor = FLinearColor::Red,
        float InDuration = 0.0f,
        float InThickness = 2.0f);
};