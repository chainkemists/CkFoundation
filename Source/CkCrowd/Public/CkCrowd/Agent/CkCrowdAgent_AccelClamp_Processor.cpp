#include "CkCrowdAgent_AccelClamp_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

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

        // Project-wide enable. Disabled mode is for A/B comparison only — production should leave
        // this on to kill snap-flips that drive vibration.
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
        // Idle agents have no active writer to _Velocity (Steering's view excludes them once
        // Walking is removed). Without an explicit zero target, AccelClamp reads _Velocity as
        // last frame's own output and Dv stays ~0 → agent glides forever past its goal.
        // Force NewVel to zero for Idle so AccelClamp ramps the leftover velocity down to a stop.
        const auto NewVel = InHandle.Has<FTag_CrowdAgent_Idle>()
            ? FVector::ZeroVector
            : InDesired.Get_Velocity().GetClampedToMaxSize(MaxSpeed);

        // Decouple direction and magnitude during velocity ramping. Linear-vector lerp produces
        // a magnitude dip during direction changes (e.g. lerping from (240,0) toward (0,240)
        // passes through (120,120) magnitude 170 — agents visibly slow down 30% during a 90°
        // turn). Pedestrian NPCs should rotate at MaxTurnRate while keeping their pace; speed
        // changes at MaxAcceleration are independent. Only the magnitude+direction recombination
        // is written back as the new velocity.
        const auto LastSpeed = static_cast<float>(LastVel.Size());
        const auto NewSpeed  = static_cast<float>(NewVel.Size());

        // Speed clamp: change at most MaxAccel*dt cm/s per frame.
        const auto MaxSpeedDelta = MaxAccel * Dt;
        const auto TargetSpeed = FMath::Clamp(
            NewSpeed,
            FMath::Max(0.0f, LastSpeed - MaxSpeedDelta),
            LastSpeed + MaxSpeedDelta);

        // Direction clamp: change at most MaxTurnRate*dt radians per frame.
        // Compute NewDir first so LastDir can fall back to it on first frame.
        // Newly spawned agents have LastSpeed ≈ 0; falling back to FVector::ForwardVector
        // caused a visible rotate-from-world-+X glitch on the first frame after path resolve.
        const auto NewDir = (NewSpeed > KINDA_SMALL_NUMBER)
            ? NewVel / NewSpeed
            : ((LastSpeed > KINDA_SMALL_NUMBER) ? (LastVel / LastSpeed) : FVector::ForwardVector);
        const auto LastDir = (LastSpeed > KINDA_SMALL_NUMBER)
            ? LastVel / LastSpeed
            : NewDir;

        const auto MaxAngleDelta = MaxTurnRate * Dt;
        const auto CosAngle = static_cast<float>(FVector::DotProduct(LastDir, NewDir));
        const auto Angle = FMath::Acos(FMath::Clamp(CosAngle, -1.0f, 1.0f));

        FVector TargetDir;
        if (Angle <= MaxAngleDelta || Angle < KINDA_SMALL_NUMBER)
        {
            TargetDir = NewDir;
        }
        else
        {
            // Slerp from LastDir toward NewDir by MaxAngleDelta of arc.
            const auto T = MaxAngleDelta / Angle;
            const auto SinAngle = FMath::Sin(Angle);
            if (SinAngle > KINDA_SMALL_NUMBER)
            {
                TargetDir = (LastDir * FMath::Sin((1.0f - T) * Angle)
                          + NewDir  * FMath::Sin(T * Angle)) / SinAngle;
                TargetDir.Normalize();
            }
            else
            {
                TargetDir = NewDir;
            }
        }

        InDesired._Velocity = TargetDir * TargetSpeed;
        InDesired._LastVelocity = InDesired._Velocity;
    }
}

// --------------------------------------------------------------------------------------------------------------------
