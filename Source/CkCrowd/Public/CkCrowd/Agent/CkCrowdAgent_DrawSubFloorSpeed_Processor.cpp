#include "CkCrowdAgent_DrawSubFloorSpeed_Processor.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkCrowd/CkCrowd_Stats.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawSubFloorSpeed);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DrawSubFloorSpeed"), STAT_CkCrowd_DrawSubFloorSpeedProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_draw_sub_floor_speed
{
    // Upper edge of the marked band, planar cm/s; 0 disables the overlay. 10 matches
    // _FacingSpeedFloorCm and _BlockedStationarySpeedThreshold — the speed below which the body
    // already holds its facing and the detectors already read the agent as standing still, i.e.
    // exactly the band whose velocity heading the diag recorder still counts.
    static TAutoConsoleVariable<float> CVarSubFloorSpeedCm(
        TEXT("ck.Crowd.Debug.SubFloorSpeedCm"),
        0.0f,
        TEXT("Draw a ring + velocity-heading needle over agents whose planar speed is inside (0, this]. 0 = off."),
        ECVF_Cheat);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawSubFloorSpeed::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Velocity_Current& InVelocity)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawSubFloorSpeedProc);

        const auto Threshold =
            ck_crowd_agent_draw_sub_floor_speed::CVarSubFloorSpeedCm.GetValueOnGameThread();
        if (Threshold <= 0.0f)
        { return; }

        const auto Velocity = InVelocity.Get_CurrentVelocity();
        const auto PlanarSpeed = static_cast<float>(Velocity.Size2D());
        if (PlanarSpeed <= KINDA_SMALL_NUMBER || PlanarSpeed > Threshold)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        const auto Feet = InTransform.Get_Transform().GetLocation();
        const auto MarkerBase = Feet + FVector{0.0, 0.0, InParams.Get_Height() + 20.0};
        const auto Heading = FVector{Velocity.X, Velocity.Y, 0.0}.GetSafeNormal();

        constexpr auto Duration_OneFrame = 0.0f;
        constexpr auto LineThickness = 2.0f;
        constexpr auto NeedleLength = 50.0f;
        constexpr auto ArrowSize = 15.0f;
        constexpr auto RingRadius = 6.0f;
        constexpr auto RingSegments = 12;
        const auto Magenta = FLinearColor{1.0f, 0.2f, 0.9f, 0.95f};

        UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
            World,
            MarkerBase,
            RingRadius,
            ECk_Plane_Axis::XY,
            RingSegments,
            Magenta,
            Duration_OneFrame,
            LineThickness);

        UCk_Utils_DebugDraw_UE::DrawDebugArrow(
            World,
            MarkerBase,
            MarkerBase + Heading * NeedleLength,
            ArrowSize,
            Magenta,
            Duration_OneFrame,
            LineThickness);
    }
}

// --------------------------------------------------------------------------------------------------------------------
