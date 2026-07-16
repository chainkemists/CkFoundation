#include "CkPhysicsOwnership_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcsExt/PhysicsOwnership/CkPhysicsOwnership_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::physics_ownership
{
    auto
        TryClaim_Jolt(
            FCk_Handle& InHandle)
        -> bool
    {
        CK_ENSURE_IF_NOT(NOT InHandle.Has<FTag_PhysicsOwnership_Chaos>(),
            TEXT("Entity [{}] is already CHAOS-simulated (OverlapBody/RaySense) — Chaos and Jolt simulation "
                 "are mutually exclusive per entity. The Jolt-side feature was NOT composed."), InHandle)
        { return false; }

        InHandle.Add<FTag_PhysicsOwnership_Jolt>();
        return true;
    }

    auto
        TryClaim_Chaos(
            FCk_Handle& InHandle)
        -> bool
    {
        CK_ENSURE_IF_NOT(NOT InHandle.Has<FTag_PhysicsOwnership_Jolt>(),
            TEXT("Entity [{}] is already JOLT-simulated (Probe/JoltBody/JoltCharacter) — Chaos and Jolt "
                 "simulation are mutually exclusive per entity. The Chaos-side feature was NOT composed."), InHandle)
        { return false; }

        InHandle.Add<FTag_PhysicsOwnership_Chaos>();
        return true;
    }

    auto
        Release_Jolt(
            FCk_Handle& InHandle)
        -> void
    {
        if (InHandle.Has<FTag_PhysicsOwnership_Jolt>())
        { InHandle.Remove<FTag_PhysicsOwnership_Jolt>(); }
    }

    auto
        Release_Chaos(
            FCk_Handle& InHandle)
        -> void
    {
        if (InHandle.Has<FTag_PhysicsOwnership_Chaos>())
        { InHandle.Remove<FTag_PhysicsOwnership_Chaos>(); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
