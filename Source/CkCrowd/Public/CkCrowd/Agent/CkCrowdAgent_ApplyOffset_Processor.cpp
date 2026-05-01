#include "CkCrowdAgent_ApplyOffset_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_ApplyOffset);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_ApplyOffset::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EulerIntegrator_Current& InIntegrator)
        -> void
    {
        if (InIntegrator.Get_DistanceOffset().IsNearlyZero())
        { return; }

        auto HandleTransform = UCk_Utils_Transform_UE::CastChecked(InHandle);

        UCk_Utils_Transform_UE::Request_AddLocationOffset
        (
            HandleTransform,
            FCk_Request_Transform_AddLocationOffset{InIntegrator.Get_DistanceOffset()}
                .Set_LocalWorld(ECk_LocalWorld::World)
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
