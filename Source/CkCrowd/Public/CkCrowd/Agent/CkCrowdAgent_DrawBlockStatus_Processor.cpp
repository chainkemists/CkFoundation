#include "CkCrowdAgent_DrawBlockStatus_Processor.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Diagnostics/CkDiagnosticVisibility.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPmg/CkPmg_Fragment_Data_DebugShapes.h"
#include "CkPmg/CkPmg_Utils_DebugShapes.h"
#include "CkPmg/CkPmg_Utils_FlatShapes.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawBlockStatus_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawBlockStatus_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DrawBlockStatus_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DrawBlockStatus_Setup"),   STAT_CkCrowd_DrawBlockStatus_SetupProc,   STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::DrawBlockStatus_Update"),  STAT_CkCrowd_DrawBlockStatus_UpdateProc,  STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::DrawBlockStatus_EndPlay"), STAT_CkCrowd_DrawBlockStatus_EndPlayProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_draw_block_status_processor
{
    // PMG Duration sentinel: -1 = persist until explicitly destroyed / the parent dies.
    constexpr auto Persist              = -1.0f;

    constexpr auto Disc_RadiusPadCm     = 12.0f;
    constexpr auto Disc_Segments        = 24;
    constexpr auto Disc_LiftZ           = 3.0f;
    constexpr auto Disc_LineThickness   = 2.0f;
    constexpr auto Disc_FillOpacity     = 0.35f;

    constexpr auto Link_LiftZ           = 6.0f;
    constexpr auto Link_Thickness       = 3.0f;
    constexpr auto Link_DurationOneFrame = 0.0f;

    // Cause colours are deliberately far apart in hue: the point of the overlay is telling the
    // three apart at a glance in a packed crowd.
    const auto Block_OccupiedColor   = FLinearColor(1.00f, 0.45f, 0.05f, 1.0f);  // orange
    const auto Block_CrowdedColor    = FLinearColor(1.00f, 0.15f, 0.75f, 1.0f);  // magenta
    const auto Block_NoProgressColor = FLinearColor(1.00f, 0.10f, 0.10f, 1.0f);  // red

    auto Get_CauseColor(ECk_CrowdAgent_BlockedReason InCause) -> FLinearColor
    {
        switch (InCause)
        {
            case ECk_CrowdAgent_BlockedReason::GoalOccupied: return Block_OccupiedColor;
            case ECk_CrowdAgent_BlockedReason::GoalCrowded:  return Block_CrowdedColor;
            case ECk_CrowdAgent_BlockedReason::NoProgress:   return Block_NoProgressColor;
            default: return Block_NoProgressColor;
        }
    }

    auto Get_FillColor(ECk_CrowdAgent_BlockedReason InCause) -> FLinearColor
    {
        auto Color = Get_CauseColor(InCause);
        Color.A = Disc_FillOpacity;
        return Color;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DrawBlockStatus_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawBlockStatus_SetupProc);

        const auto Radius = InParams.Get_Radius();
        if (Radius <= 0.0f)
        { return; }

        const auto DiscRadius = Radius + ck_crowd_agent_draw_block_status_processor::Disc_RadiusPadCm;

        // ECk_Plane_Axis::XY is load-bearing and is NOT the default. Create_Circle defaults to YZ,
        // which lays the disc in a VERTICAL plane — the same default that made the first pass of
        // this overlay draw upright arcs beside each agent instead of a ring on the floor.
        auto AgentGeneric = static_cast<FCk_Handle>(InHandle);
        auto DiscHandle = UCk_Utils_Pmg_FlatShapes::Create_Circle(
            AgentGeneric,
            FTransform(FVector(0.0f, 0.0f, ck_crowd_agent_draw_block_status_processor::Disc_LiftZ)),
            DiscRadius,
            ck_crowd_agent_draw_block_status_processor::Disc_Segments,
            ck_crowd_agent_draw_block_status_processor::Get_FillColor(
                ECk_CrowdAgent_BlockedReason::GoalOccupied),
            true,   // outline the fill so the ring still reads against a same-hue floor
            ck_crowd_agent_draw_block_status_processor::Disc_LineThickness,
            false,  // no direction line — facing is DrawBody's job, not this overlay's
            ECk_Plane_Axis::XY,
            ck_crowd_agent_draw_block_status_processor::Persist);

        if (ck::Is_NOT_Valid(DiscHandle))
        { return; }

        // Starts hidden; Update owns visibility from the toggle and the live blocked state on the
        // very next tick, so there is exactly one authority for it.
        UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(DiscHandle, false, {});

        auto& Marker = InHandle.AddOrGet<FFragment_CrowdAgent_DebugBlockMarker>();
        Marker._DiscHandle = DiscHandle;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_DrawBlockStatus_Update::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        const auto CurrentToggleOn = UCk_Utils_Crowd_DebugSettings_UE::Get_DrawBlockStatus();

        // The on -> off transition still iterates one final time below to hide every disc.
        if (NOT CurrentToggleOn && NOT _LastTickToggleOn)
        { return; }

        _LastTickToggleOn = CurrentToggleOn;
        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_CrowdAgent_DrawBlockStatus_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DebugBlockMarker& InMarker) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawBlockStatus_UpdateProc);

        const auto IsBlocked = InHandle.Has<FTag_CrowdAgent_GoalBlocked>();
        const auto WantVisible = IsBlocked
            && UCk_Utils_Crowd_DebugSettings_UE::Get_DrawBlockStatus()
            && NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode();

        if (WantVisible != InMarker._LastAppliedVisible)
        {
            UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(InMarker._DiscHandle, WantVisible, {});
            InMarker._LastAppliedVisible = WantVisible;
        }

        if (NOT WantVisible)
        { return; }

        const auto Cause = InBlockDetect.Get_BlockedCause();
        if (Cause != InMarker._LastAppliedCause)
        {
            const auto FillColor = ck_crowd_agent_draw_block_status_processor::Get_FillColor(Cause);
            UCk_Utils_Pmg_DebugShape_UE::Request_SetColor(
                InMarker._DiscHandle, FCk_Request_Pmg_DebugShape_SetColor{FillColor}, {});
            InMarker._LastAppliedCause = Cause;
            InMarker._LastAppliedColor = FillColor;
        }

        // The blocker link — the one thing neither the disc nor the entity overlay can express. A
        // CHAIN of held agents each pointing at the next IS a head-of-line deadlock, and that is
        // unreadable from any single agent's state. NoProgress names no blocker (the obstruction is
        // static geometry), so the handle is simply invalid there and no link is drawn.
        const auto BlockedBy = InBlockDetect.Get_BlockedBy();
        if (ck::Is_NOT_Valid(BlockedBy))
        { return; }

        const auto BlockerTransform = UCk_Utils_Transform_UE::Cast(BlockedBy);
        if (ck::Is_NOT_Valid(BlockerTransform))
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        const auto LinkLift = FVector(0.0f, 0.0f, ck_crowd_agent_draw_block_status_processor::Link_LiftZ);
        UCk_Utils_DebugDraw_UE::DrawDebugLine(
            World,
            InTransform.Get_Transform().GetLocation() + LinkLift,
            UCk_Utils_Transform_UE::Get_EntityCurrentLocation(BlockerTransform) + LinkLift,
            ck_crowd_agent_draw_block_status_processor::Get_CauseColor(Cause),
            ck_crowd_agent_draw_block_status_processor::Link_DurationOneFrame,
            ck_crowd_agent_draw_block_status_processor::Link_Thickness);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_DrawBlockStatus_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAgent_DebugBlockMarker& InMarker) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DrawBlockStatus_EndPlayProc);

        // A PMG shape is an ENTITY. Dropping the handle does not reclaim it.
        if (ck::Is_NOT_Valid(InMarker._DiscHandle))
        { return; }

        auto DiscGeneric = static_cast<FCk_Handle>(InMarker._DiscHandle);
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(DiscGeneric);
        InMarker._DiscHandle = {};
    }
}

// --------------------------------------------------------------------------------------------------------------------
