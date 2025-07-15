#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkProjectile/CkProjectile_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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
        FCk_Delegate_Projectile_OnAimAheadCalculated_MC, ECk_SucceededFailed, FVector, FInstancedStruct);
}

// --------------------------------------------------------------------------------------------------------------------
