#include "CkSpatialQuery_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkSpatialQuery/Probe/CkProbe_Utils.h"

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyInterface.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    // ----------------------------------------------------------------------------------------------------------------
    // Probe Enum Conversions
    // ----------------------------------------------------------------------------------------------------------------

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
    // Probe Body User Data
    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ProbeBodyUserData(
            const JPH::Body& InBody)
        -> uint64
    {
        return Get_BodyUserData(InBody);
    }

    auto
        Get_ProbeBodyUserData(
            const JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId)
        -> uint64
    {
        return Get_BodyUserData(InBodyInterface, InBodyId);
    }

    auto
        TryGet_ProbeFromBodyHit(
            const FCk_Handle& InSelf,
            const JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InHitBodyId)
        -> FCk_Handle_Probe
    {
        return UCk_Utils_Probe_UE::Cast(TryGet_EntityFromBody(InSelf, InBodyInterface, InHitBodyId));
    }
}

// --------------------------------------------------------------------------------------------------------------------