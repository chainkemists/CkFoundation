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

namespace
{
    // CVar `ck.Crowd.DrawPlannedPaths` is now declared via UPROPERTY in
    // UCk_Crowd_DebugSettings_UE — read it through the settings BPFL so values persist
    // across editor sessions. Selected agent (CVarSelectedEntityId) draws regardless.

    // Looked up by name — defined in CkCrowdAgent_DiagDraw_Processor.cpp. Sharing the same CVar
    // across the two draw processors via FindConsoleVariable lets the debugger write a single
    // selection state without us having to expose a global int between modules.
    auto GetSelectedEntityId() -> int32
    {
        const auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.SelectedEntityId"));
        return CVar != nullptr ? CVar->GetInt() : -1;
    }

    // Prefixed to avoid Unity-build collisions with same-named constants in sibling draw processors.
    constexpr auto PlannedPath_LiftZ              = 96.0f;
    constexpr auto PlannedPath_Thickness          = 2.0f;
    constexpr auto PlannedPath_Thickness_Selected = 4.0f;
    constexpr auto PlannedPath_AlphaScale         = 0.55f;
    constexpr auto PlannedPath_DurationOneFrame   = 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawPlannedPath::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_PathResult& InPathResult)
        -> void
    {
        const auto bDrawAll = UCk_Utils_Crowd_DebugSettings_UE::Get_DrawPlannedPaths();
        const auto SelectedHash = GetSelectedEntityId();
        const auto bIsSelected = SelectedHash >= 0
            && static_cast<int32>(GetTypeHash(InHandle)) == SelectedHash;
        if (NOT bDrawAll && NOT bIsSelected)
        { return; }

        const auto& Waypoints = InPathResult.Get_Waypoints();
        if (Waypoints.Num() == 0)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        // Same identity colour as the breadcrumb. Reduce alpha so the planned-vs-actual lines
        // still read distinctly when they overlap (which they will for an agent walking its
        // intended path).
        auto PathColor = UCk_Utils_CrowdAgent_UE::Get_DebugColor(InHandle);
        PathColor.A *= PlannedPath_AlphaScale;

        const auto Thickness = bIsSelected ? PlannedPath_Thickness_Selected : PlannedPath_Thickness;
        const auto Lift = FVector(0.0f, 0.0f, PlannedPath_LiftZ);

        // Start segment connects the agent's current position to the first waypoint. Fall back
        // to drawing only between waypoints if the transform feature isn't on the agent.
        auto Prev = Waypoints[0] + Lift;
        if (auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle); ck::IsValid(TransformHandle))
        {
            Prev = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle) + Lift;
        }

        for (const auto& Wp : Waypoints)
        {
            const auto Curr = Wp + Lift;
            UCk_Utils_DebugDraw_UE::DrawDebugLine(
                World, Prev, Curr, PathColor, PlannedPath_DurationOneFrame, Thickness);
            Prev = Curr;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
