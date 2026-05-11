#include "CkCrowdAgent_DrawNavProjection_Processor.h"

#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavigationData.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawNavProjection);

// --------------------------------------------------------------------------------------------------------------------

// Constants are inlined into the function body below (rather than in an anonymous namespace)
// because UE's Unity-build merges multiple .cpp files into a single TU, causing
// same-named anonymous-namespace symbols across draw processors to collide at link time.

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawNavProjection::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams)
        -> void
    {
        // Gate before any work — the synchronous ProjectPointToNavigation + 16-segment
        // circle draw per agent is the most expensive Crowd debug viz at scale (linear
        // in agent count and runs every tick). Off by default; opt in via
        // `ck.Crowd.DrawNavProjection 1` or Editor Preferences → Crowd Debug.
        if (NOT UCk_Utils_Crowd_DebugSettings_UE::Get_DrawNavProjection())
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(TransformHandle))
        { return; }
        const auto AgentPos = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

        auto* NavSys = UNavigationSystemV1::GetCurrent(World);
        if (NavSys == nullptr)
        { return; }

        // Project to navmesh using the SAME signature FProcessor_Nav_HandleRequests' gate uses —
        // explicit NavData argument so the visualisation matches the gate's verdict exactly.
        // Without the NavData arg, ProjectPointToNavigation falls back to the default agent's
        // nav data which can disagree with the specific RecastNavMesh used by path queries.
        auto* NavData = Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate));
        const auto Extent = FVector(UCk_Utils_Nav_Settings_UE::Get_NavQuerySearchHalfExtent());
        auto ProjectedLoc = FNavLocation{};
        const auto bProjected = (NavData != nullptr)
            && NavSys->ProjectPointToNavigation(AgentPos, ProjectedLoc, Extent, NavData);

        constexpr auto NavProj_MarkerLiftZ      = 2.0f;    // hover just above the floor surface
        constexpr auto NavProj_MarkerRadius     = 35.0f;   // matches the agent radius for "I'm here"
        constexpr auto NavProj_MarkerSegments   = 16;
        constexpr auto NavProj_MarkerThickness  = 2.0f;
        constexpr auto NavProj_DurationOneFrame = 0.0f;
        const auto NavProj_OnColor  = FLinearColor(0.20f, 1.0f, 0.20f, 0.85f);
        const auto NavProj_OffColor = FLinearColor(1.0f, 0.20f, 0.20f, 0.85f);

        const auto Centre = bProjected
            ? ProjectedLoc.Location + FVector(0.0f, 0.0f, NavProj_MarkerLiftZ)
            : AgentPos + FVector(0.0f, 0.0f, NavProj_MarkerLiftZ);

        const auto Color = bProjected ? NavProj_OnColor : NavProj_OffColor;

        UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
            World,
            Centre,
            NavProj_MarkerRadius,
            ECk_Plane_Axis::XY,
            NavProj_MarkerSegments,
            Color,
            NavProj_DurationOneFrame,
            NavProj_MarkerThickness);
    }
}

// --------------------------------------------------------------------------------------------------------------------
