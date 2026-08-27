#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Diag_Fragment_Data.h"
#include "CkCrowd/Agent/CkCrowdAgent_DiagBreadcrumb_Algorithm.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkPmg/CkPmg_Fragment_Data_DebugShapes.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ECS-side alias matching the project's _Fragment / _Fragment_Data split convention.
    using FFragment_CrowdAgent_DiagRecorder = FCk_Fragment_CrowdAgent_DiagRecorderData;

    // Tags an agent for path recording. The gym (or any caller of UCk_Utils_CrowdAgent_Diag_UE::
    // Track) stamps this. The recorder processor's view requires it, so untagged agents pay zero
    // overhead — the recorder is opt-in per agent, not a cross-cutting tax.
    CK_DEFINE_ECS_TAG(FTag_CrowdDiag_Tracked);

    struct CKCROWD_API FFragment_CrowdAgent_DiagBreadcrumb
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_DiagBreadcrumb);

    public:
        friend class FProcessor_CrowdAgent_DiagDrawBreadcrumb;

    private:
        crowd_diag_breadcrumb::FHistoryState _History;
        TArray<FCk_Handle_Pmg_DebugShape> _Chunks;
        bool _HasAppliedVisibility = false;
        bool _LastAppliedVisibility = false;
        bool _LastAppliedSelection = false;
        FLinearColor _LastAppliedColor = FLinearColor::Transparent;
    };
}

// --------------------------------------------------------------------------------------------------------------------
