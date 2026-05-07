#pragma once

#include "CkMontagePlayer_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkMontagePlayer_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_MontagePlayer_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_MontagePlayer_MayRequireReplication);
    CK_DEFINE_ECS_TAG(FTag_MontagePlayer_HasActiveMontage);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_MontagePlayer_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_MontagePlayer_Params);

    public:
        using ParamsType = FCk_Fragment_MontagePlayer_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_MontagePlayer_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_MontagePlayer_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_MontagePlayer_Current);

    public:
        friend class FProcessor_MontagePlayer_HandleRequests;
        friend class FProcessor_MontagePlayer_MonitorAnimInstance;
        friend class FProcessor_MontagePlayer_Replicate;
        friend class ::UCk_Utils_MontagePlayer_UE;

    private:
        FCk_MontagePlayer_State _State;

        TWeakObjectPtr<UAnimMontage> _ActiveMontage;
        TWeakObjectPtr<UAnimInstance> _LastSeenAnimInstance;

        // Catch-up countdown — when > 0 the AnimInstance is running at adjusted PlayRate for
        // the next N real-seconds, then restored to 1.0.
        FCk_Time _CatchUpRemaining;

    public:
        CK_PROPERTY_GET(_State);
        CK_PROPERTY_GET(_ActiveMontage);
        CK_PROPERTY_GET(_LastSeenAnimInstance);
        CK_PROPERTY_GET(_CatchUpRemaining);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_MontagePlayer_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_MontagePlayer_Requests);

    public:
        friend class FProcessor_MontagePlayer_HandleRequests;
        friend class ::UCk_Utils_MontagePlayer_UE;

    public:
        using PlayType = FCk_Request_MontagePlayer_Play;
        using StopType = FCk_Request_MontagePlayer_Stop;
        using PauseType = FCk_Request_MontagePlayer_Pause;
        using ResumeType = FCk_Request_MontagePlayer_Resume;
        using JumpType = FCk_Request_MontagePlayer_JumpToSection;

        using RequestType = std::variant<PlayType, StopType, PauseType, ResumeType, JumpType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKANIMATION_API,
        MontagePlayer_OnStarted,
        FCk_Delegate_MontagePlayer_OnStarted,
        FCk_Handle_MontagePlayer,
        FCk_MontagePlayer_State);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKANIMATION_API,
        MontagePlayer_OnFinished,
        FCk_Delegate_MontagePlayer_OnFinished,
        FCk_Handle_MontagePlayer,
        FCk_MontagePlayer_State,
        ECk_MontagePlayer_FinishReason);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_MontagePlayer_Requests);
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKANIMATION_API FCk_RepData_MontagePlayer
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_MontagePlayer);

    UPROPERTY()
    FCk_MontagePlayer_State Value;
};

namespace ck
{
    using FFragment_ContainerRef_MontagePlayer = TFragment_ContainerEntryRef<FCk_RepData_MontagePlayer>;
}

// --------------------------------------------------------------------------------------------------------------------
