#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkProjectile/Homing/CkHoming_Fragment_Data.h"
#include "CkProjectile/Homing/CkHoming_ProNav.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Present while homing has a target and is enabled — gates FProcessor_Homing_Update
    CK_DEFINE_ECS_TAG(FTag_Homing_Active);

    // Present while the user has explicitly disabled homing via Request_EnableDisable
    CK_DEFINE_ECS_TAG(FTag_Homing_Disabled);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Homing_Params = FCk_Fragment_Homing_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROJECTILE_API FFragment_Homing_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Homing_Current);

    public:
        friend class FProcessor_Homing_HandleRequests;
        friend class FProcessor_Homing_Update;
        friend class UCk_Utils_Homing_UE;

    private:
        FCk_Handle _TargetEntity;
        FVector _WorldPoint = FVector::ZeroVector;
        FVector _LocalOffset = FVector::ZeroVector;

        ECk_Homing_TargetMode _TargetMode = ECk_Homing_TargetMode::None;

        FVector _BaseAcceleration = FVector::ZeroVector;

        // Finite-difference fallback for targets without a Velocity feature
        FVector _PreviousTargetLocation = FVector::ZeroVector;
        bool _HasPreviousTargetLocation = false;

        // Miss-detection latch: notified once per closing→receding flip inside the threshold
        bool _PreviouslyClosing = false;
        bool _MissNotified = false;

        ECk_Homing_ImpactTiming _ImpactTiming = ECk_Homing_ImpactTiming::ArriveAsap;
        FCk_Time _DesiredTimeToImpactRemaining;

        FCk_Homing_GuidanceState _LastGuidanceState;

    public:
        CK_PROPERTY_GET(_TargetEntity);
        CK_PROPERTY_GET(_TargetMode);
        CK_PROPERTY_GET(_LastGuidanceState);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROJECTILE_API FFragment_Homing_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Homing_Requests);

    public:
        friend class FProcessor_Homing_HandleRequests;
        friend class UCk_Utils_Homing_UE;

    public:
        using SetTargetEntityRequestType = FCk_Request_Homing_SetTargetEntity;
        using SetTargetLocationRequestType = FCk_Request_Homing_SetTargetLocation;
        using ClearTargetRequestType = FCk_Request_Homing_ClearTarget;
        using SetDesiredTimeToImpactRequestType = FCk_Request_Homing_SetDesiredTimeToImpact;
        using EnableDisableRequestType = FCk_Request_Homing_EnableDisable;

        using RequestType = std::variant<
            SetTargetEntityRequestType,
            SetTargetLocationRequestType,
            ClearTargetRequestType,
            SetDesiredTimeToImpactRequestType,
            EnableDisableRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPROJECTILE_API, Homing_OnTargetMissed,
        FCk_Delegate_Homing_OnTargetMissed, FCk_Handle_Homing, FCk_Homing_Payload_OnTargetMissed);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPROJECTILE_API, Homing_OnTargetLost,
        FCk_Delegate_Homing_OnTargetLost, FCk_Handle_Homing, FCk_Homing_Payload_OnTargetLost);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Homing_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
