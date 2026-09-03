#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_OnGroundNavPathResolved_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_OnPathResolved_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /**
     * The revision of the last shadow answer this agent has already been compared on.
     *
     * The GroundNav result slot cannot record that a consumer is done with it: _HasFreshResult is
     * cleared only by the episode that owns the slot, and this processor is not one of its friends.
     * A shadow result therefore stays fresh until the next episode parks over it, which would fold
     * the same comparison into the fixture on every frame in between. Keyed by revision rather than
     * by a bare flag so a second shadow episode on the same agent is still counted.
     */
    struct CKCROWD_API FFragment_CrowdAgent_ShadowCompared
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_ShadowCompared);

        friend class FProcessor_GroundNav_ShadowCompare;

    private:
        int32 _LastComparedRevision = INDEX_NONE;

    public:
        CK_PROPERTY_GET(_LastComparedRevision);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Folds a shadow GroundNav answer and the Recast answer it shadows into the world's shadow
     * diagnostics.
     *
     * Lives in CkCrowd because the agent is the only place both results exist side by side: the two
     * providers are separate modules and neither can see the other's slot. Reads both, writes
     * neither - the shadow answer is never installed, and nothing here touches the nav slot the
     * Recast result was installed into.
     */
    class CKCROWD_API FProcessor_GroundNav_ShadowCompare : public ck_exp::TProcessor<
            FProcessor_GroundNav_ShadowCompare,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadOnly<FFragment_GroundNavPath_Result>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        // Runs in the default group, after every gameplay processor has settled both slots for the
        // frame; a dependency edge inside the gameplay group would reorder that group's schedule
        // for agents that never carry a shadow result.
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_Nav_PathResult& InNavResult,
            const FFragment_GroundNavPath_Result& InGroundNavResult) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
