#include "CkJolt_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Registry/CkRegistry.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/World/CkJoltWorld.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyInterface.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_utils
{
    // Jolt's global registration is process-wide, but PIE clients and unit tests init/deinit independently.
    static int32 GJoltRefCount = 0;

    auto
        CustomTraceFunction(
            const char* inFMT,
            ...)
        -> void
    {
        va_list List;
        va_start(List, inFMT);
        char Buffer[1024];
        vsnprintf(Buffer, sizeof(Buffer), inFMT, List);
        va_end(List);

        ck::jolt::Verbose(TEXT("Jolt Trace: [{}]"), FString{Buffer});
    }

    auto
        CustomAssertFunction(
            const char* inExpression,
            const char* inMessage,
            const char* inFile,
            JPH::uint inLine)
        -> bool
    {
        CK_TRIGGER_ENSURE(TEXT("Jolt FAILED [{}] with Message [{}].\n{}:{}"), FString{inExpression}, FString{inMessage},
            FString{inFile}, inLine);
        return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    auto
        Request_GlobalJoltInit()
        -> void
    {
        if (ck_jolt_utils::GJoltRefCount++ != 0)
        { return; }

        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory{};
        JPH::RegisterTypes();

        JPH::Trace = ck_jolt_utils::CustomTraceFunction;
        JPH::AssertFailed = ck_jolt_utils::CustomAssertFunction;
    }

    auto
        Request_GlobalJoltShutdown()
        -> void
    {
        if (--ck_jolt_utils::GJoltRefCount != 0)
        { return; }

        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

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
    // Shape Axis Correction
    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ShapeAxisCorrection_YToZ()
        -> JoltQuat
    {
        // +90 degrees about X maps (0, 1, 0) -> (0, 0, 1). See the header for why this sign (and not -90).
        return JoltQuat::sRotation(JoltVec3::sAxisX(), JPH::DegreesToRadians(90.0f));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Body User Data
    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_BodyUserData(
            const JPH::Body& InBody)
        -> uint64
    {
        return InBody.GetUserData();
    }

    auto
        Get_BodyUserData(
            const JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId)
        -> uint64
    {
        return InBodyInterface.GetUserData(InBodyId);
    }

    auto
        TryGet_EntityFromBody(
            const FCk_Handle& InSelf,
            const JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InHitBodyId)
        -> FCk_Handle
    {
        const auto Entity = static_cast<FCk_Entity::IdType>(Get_BodyUserData(InBodyInterface, InHitBodyId));

        if (InSelf.Get_Entity().Get_ID() == Entity)
        { return {}; }

        if (InSelf.Get_RegistryView().IsValid(FCk_Entity{Entity}) == false)
        { return {}; }

        return InSelf.Get_ValidHandle(Entity);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Jolt_UE::
    Get_Debug_NumPersistedContactEventsLastDrain(
        const FCk_Handle& InAnyHandleInWorld)
    -> int32
{
    if (ck::Is_NOT_Valid(InAnyHandleInWorld))
    { return 0; }

    // An absent context is legal (a world with no Jolt subsystem never publishes one).
    const auto* WorldPtr = InAnyHandleInWorld.Get_RegistryView().TryGetContext<TSharedPtr<ck::FJoltWorld>>();

    if (WorldPtr == nullptr || NOT WorldPtr->IsValid())
    { return 0; }

    return WorldPtr->Get()->Get_Debug_NumPersistedContactEventsLastDrain();
}

// --------------------------------------------------------------------------------------------------------------------
