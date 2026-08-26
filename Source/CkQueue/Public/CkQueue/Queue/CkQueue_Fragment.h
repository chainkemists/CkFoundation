#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkQueue/Queue/CkQueue_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Queue_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Queue_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Queue_NeedsFormation);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Queue_Params = FCk_Fragment_Queue_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKQUEUE_API FFragment_Queue_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Queue_Current);

    public:
        friend class FProcessor_Queue_Setup;
        friend class FProcessor_Queue_HandleRequests;
        friend class FProcessor_Queue_Reconcile;
        friend class FProcessor_Queue_Formation;
        friend class FProcessor_Queue_EndPlay;

    private:
        TArray<FCk_Queue_MemberSnapshot> _Members;
        TArray<FCk_Queue_Origin> _Origins;
        int64 _NextTicket = 1;
        int32 _Revision = 0;
        int32 _RetryEpisode = 0;
        ECk_Queue_State _State = ECk_Queue_State::NeedsSetup;
        ECk_Queue_LayoutAlgorithm _LayoutAlgorithm = ECk_Queue_LayoutAlgorithm::OrthogonalSnake;
        FCk_Queue_Pressure _Pressure;
        FTransform _LastOwnerWorldTransform = FTransform::Identity;
        int32 _LastNavigationRevision = INDEX_NONE;
        double _NextFormationRetryWorldSeconds = 0.0;

        // A ClaimFirstAvailableOnReach slot-reached report leaves _State Ready by design (the request
        // drain must stay Ready for same-frame reports from other origins), so this flag is what
        // carries "the remaining contenders must be retargeted" past the formation processor's settled early-out.
        bool _HasPendingClaimOffer = false;

    public:
        CK_PROPERTY_GET(_Members);
        CK_PROPERTY_GET(_Origins);
        CK_PROPERTY_GET(_NextTicket);
        CK_PROPERTY_GET(_Revision);
        CK_PROPERTY_GET(_RetryEpisode);
        CK_PROPERTY_GET(_State);
        CK_PROPERTY_GET(_LayoutAlgorithm);
        CK_PROPERTY_GET(_Pressure);
        CK_PROPERTY_GET(_LastOwnerWorldTransform);
        CK_PROPERTY_GET(_LastNavigationRevision);
        CK_PROPERTY_GET(_NextFormationRetryWorldSeconds);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKQUEUE_API FFragment_Queue_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Queue_Requests);

    public:
        friend class FProcessor_Queue_HandleRequests;
        friend class UCk_Utils_Queue_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_Queue_Join,
            FCk_Request_Queue_RestoreJoin,
            FCk_Request_Queue_Leave,
            FCk_Request_Queue_AdvanceOrigin,
            FCk_Request_Queue_SetOrigins,
            FCk_Request_Queue_SetLayout,
            FCk_Request_Queue_SetMovementSuppressed,
            FCk_Request_Queue_ReportMovementOutcome>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKQUEUE_API,
        OnQueueMemberStateChanged,
        FCk_Delegate_Queue_OnMemberStateChanged,
        FCk_Handle_Queue,
        FCk_Queue_MemberEvent);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKQUEUE_API,
        OnQueuePressureChanged,
        FCk_Delegate_Queue_OnPressureChanged,
        FCk_Handle_Queue,
        FCk_Queue_Pressure);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKQUEUE_API,
        OnQueueFormationStateChanged,
        FCk_Delegate_Queue_OnFormationStateChanged,
        FCk_Handle_Queue,
        FCk_Queue_FormationState);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKQUEUE_API,
        OnQueueInvalidated,
        FCk_Delegate_Queue_OnInvalidated,
        FCk_Handle_Queue,
        FCk_Queue_FormationState);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Queue_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
