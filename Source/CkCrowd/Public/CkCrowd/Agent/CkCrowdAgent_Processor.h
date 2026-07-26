#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Waits on FFragment_Transform because the probe child is SceneNode-parented to it: an agent
    // that never gets a Transform never gets a probe, and therefore never separates.
    class CKCROWD_API FProcessor_CrowdAgent_Setup : public ck_exp::TProcessor<
        FProcessor_CrowdAgent_Setup,
        FCk_Handle_CrowdAgent,
        ck::TReadOnly<FFragment_CrowdAgent_Params>,
        ck::TReadOnly<FFragment_Transform>,
        ck::TReadWrite<FFragment_CrowdAgent_ProbeRef>,
        FTag_CrowdAgent_NeedsSetup,
        TExclude<FTag_CrowdAgent_HasProbe>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_CrowdAgent_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Transform& InTransform,
            FFragment_CrowdAgent_ProbeRef& InProbeRef) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
