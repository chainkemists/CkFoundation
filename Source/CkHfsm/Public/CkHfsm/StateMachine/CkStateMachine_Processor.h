//#pragma once
//
//#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
//#include "CkEcs/Processor/CkProcessor.h"
//#include "CkHFSM/StateMachine/CkStateMachine_Fragment.h"
//
//namespace ck
//{
//    class CKHFSM_API FProcessor_StateMachine_Setup
//        : public ck_exp::TProcessor<FProcessor_StateMachine_Setup, FCk_Handle_StateMachine,
//            FFragment_StateMachine_Params, FFragment_StateMachine_Current,
//            FTag_StateMachine_Setup, CK_IGNORE_PENDING_KILL>
//    {
//    public:
//        using MarkedDirtyBy = FTag_StateMachine_Setup;
//        using TProcessor::TProcessor;
//
//        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
//            const FFragment_StateMachine_Params& InParams,
//            FFragment_StateMachine_Current& InCurrent) const -> void;
//    };
//
//    class CKHFSM_API FProcessor_StateMachine_Enter
//        : public ck_exp::TProcessor<FProcessor_StateMachine_Enter, FCk_Handle_StateMachine,
//            FFragment_StateMachine_Params, FFragment_StateMachine_Current,
//            FTag_StateMachine_Enter, CK_IGNORE_PENDING_KILL>
//    {
//    public:
//        using MarkedDirtyBy = FTag_StateMachine_Enter;
//        using TProcessor::TProcessor;
//
//        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
//            const FFragment_StateMachine_Params& InParams,
//            FFragment_StateMachine_Current& InCurrent) const -> void;
//    };
//
//    class CKHFSM_API FProcessor_StateMachine_Exit
//        : public ck_exp::TProcessor<FProcessor_StateMachine_Exit, FCk_Handle_StateMachine,
//            FFragment_StateMachine_Current, FTag_StateMachine_Exit, CK_IGNORE_PENDING_KILL>
//    {
//    public:
//        using MarkedDirtyBy = FTag_StateMachine_Exit;
//        using TProcessor::TProcessor;
//
//        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
//            FFragment_StateMachine_Current& InCurrent) const -> void;
//    };
//
//    class CKHFSM_API FProcessor_StateMachine_Transition
//        : public ck_exp::TProcessor<FProcessor_StateMachine_Transition, FCk_Handle_StateMachine,
//            FFragment_StateMachine_Current, FTag_StateMachine_Transition, CK_IGNORE_PENDING_KILL>
//    {
//    public:
//        using MarkedDirtyBy = FTag_StateMachine_Transition;
//        using TProcessor::TProcessor;
//
//        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
//            FFragment_StateMachine_Current& InCurrent) const -> void;
//    };
//
//    class CKHFSM_API FProcessor_StateMachine_Evaluate
//        : public ck_exp::TProcessor<FProcessor_StateMachine_Evaluate, FCk_Handle_StateMachine,
//            FFragment_StateMachine_Current, FTag_StateMachine_Evaluate_StateMachine,
//            CK_IGNORE_PENDING_KILL>
//    {
//    public:
//        using MarkedDirtyBy = FTag_StateMachine_Evaluate_StateMachine;
//        using TProcessor::TProcessor;
//
//        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
//            FFragment_StateMachine_Current& InCurrent) const -> void;
//    };
//
//    class CKHFSM_API FProcessor_StateMachine_HandleRequests
//        : public ck_exp::TProcessor<FProcessor_StateMachine_HandleRequests, FCk_Handle_StateMachine,
//            FFragment_StateMachine_Current, FFragment_StateMachine_Requests, CK_IGNORE_PENDING_KILL>
//    {
//    public:
//        using MarkedDirtyBy = FFragment_StateMachine_Requests;
//        using TProcessor::TProcessor;
//
//        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle,
//            FFragment_StateMachine_Current& InCurrent,
//            const FFragment_StateMachine_Requests& InRequests) const -> void;
//
//    private:
//        static auto DoHandleRequest(HandleType InHandle, FFragment_StateMachine_Current& InCurrent,
//            const FCk_Request_StateMachine_Command& InRequest) -> void;
//    };
//}