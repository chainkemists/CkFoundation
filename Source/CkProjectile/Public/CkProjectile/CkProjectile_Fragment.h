#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkProjectile/CkProjectile_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Identifies an entity as a flying projectile so FProcessor_Projectile_Update only applies
    // the integrator's _DistanceOffset to actual projectile entities. Without this gate, the
    // Update processor would fire on every EulerIntegrator-bearing entity (e.g. CrowdAgents),
    // double-applying the per-frame offset.
    CK_DEFINE_ECS_TAG(FTag_Projectile);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROJECTILE_API FFragment_Projectile_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Projectile_Requests);

    public:
        friend class FProcessor_Projectile_HandleRequests;
        friend class UCk_Utils_Projectile_UE;

   public:
        using RequestType = FCk_Request_Projectile_CalculateAimAhead;

    private:
        RequestType _Request;

    public:
        CK_PROPERTY_GET(_Request);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPROJECTILE_API, Projectile_OnAimAheadCalculated,
        FCk_Delegate_Projectile_OnAimAheadCalculated, ECk_SucceededFailed, FVector, FCk_Time, FInstancedStruct);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Projectile_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
