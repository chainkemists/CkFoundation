#include "CkCrowdAgent_FaceAngle_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_FaceAngle_Algorithm.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

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

        using namespace ck_crowd_agent_face_angle_algorithm;

        // Yaw is independent of vertical motion, so the engage gate reads the planar speed only.
        auto Heading = InDesired.Get_Velocity();
        Heading.Z = 0.0f;
        const auto SpeedXY = static_cast<float>(Heading.Size());

        const auto WasEngaged = InFaceAngle.Get_FacingEngaged();
        const auto FacingIsEngaged = Update_Engaged(
            InFaceAngle._FacingEngaged,
            InFaceAngle._PendingTargetFrames,
            SpeedXY,
            UCk_Utils_Crowd_Settings_UE::Get()->Get_FacingSpeedFloorCm());
        if (NOT FacingIsEngaged)
        { return; }

        const auto CurrentRot = InTransform.Get_Transform().Rotator();
        const auto CurrentYawRad = FMath::DegreesToRadians(CurrentRot.Yaw);

        // Resuming facing means resuming from where the body points, not from a target it never
        // finished reaching — see Seed_CommittedFromBody.
        if (NOT WasEngaged)
        { Seed_CommittedFromBody(InFaceAngle._TargetYaw, static_cast<float>(CurrentYawRad)); }

        Heading /= SpeedXY;

        InFaceAngle._TargetYaw = Commit_TrackedYaw(
            InFaceAngle.Get_TargetYaw(),
            static_cast<float>(FMath::Atan2(Heading.Y, Heading.X)),
            FMath::DegreesToRadians(UCk_Utils_Crowd_Settings_UE::Get()->Get_FacingDeadBandDeg()),
            InFaceAngle._PendingTargetYaw,
            InFaceAngle._PendingTargetFrames);

        const auto TargetYawRad = InFaceAngle.Get_TargetYaw();

        auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);

        const auto DeltaYaw = FMath::FindDeltaAngleRadians(CurrentYawRad, TargetYawRad);
        if (FMath::IsNearlyZero(DeltaYaw))
        { return; }

        const auto MaxDelta = InParams.Get_MaxTurnRate() * InDeltaT.Get_Seconds();
        const auto AppliedDelta = FMath::Clamp(DeltaYaw, -MaxDelta, MaxDelta);

        const auto Offset = FRotator(0.0f, FMath::RadiansToDegrees(AppliedDelta), 0.0f);

        UCk_Utils_Transform_UE::Request_AddRotationOffset
        (
            TransformHandle,
            FCk_Request_Transform_AddRotationOffset{Offset}.Set_LocalWorld(ECk_LocalWorld::World),
            {}
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
