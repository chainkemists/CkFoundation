#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Diag_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ECS-side alias matching the project's _Fragment / _Fragment_Data split convention.
    using FFragment_CrowdAgent_DiagRecorder = FCk_Fragment_CrowdAgent_DiagRecorderData;

    // Tags an agent for path recording. The gym (or any caller of UCk_Utils_CrowdAgent_Diag_UE::
    // Track) stamps this. The recorder processor's view requires it, so untagged agents pay zero
    // overhead — the recorder is opt-in per agent, not a cross-cutting tax.
    CK_DEFINE_ECS_TAG(FTag_CrowdDiag_Tracked);
}

// --------------------------------------------------------------------------------------------------------------------
