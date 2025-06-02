#include "CkShapes_Utils.h"

#include "CkShapes/Box/CkShapeBox_Utils.h"
#include "CkShapes/Capsule/CkShapeCapsule_Utils.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Utils.h"
#include "CkShapes/Sphere/CkShapeSphere_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Shapes_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_AnyShape& InParams)
    -> FCk_Handle
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

    return InHandle;
}

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

auto
    UCk_Utils_Shapes_UE::
    Get_Radius(
        const FCk_Handle& InHandle)
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

// --------------------------------------------------------------------------------------------------------------------
