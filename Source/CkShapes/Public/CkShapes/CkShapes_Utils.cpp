#include "CkShapes_Utils.h"

#include "CkShapes/Box/CkShapeBox_Utils.h"
#include "CkShapes/Capsule/CkShapeCapsule_Utils.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Utils.h"
#include "CkShapes/Sphere/CkShapeSphere_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Shapes_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return Get_ShapeType(InHandle) != ECk_Shape_Type::None;
}

auto
    UCk_Utils_Shapes_UE::
    Get_ShapeType(
        const FCk_Handle& InHandle)
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

// --------------------------------------------------------------------------------------------------------------------
