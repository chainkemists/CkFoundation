#include "CkTransition_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcsExt/ContextOwner/CkContextOwner_Utils.h"

#include "CkHFSM/Condition/CkCondition_Utils.h"
#include "CkHFSM/State/CkState_Utils.h"
#include "CkHFSM/CkHFSM_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Transition_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transition_Params& InParams,
            FFragment_Transition_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_Transition_Setup>();

        hfsm::VeryVerbose(TEXT("[SETUP][TRANSITION] [{}] - Entity [{}]"),
            InParams.Get_Name().ToString(), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transition_Enter::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transition_Params& InParams) const
        -> void
    {
        InHandle.Remove<FTag_Transition_Enter>();

        const auto ConditionHandle = UCk_Utils_Condition_UE::CastChecked(InParams.Get_TransitionCondition());

        if (UCk_Utils_Condition_UE::Get_IsEventDriven(ConditionHandle))
        {
            UCk_Utils_Condition_UE::Request_StartOrResumeEvaluating(ConditionHandle);
            InHandle.AddOrGet<FTag_Transition_IsEventDriven>();
        }
        else
        {
            const auto ParentStateEntity = UCk_Utils_ContextOwner_UE::Get_Owner(InHandle);
            if (ck::IsValid(ParentStateEntity))
            {
                ParentStateEntity.AddOrGet<ck::FTag_State_IsNotEventDriven>();
            }
        }

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnTransitionEnter::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        hfsm::VeryVerbose(TEXT("[ENTER][TRANSITION] [{}] - Entity [{}]"),
            InParams.Get_Name().ToString(), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transition_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transition_Params& InParams) const
        -> void
    {
        const auto ConditionHandle = UCk_Utils_Condition_UE::CastChecked(InParams.Get_TransitionCondition());
        UCk_Utils_Condition_UE::Request_StopEvaluating(ConditionHandle);

        InHandle.Remove<FTag_Transition_Exit>();
        InHandle.Remove<FTag_Transition_EvaluationPassed>();
        InHandle.Remove<FTag_Transition_EvaluationFailed>();

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnTransitionExit::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        hfsm::VeryVerbose(TEXT("[EXIT][TRANSITION] [{}] - Entity [{}]"),
            InParams.Get_Name().ToString(), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transition_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transition_Params& InParams) const
        -> void
    {
        const auto ConditionHandle = UCk_Utils_Condition_UE::CastChecked(InParams.Get_TransitionCondition());
        const auto Result = UCk_Utils_Condition_UE::Get_EvaluationResult(ConditionHandle);

        switch (Result)
        {
            case ECk_Condition_Result::Undetermined:
            {
                hfsm::VeryVerbose(TEXT("[EVALUATING][TRANSITION][Undetermined] [{}] - Entity [{}]"),
                    InParams.Get_Name().ToString(), InHandle);

                UCk_Utils_Condition_UE::Request_StartOrResumeEvaluating(ConditionHandle);
                return;
            }
            case ECk_Condition_Result::Pass:
            {
                hfsm::VeryVerbose(TEXT("[EVALUATING][TRANSITION][PASSED] [{}] - Entity [{}]"),
                    InParams.Get_Name().ToString(), InHandle);

                UCk_Utils_Condition_UE::Request_PauseEvaluation(ConditionHandle);
                InHandle.AddOrGet<FTag_Transition_EvaluationPassed>();
                InHandle.Remove<FTag_Transition_EvaluationFailed>();

                {
#if STATS
                    auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
                    UUtils_Signal_OnTransitionPassed::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
                }

                // Notify parent state to evaluate
                const auto ParentEntity = UCk_Utils_ContextOwner_UE::Get_Owner(InHandle);
                if (ck::IsValid(ParentEntity))
                {
                    auto ParentState = UCk_Utils_State_UE::CastChecked(ParentEntity);
                    UCk_Utils_State_UE::Request_Evaluate(ParentState);
                }

                break;
            }
            case ECk_Condition_Result::Fail:
            {
                hfsm::VeryVerbose(TEXT("[EVALUATING][TRANSITION][FAILED] [{}] - Entity [{}]"),
                    InParams.Get_Name().ToString(), InHandle);

                UCk_Utils_Condition_UE::Request_PauseEvaluation(ConditionHandle);
                InHandle.AddOrGet<FTag_Transition_EvaluationFailed>();
                InHandle.Remove<FTag_Transition_EvaluationPassed>();

                {
#if STATS
                    auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
                    UUtils_Signal_OnTransitionFailed::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
                }

                // Notify parent state to evaluate
                const auto ParentEntity = UCk_Utils_ContextOwner_UE::Get_Owner(InHandle);
                if (ck::IsValid(ParentEntity))
                {
                    auto ParentState = UCk_Utils_State_UE::CastChecked(ParentEntity);
                    UCk_Utils_State_UE::Request_Evaluate(ParentState);
                }

                break;
            }
            default:
            {
                CK_INVALID_ENUM(Result);
                break;
            }
        }

        InHandle.Remove<FTag_StateMachine_Evaluate_TransitionOrCondition>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Transition_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transition_Current& InCurrent,
            const FFragment_Transition_Requests& InRequests) const
        -> void
    {
        InHandle.CopyAndRemove(InRequests, [&](FFragment_Transition_Requests& InRequestsCopy)
        {
            algo::ForEachRequest(InRequestsCopy._Requests, Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_Transition_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Transition_Current& InCurrent,
            const FCk_Request_Transition_Command& InRequest)
        -> void
    {
        switch (InRequest.Get_Command())
        {
            case ECk_Transition_Command::StartEvaluating:
            {
                InHandle.AddOrGet<FTag_Transition_Enter>();
                break;
            }
            case ECk_Transition_Command::StopEvaluating:
            {
                InHandle.AddOrGet<FTag_Transition_Exit>();
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InRequest.Get_Command());
                break;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------