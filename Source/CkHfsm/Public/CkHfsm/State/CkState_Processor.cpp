#include "CkHfsm/State/CkState_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkHfsm/Service/CkService_Utils.h"
#include "CkHfsm/Transition/CkTransition_Utils.h"
#include "CkHfsm/CkHfsm_Log.h"
#include "CkHfsm/StateMachine/CkStateMachine_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct RecordOfServices_Utils : public TUtils_RecordOfEntities<FFragment_RecordOfServices> {};
    struct RecordOfTransitions_Utils : public TUtils_RecordOfEntities<FFragment_RecordOfTransitions> {};

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_State_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_State_Params& InParams,
            FFragment_State_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_State_Setup>();

        hfsm::VeryVerbose(TEXT("[SETUP][STATE] [{}] - Entity [{}]"), InParams.Get_Name(), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_State_Enter::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_State_Current& InCurrent) const
        -> void
    {
        // Start all services attached to this state
        RecordOfServices_Utils::ForEach_ValidEntry(InHandle, [&](FCk_Handle_Service InService)
        {
            UCk_Utils_Service_UE::Request_Start(InService);
        });

        InHandle.Remove<FTag_State_Enter>();
        InHandle.AddOrGet<FTag_State_Update>();

        // Request initial evaluation
        UCk_Utils_State_UE::Request_Evaluate(InHandle);

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnStateEnter::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        const auto& StateName = InHandle.Get<FFragment_State_Params>().Get_Name();
        hfsm::VeryVerbose(TEXT("[ENTER][STATE] [{}] - Entity [{}]"), StateName, InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_State_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_State_Current& InCurrent) const
        -> void
    {
        // Stop all services
        RecordOfServices_Utils::ForEach_ValidEntry(InHandle, [&](FCk_Handle_Service InService)
        {
            UCk_Utils_Service_UE::Request_Stop(InService);
        });

        // Stop evaluating all transitions
        RecordOfTransitions_Utils::ForEach_ValidEntry(InHandle, [&](FCk_Handle_Transition InTransition)
        {
            UCk_Utils_Transition_UE::Request_StopEvaluating(InTransition);
        });

        InCurrent.Set_NextState(FCk_Handle{});
        InHandle.Remove<FTag_State_Exit>();
        InHandle.Remove<FTag_State_Update>();
        InHandle.Remove<FTag_State_ReadyToTransition>();

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnStateExit::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        const auto& StateName = InHandle.Get<FFragment_State_Params>().Get_Name();
        hfsm::VeryVerbose(TEXT("[EXIT][STATE] [{}] - Entity [{}]"), StateName, InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_State_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_State_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_StateMachine_Evaluate_State>();

        // Evaluate transitions in order - first one that passes wins
        auto FoundPassingTransition = false;

        RecordOfTransitions_Utils::ForEach_ValidEntry(InHandle, [&](FCk_Handle_Transition InTransition)
        {
            if (FoundPassingTransition)
            { return; }

            const auto Result = UCk_Utils_Transition_UE::Get_EvaluationResult(InTransition);

            switch (Result)
            {
                case ECk_Transition_Result::Undetermined:
                {
                    UCk_Utils_Transition_UE::Request_StartEvaluating(InTransition);
                    FoundPassingTransition = true; // Stop checking other transitions
                    return;
                }
                case ECk_Transition_Result::Pass:
                {
                    InCurrent.Set_NextState(UCk_Utils_Transition_UE::Get_TargetState(InTransition));
                    InHandle.AddOrGet<FTag_State_ReadyToTransition>();

                    // Notify parent state machine to evaluate
                    auto ParentEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle);
                    if (ck::IsValid(ParentEntity))
                    {
                        ParentEntity.AddOrGet<FTag_StateMachine_Evaluate_StateMachine>();
                    }

                    hfsm::VeryVerbose(TEXT("[EVALUATE][STATE][Ready-To-Transition] [{}] to State [{}]"), InHandle, InCurrent.Get_NextState());

                    FoundPassingTransition = true;
                    return;
                }
                case ECk_Transition_Result::Fail:
                {
                    // Continue to next transition
                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(Result);
                    break;
                }
            }
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_State_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_State_Current& InCurrent) const
        -> void
    {
        UCk_Utils_State_UE::Request_Evaluate(InHandle);

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnStateUpdate::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------