#include "CkDebugScene/CkDebugScene_ShapeBuilder.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkDebugScene/CkDebugScene_Materials.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_debug_scene_shape_builder
{
    // Render class ids match the convention the existing debuggers already publish with, so a
    // builder-authored shape hides and shows with the same Set_RenderClassVisible toggles.
    constexpr uint8 OpaqueClassId = 1;
    constexpr uint8 TransparentClassId = 2;

    // Rotation carrying +X onto the from->to direction, used by the segment-shaped primitives.
    auto Make_DirectionRotation(const FVector& InFrom, const FVector& InTo) -> FRotator
    {
        const auto Delta = InTo - InFrom;
        return Delta.IsNearlyZero() ? FRotator::ZeroRotator : Delta.Rotation();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DebugScene_ShapeBuilder::
    Set_Color(FLinearColor InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    _Color = InColor;
    return *this;
}

auto
    FCk_DebugScene_ShapeBuilder::
    Set_Opacity(float InOpacity)
    -> FCk_DebugScene_ShapeBuilder&
{
    _Opacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
    return *this;
}

auto
    FCk_DebugScene_ShapeBuilder::
    Set_DepthPriority(ECk_DebugScene_DepthPriority InDepthPriority)
    -> FCk_DebugScene_ShapeBuilder&
{
    _DepthPriority = InDepthPriority;
    return *this;
}

auto
    FCk_DebugScene_ShapeBuilder::
    Set_PickIdentity(uint64 InPickIdentity)
    -> FCk_DebugScene_ShapeBuilder&
{
    _PickIdentity = InPickIdentity;
    return *this;
}

auto
    FCk_DebugScene_ShapeBuilder::
    Make_Appearance(const TOptional<FLinearColor>& InColor) const
    -> FCk_DebugScene_Appearance
{
    const auto IsTransparent = _Opacity < 1.0f;
    return FCk_DebugScene_Appearance{}
        .Set_BaseMaterial(IsTransparent ? ck::debug_scene::materials::TryGet_Translucent()
                                        : ck::debug_scene::materials::TryGet_Opaque())
        .Set_RenderClass(IsTransparent ? ECk_DebugScene_RenderClass::Transparent
                                       : ECk_DebugScene_RenderClass::Opaque)
        .Set_RenderClassId(IsTransparent ? ck_debug_scene_shape_builder::TransparentClassId
                                         : ck_debug_scene_shape_builder::OpaqueClassId)
        .Set_Color(InColor.Get(_Color))
        .Set_Opacity(_Opacity)
        .Set_DepthPriority(_DepthPriority);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Mesh(TSharedPtr<FCk_DebugScene_Mesh> InMesh, const FTransform& InTransform,
        const TOptional<FLinearColor>& InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    if (NOT InMesh.IsValid())
    { return *this; }

    _Instances.Add(FCk_DebugScene_Instance{}
        .Set_Mesh(MoveTemp(InMesh))
        .Set_Transform(InTransform)
        .Set_Appearance(Make_Appearance(InColor))
        .Set_PickIdentity(_PickIdentity));
    return *this;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Box(const FVector& InCenter, const FVector& InExtent, const FRotator& InRotation,
        TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    return Add_Mesh(ck::debug_scene::shapes::Get_Box(), FTransform{InRotation, InCenter, InExtent}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Sphere(const FVector& InCenter, float InRadius, TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    return Add_Mesh(ck::debug_scene::shapes::Get_Sphere(),
                    FTransform{FRotator::ZeroRotator, InCenter, FVector{InRadius}}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Cylinder(const FVector& InCenter, float InRadius, float InHeight, const FRotator& InRotation,
        TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    return Add_Mesh(ck::debug_scene::shapes::Get_Cylinder(),
                    FTransform{InRotation, InCenter, FVector{InRadius, InRadius, InHeight}}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Cone(const FVector& InBaseCenter, float InRadius, float InHeight, const FRotator& InRotation,
        TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    // The unit cone straddles the origin; lift by half the height so the caller's point is the BASE,
    // which is how a cone is normally placed (on a surface, pointing away from it).
    const auto Center = InBaseCenter + InRotation.RotateVector(FVector{0.0, 0.0, InHeight * 0.5});
    return Add_Mesh(ck::debug_scene::shapes::Get_Cone(),
                    FTransform{InRotation, Center, FVector{InRadius, InRadius, InHeight}}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Capsule(const FVector& InCenter, float InRadius, float InSegmentHeight, const FRotator& InRotation,
        TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    // UNIFORM scale, with the proportion baked into the mesh. Scaling Z alone would stretch the
    // hemispherical caps into ellipsoids -- correct only in the accident where radius equals height.
    if (FMath::IsNearlyZero(InRadius))
    { return *this; }

    const auto SegmentRatio = FMath::Max(InSegmentHeight, 0.0f) / InRadius;
    return Add_Mesh(ck::debug_scene::shapes::Get_Capsule(SegmentRatio),
                    FTransform{InRotation, InCenter, FVector{InRadius}}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Torus(const FVector& InCenter, float InOuterRadius, float InTubeRatio, const FRotator& InRotation,
        TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    return Add_Mesh(ck::debug_scene::shapes::Get_Torus(InTubeRatio),
                    FTransform{InRotation, InCenter, FVector{InOuterRadius}}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Bar(const FVector& InFrom, const FVector& InTo, float InRadius, TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    const auto Delta = InTo - InFrom;
    const auto Length = Delta.Size();
    if (FMath::IsNearlyZero(Length))
    { return *this; }

    // The unit cylinder runs along +Z, so orient +Z (not +X) onto the segment.
    const auto Rotation = FRotationMatrix::MakeFromZ(Delta / Length).Rotator();
    return Add_Mesh(ck::debug_scene::shapes::Get_Cylinder(),
                    FTransform{Rotation, InFrom + Delta * 0.5, FVector{InRadius, InRadius, Length}}, InColor);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Quad(const FVector& InCenter, const FVector2D& InSize, const FRotator& InRotation,
        TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    return Add_Mesh(ck::debug_scene::shapes::Get_Quad(),
                    FTransform{InRotation, InCenter, FVector{InSize.X, InSize.Y, 1.0}}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Disc(const FVector& InCenter, float InRadius, const FRotator& InRotation, TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    return Add_Mesh(ck::debug_scene::shapes::Get_Disc(),
                    FTransform{InRotation, InCenter, FVector{InRadius, InRadius, 1.0}}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Ring(const FVector& InCenter, float InOuterRadius, float InInnerRatio, const FRotator& InRotation,
        TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    return Add_Mesh(ck::debug_scene::shapes::Get_Ring(InInnerRatio),
                    FTransform{InRotation, InCenter, FVector{InOuterRadius, InOuterRadius, 1.0}}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Triangle(const FVector& InCenter, float InRadius, const FRotator& InRotation, TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    return Add_Mesh(ck::debug_scene::shapes::Get_Triangle(),
                    FTransform{InRotation, InCenter, FVector{InRadius, InRadius, 1.0}}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Cross(const FVector& InCenter, float InSize, float InThicknessRatio, const FRotator& InRotation,
        TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    return Add_Mesh(ck::debug_scene::shapes::Get_Cross(InThicknessRatio),
                    FTransform{InRotation, InCenter, FVector{InSize, InSize, 1.0}}, InColor);
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Arrow(const FVector& InFrom, const FVector& InTo, float InWidth, TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    const auto Delta = InTo - InFrom;
    const auto Length = Delta.Size();
    if (FMath::IsNearlyZero(Length))
    { return *this; }

    const auto Rotation = ck_debug_scene_shape_builder::Make_DirectionRotation(InFrom, InTo);
    return Add_Mesh(ck::debug_scene::shapes::Get_Arrow(),
                    FTransform{Rotation, InFrom + Delta * 0.5, FVector{Length, InWidth, 1.0}}, InColor);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Line(const FVector& InFrom, const FVector& InTo, float InThickness, TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    _Lines.Add(FCk_DebugScene_Line{InFrom, InTo, InColor.Get(_Color), InThickness});
    return *this;
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_Circle(const FVector& InCenter, float InRadius, int32 InSegments, float InThickness,
        const FRotator& InRotation, TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    const auto Segments = FMath::Max(InSegments, 3);
    const auto Color = InColor.Get(_Color);
    const auto At = [&InCenter, &InRotation, InRadius, Segments](int32 InIndex)
    {
        const auto Angle = 2.0 * UE_DOUBLE_PI * (static_cast<double>(InIndex) / Segments);
        return InCenter + InRotation.RotateVector(
            FVector{InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), 0.0});
    };

    _Lines.Reserve(_Lines.Num() + Segments);
    for (auto Index = 0; Index < Segments; ++Index)
    { _Lines.Add(FCk_DebugScene_Line{At(Index), At(Index + 1), Color, InThickness}); }
    return *this;
}

auto
    FCk_DebugScene_ShapeBuilder::
    Add_WireBox(const FVector& InCenter, const FVector& InExtent, float InThickness, const FRotator& InRotation,
        TOptional<FLinearColor> InColor)
    -> FCk_DebugScene_ShapeBuilder&
{
    const auto Color = InColor.Get(_Color);
    const auto Half = InExtent * 0.5;
    const auto Corner = [&](int32 InX, int32 InY, int32 InZ)
    {
        return InCenter + InRotation.RotateVector(
            FVector{InX ? Half.X : -Half.X, InY ? Half.Y : -Half.Y, InZ ? Half.Z : -Half.Z});
    };

    _Lines.Reserve(_Lines.Num() + 12);
    for (auto Z = 0; Z < 2; ++Z)
    {
        _Lines.Add({Corner(0, 0, Z), Corner(1, 0, Z), Color, InThickness});
        _Lines.Add({Corner(1, 0, Z), Corner(1, 1, Z), Color, InThickness});
        _Lines.Add({Corner(1, 1, Z), Corner(0, 1, Z), Color, InThickness});
        _Lines.Add({Corner(0, 1, Z), Corner(0, 0, Z), Color, InThickness});
    }
    for (auto X = 0; X < 2; ++X)
    {
        for (auto Y = 0; Y < 2; ++Y)
        { _Lines.Add({Corner(X, Y, 0), Corner(X, Y, 1), Color, InThickness}); }
    }
    return *this;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DebugScene_ShapeBuilder::
    Commit(FCk_DebugScene_Target& InTarget, uint64 InItemKey, FName InLineChannel)
    -> bool
{
    // An empty builder must CLEAR what it published last time, otherwise a caller that stops drawing
    // leaves its last frame on screen forever. It cannot do that by submitting an empty array --
    // the target ensure-rejects that as an invalid submission -- so removal is the explicit path.
    auto MeshesOk = true;
    if (_Instances.IsEmpty())
    {
        InTarget.Remove_Item(InItemKey);
    }
    else
    {
        MeshesOk = InTarget.Upsert_Item(InItemKey, _Instances);
    }

    const auto LinesOk = InLineChannel.IsNone() || InTarget.Set_LineChannel(InLineChannel, _Lines);
    return MeshesOk && LinesOk;
}

auto
    FCk_DebugScene_ShapeBuilder::
    Reset()
    -> void
{
    _Instances.Reset();
    _Lines.Reset();
}

auto
    FCk_DebugScene_ShapeBuilder::
    Get_Instances() const
    -> const TArray<FCk_DebugScene_Instance>&
{
    return _Instances;
}

auto
    FCk_DebugScene_ShapeBuilder::
    Get_Lines() const
    -> const TArray<FCk_DebugScene_Line>&
{
    return _Lines;
}

auto
    FCk_DebugScene_ShapeBuilder::
    IsEmpty() const
    -> bool
{
    return _Instances.IsEmpty() && _Lines.IsEmpty();
}
