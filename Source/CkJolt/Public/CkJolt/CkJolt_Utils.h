#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkJolt/CkJolt_Common.h"
#include "CkJolt/Character/CkJoltCharacter_Fragment_Data.h"

#include <Chaos/Vector.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Body/MotionQuality.h>
#include <Jolt/Physics/Character/CharacterBase.h>
#include <Jolt/Physics/Collision/BackFaceMode.h>

#include "CkJolt_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace JPH
{
    class Shape;
    class PhysicsSystem;
    class BodyInterface;
}

// --------------------------------------------------------------------------------------------------------------------

// ck::IsValid support for Jolt's intrusive ref-counted handle — valid = non-null referent.
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(JPH::Ref);

CK_DEFINE_CUSTOM_IS_VALID_T(T, JPH::Ref<T>, IsValid_Policy_Default, [=](const JPH::Ref<T>& InRef)
{
    return InRef.GetPtr() != nullptr;
});

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
    CKJOLT_API auto Conv(const FLinearColor& InColor) -> JoltColor;
    CKJOLT_API auto Conv(ECk_MotionType InMotionType) -> JPH::EMotionType;
    CKJOLT_API auto Conv(ECk_MotionQuality InMotionQuality) -> JPH::EMotionQuality;
    CKJOLT_API auto Conv(ECk_BackFaceMode InBackFaceMode) -> JPH::EBackFaceMode;

    /// Jolt's ground-state ordering differs from the Ck mirror's, so this is an explicit map and never a cast.
    CKJOLT_API auto Conv(JPH::CharacterBase::EGroundState InGroundState) -> ECk_JoltCharacter_GroundState;

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

UCLASS(NotBlueprintable)
class CKJOLT_API UCk_Utils_Jolt_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Jolt_UE);

public:
    // Returns how many Persisted contact events this world has consumed since it was created. Cumulative
    // on purpose: a single drain can legitimately see zero events (the fixed-step pump may not sub-step
    // between two drains), so interest is observed by diffing two samples over a window, never by reading
    // one drain. Test/diagnostic hook for the interest gate; 0 when the world has no Jolt subsystem.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Jolt|Debug",
              DisplayName="[Ck][Jolt] Get Debug Num Persisted Contact Events Total")
    static int64
    Get_Debug_NumPersistedContactEventsTotal(
        const FCk_Handle& InAnyHandleInWorld);
};

// --------------------------------------------------------------------------------------------------------------------