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

        UCk_Utils_Transform_UE::Request_AddLocationOffset
        (
            InHandle,
            FCk_Request_Transform_AddLocationOffset{ InIntegratorComp.Get_DistanceOffset(), ECk_LocalWorld::World }
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
