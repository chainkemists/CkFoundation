#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"

#include "CkTimer/CkTimer_Fragment_Data.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h" // ck::FSnapshotContext (referenced by FFragment_Timer_Current::SerializeSnapshot)

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Timer_UE;
class FArchive;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Timer_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Timer_NeedsUpdate);
    CK_DEFINE_ECS_TAG(FTag_Timer_Countdown);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Timer_Params = FCk_Fragment_Timer_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTIMER_API FFragment_Timer_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Timer_Current);
        using IsSnapshotable = void;

    public:
        friend class FProcessor_Timer_Setup;
        friend class FProcessor_Timer_HandleRequests;
        friend class FProcessor_Timer_Update;
        friend class FProcessor_Timer_Update_Countdown;
        friend class FProcessor_Timer_Replicate;

    private:
        FCk_Chrono _Chrono;

    public:
        CK_PROPERTY_GET(_Chrono);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Timer_Current, _Chrono);

        // Tier-C: the countdown progress (FCk_Chrono, incl. its Transient _CurrentValue) is authoritative timer
        // state with no entity-handle ref. Body in the .cpp delegates to FCk_Chrono::SerializeSnapshot. Registered
        // so a restored timer RESUMES mid-countdown (the run/pause + count-direction tags already round-trip free).
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTIMER_API FFragment_Timer_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Timer_Requests);

    public:
        friend class FProcessor_Timer_HandleRequests;
        friend class UCk_Utils_Timer_UE;

    public:
        using RequestType = std::variant<FCk_Request_Timer_Manipulate, FCk_Request_Timer_Jump, FCk_Request_Timer_Consume>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfTimers, FCk_Handle_Timer);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerReset, FCk_Delegate_Timer, FCk_Handle_Timer, FCk_Chrono, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerStop, FCk_Delegate_Timer, FCk_Handle_Timer, FCk_Chrono, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerPause, FCk_Delegate_Timer, FCk_Handle_Timer, FCk_Chrono, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerResume, FCk_Delegate_Timer, FCk_Handle_Timer, FCk_Chrono, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerDone, FCk_Delegate_Timer, FCk_Handle_Timer, FCk_Chrono, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerUpdate, FCk_Delegate_Timer, FCk_Handle_Timer, FCk_Chrono, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerDepleted, FCk_Delegate_Timer, FCk_Handle_Timer, FCk_Chrono, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerJump, FCk_Delegate_Timer_Jump, FCk_Handle_Timer, FCk_Chrono, FCk_Time, ECk_Timer_JumpDirection, FCk_Time);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Timer_Requests);
}

// --------------------------------------------------------------------------------------------------------------------