#include "CkProjectile_Processor.h"

#include "CkEcsBasics/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_Projectile_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Integrator_Current& InIntegratorComp) const
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
