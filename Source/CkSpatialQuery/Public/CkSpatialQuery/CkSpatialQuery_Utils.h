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
    // Shape Axis Correction
    // ----------------------------------------------------------------------------------------------------------------

    /// Jolt's Cylinder and Capsule shapes are Y-AXIS ALIGNED by convention — their caps sit at
    /// (0, -HalfHeight, 0) and (0, +HalfHeight, 0) (see Jolt's CylinderShape.h / CapsuleShape.h).
    /// CkSpatialQuery runs Jolt directly in Unreal's Z-up frame, however: Conv(FVector) is a straight
    /// X->X, Y->Y, Z->Z passthrough with NO axis swap. Uncorrected, every Jolt cylinder/capsule we build
    /// lies on its side with its axis along world Y — an anisotropic query volume, not just a bad visual.
    /// This quat is a +90 degree rotation about X, mapping the shape's local +Y to +Z: the top cap moves
    /// from (0, +HalfHeight, 0) to (0, 0, +HalfHeight), so the shape stands UP instead of upside-down.
    /// Wrap the shape in a JPH::RotatedTranslatedShapeSettings with this rotation (and zero translation).
    auto Get_ShapeAxisCorrection_YToZ() -> JoltQuat;

    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body User Data
    // ----------------------------------------------------------------------------------------------------------------

    CKSPATIALQUERY_API auto Get_ProbeBodyUserData(
        const JPH::Body& InBody) -> uint64;

    CKSPATIALQUERY_API auto Get_ProbeBodyUserData(
        const JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId) -> uint64;

    /// Resolves the Probe entity a Jolt query hit refers to (via the body's UserData).
    /// Returns an INVALID handle when the hit is InSelf itself, when the body's entity is no
    /// longer alive (a body can briefly outlive its owning entity — deferred end-of-frame
    /// destroy, or a snapshot restore that wipes the registry before physics tears the body
    /// down), or when the entity is not a Probe. Callers treat all three as "skip this hit".
    CKSPATIALQUERY_API auto TryGet_ProbeFromBodyHit(
        const FCk_Handle& InSelf,
        const JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InHitBodyId) -> FCk_Handle_Probe;
}

// --------------------------------------------------------------------------------------------------------------------