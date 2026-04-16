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

    // ================================================================================================================
    // DEBUG — Accumulate observed states/transitions, track history
    // ================================================================================================================

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
    };

    // ================================================================================================================
    // DEBUG HANDLE REQUESTS — Drains FFragment_SmDebug_Requests
    // ================================================================================================================

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
