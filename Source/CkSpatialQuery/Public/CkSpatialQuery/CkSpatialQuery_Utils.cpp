#include "CkSpatialQuery_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    // ----------------------------------------------------------------------------------------------------------------
    // Conversion Utilities
    // ----------------------------------------------------------------------------------------------------------------

    auto
        Conv(
            const FTransform& InMatrix)
        -> JoltMatrix
    {
        FMatrix UnrealMatrix = InMatrix.ToMatrixWithScale();
        return Conv(UnrealMatrix);
    }

    auto
        Conv(
            const FMatrix& InMatrix)
        -> JoltMatrix
    {
        return JPH::Mat44(
            JPH::Vec4(InMatrix.M[0][0], InMatrix.M[0][1], InMatrix.M[0][2], InMatrix.M[0][3]),
            JPH::Vec4(InMatrix.M[1][0], InMatrix.M[1][1], InMatrix.M[1][2], InMatrix.M[1][3]),
            JPH::Vec4(InMatrix.M[2][0], InMatrix.M[2][1], InMatrix.M[2][2], InMatrix.M[2][3]),
            JPH::Vec4(InMatrix.M[3][0], InMatrix.M[3][1], InMatrix.M[3][2], InMatrix.M[3][3]));
    }

    auto
        Conv(
            const JoltMatrix& InMatrix)
        -> FMatrix
    {
        auto UnrealMatrix = FMatrix{};

        const auto& Column0 = InMatrix.GetColumn4(0);
        const auto& Column1 = InMatrix.GetColumn4(1);
        const auto& Column2 = InMatrix.GetColumn4(2);
        const auto& Column3 = InMatrix.GetColumn4(3);

        UnrealMatrix.M[0][0] = Column0.GetX();
        UnrealMatrix.M[0][1] = Column0.GetY();
        UnrealMatrix.M[0][2] = Column0.GetZ();
        UnrealMatrix.M[0][3] = Column0.GetW();

        UnrealMatrix.M[1][0] = Column1.GetX();
        UnrealMatrix.M[1][1] = Column1.GetY();
        UnrealMatrix.M[1][2] = Column1.GetZ();
        UnrealMatrix.M[1][3] = Column1.GetW();

        UnrealMatrix.M[2][0] = Column2.GetX();
        UnrealMatrix.M[2][1] = Column2.GetY();
        UnrealMatrix.M[2][2] = Column2.GetZ();
        UnrealMatrix.M[2][3] = Column2.GetW();

        UnrealMatrix.M[3][0] = Column3.GetX();
        UnrealMatrix.M[3][1] = Column3.GetY();
        UnrealMatrix.M[3][2] = Column3.GetZ();
        UnrealMatrix.M[3][3] = Column3.GetW();

        return UnrealMatrix;
    }

    auto
        Conv(
            const FVector& InVector)
        -> JoltVec3
    {
        return JoltVec3{
            static_cast<float>(InVector.X),
            static_cast<float>(InVector.Y),
            static_cast<float>(InVector.Z)
        };
    }

    auto
        Conv(
            Chaos::TVector<float, 3> InVector)
        -> JoltVec3
    {
        return JoltVec3{InVector.X, InVector.Y, InVector.Z};
    }

    auto
        Conv(
            JoltVec3 InVector)
        -> FVector
    {
        return FVector{InVector.GetX(), InVector.GetY(), InVector.GetZ()};
    }

    auto
        Conv(
            JoltFloat3 InVector)
        -> FVector
    {
        return FVector{InVector.x, InVector.y, InVector.z};
    }

    auto
        Conv(
            const FRotator& InRotator)
        -> JoltQuat
    {
        return Conv(InRotator.Quaternion());
    }

    auto
        Conv(
            const FQuat& InQuat)
        -> JoltQuat
    {
        return JoltQuat{
            static_cast<float>(InQuat.X),
            static_cast<float>(InQuat.Y),
            static_cast<float>(InQuat.Z),
            static_cast<float>(InQuat.W)
        };
    }

    auto
        Conv(
            JoltQuat InQuad)
        -> FQuat
    {
        return FQuat{InQuad.GetX(), InQuad.GetY(), InQuad.GetZ(), InQuad.GetW()};
    }

    auto
        Conv(
            JoltColor InColor)
        -> FLinearColor
    {
        const auto ZeroToOne = InColor.ToVec4();
        return FLinearColor{ZeroToOne.GetX(), ZeroToOne.GetY(), ZeroToOne.GetZ(), ZeroToOne.GetW()};
    }

    auto
        Conv(
            ECk_MotionType InMotionType)
        -> JPH::EMotionType
    {
        switch (InMotionType)
        {
            case ECk_MotionType::Static:    return JPH::EMotionType::Static;
            case ECk_MotionType::Kinematic: return JPH::EMotionType::Kinematic;
            case ECk_MotionType::Dynamic:   return JPH::EMotionType::Dynamic;
            default:
                CK_INVALID_ENUM(InMotionType);
                return JPH::EMotionType::Static;
        }
    }

    auto
        Conv(
            ECk_MotionQuality InMotionQuality)
        -> JPH::EMotionQuality
    {
        switch (InMotionQuality)
        {
            case ECk_MotionQuality::Discrete:   return JPH::EMotionQuality::Discrete;
            case ECk_MotionQuality::LinearCast: return JPH::EMotionQuality::LinearCast;
            default:
                CK_INVALID_ENUM(InMotionQuality);
                return JPH::EMotionQuality::Discrete;
        }
    }

    auto
        Conv(
            ECk_BackFaceMode InBackFaceMode)
        -> JPH::EBackFaceMode
    {
        switch (InBackFaceMode)
        {
            case ECk_BackFaceMode::IgnoreBackFaces:      return JPH::EBackFaceMode::IgnoreBackFaces;
            case ECk_BackFaceMode::CollideWithBackFaces: return JPH::EBackFaceMode::CollideWithBackFaces;
            default:
                CK_INVALID_ENUM(InBackFaceMode);
                return JPH::EBackFaceMode::IgnoreBackFaces;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Internal Helpers
    // ----------------------------------------------------------------------------------------------------------------

    namespace
    {
        auto ConfigureProbeBodySettings(
            JPH::BodyCreationSettings& OutSettings,
            const FProbeBodyCreationParams& InParams) -> void
        {
            OutSettings.mIsSensor = true;
            OutSettings.mCollideKinematicVsNonDynamic = true;
            OutSettings.mGravityFactor = 0.0f;
            OutSettings.mMotionType = Conv(InParams.MotionType);
            OutSettings.mMotionQuality = Conv(InParams.MotionQuality);
        }

        auto CreateAndConfigureBody(
            JPH::BodyInterface& InBodyInterface,
            JPH::BodyCreationSettings& InSettings) -> JPH::BodyID
        {
            const auto Body = InBodyInterface.CreateBody(InSettings);

            if (ck::Is_NOT_Valid(Body, ck::IsValid_Policy_NullptrOnly{}))
            {
                CK_TRIGGER_ENSURE(TEXT("Failed to create Jolt body. Max body count may have been reached"));
                return JPH::BodyID{};
            }

            Body->SetCollideKinematicVsNonDynamic(true);
            return Body->GetID();
        }

        auto CreateBodySettingsWithShape(
            JPH::Ref<JPH::Shape> InShape,
            const FProbeBodyCreationParams& InParams) -> JPH::BodyCreationSettings
        {
            auto Settings = JPH::BodyCreationSettings{
                InShape,
                Conv(InParams.InitialTransform.GetLocation()),
                Conv(InParams.InitialTransform.GetRotation()),
                JPH::EMotionType::Kinematic,
                JPH::ObjectLayer{1}
            };

            ConfigureProbeBodySettings(Settings, InParams);

            return Settings;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body Creation - Template Specializations
    // ----------------------------------------------------------------------------------------------------------------

    template<>
    auto
        CreateProbeBody<FCk_ShapeBox_Dimensions>(
            JPH::BodyInterface& InBodyInterface,
            const FCk_ShapeBox_Dimensions& InDimensions,
            const FProbeBodyCreationParams& InParams)
        -> FProbeBodyCreationResult
    {
        const auto ShapeSettings = JPH::BoxShapeSettings{
            Conv(InDimensions.Get_HalfExtents()),
            InDimensions.Get_ConvexRadius()
        };
        ShapeSettings.SetEmbedded();

        auto ShapeResult = ShapeSettings.Create();

        if (NOT ShapeResult.IsValid())
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create Box shape for probe.\n"
                "HalfExtents [{}], ConvexRadius [{}].\n"
                "Jolt Error: [{}]"),
                InDimensions.Get_HalfExtents(), InDimensions.Get_ConvexRadius(),
                FString{ShapeResult.GetError().c_str()});
            return {};
        }

        auto Shape = ShapeResult.Get();
        auto BodySettings = CreateBodySettingsWithShape(Shape, InParams);
        const auto BodyId = CreateAndConfigureBody(InBodyInterface, BodySettings);

        if (BodyId.IsInvalid())
        { return {}; }

        return FProbeBodyCreationResult{BodyId, Shape};
    }

    template<>
    auto
        CreateProbeBody<FCk_ShapeSphere_Dimensions>(
            JPH::BodyInterface& InBodyInterface,
            const FCk_ShapeSphere_Dimensions& InDimensions,
            const FProbeBodyCreationParams& InParams)
        -> FProbeBodyCreationResult
    {
        const auto ShapeSettings = JPH::SphereShapeSettings{InDimensions.Get_Radius()};
        ShapeSettings.SetEmbedded();

        auto ShapeResult = ShapeSettings.Create();

        if (NOT ShapeResult.IsValid())
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create Sphere shape for probe.\n"
                "Radius [{}].\n"
                "Jolt Error: [{}]"),
                InDimensions.Get_Radius(),
                FString{ShapeResult.GetError().c_str()});
            return {};
        }

        auto Shape = ShapeResult.Get();
        auto BodySettings = CreateBodySettingsWithShape(Shape, InParams);
        const auto BodyId = CreateAndConfigureBody(InBodyInterface, BodySettings);

        if (BodyId.IsInvalid())
        { return {}; }

        return FProbeBodyCreationResult{BodyId, Shape};
    }

    template<>
    auto
        CreateProbeBody<FCk_ShapeCapsule_Dimensions>(
            JPH::BodyInterface& InBodyInterface,
            const FCk_ShapeCapsule_Dimensions& InDimensions,
            const FProbeBodyCreationParams& InParams)
        -> FProbeBodyCreationResult
    {
        const auto ShapeSettings = JPH::CapsuleShapeSettings{
            InDimensions.Get_HalfHeight(),
            InDimensions.Get_Radius()
        };
        ShapeSettings.SetEmbedded();

        auto ShapeResult = ShapeSettings.Create();

        if (NOT ShapeResult.IsValid())
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create Capsule shape for probe.\n"
                "HalfHeight [{}], Radius [{}].\n"
                "Jolt Error: [{}]"),
                InDimensions.Get_HalfHeight(), InDimensions.Get_Radius(),
                FString{ShapeResult.GetError().c_str()});
            return {};
        }

        auto Shape = ShapeResult.Get();
        auto BodySettings = CreateBodySettingsWithShape(Shape, InParams);
        const auto BodyId = CreateAndConfigureBody(InBodyInterface, BodySettings);

        if (BodyId.IsInvalid())
        { return {}; }

        return FProbeBodyCreationResult{BodyId, Shape};
    }

    template<>
    auto
        CreateProbeBody<FCk_ShapeCylinder_Dimensions>(
            JPH::BodyInterface& InBodyInterface,
            const FCk_ShapeCylinder_Dimensions& InDimensions,
            const FProbeBodyCreationParams& InParams)
        -> FProbeBodyCreationResult
    {
        const auto ShapeSettings = JPH::CylinderShapeSettings{
            InDimensions.Get_HalfHeight(),
            InDimensions.Get_Radius(),
            InDimensions.Get_ConvexRadius()
        };
        ShapeSettings.SetEmbedded();

        auto ShapeResult = ShapeSettings.Create();

        if (NOT ShapeResult.IsValid())
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create Cylinder shape for probe.\n"
                "HalfHeight [{}], Radius [{}], ConvexRadius [{}].\n"
                "Jolt Error: [{}]"),
                InDimensions.Get_HalfHeight(), InDimensions.Get_Radius(), InDimensions.Get_ConvexRadius(),
                FString{ShapeResult.GetError().c_str()});
            return {};
        }

        auto Shape = ShapeResult.Get();
        auto BodySettings = CreateBodySettingsWithShape(Shape, InParams);
        const auto BodyId = CreateAndConfigureBody(InBodyInterface, BodySettings);

        if (BodyId.IsInvalid())
        { return {}; }

        return FProbeBodyCreationResult{BodyId, Shape};
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body Lifecycle
    // ----------------------------------------------------------------------------------------------------------------

    auto
        ActivateProbeBody(
            JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId)
        -> void
    {
        if (InBodyId.IsInvalid())
        { return; }

        InBodyInterface.AddBody(InBodyId, JPH::EActivation::Activate);
    }

    auto
        DeactivateProbeBody(
            JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId)
        -> void
    {
        if (InBodyId.IsInvalid())
        { return; }

        if (InBodyInterface.IsAdded(InBodyId))
        {
            InBodyInterface.RemoveBody(InBodyId);
        }
    }

    auto
        DestroyProbeBody(
            JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId)
        -> void
    {
        if (InBodyId.IsInvalid())
        { return; }

        if (InBodyInterface.IsAdded(InBodyId))
        {
            InBodyInterface.RemoveBody(InBodyId);
        }

        InBodyInterface.DestroyBody(InBodyId);
    }

    auto
        Get_IsProbeBodyActive(
            const JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId)
        -> bool
    {
        if (InBodyId.IsInvalid())
        { return false; }

        return InBodyInterface.IsAdded(InBodyId);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body Transform
    // ----------------------------------------------------------------------------------------------------------------

    auto
        UpdateProbeBodyTransform(
            JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId,
            const FTransform& InTransform)
        -> void
    {
        if (InBodyId.IsInvalid())
        { return; }

        InBodyInterface.SetPositionAndRotation(
            InBodyId,
            Conv(InTransform.GetLocation()),
            Conv(InTransform.GetRotation()),
            JPH::EActivation::Activate);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body Shape Updates - Template Specializations
    // ----------------------------------------------------------------------------------------------------------------

    template<>
    auto
        UpdateProbeBodyShape<FCk_ShapeBox_Dimensions>(
            JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId,
            const FCk_ShapeBox_Dimensions& InDimensions)
        -> JPH::Ref<JPH::Shape>
    {
        const auto ShapeSettings = JPH::BoxShapeSettings{
            Conv(InDimensions.Get_HalfExtents()),
            InDimensions.Get_ConvexRadius()
        };
        ShapeSettings.SetEmbedded();

        auto ShapeResult = ShapeSettings.Create();

        if (NOT ShapeResult.IsValid())
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create Box shape for probe shape update.\n"
                "HalfExtents [{}], ConvexRadius [{}].\n"
                "Jolt Error: [{}]"),
                InDimensions.Get_HalfExtents(), InDimensions.Get_ConvexRadius(),
                FString{ShapeResult.GetError().c_str()});
            return {};
        }

        auto NewShape = ShapeResult.Get();

        constexpr auto UpdateMassProperties = false;
        InBodyInterface.SetShape(InBodyId, NewShape, UpdateMassProperties, JPH::EActivation::Activate);

        return NewShape;
    }

    template<>
    auto
        UpdateProbeBodyShape<FCk_ShapeSphere_Dimensions>(
            JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId,
            const FCk_ShapeSphere_Dimensions& InDimensions)
        -> JPH::Ref<JPH::Shape>
    {
        const auto ShapeSettings = JPH::SphereShapeSettings{InDimensions.Get_Radius()};
        ShapeSettings.SetEmbedded();

        auto ShapeResult = ShapeSettings.Create();

        if (NOT ShapeResult.IsValid())
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create Sphere shape for probe shape update.\n"
                "Radius [{}].\n"
                "Jolt Error: [{}]"),
                InDimensions.Get_Radius(),
                FString{ShapeResult.GetError().c_str()});
            return {};
        }

        auto NewShape = ShapeResult.Get();

        constexpr auto UpdateMassProperties = false;
        InBodyInterface.SetShape(InBodyId, NewShape, UpdateMassProperties, JPH::EActivation::Activate);

        return NewShape;
    }

    template<>
    auto
        UpdateProbeBodyShape<FCk_ShapeCapsule_Dimensions>(
            JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId,
            const FCk_ShapeCapsule_Dimensions& InDimensions)
        -> JPH::Ref<JPH::Shape>
    {
        const auto ShapeSettings = JPH::CapsuleShapeSettings{
            InDimensions.Get_HalfHeight(),
            InDimensions.Get_Radius()
        };
        ShapeSettings.SetEmbedded();

        auto ShapeResult = ShapeSettings.Create();

        if (NOT ShapeResult.IsValid())
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create Capsule shape for probe shape update.\n"
                "HalfHeight [{}], Radius [{}].\n"
                "Jolt Error: [{}]"),
                InDimensions.Get_HalfHeight(), InDimensions.Get_Radius(),
                FString{ShapeResult.GetError().c_str()});
            return {};
        }

        auto NewShape = ShapeResult.Get();

        constexpr auto UpdateMassProperties = false;
        InBodyInterface.SetShape(InBodyId, NewShape, UpdateMassProperties, JPH::EActivation::Activate);

        return NewShape;
    }

    template<>
    auto
        UpdateProbeBodyShape<FCk_ShapeCylinder_Dimensions>(
            JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId,
            const FCk_ShapeCylinder_Dimensions& InDimensions)
        -> JPH::Ref<JPH::Shape>
    {
        const auto ShapeSettings = JPH::CylinderShapeSettings{
            InDimensions.Get_HalfHeight(),
            InDimensions.Get_Radius(),
            InDimensions.Get_ConvexRadius()
        };
        ShapeSettings.SetEmbedded();

        auto ShapeResult = ShapeSettings.Create();

        if (NOT ShapeResult.IsValid())
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create Cylinder shape for probe shape update.\n"
                "HalfHeight [{}], Radius [{}], ConvexRadius [{}].\n"
                "Jolt Error: [{}]"),
                InDimensions.Get_HalfHeight(), InDimensions.Get_Radius(), InDimensions.Get_ConvexRadius(),
                FString{ShapeResult.GetError().c_str()});
            return {};
        }

        auto NewShape = ShapeResult.Get();

        constexpr auto UpdateMassProperties = false;
        InBodyInterface.SetShape(InBodyId, NewShape, UpdateMassProperties, JPH::EActivation::Activate);

        return NewShape;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body User Data
    // ----------------------------------------------------------------------------------------------------------------

    auto
        SetProbeBodyUserData(
            JPH::Body& InBody,
            uint64 InUserData)
        -> void
    {
        InBody.SetUserData(InUserData);
    }

    auto
        Get_ProbeBodyUserData(
            const JPH::Body& InBody)
        -> uint64
    {
        return InBody.GetUserData();
    }

    auto
        Get_ProbeBodyUserData(
            const JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId)
        -> uint64
    {
        return InBodyInterface.GetUserData(InBodyId);
    }
}

// --------------------------------------------------------------------------------------------------------------------