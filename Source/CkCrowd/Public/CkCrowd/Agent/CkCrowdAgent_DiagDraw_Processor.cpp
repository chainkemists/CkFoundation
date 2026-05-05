#include "CkCrowdAgent_DiagDraw_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagDrawBreadcrumb);

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // Global toggle: when on, draws breadcrumbs for every recorder-tracked agent. Default off
    // so the viewport stays clean by default; a debugger checkbox or `ck.Crowd.DrawBreadcrumbs 1`
    // turns it on. The selected agent (set via ck.Crowd.SelectedEntityId by the debugger) draws
    // regardless of this CVar — clicking an agent always shows its breadcrumb.
    static TAutoConsoleVariable<int32> CVarDrawBreadcrumbs(
        TEXT("ck.Crowd.DrawBreadcrumbs"),
        0,
        TEXT("Draw breadcrumb trails for every recorder-tracked crowd agent.\n")
        TEXT("  0 = off (default — but the debugger-selected agent always draws)\n")
        TEXT("  1 = on — every tracked agent's path renders at agent body height"),
        ECVF_Cheat);

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
        const auto bDrawAll = CVarDrawBreadcrumbs.GetValueOnGameThread() != 0;
        const auto SelectedHash = CVarSelectedEntityId.GetValueOnGameThread();
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
        const auto Thickness = bIsSelected ? Breadcrumb_Thickness_Selected : Breadcrumb_Thickness;
        const auto Lift = FVector(0.0f, 0.0f, BreadcrumbLiftZ);

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
                Breadcrumb_DurationOneFrame,
                Thickness);
            Prev = Curr;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
