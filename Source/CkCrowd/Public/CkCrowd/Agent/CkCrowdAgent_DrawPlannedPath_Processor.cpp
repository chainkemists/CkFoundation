#include "CkCrowdAgent_DrawPlannedPath_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawPlannedPath);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_draw_planned_path_processor
{
    // Reads the selected agent's type-hash from the shared CVar `ck.Crowd.SelectedEntityId`
    // (the CVar object itself lives in CkCrowdAgent_DiagDraw_Processor.cpp); -1 means "no selection".
    // CVar `ck.Crowd.DrawPlannedPaths` is read through the settings BPFL so it persists across sessions;
    // the selected agent draws regardless of that toggle.
    auto GetSelectedEntityId() -> int32
    {
        const auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.SelectedEntityId"));
        return CVar != nullptr ? CVar->GetInt() : -1;
    }

    constexpr auto LiftZ              = 96.0f;
    constexpr auto Thickness          = 2.0f;
    constexpr auto Thickness_Selected = 4.0f;
    constexpr auto AlphaScale         = 0.55f;
    constexpr auto DurationOneFrame   = 0.0f;
    // The planned path is drawn DASHED so it reads as "where the agent intends to go" — visually
    // distinct from the solid breadcrumb trail (FProcessor_CrowdAgent_DiagDraw), which is where the
    // agent actually went. When the two diverge (e.g. an agent orbiting its goal), the dashed line
    // still points at the goal while the solid trail circles.
    constexpr auto DashSize           = 20.0f;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawPlannedPath::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> void
    {
        namespace draw = ck_crowd_agent_draw_planned_path_processor;

        const auto bDrawAll = UCk_Utils_Crowd_DebugSettings_UE::Get_DrawPlannedPaths();
        const auto SelectedHash = draw::GetSelectedEntityId();
        const auto bIsSelected = SelectedHash >= 0
            && static_cast<int32>(GetTypeHash(InHandle)) == SelectedHash;
        if (NOT bDrawAll && NOT bIsSelected)
        { return; }

        const auto& Waypoints = InPathResult.Get_Waypoints();
        if (Waypoints.Num() == 0)
        { return; }

        // Draw only the REMAINING path — from the waypoint the agent is currently heading toward
        // onward. Starting at Waypoints[0] (the path's start) would connect the agent backward to a
        // point it has already passed, painting a useless line from the start to the agent. Once the
        // cursor is past the last waypoint (goal reached) there is nothing ahead to draw.
        const auto NextWaypoint = InPathFollow.Get_WaypointIndex();
        if (NextWaypoint >= Waypoints.Num())
        { return; }
        const auto FirstIndex = FMath::Max(0, NextWaypoint);

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        // Same identity colour as the breadcrumb. Reduce alpha so the planned-vs-actual lines
        // still read distinctly when they overlap (which they will for an agent walking its
        // intended path).
        auto PathColor = UCk_Utils_CrowdAgent_UE::Get_DebugColor(InHandle);
        PathColor.A *= draw::AlphaScale;

        const auto PathThickness = bIsSelected ? draw::Thickness_Selected : draw::Thickness;
        const auto Lift = FVector(0.0f, 0.0f, draw::LiftZ);

        // Start segment connects the agent's current position to its NEXT waypoint. Fall back to
        // the next waypoint itself if the transform feature isn't on the agent.
        auto Prev = Waypoints[FirstIndex] + Lift;
        if (auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle); ck::IsValid(TransformHandle))
        {
            Prev = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle) + Lift;
        }

        for (auto i = FirstIndex; i < Waypoints.Num(); ++i)
        {
            const auto Curr = Waypoints[i] + Lift;
            UCk_Utils_DebugDraw_UE::DrawDebugDashedLine(
                World, Prev, Curr, draw::DashSize, PathColor, draw::DurationOneFrame, PathThickness);
            Prev = Curr;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
