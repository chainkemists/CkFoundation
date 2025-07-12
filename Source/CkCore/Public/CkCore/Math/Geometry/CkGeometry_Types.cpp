#include "CkGeometry_Types.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Frame3D::
    FCk_Frame3D()
{
    _Frame = InternalFrameType{};
}

FCk_Frame3D::
    FCk_Frame3D(
        const FVector& InOrigin)
{
    _Frame = InternalFrameType{InOrigin};
}

FCk_Frame3D::
    FCk_Frame3D(
        const FVector& InOrigin,
        const FQuat& InRotation)
{
    _Frame = InternalFrameType{InOrigin, UE::Geometry::TQuaternion<double>{InRotation.X, InRotation.Y, InRotation.Z, InRotation.W}};
}

FCk_Frame3D::
    FCk_Frame3D(
        const FTransform& InTransform)
{
    _Frame = InternalFrameType{InTransform};
}

auto
    FCk_Frame3D::
    Get_InternalFrame() const
    -> const InternalFrameType&
{
    return _Frame;
}

auto
    FCk_Frame3D::
    Get_InternalFrame()
    -> InternalFrameType&
{
    return _Frame;
}

auto
    FCk_Frame3D::
    Get_Origin() const
    -> FVector
{
    return _Frame.Origin;
}

auto
    FCk_Frame3D::
    Get_Rotation() const
    -> FQuat
{
    const auto& InternalQuat = _Frame.Rotation;
    return FQuat{InternalQuat.X, InternalQuat.Y, InternalQuat.Z, InternalQuat.W};
}

auto
    FCk_Frame3D::
    Request_SetOrigin(
        const FVector& InOrigin)
    -> void
{
    _Frame.Origin = InOrigin;
}

auto
    FCk_Frame3D::
    Request_SetRotation(
        const FQuat& InRotation)
    -> void
{
    _Frame.Rotation = UE::Geometry::TQuaternion<double>{InRotation.X, InRotation.Y, InRotation.Z, InRotation.W};
}

auto
    FCk_Frame3D::
    Get_X() const
    -> FVector
{
    return _Frame.X();
}

auto
    FCk_Frame3D::
    Get_Y() const
    -> FVector
{
    return _Frame.Y();
}

auto
    FCk_Frame3D::
    Get_Z() const
    -> FVector
{
    return _Frame.Z();
}

auto
    FCk_Frame3D::
    Get_Axis(
        int32 InAxisIndex) const
    -> FVector
{
    return _Frame.GetAxis(InAxisIndex);
}

auto
    FCk_Frame3D::
    Get_Transform() const
    -> FTransform
{
    return _Frame.ToFTransform();
}

auto
    FCk_Frame3D::
    Get_InverseTransform() const
    -> FTransform
{
    return _Frame.ToInverseFTransform();
}

auto
    FCk_Frame3D::
    Get_PointAt(
        float InX,
        float InY,
        float InZ) const
    -> FVector
{
    return _Frame.PointAt(InX, InY, InZ);
}

auto
    FCk_Frame3D::
    Get_PointAt(
        const FVector& InPoint) const
    -> FVector
{
    return _Frame.PointAt(InPoint);
}

auto
    FCk_Frame3D::
    Get_ToFramePoint(
        const FVector& InPoint) const
    -> FVector
{
    return _Frame.ToFramePoint(InPoint);
}

auto
    FCk_Frame3D::
    Get_FromFramePoint(
        const FVector& InPoint) const
    -> FVector
{
    return _Frame.FromFramePoint(InPoint);
}

auto
    FCk_Frame3D::
    Get_ToFrameVector(
        const FVector& InVector) const
    -> FVector
{
    return _Frame.ToFrameVector(InVector);
}

auto
    FCk_Frame3D::
    Get_FromFrameVector(
        const FVector& InVector) const
    -> FVector
{
    return _Frame.FromFrameVector(InVector);
}

auto
    FCk_Frame3D::
    Get_ToPlaneUV(
        const FVector& InPos,
        int32 InPlaneNormalAxis) const
    -> FVector2D
{
    return _Frame.ToPlaneUV(InPos, InPlaneNormalAxis);
}

auto
    FCk_Frame3D::
    Get_FromPlaneUV(
        const FVector2D& InPosUV,
        int32 InPlaneNormalAxis) const
    -> FVector
{
    return _Frame.FromPlaneUV(InPosUV, InPlaneNormalAxis);
}

auto
    FCk_Frame3D::
    Get_ToPlane(
        const FVector& InPos,
        int32 InPlaneNormalAxis) const
    -> FVector
{
    return _Frame.ToPlane(InPos, InPlaneNormalAxis);
}

auto
    FCk_Frame3D::
    Request_Rotate(
        const FQuat& InQuat)
    -> void
{
    _Frame.Rotate(UE::Geometry::TQuaternion<double>{InQuat.X, InQuat.Y, InQuat.Z, InQuat.W});
}

auto
    FCk_Frame3D::
    Request_Transform(
        const FTransform& InTransform)
    -> void
{
    _Frame.Transform(InTransform);
}

auto
    FCk_Frame3D::
    Request_AlignAxis(
        int32 InAxisIndex,
        const FVector& InToDirection)
    -> void
{
    _Frame.AlignAxis(InAxisIndex, InToDirection);
}

auto
    FCk_Frame3D::
    Request_ConstrainedAlignAxis(
        int32 InAxisIndex,
        const FVector& InToDirection,
        const FVector& InAroundVector)
    -> void
{
    _Frame.ConstrainedAlignAxis(InAxisIndex, InToDirection, InAroundVector);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_OrientedBox2D::
    FCk_OrientedBox2D()
{
    _OrientedBox = InternalBox2Type{};
}

FCk_OrientedBox2D::
    FCk_OrientedBox2D(
        const FVector2D& InOrigin,
        const FVector2D& InExtents)
{
    _OrientedBox = InternalBox2Type{InOrigin, InExtents};
}

FCk_OrientedBox2D::
    FCk_OrientedBox2D(
        const FVector2D& InOrigin,
        const FVector2D& InXAxis,
        const FVector2D& InExtents)
{
    _OrientedBox = InternalBox2Type{InOrigin, InXAxis, InExtents};
}

FCk_OrientedBox2D::
    FCk_OrientedBox2D(
        const FVector2D& InOrigin,
        float InAngleRadians,
        const FVector2D& InExtents)
{
    _OrientedBox = InternalBox2Type{InOrigin, InAngleRadians, InExtents};
}

FCk_OrientedBox2D::
    FCk_OrientedBox2D(
        const FBox2D& InAxisBox)
{
    _OrientedBox = InternalBox2Type{UE::Geometry::FAxisAlignedBox2d{InAxisBox.Min, InAxisBox.Max}};
}

auto
    FCk_OrientedBox2D::
    Get_InternalBox() const
    -> const InternalBox2Type&
{
    return _OrientedBox;
}

auto
    FCk_OrientedBox2D::
    Get_InternalBox()
    -> InternalBox2Type&
{
    return _OrientedBox;
}

auto
    FCk_OrientedBox2D::
    Get_Origin() const
    -> FVector2D
{
    return _OrientedBox.Origin;
}

auto
    FCk_OrientedBox2D::
    Get_UnitAxisX() const
    -> FVector2D
{
    return _OrientedBox.UnitAxisX;
}

auto
    FCk_OrientedBox2D::
    Get_Extents() const
    -> FVector2D
{
    return _OrientedBox.Extents;
}

auto
    FCk_OrientedBox2D::
    Request_SetOrigin(
        const FVector2D& InOrigin)
    -> void
{
    _OrientedBox.Origin = InOrigin;
}

auto
    FCk_OrientedBox2D::
    Request_SetExtents(
        const FVector2D& InExtents)
    -> void
{
    _OrientedBox.Extents = InExtents;
}

auto
    FCk_OrientedBox2D::
    Request_SetAngleRadians(
        float InAngleRadians)
    -> void
{
    _OrientedBox.SetAngleRadians(InAngleRadians);
}

auto
    FCk_OrientedBox2D::
    Get_Center() const
    -> FVector2D
{
    return _OrientedBox.Center();
}

auto
    FCk_OrientedBox2D::
    Get_AxisX() const
    -> FVector2D
{
    return _OrientedBox.AxisX();
}

auto
    FCk_OrientedBox2D::
    Get_AxisY() const
    -> FVector2D
{
    return _OrientedBox.AxisY();
}

auto
    FCk_OrientedBox2D::
    Get_Axis(
        int32 InAxisIndex) const
    -> FVector2D
{
    return _OrientedBox.GetAxis(InAxisIndex);
}

auto
    FCk_OrientedBox2D::
    Get_Diagonal() const
    -> FVector2D
{
    return _OrientedBox.Diagonal();
}

auto
    FCk_OrientedBox2D::
    Get_Area() const
    -> float
{
    return _OrientedBox.Area();
}

auto
    FCk_OrientedBox2D::
    Get_Perimeter() const
    -> float
{
    return _OrientedBox.Perimeter();
}

auto
    FCk_OrientedBox2D::
    Get_ToLocalSpace(
        const FVector2D& InPoint) const
    -> FVector2D
{
    return _OrientedBox.ToLocalSpace(InPoint);
}

auto
    FCk_OrientedBox2D::
    Get_FromLocalSpace(
        const FVector2D& InPoint) const
    -> FVector2D
{
    return _OrientedBox.FromLocalSpace(InPoint);
}

auto
    FCk_OrientedBox2D::
    Get_Contains(
        const FVector2D& InPoint) const
    -> bool
{
    return _OrientedBox.Contains(InPoint);
}

auto
    FCk_OrientedBox2D::
    Get_DistanceSquared(
        const FVector2D& InPoint) const
    -> float
{
    return _OrientedBox.DistanceSquared(InPoint);
}

auto
    FCk_OrientedBox2D::
    Get_ClosestPoint(
        const FVector2D& InPoint) const
    -> FVector2D
{
    return _OrientedBox.ClosestPoint(InPoint);
}

auto
    FCk_OrientedBox2D::
    Get_Corner(
        int32 InIndex) const
    -> FVector2D
{
    return _OrientedBox.GetCorner(InIndex);
}

auto
    FCk_OrientedBox2D::
    Get_AllCorners() const
    -> TArray<FVector2D>
{
    auto Corners = TArray<FVector2D>{};
    Corners.Reserve(4);
    for (auto I = 0; I < 4; ++I)
    {
        Corners.Add(Get_Corner(I));
    }
    return Corners;
}

auto
    FCk_OrientedBox2D::
    Get_UnitZeroCentered()
    -> FCk_OrientedBox2D
{
    auto Result = FCk_OrientedBox2D{};
    Result._OrientedBox = InternalBox2Type::UnitZeroCentered();
    return Result;
}

auto
    FCk_OrientedBox2D::
    Get_UnitPositive()
    -> FCk_OrientedBox2D
{
    auto Result = FCk_OrientedBox2D{};
    Result._OrientedBox = InternalBox2Type::UnitPositive();
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_OrientedBox3D::
    FCk_OrientedBox3D()
{
    _OrientedBox = InternalBox3Type{};
}

FCk_OrientedBox3D::
    FCk_OrientedBox3D(
        const FVector& InOrigin,
        const FVector& InExtents)
{
    _OrientedBox = InternalBox3Type{InOrigin, InExtents};
}

FCk_OrientedBox3D::
    FCk_OrientedBox3D(
        const FCk_Frame3D& InFrame,
        const FVector& InExtents)
{
    _OrientedBox = InternalBox3Type{InFrame.Get_InternalFrame(), InExtents};
}

FCk_OrientedBox3D::
    FCk_OrientedBox3D(
        const FBox& InAxisBox)
{
    _OrientedBox = InternalBox3Type{UE::Geometry::FAxisAlignedBox3d{InAxisBox.Min, InAxisBox.Max}};
}

auto
    FCk_OrientedBox3D::
    Get_InternalBox() const
    -> const InternalBox3Type&
{
    return _OrientedBox;
}

auto
    FCk_OrientedBox3D::
    Get_InternalBox()
    -> InternalBox3Type&
{
    return _OrientedBox;
}

auto
    FCk_OrientedBox3D::
    Get_Frame() const
    -> FCk_Frame3D
{
    auto Result = FCk_Frame3D{};
    Result.Get_InternalFrame() = _OrientedBox.Frame;
    return Result;
}

auto
    FCk_OrientedBox3D::
    Get_Extents() const
    -> FVector
{
    return _OrientedBox.Extents;
}

auto
    FCk_OrientedBox3D::
    Request_SetFrame(
        const FCk_Frame3D& InFrame)
    -> void
{
    _OrientedBox.Frame = InFrame.Get_InternalFrame();
}

auto
    FCk_OrientedBox3D::
    Request_SetExtents(
        const FVector& InExtents)
    -> void
{
    _OrientedBox.Extents = InExtents;
}

auto
    FCk_OrientedBox3D::
    Get_Center() const
    -> FVector
{
    return _OrientedBox.Center();
}

auto
    FCk_OrientedBox3D::
    Get_AxisX() const
    -> FVector
{
    return _OrientedBox.AxisX();
}

auto
    FCk_OrientedBox3D::
    Get_AxisY() const
    -> FVector
{
    return _OrientedBox.AxisY();
}

auto
    FCk_OrientedBox3D::
    Get_AxisZ() const
    -> FVector
{
    return _OrientedBox.AxisZ();
}

auto
    FCk_OrientedBox3D::
    Get_Axis(
        int32 InAxisIndex) const
    -> FVector
{
    return _OrientedBox.GetAxis(InAxisIndex);
}

auto
    FCk_OrientedBox3D::
    Get_Diagonal() const
    -> FVector
{
    return _OrientedBox.Diagonal();
}

auto
    FCk_OrientedBox3D::
    Get_Volume() const
    -> float
{
    return _OrientedBox.Volume();
}

auto
    FCk_OrientedBox3D::
    Get_SurfaceArea() const
    -> float
{
    return _OrientedBox.SurfaceArea();
}

auto
    FCk_OrientedBox3D::
    Get_Contains(
        const FVector& InPoint) const
    -> bool
{
    return _OrientedBox.Contains(InPoint);
}

auto
    FCk_OrientedBox3D::
    Get_DistanceSquared(
        const FVector& InPoint) const
    -> float
{
    return _OrientedBox.DistanceSquared(InPoint);
}

auto
    FCk_OrientedBox3D::
    Get_SignedDistance(
        const FVector& InPoint) const
    -> float
{
    return _OrientedBox.SignedDistance(InPoint);
}

auto
    FCk_OrientedBox3D::
    Get_ClosestPoint(
        const FVector& InPoint) const
    -> FVector
{
    return _OrientedBox.ClosestPoint(InPoint);
}

auto
    FCk_OrientedBox3D::
    Get_Corner(
        int32 InIndex) const
    -> FVector
{
    return _OrientedBox.GetCorner(InIndex);
}

auto
    FCk_OrientedBox3D::
    Get_AllCorners() const
    -> TArray<FVector>
{
    auto Corners = TArray<FVector>{};
    Corners.Reserve(8);
    for (auto I = 0; I < 8; ++I)
    {
        Corners.Add(Get_Corner(I));
    }
    return Corners;
}

auto
    FCk_OrientedBox3D::
    Get_UnitZeroCentered()
    -> FCk_OrientedBox3D
{
    auto Result = FCk_OrientedBox3D{};
    Result._OrientedBox = InternalBox3Type::UnitZeroCentered();
    return Result;
}

auto
    FCk_OrientedBox3D::
    Get_UnitPositive()
    -> FCk_OrientedBox3D
{
    auto Result = FCk_OrientedBox3D{};
    Result._OrientedBox = InternalBox3Type::UnitPositive();
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------