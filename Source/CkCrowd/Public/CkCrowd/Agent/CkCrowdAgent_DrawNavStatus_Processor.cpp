#include "CkCrowdAgent_DrawNavStatus_Processor.h"

#include "CkCrowd/CkCrowd_Stats.h"

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "DrawDebugHelpers.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawNavStatus);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DrawNavStatus"), STAT_CkCrowd_DrawNavStatusProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_draw_nav_status_processor
{
    constexpr auto NavStatus_MarkerHeightAbove = 230.0f;
    constexpr auto NavStatus_MarkerHalfSize    = 30.0f;
    constexpr auto NavStatus_MarkerThickness   = 4.0f;
    constexpr auto NavStatus_LabelFontScale    = 1.5f;
    constexpr auto NavStatus_DurationOneFrame  = 0.0f;

    const auto NavStatus_FailedColor      = FLinearColor(1.0f, 0.15f, 0.15f, 1.0f);
    const auto NavStatus_PendingColor     = FLinearColor(1.0f, 0.85f, 0.20f, 1.0f);
    const auto NavStatus_FailedTextColor  = FColor(255, 60, 60);
    const auto NavStatus_PendingTextColor = FColor(255, 215, 50);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawNavStatus::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_Nav_PathResult& InPathResult)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawNavStatusProc);

        const auto Status = InPathResult.Get_Status();
        const auto bIsFailed  = Status == ECk_Nav_PathStatus::Failed;
        const auto bIsPending = Status == ECk_Nav_PathStatus::Pending;
        if (NOT bIsFailed && NOT bIsPending)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        const auto Pos = InTransform.Get_Transform().GetLocation();
        const auto MarkerCentre = Pos + FVector(0.0f, 0.0f, ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHeightAbove);

        const auto MarkerColor   = bIsFailed ? ck_crowd_agent_draw_nav_status_processor::NavStatus_FailedColor     : ck_crowd_agent_draw_nav_status_processor::NavStatus_PendingColor;
        const auto LabelColor    = bIsFailed ? ck_crowd_agent_draw_nav_status_processor::NavStatus_FailedTextColor : ck_crowd_agent_draw_nav_status_processor::NavStatus_PendingTextColor;

        // Red marks a terminal failure; yellow an in-flight Pending, which can stick forever when
        // the deferred queue is parked waiting for a navmesh that never arrives.
        UCk_Utils_DebugDraw_UE::DrawDebugLine(
            World,
            MarkerCentre + FVector(-ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, -ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, 0.0f),
            MarkerCentre + FVector(+ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, +ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, 0.0f),
            MarkerColor, ck_crowd_agent_draw_nav_status_processor::NavStatus_DurationOneFrame, ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerThickness);
        UCk_Utils_DebugDraw_UE::DrawDebugLine(
            World,
            MarkerCentre + FVector(-ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, +ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, 0.0f),
            MarkerCentre + FVector(+ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, -ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, 0.0f),
            MarkerColor, ck_crowd_agent_draw_nav_status_processor::NavStatus_DurationOneFrame, ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerThickness);

        FString Label;
        if (bIsFailed)
        {
            const auto ReasonName = StaticEnum<ECk_Nav_PathFailReason>()->GetNameStringByValue(
                static_cast<int64>(InPathResult.Get_Diagnostics().Get_LastFailReason()));
            Label = FString::Printf(TEXT("NO PATH: %s"), *ReasonName);
        }
        else
        {
            Label = TEXT("PENDING: waiting for navmesh");
        }

        DrawDebugString(
            World,
            MarkerCentre + FVector(0.0f, 0.0f, 40.0f),
            Label,
            /*TestBaseActor*/ nullptr,
            LabelColor,
            ck_crowd_agent_draw_nav_status_processor::NavStatus_DurationOneFrame,
            /*bDrawShadow*/ true,
            ck_crowd_agent_draw_nav_status_processor::NavStatus_LabelFontScale);
    }
}

// --------------------------------------------------------------------------------------------------------------------
