#include "CkCondition_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcsExt/ContextOwner/CkContextOwner_Utils.h"

#include "CkHFSM/CkHFSM_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Condition_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Condition_Params& InParams,
            FFragment_Condition_Current& InCurrent) const
        -> void
    {
        // Apply configuration from params
        if (InParams.Get_NegateResult())
        {
            InHandle.AddOrGet<FTag_Condition_NegateResult>();
        }

        if (InParams.Get_IsEventDriven())
        {
            InHandle.AddOrGet<FTag_Condition_IsEventDriven>();
        }
        else
        {
            InHandle.AddOrGet<FTag_Condition_IsNotEventDriven>();
        }

        InHandle.Remove<FTag_Condition_Setup>();

        hfsm::VeryVerbose(TEXT("[SETUP][CONDITION] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Condition_Enter::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_Condition_Enter>();

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnConditionEnter::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        hfsm::VeryVerbose(TEXT("[ENTER][CONDITION] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Condition_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_Condition_Exit>();
        InHandle.Remove<FTag_Condition_EvaluationPaused>();
        InHandle.Remove<FTag_Condition_EvaluationPassed>();
        InHandle.Remove<FTag_Condition_EvaluationFailed>();

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnConditionExit::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        hfsm::VeryVerbose(TEXT("[EXIT][CONDITION] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Condition_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_StateMachine_Evaluate_TransitionOrCondition>();

        // Derived condition types will implement their own evaluation logic
        // Base condition just checks if tags are already set
        // (e.g., via Request_MarkResult)

        hfsm::VeryVerbose(TEXT("[EVALUATE][CONDITION] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Condition_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Condition_Params& InParams,
            FFragment_Condition_Current& InCurrent,
            const FFragment_Condition_Requests& InRequests) const
        -> void
    {
        InHandle.CopyAndRemove(InRequests, [&](FFragment_Condition_Requests& InRequestsCopy)
        {
            algo::ForEachRequest(InRequestsCopy._Requests, Visitor([&](const auto& InRequest)
            {
                using RequestType = std::decay_t<decltype(InRequest)>;

                if constexpr (std::is_same_v<RequestType, FCk_Request_Condition_Command>)
                {
                    DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
                }
                else if constexpr (std::is_same_v<RequestType, FCk_Request_Condition_MarkResult>)
                {
                    DoHandleRequest(InHandle, InCurrent, InRequest);
                }

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_Condition_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Condition_Params& InParams,
            FFragment_Condition_Current& InCurrent,
            const FCk_Request_Condition_Command& InRequest)
        -> void
    {
        switch (InRequest.Get_Command())
        {
            case ECk_Condition_Command::StartOrResumeEvaluating:
            {
                if (InHandle.Has<FTag_Condition_EvaluationPaused>())
                {
                    InHandle.Remove<FTag_Condition_EvaluationPaused>();
                }
                else
                {
                    InHandle.AddOrGet<FTag_Condition_Enter>();
                }

                InHandle.AddOrGet<FTag_StateMachine_Evaluate_TransitionOrCondition>();
                InHandle.Remove<FTag_Condition_EvaluationPassed>();
                InHandle.Remove<FTag_Condition_EvaluationFailed>();
                break;
            }
            case ECk_Condition_Command::PauseEvaluation:
            {
                InHandle.Remove<FTag_StateMachine_Evaluate_TransitionOrCondition>();
                InHandle.AddOrGet<FTag_Condition_EvaluationPaused>();
                break;
            }
            case ECk_Condition_Command::StopEvaluating:
            {
                InHandle.AddOrGet<FTag_Condition_Exit>();
                InHandle.Remove<FTag_Condition_EvaluationPaused>();
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InRequest.Get_Command());
                break;
            }
        }
    }

    auto
        FProcessor_Condition_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent,
            const FCk_Request_Condition_MarkResult& InRequest)
        -> void
    {
        switch (InRequest.Get_Result())
        {
            case ECk_Condition_MarkResult::Passed:
            {
                InHandle.AddOrGet<FTag_Condition_EvaluationPassed>();
                InHandle.Remove<FTag_Condition_EvaluationFailed>();

                {
#if STATS
                    auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
                    UUtils_Signal_OnConditionPassed::Broadcast(InHandle, MakePayload(InHandle, FCk_Time{}));
                }
                break;
            }
            case ECk_Condition_MarkResult::Failed:
            {
                InHandle.AddOrGet<FTag_Condition_EvaluationFailed>();
                InHandle.Remove<FTag_Condition_EvaluationPassed>();

                {
#if STATS
                    auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
                    UUtils_Signal_OnConditionFailed::Broadcast(InHandle, MakePayload(InHandle, FCk_Time{}));
                }
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InRequest.Get_Result());
                break;
            }
        }

        // Notify parent transition to evaluate
        const auto ParentEntity = UCk_Utils_ContextOwner_UE::Get_Owner(InHandle);
        if (ck::IsValid(ParentEntity))
        {
            ParentEntity.AddOrGet<FTag_StateMachine_Evaluate_TransitionOrCondition>();
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------