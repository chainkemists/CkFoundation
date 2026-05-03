#include "CkCrowdAgent_AccelClamp_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_AccelClamp);

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
        // Project-wide enable. Disabled mode is for A/B comparison only — production should leave
        // this on to kill snap-flips that drive Gate-3 vibration.
        if (UCk_Utils_Crowd_Settings_UE::Get_AccelClampMode() == ECk_AccelClampMode::Disabled)
        {
            InDesired._LastVelocity = InDesired._Velocity;
            return;
        }

        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();
        const auto MaxDelta = MaxAccel * static_cast<float>(InDeltaT.Get_Seconds());

        const auto LastVel = InDesired.Get_LastVelocity();
        // Idle agents have no active writer to _Velocity (Steering's view excludes them once
        // Walking is removed). Without an explicit zero target, AccelClamp reads _Velocity as
        // last frame's own output and Dv stays ~0 → agent glides forever past its goal.
        // Force NewVel to zero for Idle so AccelClamp ramps the leftover velocity down to a stop.
        const auto NewVel = InHandle.Has<FTag_CrowdAgent_Idle>()
            ? FVector::ZeroVector
            : InDesired.Get_Velocity().GetClampedToMaxSize(MaxSpeed);
        const auto Dv      = NewVel - LastVel;
        const auto DvLen   = static_cast<float>(Dv.Size());

        if (DvLen > MaxDelta && DvLen > KINDA_SMALL_NUMBER)
        {
            InDesired._Velocity = LastVel + Dv * (MaxDelta / DvLen);
        }
        else
        {
            InDesired._Velocity = NewVel;
        }

        InDesired._LastVelocity = InDesired._Velocity;
    }
}

// --------------------------------------------------------------------------------------------------------------------
