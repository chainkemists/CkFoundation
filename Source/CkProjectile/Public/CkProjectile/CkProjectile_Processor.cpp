#include "CkProjectile_Processor.h"

#include "CkEcsBasics/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Projectile_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EulerIntegrator_Current& InIntegratorComp) const
        -> void
    {
        if (InIntegratorComp.Get_DistanceOffset().IsNearlyZero())
        { return; }

        auto HandleTransform = UCk_Utils_Transform_UE::CastChecked(InHandle);

        UCk_Utils_Transform_UE::Request_AddLocationOffset
        (
            HandleTransform,
            FCk_Request_Transform_AddLocationOffset{InIntegratorComp.Get_DistanceOffset()}.Set_LocalWorld(ECk_LocalWorld::World)
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
