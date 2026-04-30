#include "CkCrowdAgent_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams) const
        -> void
    {
        // Gate 0: structural setup only. Add() already attached the params fragment + tags.
        // Tag is consumed by view membership; we just log for traceability.
        ck::crowd::VeryVerbose(TEXT("CrowdAgent setup: [{}] (radius={}, height={})"),
            InHandle, InParams.Get_Radius(), InParams.Get_Height());

        // Subsequent gates expand this body:
        //   Gate 3: spawn child probe entity (Cylinder + Probe) for neighbor queries
        //   Gate 5: interpret PLAYER_PROXY flag if set
    }
}

// --------------------------------------------------------------------------------------------------------------------
