#include "CkCrowdAgent_VelocityBridge_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_VelocityBridge);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_VelocityBridge::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_DesiredVelocity& InDesired)
        -> void
    {
        // The agent must have the Velocity feature for the bridge to land. The gym / game code is
        // responsible for adding it before the agent enters Walking state. If it's missing, Cast
        // returns an invalid handle and we short-circuit — CastChecked would ensure-spam every
        // frame for any structural-only agent (e.g. the Foundation gym pre-3A).
        auto VelocityHandle = UCk_Utils_Velocity_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(VelocityHandle))
        { return; }

        UCk_Utils_Velocity_UE::Request_OverrideVelocity(VelocityHandle, InDesired.Get_Velocity());
    }
}

// --------------------------------------------------------------------------------------------------------------------
