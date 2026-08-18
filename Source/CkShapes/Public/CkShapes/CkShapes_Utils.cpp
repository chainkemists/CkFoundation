#include "CkShapes_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkShapes/Box/CkShapeBox_Utils.h"
#include "CkShapes/Capsule/CkShapeCapsule_Utils.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Utils.h"
#include "CkShapes/Sphere/CkShapeSphere_Utils.h"

#include <Engine/StaticMesh.h>
#include <PhysicsEngine/BodySetup.h>

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
namespace ck_shapes_utils
{
    auto Get_OnDimensionsChanged() -> FCk_Shape_OnDimensionsChanged&
    {
        static auto Delegate = FCk_Shape_OnDimensionsChanged{};
        return Delegate;
    }
}

auto
    UCk_Utils_Shapes_UE::
    Get_OnDimensionsChanged()
    -> FCk_Shape_OnDimensionsChanged&
{
    return ck_shapes_utils::Get_OnDimensionsChanged();
}

auto
    UCk_Utils_Shapes_UE::
    Notify_DimensionsChanged(
        const FCk_Handle& InHandle)
    -> void
{
    ck_shapes_utils::Get_OnDimensionsChanged().Broadcast(InHandle);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

FCk_AnyShape::
    FCk_AnyShape(
        FCk_ShapeBox_Dimensions InDimensions)
    : _ShapeType(ECk_Shape_Type::Box)
    , _Box(InDimensions)
{
}

FCk_AnyShape::
    FCk_AnyShape(
        FCk_ShapeCapsule_Dimensions InDimensions)
    : _ShapeType(ECk_Shape_Type::Capsule)
    , _Capsule(InDimensions)
{
}

FCk_AnyShape::
    FCk_AnyShape(
        FCk_ShapeCylinder_Dimensions InDimensions)
    : _ShapeType(ECk_Shape_Type::Cylinder)
    , _Cylinder(InDimensions)
{
}

FCk_AnyShape::
    FCk_AnyShape(
        FCk_ShapeSphere_Dimensions InDimensions)
    : _ShapeType(ECk_Shape_Type::Sphere)
    , _Sphere(InDimensions)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Shapes_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    if (UCk_Utils_ShapeBox_UE::Has(InHandle))
    { return true; }

    if (UCk_Utils_ShapeSphere_UE::Has(InHandle))
    { return true; }

    if (UCk_Utils_ShapeCapsule_UE::Has(InHandle))
    { return true; }

    if (UCk_Utils_ShapeCylinder_UE::Has(InHandle))
    { return true; }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Shapes_UE::
    DoCast(
        FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult)
    -> FCk_Handle_Shape
{
    if (ck::Is_NOT_Valid(InHandle))
    {
        OutResult = ECk_SucceededFailed::Failed;
        return {};
    }

    if (NOT Has(InHandle))
    {
        OutResult = ECk_SucceededFailed::Failed;
        return {};
    }

    OutResult = ECk_SucceededFailed::Succeeded;
    return ck::StaticCast<FCk_Handle_Shape>(InHandle);
}

auto
    UCk_Utils_Shapes_UE::
    DoCastChecked(
        FCk_Handle InHandle)
    -> FCk_Handle_Shape
{
    if (ck::Is_NOT_Valid(InHandle))
    { return {}; }

    CK_ENSURE_IF_NOT(Has(InHandle),
        TEXT("Handle [{}] does NOT have a [{}]. Unable to convert Handle."),
        InHandle, ck::Get_RuntimeTypeToString<FCk_Handle_Shape>())
    { return {}; }

    return ck::StaticCast<FCk_Handle_Shape>(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Shapes_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_AnyShape& InParams)
    -> FCk_Handle_Shape
{
    switch(InParams.Get_ShapeType())
    {
        case ECk_Shape_Type::Box:
        {
            UCk_Utils_ShapeBox_UE::Add(InHandle, FCk_Fragment_ShapeBox_ParamsData{InParams.Get_Box()});
            break;
        }
        case ECk_Shape_Type::Capsule:
        {
            UCk_Utils_ShapeCapsule_UE::Add(InHandle, FCk_Fragment_ShapeCapsule_ParamsData{InParams.Get_Capsule()});
            break;
        }
        case ECk_Shape_Type::Cylinder:
        {
            UCk_Utils_ShapeCylinder_UE::Add(InHandle, FCk_Fragment_ShapeCylinder_ParamsData{InParams.Get_Cylinder()});
            break;
        }
        case ECk_Shape_Type::Sphere:
        {
            UCk_Utils_ShapeSphere_UE::Add(InHandle, FCk_Fragment_ShapeSphere_ParamsData{InParams.Get_Sphere()});
            break;
        }
        case ECk_Shape_Type::None:
        {
            CK_TRIGGER_ENSURE(TEXT("Unable to add a Shape to Handle [{}] since the ShapeType is [{}]"), InHandle,
                InParams.Get_ShapeType());
            break;
        }
    }

    return Cast(InHandle);
}

auto
    UCk_Utils_Shapes_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_AnyShape& InParams)
    -> FCk_Handle_Shape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InParams);
}

auto
    UCk_Utils_Shapes_UE::
    Get_ShapeType(
        const FCk_Handle_Shape& InHandle)
    -> ECk_Shape_Type
{
    if (UCk_Utils_ShapeBox_UE::Has(InHandle))
    { return ECk_Shape_Type::Box; }

    if (UCk_Utils_ShapeSphere_UE::Has(InHandle))
    { return ECk_Shape_Type::Sphere; }

    if (UCk_Utils_ShapeCapsule_UE::Has(InHandle))
    { return ECk_Shape_Type::Capsule; }

    if (UCk_Utils_ShapeCylinder_UE::Has(InHandle))
    { return ECk_Shape_Type::Cylinder; }

    return ECk_Shape_Type::None;
}

auto
    UCk_Utils_Shapes_UE::
    Get_Radius(
        const FCk_Handle_Shape& InHandle)
    -> float
{
    switch (const auto& ShapeType = Get_ShapeType(InHandle))
    {
        case ECk_Shape_Type::Box:
        {
            const auto& ShapeHandle = UCk_Utils_ShapeBox_UE::Cast(InHandle);
            const auto& Dimensions = UCk_Utils_ShapeBox_UE::Get_Dimensions(ShapeHandle);
            const auto& HalfExtents = Dimensions.Get_HalfExtents();
            const auto& OuterRadius = FMath::Sqrt(HalfExtents.X * HalfExtents.X + HalfExtents.Y * HalfExtents.Y + HalfExtents.Z * HalfExtents.Z);

            return OuterRadius;
        }
        case ECk_Shape_Type::Capsule:
        {
            const auto& ShapeHandle = UCk_Utils_ShapeCapsule_UE::Cast(InHandle);
            const auto& Dimensions = UCk_Utils_ShapeCapsule_UE::Get_Dimensions(ShapeHandle);
            return Dimensions.Get_Radius();
        }
        case ECk_Shape_Type::Cylinder:
        {
            const auto& ShapeHandle = UCk_Utils_ShapeCylinder_UE::Cast(InHandle);
            const auto& Dimensions = UCk_Utils_ShapeCylinder_UE::Get_Dimensions(ShapeHandle);
            return Dimensions.Get_Radius();
        }
        case ECk_Shape_Type::Sphere:
        {
            const auto& ShapeHandle = UCk_Utils_ShapeSphere_UE::Cast(InHandle);
            const auto& Dimensions = UCk_Utils_ShapeSphere_UE::Get_Dimensions(ShapeHandle);
            return Dimensions.Get_Radius();
        }
        case ECk_Shape_Type::None:
        {
            CK_TRIGGER_ENSURE(TEXT("Entity [{}] does NOT have ANY Shape fragment! Cannot calculate its Shape Radius"), InHandle);
            return {};
        }
        default:
        {
            CK_INVALID_ENUM(ShapeType);
            return {};
        }
    }
}

auto
    UCk_Utils_Shapes_UE::
    Make_Box(
        const FCk_ShapeBox_Dimensions& InDimensions)
    -> FCk_AnyShape
{
    return FCk_AnyShape{InDimensions};
}

auto
    UCk_Utils_Shapes_UE::
    Make_Sphere(
        const FCk_ShapeSphere_Dimensions& InDimensions)
    -> FCk_AnyShape
{
    return FCk_AnyShape{InDimensions};
}

auto
    UCk_Utils_Shapes_UE::
    Make_Capsule(
        const FCk_ShapeCapsule_Dimensions& InDimensions)
    -> FCk_AnyShape
{
    return FCk_AnyShape{InDimensions};
}

auto
    UCk_Utils_Shapes_UE::
    Make_Cylinder(
        const FCk_ShapeCylinder_Dimensions& InDimensions)
    -> FCk_AnyShape
{
    return FCk_AnyShape{InDimensions};
}

// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------

namespace ck_shapes_utils
{
    auto
        DoMake_FromVisualBounds(
            const FBoxSphereBounds& InVisualBounds,
            const FVector& InScale)
        -> FCk_Shape_FromMeshResult
    {
        const auto HalfExtents = (InVisualBounds.BoxExtent * InScale).GetAbs();

        return FCk_Shape_FromMeshResult
        {
            FCk_AnyShape{FCk_ShapeBox_Dimensions{HalfExtents}},
            FTransform{InVisualBounds.Origin * InScale},
            ECk_Shape_FromMeshFidelity::VisualBounds
        };
    }

    auto
        DoGet_IsScaleUniform(
            const FVector& InScale)
        -> bool
    {
        const auto ScaleAbs = InScale.GetAbs();

        return FMath::IsNearlyEqual(ScaleAbs.X, ScaleAbs.Y)
            && FMath::IsNearlyEqual(ScaleAbs.Y, ScaleAbs.Z);
    }

    // Each primitive has its own faithfulness rule because the engine scales each differently:
    // a box shears only when rotated, a sphere collapses to MinScaleAbs, a capsule radius to
    // max(|X|,|Y|).
    auto
        DoGet_BoxFidelity(
            const FRotator& InRotation,
            const FVector& InScale)
        -> ECk_Shape_FromMeshFidelity
    {
        if (DoGet_IsScaleUniform(InScale) || InRotation.IsNearlyZero())
        { return ECk_Shape_FromMeshFidelity::Exact; }

        return ECk_Shape_FromMeshFidelity::Approximated;
    }

    auto
        DoGet_SphereFidelity(
            const FVector& InScale)
        -> ECk_Shape_FromMeshFidelity
    {
        return DoGet_IsScaleUniform(InScale)
            ? ECk_Shape_FromMeshFidelity::Exact
            : ECk_Shape_FromMeshFidelity::Approximated;
    }

    auto
        DoGet_CapsuleFidelity(
            const FRotator& InRotation,
            const FVector& InScale)
        -> ECk_Shape_FromMeshFidelity
    {
        if (DoGet_IsScaleUniform(InScale))
        { return ECk_Shape_FromMeshFidelity::Exact; }

        const auto ScaleAbs = InScale.GetAbs();
        const auto IsRadiallyUniform = FMath::IsNearlyEqual(ScaleAbs.X, ScaleAbs.Y);

        if (IsRadiallyUniform && InRotation.IsNearlyZero())
        { return ECk_Shape_FromMeshFidelity::Exact; }

        return ECk_Shape_FromMeshFidelity::Approximated;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::shapes
{
    auto
        Derive_FromCollision(
            const UBodySetup* InBodySetup,
            const FBoxSphereBounds& InVisualBounds,
            const FVector& InScale)
        -> FCk_Shape_FromMeshResult
    {
        if (ck::Is_NOT_Valid(InBodySetup, ck::IsValid_Policy_NullptrOnly{}))
        { return ck_shapes_utils::DoMake_FromVisualBounds(InVisualBounds, InScale); }

        if (InBodySetup->GetCollisionTraceFlag() == ECollisionTraceFlag::CTF_UseComplexAsSimple)
        { return ck_shapes_utils::DoMake_FromVisualBounds(InVisualBounds, InScale); }

        const auto& AggGeom = InBodySetup->AggGeom;

        const auto NumBox = AggGeom.BoxElems.Num();
        const auto NumSphere = AggGeom.SphereElems.Num();
        const auto NumSphyl = AggGeom.SphylElems.Num();

        const auto NumExpressible = NumBox + NumSphere + NumSphyl;
        const auto NumTotal = AggGeom.GetElementCount();

        if (NumTotal == 0 || NumTotal != NumExpressible)
        { return ck_shapes_utils::DoMake_FromVisualBounds(InVisualBounds, InScale); }

        if (NumExpressible == 1)
        {
            if (NumBox == 1)
            {
                const auto& Elem = AggGeom.BoxElems[0];
                const auto ScaledElem = Elem.GetFinalScaled(InScale, FTransform::Identity);
                const auto HalfExtents = FVector{ScaledElem.X, ScaledElem.Y, ScaledElem.Z} * 0.5;

                return FCk_Shape_FromMeshResult
                {
                    FCk_AnyShape{FCk_ShapeBox_Dimensions{HalfExtents}},
                    FTransform{Elem.Rotation, Elem.Center},
                    ck_shapes_utils::DoGet_BoxFidelity(ScaledElem.Rotation, InScale)
                };
            }

            if (NumSphere == 1)
            {
                const auto& Elem = AggGeom.SphereElems[0];
                const auto ScaledElem = Elem.GetFinalScaled(InScale, FTransform::Identity);

                return FCk_Shape_FromMeshResult
                {
                    FCk_AnyShape{FCk_ShapeSphere_Dimensions{ScaledElem.Radius}},
                    FTransform{Elem.Center},
                    ck_shapes_utils::DoGet_SphereFidelity(InScale)
                };
            }

            const auto& SphylElem = AggGeom.SphylElems[0];

            // HalfHeight is half the CYLINDER SEGMENT. Scaling Length by |Z| agrees only under
            // uniform scale - the engine scales the TOTAL half-length, then subtracts the radius.
            const auto Radius = SphylElem.GetScaledRadius(InScale);
            const auto HalfHeight = SphylElem.GetScaledCylinderLength(InScale) * 0.5f;

            return FCk_Shape_FromMeshResult
            {
                FCk_AnyShape{FCk_ShapeCapsule_Dimensions{HalfHeight, Radius}},
                FTransform{SphylElem.Rotation, SphylElem.Center},
                ck_shapes_utils::DoGet_CapsuleFidelity(SphylElem.Rotation, InScale)
            };
        }

        // NOT FKAggregateGeom::CalcAABB: it collapses a non-uniform scale to one min-absolute
        // scalar (SelectMinScale), under-scaling every axis but the smallest.
        constexpr auto AlreadyScaled = 1.0f;
        auto Bounds = FBox{ForceInit};

        for (const auto& Elem : AggGeom.BoxElems)
        { Bounds += Elem.GetFinalScaled(InScale, FTransform::Identity).CalcAABB(FTransform::Identity, AlreadyScaled); }

        for (const auto& Elem : AggGeom.SphereElems)
        { Bounds += Elem.GetFinalScaled(InScale, FTransform::Identity).CalcAABB(FTransform::Identity, AlreadyScaled); }

        for (const auto& Elem : AggGeom.SphylElems)
        { Bounds += Elem.GetFinalScaled(InScale, FTransform::Identity).CalcAABB(FTransform::Identity, AlreadyScaled); }

        const auto UnscaledCentre = InScale.IsNearlyZero()
            ? FVector::ZeroVector
            : FVector{Bounds.GetCenter() / InScale};

        return FCk_Shape_FromMeshResult
        {
            FCk_AnyShape{FCk_ShapeBox_Dimensions{Bounds.GetExtent()}},
            FTransform{UnscaledCentre},
            ECk_Shape_FromMeshFidelity::PrimitiveUnion
        };
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Shapes_UE::
    Get_ShapeFromMeshCollision(
        const UStaticMesh* InMesh,
        const FVector& InScale)
    -> FCk_Shape_FromMeshResult
{
    const auto MeshIsValid = ck::IsValid(InMesh, ck::IsValid_Policy_NullptrOnly{});
    CK_ENSURE_IF_NOT(MeshIsValid, TEXT("Cannot derive a collision shape from an INVALID StaticMesh"))
    { return {}; }

    // GetBodySetup() and not the deprecated property: the accessor waits on the async mesh build.
    return ck::shapes::Derive_FromCollision(InMesh->GetBodySetup(), InMesh->GetBounds(), InScale);
}

// --------------------------------------------------------------------------------------------------------------------
