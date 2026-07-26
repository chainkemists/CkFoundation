#include "CkCrowdAgent_DrawPlannedPath_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawPlannedPath);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DrawPlannedPath"), STAT_CkCrowd_DrawPlannedPathProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_draw_planned_path_processor
{
    // The `ck.Crowd.SelectedEntityId` CVar object lives in CkCrowdAgent_DiagDraw_Processor.cpp;
    // -1 means "no selection". The selected agent draws regardless of the DrawPlannedPaths toggle.
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
    // Dashed = intent, distinct from FProcessor_CrowdAgent_DiagDraw's solid actual-path trail.
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
            const FFragment_Transform& InTransform,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawPlannedPathProc);

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

        // Only the REMAINING path: starting at Waypoints[0] would paint a line backward from the
        // path's start to the agent.
        const auto NextWaypoint = InPathFollow.Get_WaypointIndex();
        if (NextWaypoint >= Waypoints.Num())
        { return; }
        const auto FirstIndex = FMath::Max(0, NextWaypoint);

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        // Same identity colour as the breadcrumb, alpha-reduced so the two still read distinctly
        // where they overlap.
        auto PathColor = UCk_Utils_CrowdAgent_UE::Get_DebugColor(InHandle);
        PathColor.A *= draw::AlphaScale;

        const auto PathThickness = bIsSelected ? draw::Thickness_Selected : draw::Thickness;
        const auto Lift = FVector(0.0f, 0.0f, draw::LiftZ);

        auto Prev = InTransform.Get_Transform().GetLocation() + Lift;

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
