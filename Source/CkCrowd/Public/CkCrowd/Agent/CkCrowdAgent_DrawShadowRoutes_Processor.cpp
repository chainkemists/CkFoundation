#include "CkCrowdAgent_DrawShadowRoutes_Processor.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Diagnostics/CkDiagnosticVisibility.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment_Data.h"
#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "DrawDebugHelpers.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawShadowRoutes);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DrawShadowRoutes"), STAT_CkCrowd_DrawShadowRoutesProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_draw_shadow_routes_processor
{
    // Both routes are lifted clear of FProcessor_CrowdAgent_DrawPlannedPath's 96uu band and of each
    // other: two plans over the same ground are coincident far more often than not, and a shared
    // height would leave the pair reading as one z-fighting line exactly where they agree.
    constexpr auto Recast_LiftZ          = 108.0f;
    constexpr auto Shadow_LiftZ          = 132.0f;
    constexpr auto Route_Thickness       = 2.0f;
    constexpr auto Connector_DashSize    = 24.0f;
    constexpr auto Connector_Thickness   = 2.0f;
    constexpr auto Endpoint_MarkerSize   = 18.0f;
    constexpr auto Endpoint_MarkerPoints = 4;
    constexpr auto Label_HeightAbove     = 190.0f;
    constexpr auto Label_FontScale       = 1.3f;
    constexpr auto DurationOneFrame      = 0.0f;

    const auto Recast_Color    = FLinearColor(0.10f, 0.85f, 1.00f, 1.0f);
    const auto Shadow_Color    = FLinearColor(1.00f, 0.25f, 0.90f, 1.0f);
    const auto Connector_Color = FLinearColor(1.00f, 0.95f, 0.30f, 1.0f);

    auto Get_PolylineLengthUu(
        const TArray<FVector>& InWaypoints)
        -> double
    {
        auto TotalUu = 0.0;
        for (auto Index = 1; Index < InWaypoints.Num(); ++Index)
        { TotalUu += FVector::Dist(InWaypoints[Index - 1], InWaypoints[Index]); }
        return TotalUu;
    }

    auto DrawPolyline(
        const UObject* InWorldContext,
        const TArray<FVector>& InWaypoints,
        const FVector& InLift,
        const FLinearColor& InColor)
        -> void
    {
        for (auto Index = 1; Index < InWaypoints.Num(); ++Index)
        {
            UCk_Utils_DebugDraw_UE::DrawDebugLine(
                InWorldContext,
                InWaypoints[Index - 1] + InLift,
                InWaypoints[Index] + InLift,
                InColor,
                DurationOneFrame,
                Route_Thickness);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawShadowRoutes::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_GroundNavPath_Result& InGroundNavResult)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawShadowRoutesProc);

        namespace draw = ck_crowd_agent_draw_shadow_routes_processor;

        if (ck::diagnostic_visibility::Is_HiddenForStreamerMode())
        { return; }

        if (NOT UCk_Utils_Crowd_DebugSettings_UE::Get_DrawShadowRoutes())
        { return; }

        const auto& ShadowResult = InGroundNavResult.Get_Result();
        if (ShadowResult.Get_IsShadow() != ECk_EnableDisable::Enable)
        { return; }

        // Same-revision pairing, the gate the install seam itself uses: a shadow answer to a
        // superseded query says nothing about the route the agent is walking now. Freshness is
        // deliberately NOT part of the gate — the compare processor clears that flag after one
        // reading, and an overlay that vanished on the frame after the answer arrived is unusable.
        if (ShadowResult.Get_RequestRevision() != InPathResult.Get_RequestRevision())
        { return; }

        const auto& RecastWaypoints = InPathResult.Get_Waypoints();
        const auto& ShadowWaypoints = ShadowResult.Get_Waypoints();
        if (RecastWaypoints.IsEmpty() && ShadowWaypoints.IsEmpty())
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        const auto RecastLift = FVector(0.0f, 0.0f, draw::Recast_LiftZ);
        const auto ShadowLift = FVector(0.0f, 0.0f, draw::Shadow_LiftZ);

        draw::DrawPolyline(World, RecastWaypoints, RecastLift, draw::Recast_Color);
        draw::DrawPolyline(World, ShadowWaypoints, ShadowLift, draw::Shadow_Color);

        // The endpoint delta is the one metric that cannot be eyeballed off two near-coincident
        // polylines, so it is drawn rather than left implicit in where the two lines stop.
        if (NOT RecastWaypoints.IsEmpty() && NOT ShadowWaypoints.IsEmpty())
        {
            const auto RecastEnd = RecastWaypoints.Last() + RecastLift;
            const auto ShadowEnd = ShadowWaypoints.Last() + ShadowLift;

            UCk_Utils_DebugDraw_UE::DrawDebugDashedLine(
                World,
                RecastEnd,
                ShadowEnd,
                draw::Connector_DashSize,
                draw::Connector_Color,
                draw::DurationOneFrame,
                draw::Connector_Thickness);
            UCk_Utils_DebugDraw_UE::DrawDebugStar(
                World,
                ShadowEnd,
                draw::Endpoint_MarkerSize,
                draw::Endpoint_MarkerPoints,
                draw::Shadow_Color,
                draw::DurationOneFrame,
                draw::Connector_Thickness);
        }

        // Both halves are measured the same way, off the waypoints being drawn — NOT off GroundNav's
        // own _LengthUu, which prices the plan rather than the polyline and would make this overlay
        // disagree with the compare processor's metric.
        const auto RecastLengthUu = draw::Get_PolylineLengthUu(RecastWaypoints);
        const auto ShadowLengthUu = draw::Get_PolylineLengthUu(ShadowWaypoints);
        const auto LengthDeltaUu  = ShadowLengthUu - RecastLengthUu;
        const auto WaypointDelta  = ShadowWaypoints.Num() - RecastWaypoints.Num();

        const auto RecastStatusName = StaticEnum<ECk_Nav_PathStatus>()->GetNameStringByValue(
            static_cast<int64>(InPathResult.Get_Status()));
        const auto ShadowStatusName = StaticEnum<ECk_GroundNav_PathStatus>()->GetNameStringByValue(
            static_cast<int64>(ShadowResult.Get_Status()));

        constexpr auto DrawStringShadow = true;

        DrawDebugString(
            World,
            InTransform.Get_Transform().GetLocation() + FVector(0.0f, 0.0f, draw::Label_HeightAbove),
            FString::Printf(
                TEXT("shadow Δlen=%+.0f uu Δwp=%+d %s/%s"),
                LengthDeltaUu,
                WaypointDelta,
                *RecastStatusName,
                *ShadowStatusName),
            /*TestBaseActor*/ nullptr,
            draw::Shadow_Color.ToFColor(true),
            draw::DurationOneFrame,
            DrawStringShadow,
            draw::Label_FontScale);
    }
}

// --------------------------------------------------------------------------------------------------------------------
