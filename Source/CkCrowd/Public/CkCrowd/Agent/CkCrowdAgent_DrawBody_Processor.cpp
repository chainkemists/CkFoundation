#include "CkCrowdAgent_DrawBody_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPmg/CkPmg_Fragment_Data_DebugShapes.h"
#include "CkPmg/CkPmg_Utils_DebugShapes.h"
#include "CkPmg/CkPmg_Utils_BasicShapes.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawBody_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawBody_Update);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DrawBody_Setup"), STAT_CkCrowd_DrawBody_SetupProc, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::DrawBody_Update"), STAT_CkCrowd_DrawBody_UpdateProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_draw_body_processor
{
    constexpr auto PathPending_BlendT      = 0.55f;
    const auto     PathPending_BlendColor   = FLinearColor(1.0f, 0.92f, 0.20f, 1.0f);

    constexpr auto Asleep_DesaturateT      = 0.65f;
    const auto     Asleep_BlendColor        = FLinearColor(0.45f, 0.45f, 0.45f, 1.0f);

    constexpr auto Cone_RadiusFraction      = 0.36f;
    constexpr auto Cone_LengthFraction      = 0.625f;  // of HalfHeight
    constexpr auto Cone_ForwardLiftFraction = 1.4f;    // of Radius — apex sits forward of body
    constexpr auto Cone_Segments            = 12;
    constexpr auto Capsule_Segments         = 16;
    constexpr auto Capsule_Rings            = 8;
    constexpr auto LineThickness            = 2.0f;
    constexpr auto Cone_LineThickness       = 1.5f;

    // PMG Duration sentinel: -1 = persist until the parent dies; 0 = single-tick.
    constexpr auto Persist                  = -1.0f;

    // PathPending wins over Asleep: pending is transient and actionable, sleep is steady-state.
    auto ResolveTintedColor(
        const FCk_Handle_CrowdAgent& InAgent,
        const FLinearColor& InBaseColor) -> FLinearColor
    {
        if (InAgent.Has<ck::FTag_CrowdAgent_PathPending>())
        { return FMath::Lerp(InBaseColor, PathPending_BlendColor, PathPending_BlendT); }
        if (InAgent.Has<ck::FTag_CrowdAgent_Asleep>())
        { return FMath::Lerp(InBaseColor, Asleep_BlendColor, Asleep_DesaturateT); }
        return InBaseColor;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawBody_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawBody_SetupProc);

        auto AgentTransform = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(AgentTransform))
        { return; }

        const auto Radius     = InParams.Get_Radius();
        const auto HalfHeight = InParams.Get_Height() * 0.5f;

        if (Radius <= 0.0f || HalfHeight <= 0.0f)
        { return; }

        // Spawning with the resolved color avoids one frame of flicker before the first Update.
        const auto BaseColor = UCk_Utils_CrowdAgent_UE::Get_DebugColor(InHandle);

        // ---- Body capsule ----------------------------------------------------------
        // The SceneNode local offset lifts the capsule centre by HalfHeight so its bottom rests
        // at the agent's feet (bottom-anchor convention).
        auto AgentGeneric = static_cast<FCk_Handle>(InHandle);
        auto CapsuleHandle = UCk_Utils_Pmg_BasicShapes::Create_Capsule(
            AgentGeneric,
            FTransform::Identity,
            Radius,
            HalfHeight,
            ck_crowd_agent_draw_body_processor::Capsule_Segments,
            ck_crowd_agent_draw_body_processor::Capsule_Rings,
            ECk_Plane_Axis::XY,
            BaseColor,
            true,             // draw wireframe overlay too
            ck_crowd_agent_draw_body_processor::LineThickness,
            ck_crowd_agent_draw_body_processor::Persist);

        auto CapsuleGeneric = static_cast<FCk_Handle>(CapsuleHandle);
        auto CapsuleTransform = UCk_Utils_Transform_UE::Cast(CapsuleGeneric);
        const auto CapsuleLocalOffset = FTransform(
            FRotator::ZeroRotator,
            FVector(0.0f, 0.0f, HalfHeight),
            FVector::OneVector);
        UCk_Utils_SceneNode_UE::Add(CapsuleTransform, AgentTransform, CapsuleLocalOffset);

        // ---- Forward-facing cone --------------------------------------------------
        const auto ConeRadius        = Radius * ck_crowd_agent_draw_body_processor::Cone_RadiusFraction;
        const auto ConeLength        = HalfHeight * ck_crowd_agent_draw_body_processor::Cone_LengthFraction;
        const auto ConeForwardOffset = Radius * ck_crowd_agent_draw_body_processor::Cone_ForwardLiftFraction;

        auto ConeHandle = UCk_Utils_Pmg_BasicShapes::Create_Cone(
            AgentGeneric,
            FTransform::Identity,
            ConeRadius,
            ConeLength,
            ck_crowd_agent_draw_body_processor::Cone_Segments,
            ECk_Plane_Axis::XY,
            BaseColor,
            true,
            ck_crowd_agent_draw_body_processor::Cone_LineThickness,
            ck_crowd_agent_draw_body_processor::Persist,
            ECk_Pmg_ConeOrientation::Forward);

        auto ConeGeneric = static_cast<FCk_Handle>(ConeHandle);
        auto ConeTransform = UCk_Utils_Transform_UE::Cast(ConeGeneric);
        const auto ConeLocalOffset = FTransform(
            FRotator::ZeroRotator,
            FVector(ConeForwardOffset, 0.0f, HalfHeight),
            FVector::OneVector);
        UCk_Utils_SceneNode_UE::Add(ConeTransform, AgentTransform, ConeLocalOffset);

        // ---- Stamp fragment + setup tag ------------------------------------------
        // Handle copies alias the same registry state — the copy only buys a mutable handle.
        auto AgentMutable = InHandle;
        auto& DebugBody = AgentMutable.AddOrGet<FFragment_CrowdAgent_DebugBody>();
        DebugBody._CapsuleHandle     = CapsuleHandle;
        DebugBody._ConeHandle        = ConeHandle;
        DebugBody._LastAppliedColor  = BaseColor;

        AgentMutable.Add<FTag_CrowdAgent_DebugBody_Setup>();

        // One Request_SetVisible covers the procmesh AND its wireframe overlay — the DrawLines
        // processor honours RenderMode==Hidden.
        const auto WantVisible = UCk_Utils_Crowd_DebugSettings_UE::Get_DrawAgentBody();
        UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(CapsuleHandle, WantVisible, {});
        UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(ConeHandle,    WantVisible, {});
        DebugBody._LastAppliedVisible = WantVisible;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_DrawBody_Update::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        const auto CurrentToggleOn = UCk_Utils_Crowd_DebugSettings_UE::Get_DrawAgentBody();

        // The on→off transition still iterates one final time below to flip every shape Hidden.
        if (NOT CurrentToggleOn && NOT _LastTickToggleOn)
        { return; }

        _LastTickToggleOn = CurrentToggleOn;
        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_CrowdAgent_DrawBody_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAgent_DebugBody& InDebugBody) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawBody_UpdateProc);

        const auto WantVisible = UCk_Utils_Crowd_DebugSettings_UE::Get_DrawAgentBody();

        if (WantVisible != InDebugBody._LastAppliedVisible)
        {
            UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(InDebugBody._CapsuleHandle, WantVisible, {});
            UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(InDebugBody._ConeHandle,    WantVisible, {});
            InDebugBody._LastAppliedVisible = WantVisible;
        }

        if (NOT WantVisible)
        { return; }

        const auto BaseColor    = UCk_Utils_CrowdAgent_UE::Get_DebugColor(InHandle);
        const auto TintedColor  = ck_crowd_agent_draw_body_processor::ResolveTintedColor(InHandle, BaseColor);

        if (TintedColor != InDebugBody._LastAppliedColor)
        {
            UCk_Utils_Pmg_DebugShape_UE::Request_SetColor(
                InDebugBody._CapsuleHandle, FCk_Request_Pmg_DebugShape_SetColor{TintedColor}, {});
            UCk_Utils_Pmg_DebugShape_UE::Request_SetColor(
                InDebugBody._ConeHandle,    FCk_Request_Pmg_DebugShape_SetColor{TintedColor}, {});
            InDebugBody._LastAppliedColor = TintedColor;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
