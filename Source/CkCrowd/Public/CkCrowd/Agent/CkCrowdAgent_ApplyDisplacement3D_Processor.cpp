#include "CkCrowdAgent_ApplyDisplacement3D_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_ApplyDisplacement3D);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::ApplyDisplacement3D"), STAT_CkCrowd_ApplyDisplacement3DProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_ApplyDisplacement3D::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAgent_PendingDisplacement& InPending)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_ApplyDisplacement3DProc);

        const auto Displacement = InPending.Get_Displacement();
        if (Displacement.IsNearlyZero())
        { return; }

        InPending._Displacement = FVector::ZeroVector;

        auto SelfTransform = UCk_Utils_Transform_UE::Cast(InHandle);

        UCk_Utils_Transform_UE::Request_AddLocationOffset(
            SelfTransform,
            FCk_Request_Transform_AddLocationOffset{Displacement}
                .Set_LocalWorld(ECk_LocalWorld::World), {});
    }
}

// --------------------------------------------------------------------------------------------------------------------
