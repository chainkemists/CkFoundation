#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkProjectile/BallisticMotion/CkBallisticMotion_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_BallisticMotion_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Present while the projectile is in flight. Removed on Stop — the trajectory state remains
    // readable but the Update/Impact processors no longer visit the entity
    CK_DEFINE_ECS_TAG(FTag_BallisticMotion_Active);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_BallisticMotion_Params = FCk_Fragment_BallisticMotion_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROJECTILE_API FFragment_BallisticMotion_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_BallisticMotion_Current);

    public:
        friend class FProcessor_BallisticMotion_HandleRequests;
        friend class FProcessor_BallisticMotion_HandleImpacts;
        friend class FProcessor_BallisticMotion_UpdateTrajectory;
        friend class ::UCk_Utils_BallisticMotion_UE;

    private:
        FCk_Ballistic_InitialConditions _InitialConditions;
        FVector _CurrentVelocity = FVector::ZeroVector;
        int32 _TrajectorySegmentIndex = 0;

        // Entities whose overlap has already produced an impact response for as long as the
        // overlap persists — prevents a continuous contact from re-triggering every frame
        TSet<FCk_Handle> _HandledImpactEntities;

    public:
        CK_PROPERTY_GET(_InitialConditions);
        CK_PROPERTY_GET(_CurrentVelocity);
        CK_PROPERTY_GET(_TrajectorySegmentIndex);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROJECTILE_API FFragment_BallisticMotion_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_BallisticMotion_Requests);

    public:
        friend class FProcessor_BallisticMotion_HandleRequests;
        friend class ::UCk_Utils_BallisticMotion_UE;

    public:
        using LaunchRequestType = FCk_Request_BallisticMotion_Launch;
        using StopRequestType = FCk_Request_BallisticMotion_Stop;
        using RequestType = std::variant<LaunchRequestType, StopRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKPROJECTILE_API,
        BallisticMotion_OnImpact,
        FCk_Delegate_BallisticMotion_OnImpact,
        FCk_Handle_BallisticMotion,
        FCk_BallisticMotion_Payload_OnImpact);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKPROJECTILE_API,
        BallisticMotion_OnTrajectoryChanged,
        FCk_Delegate_BallisticMotion_OnTrajectoryChanged,
        FCk_Handle_BallisticMotion,
        FCk_Ballistic_InitialConditions,
        int32);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKPROJECTILE_API,
        BallisticMotion_OnStopped,
        FCk_Delegate_BallisticMotion_OnStopped,
        FCk_Handle_BallisticMotion,
        FVector);
}

// --------------------------------------------------------------------------------------------------------------------
