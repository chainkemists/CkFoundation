#pragma once

#include "CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Processor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // COMMIT PENDING SCRIPT ATTACH — Materializes deferred EntityScripts on SM child entities
    // before the EntityScript construction pipeline observes them. Runs in the same script
    // group as FProcessor_EntityScript_ContinueConstruction and must precede it so the
    // attach's FTag_EntityScript_ContinueConstruction is in place for the same frame.
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmScript_CommitPendingAttach : public ck_exp::TProcessor<
        FProcessor_SmScript_CommitPendingAttach,
        FCk_Handle,
        ck::TReadOnly<FFragment_SmScript_PendingAttach>,
        FTag_SmScript_PendingAttach,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay_Script;
        using RunBefore     = TDepList<FProcessor_EntityScript_ContinueConstruction>;
        using MarkedDirtyBy = FTag_SmScript_PendingAttach;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SmScript_PendingAttach& InPending) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ================================================================================================================
    // SETUP — One-time initialization, auto-start if configured
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_Setup : public ck_exp::TProcessor<
        FProcessor_Sm_Setup,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_Current>,
        FTag_Sm_RequiresSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using MarkedDirtyBy = FTag_Sm_RequiresSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent) -> void;
    };

    // ================================================================================================================
    // HANDLE REQUESTS — Process Start/Stop/Pause/Resume/Transition
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Sm_HandleRequests,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_Current>,
        TReadOnly<FFragment_Sm_Requests>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_Sm_Setup>;
        using MarkedDirtyBy = FFragment_Sm_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FFragment_Sm_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Start& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Stop& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Pause& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Resume& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Transition& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_AddOverrideState& InRequest) -> void;

    private:
        static auto
        DoEnterState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent,
            TSubclassOf<UCk_SmState_EntityScript> InStateClass) -> void;

        static auto
        DoExitCurrentState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent) -> void;
    };

    // ================================================================================================================
    // ENDPLAY — Cleanup on SM entity destruction
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_EndPlay : public ck_exp::TProcessor<
        FProcessor_Sm_EndPlay,
        FCk_Handle_StateMachine,
        TReadWrite<FFragment_Sm_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sm_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
