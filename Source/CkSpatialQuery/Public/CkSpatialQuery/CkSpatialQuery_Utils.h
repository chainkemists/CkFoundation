#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkShapes/Box/CkShapeBox_Fragment_Data.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment_Data.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment_Data.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment_Data.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Body/MotionQuality.h>
#include <Jolt/Physics/Collision/BackFaceMode.h>

// --------------------------------------------------------------------------------------------------------------------

namespace JPH
{
    class Shape;
    class PhysicsSystem;
    class BodyInterface;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    // Type aliases for common Jolt types
    using JoltMatrix = JPH::Mat44;
    using JoltFloat3 = JPH::Float3;
    using JoltVec3 = JPH::Vec3;
    using JoltQuat = JPH::Quat;
    using JoltColor = JPH::Color;

    // ----------------------------------------------------------------------------------------------------------------
    // Conversion Utilities
    // ----------------------------------------------------------------------------------------------------------------

    auto Conv(const FTransform& InMatrix) -> JoltMatrix;
    auto Conv(const FMatrix& InMatrix) -> JoltMatrix;
    auto Conv(const JoltMatrix& InMatrix) -> FMatrix;
    auto Conv(const FVector& InVector) -> JoltVec3;
    auto Conv(Chaos::TVector<float, 3> InVector) -> JoltVec3;
    auto Conv(JoltVec3 InVector) -> FVector;
    auto Conv(JoltFloat3 InVector) -> FVector;
    auto Conv(const FRotator& InRotator) -> JoltQuat;
    auto Conv(const FQuat& InQuat) -> JoltQuat;
    auto Conv(JoltQuat InQuad) -> FQuat;
    auto Conv(JoltColor InColor) -> FLinearColor;
    auto Conv(ECk_MotionType InMotionType) -> JPH::EMotionType;
    auto Conv(ECk_MotionQuality InMotionQuality) -> JPH::EMotionQuality;
    auto Conv(ECk_BackFaceMode InBackFaceMode) -> JPH::EBackFaceMode;

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body Creation Parameters & Result
    // ----------------------------------------------------------------------------------------------------------------

    struct FProbeBodyCreationParams
    {
        CK_GENERATED_BODY(FProbeBodyCreationParams);

        ECk_MotionType MotionType = ECk_MotionType::Static;
        ECk_MotionQuality MotionQuality = ECk_MotionQuality::Discrete;
        FTransform InitialTransform;

        CK_DEFINE_CONSTRUCTORS(FProbeBodyCreationParams, MotionType, MotionQuality, InitialTransform);
    };

    struct FProbeBodyCreationResult
    {
        CK_GENERATED_BODY(FProbeBodyCreationResult);

        JPH::BodyID BodyId;
        JPH::Ref<JPH::Shape> Shape;

        CK_PROPERTY_GET(BodyId);
        CK_PROPERTY_GET(Shape);

        CK_DEFINE_CONSTRUCTORS(FProbeBodyCreationResult, BodyId, Shape);
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body Creation (Templated)
    // ----------------------------------------------------------------------------------------------------------------

    template<typename TDimensions>
    auto CreateProbeBody(
        JPH::BodyInterface& InBodyInterface,
        const TDimensions& InDimensions,
        const FProbeBodyCreationParams& InParams) -> FProbeBodyCreationResult;

    // Explicit specialization declarations
    template<> CKSPATIALQUERY_API auto CreateProbeBody<FCk_ShapeBox_Dimensions>(
        JPH::BodyInterface& InBodyInterface,
        const FCk_ShapeBox_Dimensions& InDimensions,
        const FProbeBodyCreationParams& InParams) -> FProbeBodyCreationResult;

    template<> CKSPATIALQUERY_API auto CreateProbeBody<FCk_ShapeSphere_Dimensions>(
        JPH::BodyInterface& InBodyInterface,
        const FCk_ShapeSphere_Dimensions& InDimensions,
        const FProbeBodyCreationParams& InParams) -> FProbeBodyCreationResult;

    template<> CKSPATIALQUERY_API auto CreateProbeBody<FCk_ShapeCapsule_Dimensions>(
        JPH::BodyInterface& InBodyInterface,
        const FCk_ShapeCapsule_Dimensions& InDimensions,
        const FProbeBodyCreationParams& InParams) -> FProbeBodyCreationResult;

    template<> CKSPATIALQUERY_API auto CreateProbeBody<FCk_ShapeCylinder_Dimensions>(
        JPH::BodyInterface& InBodyInterface,
        const FCk_ShapeCylinder_Dimensions& InDimensions,
        const FProbeBodyCreationParams& InParams) -> FProbeBodyCreationResult;

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body Lifecycle
    // ----------------------------------------------------------------------------------------------------------------

    CKSPATIALQUERY_API auto ActivateProbeBody(
        JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId) -> void;

    CKSPATIALQUERY_API auto DeactivateProbeBody(
        JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId) -> void;

    CKSPATIALQUERY_API auto DestroyProbeBody(
        JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId) -> void;

    CKSPATIALQUERY_API auto Get_IsProbeBodyActive(
        const JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId) -> bool;

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body Transform
    // ----------------------------------------------------------------------------------------------------------------

    CKSPATIALQUERY_API auto UpdateProbeBodyTransform(
        JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId,
        const FTransform& InTransform) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body Shape Updates (Templated)
    // ----------------------------------------------------------------------------------------------------------------

    template<typename TDimensions>
    auto UpdateProbeBodyShape(
        JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId,
        const TDimensions& InDimensions) -> JPH::Ref<JPH::Shape>;

    // Explicit specialization declarations
    template<> CKSPATIALQUERY_API auto UpdateProbeBodyShape<FCk_ShapeBox_Dimensions>(
        JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId,
        const FCk_ShapeBox_Dimensions& InDimensions) -> JPH::Ref<JPH::Shape>;

    template<> CKSPATIALQUERY_API auto UpdateProbeBodyShape<FCk_ShapeSphere_Dimensions>(
        JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId,
        const FCk_ShapeSphere_Dimensions& InDimensions) -> JPH::Ref<JPH::Shape>;

    template<> CKSPATIALQUERY_API auto UpdateProbeBodyShape<FCk_ShapeCapsule_Dimensions>(
        JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId,
        const FCk_ShapeCapsule_Dimensions& InDimensions) -> JPH::Ref<JPH::Shape>;

    template<> CKSPATIALQUERY_API auto UpdateProbeBodyShape<FCk_ShapeCylinder_Dimensions>(
        JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId,
        const FCk_ShapeCylinder_Dimensions& InDimensions) -> JPH::Ref<JPH::Shape>;

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body User Data
    // ----------------------------------------------------------------------------------------------------------------

    CKSPATIALQUERY_API auto SetProbeBodyUserData(
        JPH::Body& InBody,
        uint64 InUserData) -> void;

    CKSPATIALQUERY_API auto Get_ProbeBodyUserData(
        const JPH::Body& InBody) -> uint64;

    CKSPATIALQUERY_API auto Get_ProbeBodyUserData(
        const JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId) -> uint64;
}

// --------------------------------------------------------------------------------------------------------------------