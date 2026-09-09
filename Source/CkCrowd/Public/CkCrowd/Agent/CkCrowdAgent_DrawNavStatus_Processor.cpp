#include "CkCrowdAgent_DrawNavStatus_Processor.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment.h"

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Diagnostics/CkDiagnosticVisibility.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "Components/LineBatchComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

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
    constexpr auto Goal_LiftZ                  = 96.0f;
    constexpr auto Goal_DashSize               = 20.0f;
    constexpr auto Goal_MaxDashCount           = 64.0f;
    constexpr auto Goal_LineThickness          = 3.0f;
    constexpr auto Goal_MarkerSize             = 24.0f;
    constexpr auto Goal_MarkerPoints           = 5;
    constexpr auto Goal_LabelFontScale         = 1.2f;

    const auto NavStatus_FailedColor  = FLinearColor(1.0f, 0.10f, 0.10f, 1.0f);
    const auto NavStatus_PendingColor = FLinearColor(1.0f, 0.85f, 0.20f, 1.0f);
    const auto Sidewalk_FailedColor   = FLinearColor(1.0f, 0.35f, 0.05f, 1.0f);
    const auto Both_FailedColor       = FLinearColor(1.0f, 0.05f, 0.60f, 1.0f);

    auto
    Get_AreDrawDebugHelpersEnabled() -> bool
    {
#if ENABLE_DRAW_DEBUG
        static auto* const EnableDrawDebugHelpers = IConsoleManager::Get().FindConsoleVariable(
            TEXT("r.EnableDrawDebugHelpers"));
        return EnableDrawDebugHelpers && EnableDrawDebugHelpers->GetBool();
#else
        return false;
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawNavStatus::
        Build_GeometryLines(
            const FVector InMarkerCentre,
            const FVector InGoalLineStart,
            const FVector InGoalLineEnd,
            const float InGoalDashSize,
            const FLinearColor InMarkerColor,
            const float InLifetime,
            TArray<FBatchedLine>& OutLines) -> void
    {
        OutLines.Reset();

        const auto LineColor = FLinearColor{InMarkerColor.ToFColor(true)};
        const auto AddLine = [&OutLines, &LineColor, InLifetime](
            const FVector InStart,
            const FVector InEnd,
            const float InThickness)
        {
            OutLines.Emplace(InStart, InEnd, LineColor, InLifetime, InThickness, SDPG_World);
        };

        AddLine(
            InMarkerCentre + FVector(-ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, -ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, 0.0f),
            InMarkerCentre + FVector(+ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, +ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, 0.0f),
            ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerThickness);
        AddLine(
            InMarkerCentre + FVector(-ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, +ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, 0.0f),
            InMarkerCentre + FVector(+ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, -ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHalfSize, 0.0f),
            ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerThickness);

        const auto LineVector = InGoalLineEnd - InGoalLineStart;
        const auto LineLength = LineVector.Size();
        if (NOT FMath::IsNearlyZero(LineLength))
        {
            const auto Direction = LineVector / LineLength;
            const auto NumDashes = FMath::Max(1, FMath::FloorToInt(LineLength / (InGoalDashSize * 2.0f)));
            const auto ActualDashSize = LineLength / (NumDashes * 2.0f);
            for (int32 DashIndex = 0; DashIndex < NumDashes; ++DashIndex)
            {
                const auto DashStart = InGoalLineStart + Direction * (DashIndex * 2.0f * ActualDashSize);
                const auto DashEnd = DashStart + Direction * ActualDashSize;
                AddLine(DashStart, DashEnd, ck_crowd_agent_draw_nav_status_processor::Goal_LineThickness);
            }
        }

        const auto OuterRadius = ck_crowd_agent_draw_nav_status_processor::Goal_MarkerSize;
        const auto InnerRadius = OuterRadius * 0.4f;
        const auto AngleStep = 2.0f * PI / ck_crowd_agent_draw_nav_status_processor::Goal_MarkerPoints;
        auto Vertices = TArray<FVector>{};
        Vertices.Reserve(ck_crowd_agent_draw_nav_status_processor::Goal_MarkerPoints * 2);
        for (int32 PointIndex = 0; PointIndex < ck_crowd_agent_draw_nav_status_processor::Goal_MarkerPoints; ++PointIndex)
        {
            const auto Angle = PointIndex * AngleStep - PI * 0.5f;
            Vertices.Add(InGoalLineEnd + FVector(
                FMath::Cos(Angle) * OuterRadius,
                FMath::Sin(Angle) * OuterRadius,
                0.0f));

            const auto InnerAngle = Angle + AngleStep * 0.5f;
            Vertices.Add(InGoalLineEnd + FVector(
                FMath::Cos(InnerAngle) * InnerRadius,
                FMath::Sin(InnerAngle) * InnerRadius,
                0.0f));
        }
        for (int32 VertexIndex = 0; VertexIndex < Vertices.Num(); ++VertexIndex)
        {
            AddLine(
                Vertices[VertexIndex],
                Vertices[(VertexIndex + 1) % Vertices.Num()],
                ck_crowd_agent_draw_nav_status_processor::Goal_LineThickness);
        }
    }

    auto
        FProcessor_CrowdAgent_DrawNavStatus::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_CrowdAgent_PathTrouble& InPathTrouble)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawNavStatusProc);

        if (ck::diagnostic_visibility::Is_HiddenForStreamerMode())
        { return; }

        if (NOT UCk_Utils_Crowd_DebugSettings_UE::Get_DrawPathTrouble())
        { return; }

        const auto Status = InPathResult.Get_Status();
        const auto IsPending = Status == ECk_Nav_PathStatus::Pending;
        const auto EventAgeSeconds = InPathTrouble.Get_HasEvent()
            ? FPlatformTime::Seconds() - InPathTrouble.Get_EventTimeSeconds()
            : TNumericLimits<double>::Max();
        const auto FadeAlpha = static_cast<float>(FMath::Clamp(
            1.0 - EventAgeSeconds / FFragment_CrowdAgent_PathTrouble::FadeDurationSeconds,
            0.0,
            1.0));
        const auto HasVisibleEvent = InPathTrouble.Get_HasEvent() && FadeAlpha > 0.0f;
        if (NOT IsPending && NOT HasVisibleEvent)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        // A terminal result can immediately move the agent back into Walking/Idle. Use the frozen
        // attempt endpoints while fading so the evidence does not slide away with the character.
        const auto UseRetainedEndpoints = NOT IsPending && InPathTrouble.Get_HasEvent();
        const auto UseRetainedClassification = InPathTrouble.Get_HasEvent()
            && (NOT IsPending || InPathTrouble.Get_NavigationStatus() == ECk_Nav_PathStatus::Pending);
        const auto Pos = UseRetainedEndpoints
            ? InPathTrouble.Get_AgentLocation()
            : InTransform.Get_Transform().GetLocation();
        const auto MarkerCentre = Pos + FVector(0.0f, 0.0f, ck_crowd_agent_draw_nav_status_processor::NavStatus_MarkerHeightAbove);

        const auto NavigationStatus = UseRetainedClassification
            ? InPathTrouble.Get_NavigationStatus()
            : Status;
        const auto HasNavigationTrouble =
            NavigationStatus == ECk_Nav_PathStatus::Partial
            || NavigationStatus == ECk_Nav_PathStatus::Failed;
        auto BaseColor = IsPending
            ? ck_crowd_agent_draw_nav_status_processor::NavStatus_PendingColor
            : ck_crowd_agent_draw_nav_status_processor::NavStatus_FailedColor;
        if (UseRetainedClassification && InPathTrouble.Get_HadPathNetworkFailure())
        {
            BaseColor = HasNavigationTrouble
                ? ck_crowd_agent_draw_nav_status_processor::Both_FailedColor
                : ck_crowd_agent_draw_nav_status_processor::Sidewalk_FailedColor;
        }

        const auto Alpha = IsPending ? 1.0f : FadeAlpha;
        auto MarkerColor = BaseColor;
        MarkerColor.A = Alpha;
        const auto LabelColor = MarkerColor.ToFColor(true);
        const auto Goal = UseRetainedEndpoints
            ? InPathTrouble.Get_GoalLocation()
            : InPathFollow.Get_ActiveGoal();
        const auto GoalLift      = FVector(0.0f, 0.0f, ck_crowd_agent_draw_nav_status_processor::Goal_LiftZ);
        const auto GoalLineStart = Pos + GoalLift;
        const auto GoalLineEnd   = Goal + GoalLift;
        const auto GoalDistanceCm = FVector::Dist(Pos, Goal);
        const auto GoalDashSize = FMath::Max(
            ck_crowd_agent_draw_nav_status_processor::Goal_DashSize,
            GoalDistanceCm / (2.0f * ck_crowd_agent_draw_nav_status_processor::Goal_MaxDashCount));

        {
            TRACE_CPUPROFILER_EVENT_SCOPE(CkCrowd_DrawNavStatus_Geometry);

#if ENABLE_DRAW_DEBUG
            if (GEngine->GetNetMode(World) == NM_DedicatedServer)
            {
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
                UCk_Utils_DebugDraw_UE::DrawDebugDashedLine(
                    World,
                    GoalLineStart,
                    GoalLineEnd,
                    GoalDashSize,
                    MarkerColor,
                    ck_crowd_agent_draw_nav_status_processor::NavStatus_DurationOneFrame,
                    ck_crowd_agent_draw_nav_status_processor::Goal_LineThickness);
                UCk_Utils_DebugDraw_UE::DrawDebugStar(
                    World,
                    GoalLineEnd,
                    ck_crowd_agent_draw_nav_status_processor::Goal_MarkerSize,
                    ck_crowd_agent_draw_nav_status_processor::Goal_MarkerPoints,
                    MarkerColor,
                    ck_crowd_agent_draw_nav_status_processor::NavStatus_DurationOneFrame,
                    ck_crowd_agent_draw_nav_status_processor::Goal_LineThickness);
            }
            else if (ck_crowd_agent_draw_nav_status_processor::Get_AreDrawDebugHelpersEnabled())
            {
                if (auto* const LineBatcher = World->GetLineBatcher(UWorld::ELineBatcherType::World))
                {
                    auto Lines = TArray<FBatchedLine>{};
                    Lines.Reserve(2 + static_cast<int32>(ck_crowd_agent_draw_nav_status_processor::Goal_MaxDashCount)
                        + ck_crowd_agent_draw_nav_status_processor::Goal_MarkerPoints * 2);
                    Build_GeometryLines(
                        MarkerCentre,
                        GoalLineStart,
                        GoalLineEnd,
                        GoalDashSize,
                        MarkerColor,
                        LineBatcher->DefaultLifeTime,
                        Lines);
                    LineBatcher->DrawLines(Lines);
                }
            }
#endif
        }

        {
            TRACE_CPUPROFILER_EVENT_SCOPE(CkCrowd_DrawNavStatus_GoalText);
            DrawDebugString(
                World,
                FMath::Lerp(GoalLineStart, GoalLineEnd, 0.5f) + FVector(0.0f, 0.0f, 16.0f),
                FString::Printf(TEXT("%.0f cm (3D)"), GoalDistanceCm),
                /*TestBaseActor*/ nullptr,
                LabelColor,
                ck_crowd_agent_draw_nav_status_processor::NavStatus_DurationOneFrame,
                /*bDrawShadow*/ true,
                ck_crowd_agent_draw_nav_status_processor::Goal_LabelFontScale);
        }

        {
            TRACE_CPUPROFILER_EVENT_SCOPE(CkCrowd_DrawNavStatus_StatusText);
            auto Label = FString{};
            if (UseRetainedClassification && InPathTrouble.Get_HadPathNetworkFailure())
            {
                const auto SidewalkReason = StaticEnum<ECk_PathNetwork_RouteFailReason>()->GetNameStringByValue(
                    static_cast<int64>(InPathTrouble.Get_PathNetworkFailReason()));
                const auto NavigationStatusName = StaticEnum<ECk_Nav_PathStatus>()->GetNameStringByValue(
                    static_cast<int64>(NavigationStatus));
                Label = FString::Printf(
                    TEXT("SIDEWALK: %s -> UNREAL NAV: %s"),
                    *SidewalkReason,
                    *NavigationStatusName);
            }
            else if (IsPending)
            {
                // MarkPathPending is provider-independent — every backend parks this one slot — so
                // naming CkNavigation here would report a stalled sidewalk or volumetric query as an
                // Unreal-navmesh problem and send the reader to the wrong layer.
                switch (InPathFollow.Get_ActiveProvider())
                {
                    case ECk_CrowdAgent_PathProvider::PathNetwork:
                    {
                        Label = TEXT("SIDEWALK: Pending");
                        break;
                    }
                    case ECk_CrowdAgent_PathProvider::VoxelNav:
                    {
                        Label = TEXT("VOXEL NAV: Pending");
                        break;
                    }
                    case ECk_CrowdAgent_PathProvider::GroundNav:
                    {
                        Label = TEXT("GROUND NAV: Pending");
                        break;
                    }
                    case ECk_CrowdAgent_PathProvider::Navigation:
                    case ECk_CrowdAgent_PathProvider::None:
                    default:
                    {
                        Label = TEXT("UNREAL NAV: Pending");
                        break;
                    }
                }
            }
            else
            {
                const auto NavigationStatusName = StaticEnum<ECk_Nav_PathStatus>()->GetNameStringByValue(
                    static_cast<int64>(NavigationStatus));
                const auto NavigationReason = StaticEnum<ECk_Nav_PathFailReason>()->GetNameStringByValue(
                    static_cast<int64>(InPathTrouble.Get_NavigationFailReason()));
                Label = InPathTrouble.Get_NavigationFailReason() == ECk_Nav_PathFailReason::None
                    ? FString::Printf(TEXT("UNREAL NAV: %s"), *NavigationStatusName)
                    : FString::Printf(TEXT("UNREAL NAV: %s (%s)"), *NavigationStatusName, *NavigationReason);
            }

            // Read off the handle rather than the view: a fourth TReadOnly<FFragment_GroundNavPath_Result>
            // would narrow this processor to agents carrying the GroundNav feature and silently stop
            // drawing path trouble for every other agent. The routes themselves are
            // FProcessor_CrowdAgent_DrawShadowRoutes' business; this is only the token that says one exists.
            if (InHandle.Has<FFragment_GroundNavPath_Result>())
            {
                const auto& ShadowSlot = InHandle.Get<FFragment_GroundNavPath_Result>().Get_Result();
                if (ShadowSlot.Get_IsShadow() == ECk_EnableDisable::Enable
                    && ShadowSlot.Get_RequestRevision() == InPathResult.Get_RequestRevision())
                {
                    Label += FString::Printf(
                        TEXT(" | +SHADOW Δwp=%+d"),
                        ShadowSlot.Get_Waypoints().Num() - InPathResult.Get_Waypoints().Num());
                }
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
}

// --------------------------------------------------------------------------------------------------------------------
