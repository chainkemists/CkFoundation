#include "CkCrowdAgent_FaceAngle3D_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_FaceAngle_Algorithm.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

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

        using namespace ck_crowd_agent_face_angle_algorithm;

        // A hovering flyer's heading is as noisy as a pressing walker's, in pitch as well as yaw, so
        // the engage gate reads the full 3D speed and holds BOTH axes below it.
        auto Heading = InDesired.Get_Velocity();
        const auto Speed = static_cast<float>(Heading.Size());

        const auto WasEngaged = InFaceAngle.Get_FacingEngaged();
        const auto FacingIsEngaged = Update_Engaged(
            InFaceAngle._FacingEngaged,
            InFaceAngle._PendingTargetFrames,
            Speed,
            UCk_Utils_Crowd_Settings_UE::Get()->Get_FacingSpeedFloorCm());
        if (NOT FacingIsEngaged)
        { return; }

        const auto CurrentRot = InTransform.Get_Transform().Rotator();
        const auto CurrentYawRad = FMath::DegreesToRadians(CurrentRot.Yaw);
        const auto CurrentPitchRad = FMath::DegreesToRadians(CurrentRot.Pitch);

        // Only yaw is seeded: it is the axis behind the persistence filter, and it is also the axis a
        // pure climb or dive declines to commit at all, so a stale yaw is what the body would slew
        // toward on resume. Pitch commits fresh below on every engaged frame, so it has no stale
        // window to seed away.
        if (NOT WasEngaged)
        { Seed_CommittedFromBody(InFaceAngle._TargetYaw, static_cast<float>(CurrentYawRad)); }

        Heading /= Speed;

        // A straight climb or dive has no horizontal component to read a yaw from — pitch alone moves,
        // and the agent holds the heading it entered the vertical span on rather than snapping to +X.
        const auto HorizontalSpeed = Heading.Size2D();
        if (HorizontalSpeed >= KINDA_SMALL_NUMBER)
        {
            InFaceAngle._TargetYaw = Commit_TrackedYaw(
                InFaceAngle.Get_TargetYaw(),
                static_cast<float>(FMath::Atan2(Heading.Y, Heading.X)),
                InFaceAngle._PendingTargetYaw,
                InFaceAngle._PendingTargetFrames);
        }
        else
        {
            // A frame that supplied no heading at all breaks the candidate chain: "consecutive" has
            // to mean consecutive frames that actually named a yaw.
            InFaceAngle._PendingTargetFrames = 0;
        }

        InFaceAngle._TargetPitch = static_cast<float>(FMath::Atan2(Heading.Z, HorizontalSpeed));

        const auto TargetYawRad = InFaceAngle.Get_TargetYaw();
        const auto TargetPitchRad = InFaceAngle.Get_TargetPitch();

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
