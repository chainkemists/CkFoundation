#include "CkCrowdAgent_DebugDraw_Processor.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DebugDraw);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DebugDraw"), STAT_CkCrowd_DebugDrawProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DebugDraw::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        // The visualization switch is global and live. Gate before TProcessor::DoTick so the
        // disabled default performs zero query visits.
        if (NOT UCk_Utils_Crowd_DebugSettings_UE::Get_DrawSeparation())
        {
            _LastVisitedCount = 0;
            return;
        }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_CrowdAgent_DebugDraw::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            const FFragment_CrowdAgent_SeparationForce& InSeparationForce)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DebugDrawProc);

        if (NOT UCk_Utils_Crowd_DebugSettings_UE::Get_DrawSeparation())
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        const auto Feet = InTransform.Get_Transform().GetLocation();
        const auto Center = Feet + FVector{0.0, 0.0, InParams.Get_Height() * 0.5};

        // --- Separation radius circle (yellow, on the ground at agent feet) -------------------
        constexpr auto CircleSegments = 24;
        constexpr auto LineThickness = 1.5f;
        constexpr auto Duration_OneFrame = 0.0f;
        UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
            World,
            Feet,
            InParams.Get_SeparationRadius(),
            ECk_Plane_Axis::XY,
            CircleSegments,
            FLinearColor{1.0f, 0.85f, 0.0f, 0.6f},
            Duration_OneFrame,
            LineThickness);

        // --- Lines to each neighbor (cyan, agent center to neighbor center) -------------------
        for (const auto& Nbr : InNeighborCache.Get_Neighbors())
        {
            const auto NeighborCenter = Center + Nbr.Get_RelativeOffset();
            UCk_Utils_DebugDraw_UE::DrawDebugLine(
                World,
                Center,
                NeighborCenter,
                FLinearColor{0.42f, 0.85f, 1.0f, 0.7f},
                Duration_OneFrame,
                LineThickness);
        }

        // --- Separation force arrow (orange, from agent center) -------------------------------
        const auto& Force = InSeparationForce.Get_Force();
        const auto ForceMag = Force.Size();
        if (ForceMag > 1.0)
        {
            constexpr auto VisualScale = 0.5f;
            constexpr auto MaxArrowLength = 300.0f;
            const auto ArrowLength = FMath::Min<double>(ForceMag * VisualScale, MaxArrowLength);
            const auto ArrowEnd = Center + (Force / ForceMag) * ArrowLength;

            constexpr auto ArrowSize = 20.0f;
            UCk_Utils_DebugDraw_UE::DrawDebugArrow(
                World,
                Center,
                ArrowEnd,
                ArrowSize,
                FLinearColor{1.0f, 0.55f, 0.15f, 0.9f},
                Duration_OneFrame,
                LineThickness * 1.5f);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
