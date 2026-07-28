#include "CkCrowdAgent_AccelClamp_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkCrowd/Agent/CkCrowdAgent_AccelClamp_Algorithm.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_AccelClamp);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::AccelClamp"), STAT_CkCrowd_AccelClampProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_AccelClamp::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_DesiredVelocity& InDesired)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_AccelClampProc);

        if (UCk_Utils_Crowd_Settings_UE::Get_AccelClampMode() == ECk_AccelClampMode::Disabled)
        {
            InDesired._LastVelocity = InDesired._Velocity;
            return;
        }

        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();
        const auto MaxTurnRate = InParams.Get_MaxTurnRate();
        const auto Dt = static_cast<float>(InDeltaT.Get_Seconds());

        const auto LastVel = InDesired.Get_LastVelocity();
        // Idle agents have no writer to _Velocity (Steering's view drops them with Walking), so
        // without an explicit zero target Dv stays ~0 and the agent glides past its goal forever.
        const auto NewVel = InHandle.Has<FTag_CrowdAgent_Idle>()
            ? FVector::ZeroVector
            : InDesired.Get_Velocity();

        InDesired._Velocity = ck_crowd_agent_accel_clamp_algorithm::ProjectVelocity(
            LastVel,
            NewVel,
            MaxSpeed,
            MaxAccel,
            MaxTurnRate,
            Dt);
        InDesired._LastVelocity = InDesired._Velocity;
    }
}

// --------------------------------------------------------------------------------------------------------------------
