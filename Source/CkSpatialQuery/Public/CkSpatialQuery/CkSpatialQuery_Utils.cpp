#include "CkSpatialQuery_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkSpatialQuery/Probe/CkProbe_Utils.h"

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyInterface.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
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