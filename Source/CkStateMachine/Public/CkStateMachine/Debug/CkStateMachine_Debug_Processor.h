#pragma once

#include "CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward declaration for RunAfter dependency
    class FProcessor_SmTask_FireFinishedSignal;

    // --------------------------------------------------------------------------------------------------------------------
    // DEBUG — Accumulate observed states/transitions, track history

    class CKSTATEMACHINE_API FProcessor_Sm_Debug : public ck_exp::TProcessor<
        FProcessor_Sm_Debug,
        FCk_Handle_StateMachine,
        ck::TReadOnly<FFragment_Sm_Current>,
        ck::TReadOnly<FFragment_Sm_Params>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmTask_FireFinishedSignal>;

    public:
        using TProcessor::TProcessor;

    public:
        // DoTick gates: the poll walks every SM entity every frame but its output only feeds
        // debugger UI. Skip the view iteration entirely unless a debugger consumed the data
        // recently or the on-screen debug overlay (ck.DebugOverlay) is on — see
        // UCk_Utils_StateMachineDebug_UE::Get_IsDebugDataDesired. The on→off transition still
        // iterates one final time so consumers see a coherent last snapshot.
        auto
        DoTick(
            FCk_Time InDeltaT) -> void;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Current& InCurrent,
            const FFragment_Sm_Params& InParams) -> void;

    private:
        static auto
        DoCacheCurrentState(
            HandleType InHandle,
            FFragment_Sm_Debug& InDebug,
            const FFragment_Sm_Current& InCurrent) -> void;

    private:
        // Tracks the previous tick's toggle state so an on→off transition can fire one final
        // pass, without iterating every subsequent tick while no debugger is watching.
        bool _LastTickToggleOn = false;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // DEBUG HANDLE REQUESTS — Drains FFragment_SmDebug_Requests

    class CKSTATEMACHINE_API FProcessor_SmDebug_HandleRequests : public ck_exp::TProcessor<
        FProcessor_SmDebug_HandleRequests,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_SmDebug_Requests>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using MarkedDirtyBy = FFragment_SmDebug_Requests;
        using RunAfter = TDepList<FProcessor_Sm_HandleRequests>;

    public:
        using TProcessor::TProcessor;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SmDebug_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_SmDebug_RecordTransition& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
