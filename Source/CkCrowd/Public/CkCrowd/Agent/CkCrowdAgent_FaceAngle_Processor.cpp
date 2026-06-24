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
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_DesiredVelocity& InDesired,
            FFragment_CrowdAgent_FaceAngle& InFaceAngle)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_FaceAngleProc);

        // Project desired velocity onto the XY plane — yaw is independent of vertical motion (a path
        // descent shouldn't make the agent face the ground). When horizontal speed is ~0 there's no
        // meaningful target heading; keep the last cached _TargetYaw and skip the rotation request.
        auto Heading = InDesired.Get_Velocity();
        Heading.Z = 0.0f;
        const auto SpeedXY = Heading.Size();
        if (SpeedXY < KINDA_SMALL_NUMBER)
        { return; }

        Heading /= SpeedXY;

        const auto TargetYawRad = FMath::Atan2(Heading.Y, Heading.X);
        InFaceAngle._TargetYaw = TargetYawRad;

        auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);
        if (ck::Is_NOT_Valid(TransformHandle))
        { return; }

        const auto CurrentRot = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(TransformHandle);
        const auto CurrentYawRad = FMath::DegreesToRadians(CurrentRot.Yaw);

        // Shortest-arc delta in (-pi, pi]. FindDeltaAngleRadians handles the wrap so we always pick
        // the direction that minimizes total rotation — no spinning the long way around.
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
