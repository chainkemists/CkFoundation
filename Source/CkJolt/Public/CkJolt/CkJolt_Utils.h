#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include <Chaos/Vector.h>

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

    CKJOLT_API auto Conv(const FTransform& InMatrix) -> JoltMatrix;
    CKJOLT_API auto Conv(const FMatrix& InMatrix) -> JoltMatrix;
    CKJOLT_API auto Conv(const JoltMatrix& InMatrix) -> FMatrix;
    CKJOLT_API auto Conv(const FVector& InVector) -> JoltVec3;
    CKJOLT_API auto Conv(Chaos::TVector<float, 3> InVector) -> JoltVec3;
    CKJOLT_API auto Conv(JoltVec3 InVector) -> FVector;
    CKJOLT_API auto Conv(JoltFloat3 InVector) -> FVector;
    CKJOLT_API auto Conv(const FRotator& InRotator) -> JoltQuat;
    CKJOLT_API auto Conv(const FQuat& InQuat) -> JoltQuat;
    CKJOLT_API auto Conv(JoltQuat InQuad) -> FQuat;
    CKJOLT_API auto Conv(JoltColor InColor) -> FLinearColor;

    // ----------------------------------------------------------------------------------------------------------------
    // Shape Axis Correction
    // ----------------------------------------------------------------------------------------------------------------

    /// Jolt's Cylinder and Capsule shapes are Y-AXIS ALIGNED by convention — their caps sit at
    /// (0, -HalfHeight, 0) and (0, +HalfHeight, 0) (see Jolt's CylinderShape.h / CapsuleShape.h).
    /// CkJolt runs Jolt directly in Unreal's Z-up frame, however: Conv(FVector) is a straight
    /// X->X, Y->Y, Z->Z passthrough with NO axis swap. Uncorrected, every Jolt cylinder/capsule we build
    /// lies on its side with its axis along world Y — an anisotropic query volume, not just a bad visual.
    /// This quat is a +90 degree rotation about X, mapping the shape's local +Y to +Z: the top cap moves
    /// from (0, +HalfHeight, 0) to (0, 0, +HalfHeight), so the shape stands UP instead of upside-down.
    /// Wrap the shape in a JPH::RotatedTranslatedShapeSettings with this rotation (and zero translation).
    CKJOLT_API auto Get_ShapeAxisCorrection_YToZ() -> JoltQuat;

    // ----------------------------------------------------------------------------------------------------------------
    // Body User Data
    // ----------------------------------------------------------------------------------------------------------------

    /// Body UserData carries the raw (versioned) entity id baked in at body registration.
    CKJOLT_API auto Get_BodyUserData(
        const JPH::Body& InBody) -> uint64;

    CKJOLT_API auto Get_BodyUserData(
        const JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId) -> uint64;

    /// Resolves the entity a Jolt query hit refers to (via the body's UserData).
    /// Returns an INVALID handle when the hit is InSelf itself, or when the body's entity is no
    /// longer alive (a body can briefly outlive its owning entity — deferred end-of-frame
    /// destroy, or a snapshot restore that wipes the registry before physics tears the body
    /// down). Callers treat both as "skip this hit".
    CKJOLT_API auto TryGet_EntityFromBody(
        const FCk_Handle& InSelf,
        const JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InHitBodyId) -> FCk_Handle;
}

// --------------------------------------------------------------------------------------------------------------------