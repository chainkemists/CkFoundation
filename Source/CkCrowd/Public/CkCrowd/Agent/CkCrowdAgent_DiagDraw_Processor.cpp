#include "CkCrowdAgent_DiagDraw_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagDrawBreadcrumb);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DiagDrawBreadcrumb"), STAT_CkCrowd_DiagDrawBreadcrumbProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_diag_draw_processor
{
    // CVar `ck.Crowd.DrawBreadcrumbs` is now declared via UPROPERTY in
    // UCk_Crowd_DebugSettings_UE — read it through the settings BPFL so values persist
    // across editor sessions. The selected agent (CVarSelectedEntityId below) still draws
    // regardless of the global toggle.

    // Selected entity id: -1 means none. Written by the CkCrowdDebugger when an agent is
    // selected in the Agent List. Draw processors compare GetTypeHash(InHandle) against this
    // value and always render the matching agent's path regardless of the global CVar.
    static TAutoConsoleVariable<int32> CVarSelectedEntityId(
        TEXT("ck.Crowd.SelectedEntityId"),
        -1,
        TEXT("Hash id of the crowd agent currently selected in the Crowd Debugger. -1 = none.\n")
        TEXT("Draw processors render this agent regardless of the per-overlay CVars."),
        ECVF_Default);

    // Lift the polyline to body centre so it sits where the visible capsule went. Recorder data
    // itself stays as the unaltered transform position; this is purely visualisation. Matches the
    // capsule SceneNode offset the diag gym uses (HalfHeight=96 on the standard agent).
    constexpr auto BreadcrumbLiftZ = 96.0f;
    constexpr auto Breadcrumb_Thickness          = 3.0f;
    constexpr auto Breadcrumb_Thickness_Selected = 6.0f;
    constexpr auto Breadcrumb_DurationOneFrame            = 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DiagDrawBreadcrumb::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_DiagRecorder& InRecorder)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DiagDrawBreadcrumbProc);

        const auto bDrawAll = UCk_Utils_Crowd_DebugSettings_UE::Get_DrawBreadcrumbs();
        const auto SelectedHash = ck_crowd_agent_diag_draw_processor::CVarSelectedEntityId.GetValueOnGameThread();
        const auto bIsSelected = SelectedHash >= 0
            && static_cast<int32>(GetTypeHash(InHandle)) == SelectedHash;
        if (NOT bDrawAll && NOT bIsSelected)
        { return; }

        const auto& Samples = InRecorder.Get_Samples();
        if (Samples.Num() == 0)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        // Per-agent identity colour — coordinated with the visible capsule + the debugger swatch.
        // Get_DebugColor falls back to a hash-derived stable colour if no explicit Set_DebugColor
        // has been called (so untracked agents still get distinct colours).
        const auto BreadcrumbColor = UCk_Utils_CrowdAgent_UE::Get_DebugColor(InHandle);
        const auto Thickness = bIsSelected ? ck_crowd_agent_diag_draw_processor::Breadcrumb_Thickness_Selected : ck_crowd_agent_diag_draw_processor::Breadcrumb_Thickness;
        const auto Lift = FVector(0.0f, 0.0f, ck_crowd_agent_diag_draw_processor::BreadcrumbLiftZ);

        // First segment connects the recorded start position to sample 0 so the trail begins at
        // spawn rather than at wherever the first sample landed (recorder takes its first sample
        // a frame or so after Track()).
        auto Prev = InRecorder.Get_StartPos() + Lift;
        for (const auto& Sample : Samples)
        {
            const auto Curr = Sample.Get_Pos() + Lift;
            UCk_Utils_DebugDraw_UE::DrawDebugLine(
                World,
                Prev,
                Curr,
                BreadcrumbColor,
                ck_crowd_agent_diag_draw_processor::Breadcrumb_DurationOneFrame,
                Thickness);
            Prev = Curr;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
