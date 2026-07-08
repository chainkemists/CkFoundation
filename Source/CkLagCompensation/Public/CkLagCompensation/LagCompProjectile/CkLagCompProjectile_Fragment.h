#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkLagCompensation/LagCompProjectile/CkLagCompProjectile_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_LagCompProjectile_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKLAGCOMPENSATION_API FFragment_LagCompProjectile_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_LagCompProjectile_Requests);

    public:
        friend class FProcessor_LagCompProjectile_HandleRequests;
        friend class ::UCk_Utils_LagCompProjectile_UE;

    public:
        using LaunchCompensatedRequestType = FCk_Request_LagCompProjectile_LaunchCompensated;
        using RequestType = std::variant<LaunchCompensatedRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKLAGCOMPENSATION_API,
        LagCompProjectile_OnRewindHit,
        FCk_Delegate_LagCompProjectile_OnRewindHit,
        FCk_Handle_BallisticMotion,
        FCk_LagComp_RewindHit);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKLAGCOMPENSATION_API,
        LagCompProjectile_OnLaunchCompensated,
        FCk_Delegate_LagCompProjectile_OnLaunchCompensated,
        FCk_Handle_BallisticMotion,
        FCk_Ballistic_InitialConditions,
        TArray<FCk_LagComp_RewindHit>);
}

// --------------------------------------------------------------------------------------------------------------------
