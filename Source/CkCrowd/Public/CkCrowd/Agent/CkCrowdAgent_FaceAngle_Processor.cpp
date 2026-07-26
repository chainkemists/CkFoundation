#include "CkCrowdAgent_FaceAngle_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_FaceAngle);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::FaceAngle"), STAT_CkCrowd_FaceAngleProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_FaceAngle::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_DesiredVelocity& InDesired,
            FFragment_CrowdAgent_FaceAngle& InFaceAngle)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_FaceAngleProc);

        // Yaw is independent of vertical motion; with no horizontal speed there is no meaningful
        // target heading, so the last cached _TargetYaw stands.
        auto Heading = InDesired.Get_Velocity();
        Heading.Z = 0.0f;
        const auto SpeedXY = Heading.Size();
        if (SpeedXY < KINDA_SMALL_NUMBER)
        { return; }

        Heading /= SpeedXY;

        const auto TargetYawRad = FMath::Atan2(Heading.Y, Heading.X);
        InFaceAngle._TargetYaw = TargetYawRad;

        auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);

        const auto CurrentRot = InTransform.Get_Transform().Rotator();
        const auto CurrentYawRad = FMath::DegreesToRadians(CurrentRot.Yaw);

        const auto DeltaYaw = FMath::FindDeltaAngleRadians(CurrentYawRad, TargetYawRad);
        if (FMath::IsNearlyZero(DeltaYaw))
        { return; }

        const auto MaxDelta = InParams.Get_MaxTurnRate() * InDeltaT.Get_Seconds();
        const auto AppliedDelta = FMath::Clamp(DeltaYaw, -MaxDelta, MaxDelta);

        const auto Offset = FRotator(0.0f, FMath::RadiansToDegrees(AppliedDelta), 0.0f);

        UCk_Utils_Transform_UE::Request_AddRotationOffset
        (
            TransformHandle,
            FCk_Request_Transform_AddRotationOffset{Offset}.Set_LocalWorld(ECk_LocalWorld::World)
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
