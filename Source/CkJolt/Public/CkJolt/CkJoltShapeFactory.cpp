#include "CkJoltShapeFactory.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Utils.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_shape_factory
{
    static auto Create_FromSettings(
        const JPH::ShapeSettings& InSettings,
        const FString& InDebugName)
        -> JPH::Ref<JPH::Shape>
    {
        const auto Result = InSettings.Create();

        CK_ENSURE_IF_NOT(Result.IsValid(), TEXT("Jolt shape creation FAILED for [{}]: [{}]"),
            InDebugName, FString{Result.GetError().c_str()})
        { return {}; }

        return Result.Get();
    }

    static auto Create_UprightWrap(
        const JPH::Ref<JPH::Shape>& InShape,
        const FString& InDebugName)
        -> JPH::Ref<JPH::Shape>
    {
        if (InShape == nullptr)
        { return {}; }

        const auto Settings = JPH::RotatedTranslatedShapeSettings{
            JPH::Vec3::sZero(), ck::jolt::Get_ShapeAxisCorrection_YToZ(), InShape.GetPtr()};
        return Create_FromSettings(Settings, InDebugName);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    auto
        CreateShape_FromDimensions(
            const FCk_Jolt_ShapeDimensions& InDimensions,
            const FString& InDebugName)
        -> JPH::Ref<JPH::Shape>
    {
        using namespace ck_jolt_shape_factory;

        switch (InDimensions.Get_ShapeType())
        {
            case ECk_Jolt_ShapeType::Box:
            {
                const auto Settings = JPH::BoxShapeSettings{Conv(InDimensions.Get_HalfExtents())};
                return Create_FromSettings(Settings, InDebugName);
            }
            case ECk_Jolt_ShapeType::Sphere:
            {
                const auto Settings = JPH::SphereShapeSettings{InDimensions.Get_Radius()};
                return Create_FromSettings(Settings, InDebugName);
            }
            case ECk_Jolt_ShapeType::Capsule:
            {
                const auto Settings = JPH::CapsuleShapeSettings{
                    InDimensions.Get_HalfHeight(), InDimensions.Get_Radius()};
                return Create_UprightWrap(Create_FromSettings(Settings, InDebugName), InDebugName);
            }
            case ECk_Jolt_ShapeType::Cylinder:
            {
                const auto Settings = JPH::CylinderShapeSettings{
                    InDimensions.Get_HalfHeight(), InDimensions.Get_Radius()};
                return Create_UprightWrap(Create_FromSettings(Settings, InDebugName), InDebugName);
            }
            default:
            {
                CK_INVALID_ENUM(InDimensions.Get_ShapeType());
                return {};
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
