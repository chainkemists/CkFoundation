#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkJolt/CkJolt_Common.h"

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
    CKJOLT_API auto Conv(ECk_MotionType InMotionType) -> JPH::EMotionType;
    CKJOLT_API auto Conv(ECk_MotionQuality InMotionQuality) -> JPH::EMotionQuality;
    CKJOLT_API auto Conv(ECk_BackFaceMode InBackFaceMode) -> JPH::EBackFaceMode;

    // ----------------------------------------------------------------------------------------------------------------
    // Shape Axis Correction
    // ----------------------------------------------------------------------------------------------------------------

    /// Jolt's Cylinder/Capsule shapes are Y-AXIS ALIGNED, and Conv() is a Z-up passthrough with no axis swap,
    /// so uncorrected they lie on their side — an anisotropic query volume, not just a bad visual. This +90
    /// degree rotation about X maps local +Y to +Z; wrap the shape in a RotatedTranslatedShapeSettings with it.
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

    // ----------------------------------------------------------------------------------------------------------------
    // Global Jolt init (ref-counted)
    // ----------------------------------------------------------------------------------------------------------------

    /// Ref-counted because multiple worlds (PIE clients) and standalone unit tests may init/deinit
    /// independently. Tests that create JPH shapes WITHOUT a world must call these too.
    CKJOLT_API auto Request_GlobalJoltInit() -> void;
    CKJOLT_API auto Request_GlobalJoltShutdown() -> void;

    /// Returns an INVALID handle when the hit is InSelf itself, or when the body's entity is no longer alive
    /// (a body can briefly outlive its entity — deferred end-of-frame destroy, or a snapshot restore that
    /// wipes the registry first). Callers treat both as "skip this hit".
    CKJOLT_API auto TryGet_EntityFromBody(
        const FCk_Handle& InSelf,
        const JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InHitBodyId) -> FCk_Handle;
}

// --------------------------------------------------------------------------------------------------------------------