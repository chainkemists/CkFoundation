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
            : InDesired.Get_Velocity().GetClampedToMaxSize(MaxSpeed);

        const auto LastSpeed = static_cast<float>(LastVel.Size());
        const auto NewSpeed  = static_cast<float>(NewVel.Size());

        const auto MaxSpeedDelta = MaxAccel * Dt;
        const auto TargetSpeed = FMath::Clamp(
            NewSpeed,
            FMath::Max(0.0f, LastSpeed - MaxSpeedDelta),
            LastSpeed + MaxSpeedDelta);

        // NewDir is computed before LastDir so a first-frame agent (LastSpeed ~ 0) falls back to
        // its new direction instead of world +X.
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
            const auto SlerpAlpha = MaxAngleDelta / Angle;
            const auto SinAngle = FMath::Sin(Angle);
            if (SinAngle > KINDA_SMALL_NUMBER)
            {
                TargetDir = (LastDir * FMath::Sin((1.0f - SlerpAlpha) * Angle)
                          + NewDir  * FMath::Sin(SlerpAlpha * Angle)) / SinAngle;
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
