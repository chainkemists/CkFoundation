#include "CkCrowdAgent_DrawNavProjection_Processor.h"

#include "CkCrowd/CkCrowd_Stats.h"
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

DECLARE_CYCLE_STAT(TEXT("Crowd::DrawNavProjection"), STAT_CkCrowd_DrawNavProjectionProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawNavProjection::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        // The disabled default must not iterate agents or enter their synchronous nav projection path.
        if (NOT UCk_Utils_Crowd_DebugSettings_UE::Get_DrawNavProjection())
        {
            _LastVisitedCount = 0;
            return;
        }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_CrowdAgent_DrawNavProjection::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawNavProjectionProc);

        // Gate before any work — the synchronous ProjectPointToNavigation per agent per tick is
        // the most expensive Crowd debug viz at scale. Off by default.
        if (NOT UCk_Utils_Crowd_DebugSettings_UE::Get_DrawNavProjection())
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        const auto AgentPos = InTransform.Get_Transform().GetLocation();

        auto* NavSys = UNavigationSystemV1::GetCurrent(World);
        if (NavSys == nullptr)
        { return; }

        // Explicit NavData + the path-query gate's own projection extent, so the overlay's verdict
        // matches FProcessor_Nav_HandleRequests' gate exactly. Without them the projection falls
        // back to the default agent's nav data, which can disagree with the queried RecastNavMesh.
        auto* NavData = Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate));
        const auto Extent = UCk_Utils_Nav_Settings_UE::Get_NavQueryProjectionExtentVec();
        auto ProjectedLoc = FNavLocation{};
        const auto bProjected = (NavData != nullptr)
            && NavSys->ProjectPointToNavigation(AgentPos, ProjectedLoc, Extent, NavData);

        constexpr auto NavProj_MarkerLiftZ      = 2.0f;
        constexpr auto NavProj_MarkerRadius     = 35.0f;
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
