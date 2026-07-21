#include "CkCrowdAgent_DrawAgentRings_Processor.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawAgentRings);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DrawAgentRings"), STAT_CkCrowd_DrawAgentRingsProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_draw_agent_rings_processor
{
    // Reads the selected agent's type-hash from the shared CVar `ck.Crowd.SelectedEntityId`
    // (the CVar object itself lives in CkCrowdAgent_DiagDraw_Processor.cpp); -1 means "no selection".
    auto GetSelectedEntityId() -> int32
    {
        const auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.SelectedEntityId"));
        return CVar != nullptr ? CVar->GetInt() : -1;
    }

    constexpr auto LiftZ            = 2.0f;
    constexpr auto Segments         = 32;
    constexpr auto Thickness        = 2.0f;
    constexpr auto DurationOneFrame = 0.0f;
    constexpr auto MinSpeedToDraw   = 5.0f;   // cm/s — below this the agent is effectively stopped
    constexpr auto MaxVelArrowLen   = 300.0f; // visual cap so a sprinting arrow isn't room-length

    // Colours mirror the debugger mockup legend.
    const auto Color_Arrival = FLinearColor{0.21f, 0.82f, 0.48f, 0.9f};  // green
    const auto Color_Orbit   = FLinearColor{1.0f,  0.36f, 0.36f, 0.9f};  // red (predicted)
    const auto Color_Turn    = FLinearColor{0.31f, 0.63f, 1.0f,  0.7f};  // blue
    const auto Color_Vel     = FLinearColor{1.0f,  0.82f, 0.30f, 0.95f}; // yellow
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawAgentRings::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_CrowdAgent_DesiredVelocity& InDesiredVelocity)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawAgentRingsProc);

        namespace draw = ck_crowd_agent_draw_agent_rings_processor;

        const auto DrawAll = UCk_Utils_Crowd_DebugSettings_UE::Get_DrawAgentRings();
        const auto SelectedHash = draw::GetSelectedEntityId();
        const auto IsSelected = SelectedHash >= 0
            && static_cast<int32>(GetTypeHash(InHandle)) == SelectedHash;
        if (NOT DrawAll && NOT IsSelected)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        const auto Lift     = FVector{0.0, 0.0, draw::LiftZ};
        const auto Feet     = InTransform.Get_Transform().GetLocation() + Lift;
        const auto Velocity = InDesiredVelocity.Get_Velocity();
        const auto Speed    = Velocity.Size();

        const auto MaxTurnRate = InParams.Get_MaxTurnRate();
        const auto MaxSpeed    = InParams.Get_MaxSpeed();

        // --- Goal rings (arrival + predicted orbit), only while actually heading to a goal ---------
        // A stale _ActiveGoal on an idle agent would draw a misleading ring at wherever it last went.
        if (InHandle.Has<FTag_CrowdAgent_Walking>())
        {
            const auto Goal = InPathFollow.Get_ActiveGoal() + Lift;

            UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
                World, Goal, InPathFollow.Get_ActiveArrivalRadius(), ECk_Plane_Axis::XY,
                draw::Segments, draw::Color_Arrival, draw::DurationOneFrame, draw::Thickness);

            if (MaxTurnRate > 0.0f)
            {
                const auto PredictedOrbitRadius = MaxSpeed / MaxTurnRate;
                UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
                    World, Goal, PredictedOrbitRadius, ECk_Plane_Axis::XY,
                    draw::Segments, draw::Color_Orbit, draw::DurationOneFrame, draw::Thickness);
            }
        }

        // --- Turn-radius circle + velocity vector, only while moving -------------------------------
        if (Speed < draw::MinSpeedToDraw)
        { return; }

        const auto VelDir = Velocity / Speed;

        // Tightest arc the agent can currently make: r = v / MaxTurnRate, centre perpendicular-left
        // of the velocity. Drawn tangent to the agent so you can read it against the arrival ring —
        // when this circle can't fit inside the arrival ring, the agent orbits (fix A caps speed to
        // shrink it). Perpendicular-left in XY = (-vy, vx).
        if (MaxTurnRate > 0.0f)
        {
            const auto TurnRadius = static_cast<float>(Speed / MaxTurnRate);
            const auto PerpLeft   = FVector{-VelDir.Y, VelDir.X, 0.0};
            const auto Centre     = Feet + PerpLeft * TurnRadius;
            UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
                World, Centre, TurnRadius, ECk_Plane_Axis::XY,
                draw::Segments, draw::Color_Turn, draw::DurationOneFrame, draw::Thickness);
        }

        const auto ArrowLen = FMath::Min<double>(Speed, draw::MaxVelArrowLen);
        const auto ArrowEnd = Feet + VelDir * ArrowLen;
        constexpr auto ArrowSize = 20.0f;
        UCk_Utils_DebugDraw_UE::DrawDebugArrow(
            World, Feet, ArrowEnd, ArrowSize, draw::Color_Vel, draw::DurationOneFrame, draw::Thickness);
    }
}

// --------------------------------------------------------------------------------------------------------------------
