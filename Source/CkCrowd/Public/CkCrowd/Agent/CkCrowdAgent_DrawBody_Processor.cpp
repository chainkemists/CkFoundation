#include "CkCrowdAgent_DrawBody_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPmg/CkPmg_Fragment_Data.h"
#include "CkPmg/CkPmg_Utils.h"
#include "CkPmg/CkPmg_Utils_BasicShapes.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawBody_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawBody_Update);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DrawBody_Setup"), STAT_CkCrowd_DrawBody_SetupProc, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::DrawBody_Update"), STAT_CkCrowd_DrawBody_UpdateProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // PathPending blend — visually communicates "agent is waiting on a path query, can't move
    // yet". Lerp factor picked for legibility against varied background colors.
    constexpr auto PathPending_BlendT      = 0.55f;
    const auto     PathPending_BlendColor   = FLinearColor(1.0f, 0.92f, 0.20f, 1.0f);

    // Asleep desaturate — agent is excluded from steering / separation; visually distinguish
    // sleeping agents from active ones without hiding them entirely.
    constexpr auto Asleep_DesaturateT      = 0.65f;
    const auto     Asleep_BlendColor        = FLinearColor(0.45f, 0.45f, 0.45f, 1.0f);

    // Visual sizing — derived from agent radius/height at setup time so any agent sizing
    // flows through naturally. Same proportions as the inline gym viz that this replaces.
    constexpr auto Cone_RadiusFraction      = 0.36f;
    constexpr auto Cone_LengthFraction      = 0.625f;  // of HalfHeight
    constexpr auto Cone_ForwardLiftFraction = 1.4f;    // of Radius — apex sits forward of body
    constexpr auto Cone_Segments            = 12;
    constexpr auto Capsule_Segments         = 16;
    constexpr auto Capsule_Rings            = 8;
    constexpr auto LineThickness            = 2.0f;
    constexpr auto Cone_LineThickness       = 1.5f;

    // Persistent: live until the agent (parent) is destroyed. PMG's Duration sentinel is
    // -1 = persist; 0 = single-tick (the gotcha — see CkPmg/CLAUDE.md).
    constexpr auto Persist                  = -1.0f;

    // Color modulation per state. PathPending wins over Asleep visually — pending is transient
    // and important; sleep is steady-state.
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

        // Initial color from the agent's debug-color (per-agent override if Set_DebugColor was
        // called, hash-derived fallback otherwise). The Update processor refreshes this each
        // tick if state tags change, but spawning with the right color avoids one frame of
        // flicker between Setup and the first Update.
        const auto BaseColor = UCk_Utils_CrowdAgent_UE::Get_DebugColor(InHandle);

        // ---- Body capsule ----------------------------------------------------------
        // Local transform identity — the SceneNode parent applies the agent's world position;
        // local offset on the SceneNode lifts the capsule center up by HalfHeight so its
        // bottom rests at the agent's feet (bottom-anchor convention).
        auto AgentGeneric = static_cast<FCk_Handle>(InHandle);
        auto CapsuleHandle = UCk_Utils_Pmg_BasicShapes::Create_Capsule(
            AgentGeneric,
            FTransform::Identity,
            Radius,
            HalfHeight,
            Capsule_Segments,
            Capsule_Rings,
            ECk_Plane_Axis::XY,
            BaseColor,
            true,             // draw wireframe overlay too
            LineThickness,
            Persist);

        auto CapsuleGeneric = static_cast<FCk_Handle>(CapsuleHandle);
        auto CapsuleTransform = UCk_Utils_Transform_UE::Cast(CapsuleGeneric);
        const auto CapsuleLocalOffset = FTransform(
            FRotator::ZeroRotator,
            FVector(0.0f, 0.0f, HalfHeight),
            FVector::OneVector);
        UCk_Utils_SceneNode_UE::Add(CapsuleTransform, AgentTransform, CapsuleLocalOffset);

        // ---- Forward-facing cone --------------------------------------------------
        // Use ECk_Pmg_ConeOrientation::Forward so the apex is baked along +X (agent's forward).
        // No more Pitch=-90 SceneNode workaround needed — the orientation is in the mesh.
        const auto ConeRadius        = Radius * Cone_RadiusFraction;
        const auto ConeLength        = HalfHeight * Cone_LengthFraction;
        const auto ConeForwardOffset = Radius * Cone_ForwardLiftFraction;

        auto ConeHandle = UCk_Utils_Pmg_BasicShapes::Create_Cone(
            AgentGeneric,
            FTransform::Identity,
            ConeRadius,
            ConeLength,
            Cone_Segments,
            ECk_Plane_Axis::XY,
            BaseColor,
            true,
            Cone_LineThickness,
            Persist,
            ECk_Pmg_ConeOrientation::Forward);

        auto ConeGeneric = static_cast<FCk_Handle>(ConeHandle);
        auto ConeTransform = UCk_Utils_Transform_UE::Cast(ConeGeneric);
        const auto ConeLocalOffset = FTransform(
            FRotator::ZeroRotator,
            FVector(ConeForwardOffset, 0.0f, HalfHeight),
            FVector::OneVector);
        UCk_Utils_SceneNode_UE::Add(ConeTransform, AgentTransform, ConeLocalOffset);

        // ---- Stamp fragment + setup tag ------------------------------------------
        // Copy the handle so we have a non-const reference; mutates the registry, not the
        // local copy. Same idiom as FProcessor_CrowdAgent_Setup uses.
        auto AgentMutable = InHandle;
        auto& DebugBody = AgentMutable.AddOrGet<FFragment_CrowdAgent_DebugBody>();
        DebugBody._CapsuleHandle     = CapsuleHandle;
        DebugBody._ConeHandle        = ConeHandle;
        DebugBody._LastAppliedColor  = BaseColor;

        AgentMutable.Add<FTag_CrowdAgent_DebugBody_Setup>();

        // First-tick visibility honors the current toggle. Single Request_SetVisible covers
        // both the procmesh and the wireframe — the DrawLines processor honors RenderMode==Hidden.
        const auto WantVisible = UCk_Utils_Crowd_DebugSettings_UE::Get_DrawAgentBody();
        UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(CapsuleHandle, WantVisible);
        UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(ConeHandle,    WantVisible);
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

        // Skip the view iteration entirely when the toggle was off both this tick and last —
        // there's nothing to update and shapes are already Hidden from the prior pass. The
        // on→off transition still iterates one final time below to flip every shape Hidden.
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

        // Visibility flip — only request when changing. The single Request_SetVisible call
        // covers both the procmesh AND the wireframe overlay.
        if (WantVisible != InDebugBody._LastAppliedVisible)
        {
            UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(InDebugBody._CapsuleHandle, WantVisible);
            UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(InDebugBody._ConeHandle,    WantVisible);
            InDebugBody._LastAppliedVisible = WantVisible;
        }

        // Color update — only when visible AND the resolved color actually changed. Avoids
        // spamming Set_Color requests every frame for steady-state agents.
        if (NOT WantVisible)
        { return; }

        const auto BaseColor    = UCk_Utils_CrowdAgent_UE::Get_DebugColor(InHandle);
        const auto TintedColor  = ResolveTintedColor(InHandle, BaseColor);

        if (TintedColor != InDebugBody._LastAppliedColor)
        {
            UCk_Utils_Pmg_DebugShape_UE::Request_SetColor(
                InDebugBody._CapsuleHandle, FCk_Request_Pmg_DebugShape_SetColor{TintedColor});
            UCk_Utils_Pmg_DebugShape_UE::Request_SetColor(
                InDebugBody._ConeHandle,    FCk_Request_Pmg_DebugShape_SetColor{TintedColor});
            InDebugBody._LastAppliedColor = TintedColor;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
