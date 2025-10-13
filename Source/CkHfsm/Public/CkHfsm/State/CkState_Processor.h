#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkHFSM/State/CkState_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKHFSM_API FProcessor_State_Setup
        : public ck_exp::TProcessor<FProcessor_State_Setup, FCk_Handle_State,
            FFragment_State_Params, FFragment_State_Current, FTag_State_Setup, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_State_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_State_Params& InParams,
            FFragment_State_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_State_Enter
        : public ck_exp::TProcessor<FProcessor_State_Enter, FCk_Handle_State,
            FFragment_State_Current, FTag_State_Enter, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_State_Enter;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_State_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_State_Exit
        : public ck_exp::TProcessor<FProcessor_State_Exit, FCk_Handle_State,
            FFragment_State_Current, FTag_State_Exit, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_State_Exit;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_State_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_State_Evaluate
        : public ck_exp::TProcessor<FProcessor_State_Evaluate, FCk_Handle_State,
            FFragment_State_Current, FTag_StateMachine_Evaluate_State,
            TExclude<FTag_State_ReadyToTransition>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_StateMachine_Evaluate_State;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_State_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_State_Update
        : public ck_exp::TProcessor<FProcessor_State_Update, FCk_Handle_State,
            FFragment_State_Current, FTag_State_Update, FTag_State_IsNotEventDriven, CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_State_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_State_HandleRequests
        : public ck_exp::TProcessor<FProcessor_State_HandleRequests, FCk_Handle_State,
            FFragment_State_Current, FFragment_State_Requests, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_State_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_State_Current& InCurrent,
            const FFragment_State_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_State_Current& InCurrent,
            const FCk_Request_State_Command& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------