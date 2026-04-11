#pragma once

#include "CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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

        static auto
        GetCleanClassName(
            const UClass* InClass) -> FString;
    };
}

// --------------------------------------------------------------------------------------------------------------------
