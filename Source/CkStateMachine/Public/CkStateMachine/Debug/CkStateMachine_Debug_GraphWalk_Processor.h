#pragma once

#include "CkStateMachine_Debug_GraphWalk_Fragment.h"
#include "CkStateMachine_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#if CK_BUILD_SM_GRAPH_WALK

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // GRAPH WALK (ONE-SHOT) — Kicks off the multi-frame graph walk
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_Debug_GraphWalk : public ck_exp::TProcessor<
        FProcessor_Sm_Debug_GraphWalk,
        FCk_Handle_StateMachine,
        ck::TReadOnly<FFragment_Sm_Params>,
        FTag_Sm_Debug_RequiresGraphWalk,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;

    public:
        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams) -> void;

        static auto
        CreateBatch(
            FFragment_Sm_Debug_GraphWalk_Progress& InOutProgress,
            HandleType InSmHandle) -> void;

        static auto
        GetCleanClassName(
            const UClass* InClass) -> FString;
    };

    // ================================================================================================================
    // GRAPH WALK ITERATE — Runs each frame while walk is in progress
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_Debug_GraphWalk_Iterate : public ck_exp::TProcessor<
        FProcessor_Sm_Debug_GraphWalk_Iterate,
        FCk_Handle_StateMachine,
        ck::TReadOnly<FFragment_Sm_Debug_GraphWalk_Progress>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;

    public:
        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Debug_GraphWalk_Progress& InProgress) -> void;

    private:
        static auto
        CreateBatch(
            FFragment_Sm_Debug_GraphWalk_Progress& InOutProgress,
            HandleType InSmHandle) -> void;

        static auto
        AssignSubSmStateDefinitions(
            FFragment_Sm_Debug_GraphWalk_Progress& InOutProgress) -> void;

        static auto
        GetCleanClassName(
            const UClass* InClass) -> FString;
    };
}

#endif // CK_BUILD_SM_GRAPH_WALK

// --------------------------------------------------------------------------------------------------------------------
