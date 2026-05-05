#include "CkCrowdAgent_DrawNavProjection_Processor.h"

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

namespace
{
    constexpr auto MarkerRadius     = 35.0f;   // matches the agent radius for an "I'm here" feel
    constexpr auto MarkerSegments   = 16;
    constexpr auto MarkerThickness  = 2.0f;
    constexpr auto MarkerLiftZ      = 2.0f;    // hover just above the floor surface so it's visible
    constexpr auto Duration_OneFrame = 0.0f;

    constexpr auto OnNavmeshColor  = FLinearColor(0.20f, 1.0f, 0.20f, 0.85f);  // bright green
    constexpr auto OffNavmeshColor = FLinearColor(1.0f, 0.20f, 0.20f, 0.85f);  // bright red — actionable
}

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

        const auto Centre = bProjected
            ? ProjectedLoc.Location + FVector(0.0f, 0.0f, MarkerLiftZ)
            : AgentPos + FVector(0.0f, 0.0f, MarkerLiftZ);     // anchor red marker under the agent's foot

        const auto Color = bProjected ? OnNavmeshColor : OffNavmeshColor;

        UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
            World,
            Centre,
            MarkerRadius,
            ECk_Plane_Axis::XY,
            MarkerSegments,
            Color,
            Duration_OneFrame,
            MarkerThickness);
    }
}

// --------------------------------------------------------------------------------------------------------------------
