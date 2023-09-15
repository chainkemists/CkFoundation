#include "CkPhysics_Common.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_BoxExtents::
    FCk_BoxExtents(
        FVector InExtents,
        ECk_ScaledUnscaled InScaledUnscaled)
    : _Extents(InExtents)
    , _ScaledUnscaled(InScaledUnscaled)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SphereRadius::
    FCk_SphereRadius(
        float InRadius,
        ECk_ScaledUnscaled InScaledUnscaled)
    : _Radius(InRadius)
    , _ScaledUnscaled(InScaledUnscaled)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_CapsuleSize::
    FCk_CapsuleSize(
        float InRadius,
        float InHalfHeight,
        ECk_ScaledUnscaled InScaledUnscaled)
    : _Radius(InRadius)
    , _HalfHeight(InHalfHeight)
    , _ScaledUnscaled(InScaledUnscaled)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_ShapeDimensions::
    FCk_ShapeDimensions(
        FCk_BoxExtents InBoxExtents)
    : _ShapeType(ECk_ShapeType::Box)
    , _BoxExtents(InBoxExtents)

{
}

FCk_ShapeDimensions::
    FCk_ShapeDimensions(
        FCk_SphereRadius InSphereRadius)
    : _ShapeType(ECk_ShapeType::Sphere)
    , _SphereRadius(InSphereRadius)
{
}

FCk_ShapeDimensions::
    FCk_ShapeDimensions(
        FCk_CapsuleSize InCapsuleSize)
    : _ShapeType(ECk_ShapeType::Capsule)
    , _CapsuleSize(InCapsuleSize)
{
}

// --------------------------------------------------------------------------------------------------------------------
