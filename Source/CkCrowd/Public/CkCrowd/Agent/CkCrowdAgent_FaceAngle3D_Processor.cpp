#include "CkCrowdAgent_FaceAngle3D_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_FaceAngle3D);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::FaceAngle3D"), STAT_CkCrowd_FaceAngle3DProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_FaceAngle3D::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_DesiredVelocity& InDesired,
            FFragment_CrowdAgent_FaceAngle& InFaceAngle)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_FaceAngle3DProc);

        // With no motion at all there is no meaningful target heading, so the last cached targets stand.
        auto Heading = InDesired.Get_Velocity();
        const auto Speed = Heading.Size();
        if (Speed < KINDA_SMALL_NUMBER)
        { return; }

        Heading /= Speed;

        // A straight climb or dive has no horizontal component to read a yaw from — pitch alone moves,
        // and the agent holds the heading it entered the vertical span on rather than snapping to +X.
        const auto HorizontalSpeed = Heading.Size2D();
        const auto TargetYawRad = HorizontalSpeed < KINDA_SMALL_NUMBER
            ? InFaceAngle.Get_TargetYaw()
            : FMath::Atan2(Heading.Y, Heading.X);
        const auto TargetPitchRad = FMath::Atan2(Heading.Z, HorizontalSpeed);

        InFaceAngle._TargetYaw = TargetYawRad;
        InFaceAngle._TargetPitch = TargetPitchRad;

        const auto CurrentRot = InTransform.Get_Transform().Rotator();
        const auto CurrentYawRad = FMath::DegreesToRadians(CurrentRot.Yaw);
        const auto CurrentPitchRad = FMath::DegreesToRadians(CurrentRot.Pitch);

        const auto DeltaYaw = FMath::FindDeltaAngleRadians(CurrentYawRad, TargetYawRad);
        const auto DeltaPitch = FMath::FindDeltaAngleRadians(CurrentPitchRad, TargetPitchRad);
        if (FMath::IsNearlyZero(DeltaYaw) && FMath::IsNearlyZero(DeltaPitch))
        { return; }

        // One budget per axis, the yaw-only processor's constant: a turn that costs both is allowed to
        // spend the full rate on each, exactly as a pure yaw turn does.
        const auto MaxDelta = InParams.Get_MaxTurnRate() * InDeltaT.Get_Seconds();
        const auto AppliedYawRad = CurrentYawRad + FMath::Clamp(DeltaYaw, -MaxDelta, MaxDelta);
        const auto AppliedPitchRad = CurrentPitchRad + FMath::Clamp(DeltaPitch, -MaxDelta, MaxDelta);

        auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);

        // The ABSOLUTE rotation, not a delta: composing a world-space pitch onto an already-yawed
        // rotation tilts about the world Y axis and leaks roll into the agent frame every frame.
        // Naming the reached orientation instead keeps roll structurally zero.
        const auto Reached = FRotator
        {
            FMath::RadiansToDegrees(AppliedPitchRad),
            FMath::RadiansToDegrees(AppliedYawRad),
            0.0f
        };

        UCk_Utils_Transform_UE::Request_SetRotation
        (
            TransformHandle,
            FCk_Request_Transform_SetRotation{Reached}.Set_LocalWorld(ECk_LocalWorld::World),
            {}
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
