#pragma once

#include "CoreMinimal.h"
#include "GeometryTypes.h"
#include "OrientedBoxTypes.h"
#include "FrameTypes.h"

#include "CkGeometry_Types.generated.h"

// Forward declarations
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_Vector2D, FVector2D, Corner);
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_Vector, FVector, Corner);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FCk_Delegate_Vector2D_Predicate, FVector2D, Corner);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FCk_Delegate_Vector_Predicate, FVector, Corner);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_LineSegment
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LineSegment);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector _LineStart = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector _LineEnd = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_LineStart);
    CK_PROPERTY_GET(_LineEnd);

    CK_DEFINE_CONSTRUCTORS(FCk_LineSegment, _LineStart, _LineEnd);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_LineSegment2D
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LineSegment2D);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _LineStart = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _LineEnd = FVector2D::ZeroVector;

public:
    CK_PROPERTY_GET(_LineStart);
    CK_PROPERTY_GET(_LineEnd);

    CK_DEFINE_CONSTRUCTORS(FCk_LineSegment2D, _LineStart, _LineEnd);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Frame3D
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Frame3D);

private:
    using InternalFrameType = UE::Geometry::FFrame3d;

    InternalFrameType _Frame;

public:
    // Constructors
    FCk_Frame3D();

    FCk_Frame3D(
        const FVector& InOrigin);

    FCk_Frame3D(
        const FVector& InOrigin,
        const FQuat& InRotation);

    FCk_Frame3D(
        const FTransform& InTransform);

    // Access to internal frame
    auto
    Get_InternalFrame() const -> const InternalFrameType&;

    auto
    Get_InternalFrame() -> InternalFrameType&;

    // Basic properties
    auto
    Get_Origin() const -> FVector;

    auto
    Get_Rotation() const -> FQuat;

    auto
    Request_SetOrigin(
        const FVector& InOrigin) -> void;

    auto
    Request_SetRotation(
        const FQuat& InRotation) -> void;

    // Axis access
    auto
    Get_X() const -> FVector;

    auto
    Get_Y() const -> FVector;

    auto
    Get_Z() const -> FVector;

    auto
    Get_Axis(
        int32 InAxisIndex) const -> FVector;

    // Conversions
    auto
    Get_Transform() const -> FTransform;

    auto
    Get_InverseTransform() const -> FTransform;

    // Point/Vector transformations
    auto
    Get_PointAt(
        float InX,
        float InY,
        float InZ) const -> FVector;

    auto
    Get_PointAt(
        const FVector& InPoint) const -> FVector;

    auto
    Get_ToFramePoint(
        const FVector& InPoint) const -> FVector;

    auto
    Get_FromFramePoint(
        const FVector& InPoint) const -> FVector;

    auto
    Get_ToFrameVector(
        const FVector& InVector) const -> FVector;

    auto
    Get_FromFrameVector(
        const FVector& InVector) const -> FVector;

    // Plane operations
    auto
    Get_ToPlaneUV(
        const FVector& InPos,
        int32 InPlaneNormalAxis = 2) const -> FVector2D;

    auto
    Get_FromPlaneUV(
        const FVector2D& InPosUV,
        int32 InPlaneNormalAxis = 2) const -> FVector;

    auto
    Get_ToPlane(
        const FVector& InPos,
        int32 InPlaneNormalAxis = 2) const -> FVector;

    // Transformations
    auto
    Request_Rotate(
        const FQuat& InQuat) -> void;

    auto
    Request_Transform(
        const FTransform& InTransform) -> void;

    auto
    Request_AlignAxis(
        int32 InAxisIndex,
        const FVector& InToDirection) -> void;

    auto
    Request_ConstrainedAlignAxis(
        int32 InAxisIndex,
        const FVector& InToDirection,
        const FVector& InAroundVector) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_OrientedBox2D
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_OrientedBox2D);

private:
    using InternalBox2Type = UE::Geometry::FOrientedBox2d;

    // Internal UE::Geometry::FOrientedBox2d
    InternalBox2Type _OrientedBox;

public:
    // Constructors
    FCk_OrientedBox2D();

    FCk_OrientedBox2D(
        const FVector2D& InOrigin,
        const FVector2D& InExtents);

    FCk_OrientedBox2D(
        const FVector2D& InOrigin,
        const FVector2D& InXAxis,
        const FVector2D& InExtents);

    FCk_OrientedBox2D(
        const FVector2D& InOrigin,
        float InAngleRadians,
        const FVector2D& InExtents);

    FCk_OrientedBox2D(
        const FBox2D& InAxisBox);

    // Access to internal box
    auto
    Get_InternalBox() const -> const InternalBox2Type&;

    auto
    Get_InternalBox() -> InternalBox2Type&;

    // Basic properties
    auto
    Get_Origin() const -> FVector2D;

    auto
    Get_UnitAxisX() const -> FVector2D;

    auto
    Get_Extents() const -> FVector2D;

    auto
    Request_SetOrigin(
        const FVector2D& InOrigin) -> void;

    auto
    Request_SetExtents(
        const FVector2D& InExtents) -> void;

    auto
    Request_SetAngleRadians(
        float InAngleRadians) -> void;

    // Derived properties
    auto
    Get_Center() const -> FVector2D;

    auto
    Get_AxisX() const -> FVector2D;

    auto
    Get_AxisY() const -> FVector2D;

    auto
    Get_Axis(
        int32 InAxisIndex) const -> FVector2D;

    auto
    Get_Diagonal() const -> FVector2D;

    auto
    Get_Area() const -> float;

    auto
    Get_Perimeter() const -> float;

    auto
    Get_ToLocalSpace(
        const FVector2D& InPoint) const -> FVector2D;

    auto
    Get_FromLocalSpace(
        const FVector2D& InPoint) const -> FVector2D;

    auto
    Get_Contains(
        const FVector2D& InPoint) const -> bool;

    auto
    Get_DistanceSquared(
        const FVector2D& InPoint) const -> float;

    auto
    Get_ClosestPoint(
        const FVector2D& InPoint) const -> FVector2D;

    auto
    Get_Corner(
        int32 InIndex) const -> FVector2D;

    auto
    Get_AllCorners() const -> TArray<FVector2D>;

    static auto
    Get_UnitZeroCentered() -> FCk_OrientedBox2D;

    static auto
    Get_UnitPositive() -> FCk_OrientedBox2D;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_OrientedBox3D
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_OrientedBox3D);

private:
    using InternalBox3Type = UE::Geometry::FOrientedBox3d;

    // Internal UE::Geometry::FOrientedBox3d
    InternalBox3Type _OrientedBox;

public:
    // Constructors
    FCk_OrientedBox3D();

    FCk_OrientedBox3D(
        const FVector& InOrigin,
        const FVector& InExtents);

    FCk_OrientedBox3D(
        const FCk_Frame3D& InFrame,
        const FVector& InExtents);

    FCk_OrientedBox3D(
        const FBox& InAxisBox);

    auto
    Get_InternalBox() const -> const InternalBox3Type&;

    auto
    Get_InternalBox() -> InternalBox3Type&;

    auto
    Get_Frame() const -> FCk_Frame3D;

    auto
    Get_Extents() const -> FVector;

    auto
    Request_SetFrame(
        const FCk_Frame3D& InFrame) -> void;

    auto
    Request_SetExtents(
        const FVector& InExtents) -> void;

    auto
    Get_Center() const -> FVector;

    auto
    Get_AxisX() const -> FVector;

    auto
    Get_AxisY() const -> FVector;

    auto
    Get_AxisZ() const -> FVector;

    auto
    Get_Axis(
        int32 InAxisIndex) const -> FVector;

    auto
    Get_Diagonal() const -> FVector;

    auto
    Get_Volume() const -> float;

    auto
    Get_SurfaceArea() const -> float;

    auto
    Get_Contains(
        const FVector& InPoint) const -> bool;

    auto
    Get_DistanceSquared(
        const FVector& InPoint) const -> float;

    auto
    Get_SignedDistance(
        const FVector& InPoint) const -> float;

    auto
    Get_ClosestPoint(
        const FVector& InPoint) const -> FVector;

    auto
    Get_Corner(
        int32 InIndex) const -> FVector;

    auto
    Get_AllCorners() const -> TArray<FVector>;

    static auto
    Get_UnitZeroCentered() -> FCk_OrientedBox3D;

    static auto
    Get_UnitPositive() -> FCk_OrientedBox3D;
};

// --------------------------------------------------------------------------------------------------------------------