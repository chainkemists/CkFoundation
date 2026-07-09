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
    // Probe Body User Data
    // ----------------------------------------------------------------------------------------------------------------

    CKSPATIALQUERY_API auto Get_ProbeBodyUserData(
        const JPH::Body& InBody) -> uint64;

    CKSPATIALQUERY_API auto Get_ProbeBodyUserData(
        const JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId) -> uint64;
}

// --------------------------------------------------------------------------------------------------------------------