#include "CkPhysics_Common.h"

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
