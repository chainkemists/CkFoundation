#pragma once

#include "CkRotateTowards/CkRotateTowards_Fragment_Data.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_RotateTowards_UE;

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_RotateTowards_HasTarget);
    CK_DEFINE_ECS_TAG(FTag_RotateTowards_AtTarget);
    CK_DEFINE_ECS_TAG(FTag_RotateTowards_Disabled);

    // --------------------------------------------------------------------------------------------------------------------

    // Wholesale aliases (CkSway precedent). Tunables, not Params: Request_UpdateTunables replaces it at runtime.
    using FFragment_RotateTowards_Tunables = FCk_RotateTowards_Tunables;

    // Present only while at least one axis range is enabled (Add / Request_SetRangeClamp add it, Request_ClearRangeClamp
    // and a lost rest reference remove it).
    using FFragment_RotateTowards_RangeClamp = FCk_RotateTowards_RangeClamp;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKROTATETOWARDS_API FFragment_RotateTowards
    {
    public:
        CK_GENERATED_BODY(FFragment_RotateTowards);

    public:
        friend class FProcessor_RotateTowards_HandleRequests;
        friend class FProcessor_RotateTowards_Update;
        friend class UCk_Utils_RotateTowards_UE;

    private:
        FCk_Handle_Transform _Target;

    public:
        CK_PROPERTY_GET(_Target);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_RotateTowards, _Target);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Last solve. Present only while a target is set (CkEcsExt's FFragment_Transform_Previous shape): Add/SetTarget add it,
    // ClearTarget and a lost target remove it. Absence means "no solve".
    struct CKROTATETOWARDS_API FFragment_RotateTowards_SolveResult
    {
    public:
        CK_GENERATED_BODY(FFragment_RotateTowards_SolveResult);

    public:
        friend class FProcessor_RotateTowards_HandleRequests;
        friend class FProcessor_RotateTowards_Update;
        friend class UCk_Utils_RotateTowards_UE;

    private:
        FRotator _DesiredRotation = FRotator::ZeroRotator;
        FRotator _RemainingDelta = FRotator::ZeroRotator;
        FRotator _LastAppliedRotation = FRotator::ZeroRotator;

    public:
        CK_PROPERTY_GET(_DesiredRotation);
        CK_PROPERTY_GET(_RemainingDelta);
        CK_PROPERTY_GET(_LastAppliedRotation);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKROTATETOWARDS_API FFragment_RotateTowards_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_RotateTowards_Requests);

        friend class FProcessor_RotateTowards_HandleRequests;
        friend class FProcessor_RotateTowards_CancelPendingRequests;
        friend class UCk_Utils_RotateTowards_UE;

        using RequestType = std::variant<
            FCk_Request_RotateTowards_SetTarget,
            FCk_Request_RotateTowards_ClearTarget,
            FCk_Request_RotateTowards_UpdateTunables,
            FCk_Request_RotateTowards_SetRangeClamp,
            FCk_Request_RotateTowards_ClearRangeClamp,
            FCk_Request_RotateTowards_EnableDisable>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKROTATETOWARDS_API, OnRotateTowardsTargetChanged, FCk_Delegate_RotateTowards_OnTargetChanged, FCk_Handle_RotateTowards, FCk_Handle_Transform, FCk_Handle_Transform);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKROTATETOWARDS_API, OnRotateTowardsTargetReached, FCk_Delegate_RotateTowards_OnTargetReached, FCk_Handle_RotateTowards, FCk_Handle_Transform);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKROTATETOWARDS_API, OnRotateTowardsTargetCleared, FCk_Delegate_RotateTowards_OnTargetCleared, FCk_Handle_RotateTowards, FCk_Handle_Transform, ECk_RotateTowards_ClearReason);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_RotateTowards_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
