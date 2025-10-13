#include "CkHfsm/StateMachine/CkStateMachine_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkHfsm/State/CkState_Utils.h"
#include "CkHfsm/CkHfsm_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct RecordOfStates_Utils : public TUtils_RecordOfEntities<FFragment_RecordOfStates> {};

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_StateMachine_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_StateMachine_Params& InParams,
            FFragment_StateMachine_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_StateMachine_Setup>();

        hfsm::VeryVerbose(TEXT("[SETUP][STATEMACHINE] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_StateMachine_Enter::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_StateMachine_Params& InParams,
            FFragment_StateMachine_Current& InCurrent) const
        -> void
    {
        auto StartingStateHandle = UCk_Utils_State_UE::CastChecked(InParams.Get_StartingState());
        UCk_Utils_State_UE::Request_Enter(StartingStateHandle);
        InCurrent._CurrentState = InParams.Get_StartingState();

        InHandle.Remove<FTag_StateMachine_Enter>();

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnStateMachineStart::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        hfsm::VeryVerbose(TEXT("[ENTER][STATEMACHINE] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_StateMachine_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_StateMachine_Current& InCurrent) const
        -> void
    {
        auto CurrentStateHandle = UCk_Utils_State_UE::CastChecked(InCurrent.Get_CurrentState());
        UCk_Utils_State_UE::Request_Exit(CurrentStateHandle);
        InCurrent._CurrentState = FCk_Handle{};

        InHandle.Remove<FTag_StateMachine_Exit>();
        InHandle.Remove<FTag_StateMachine_Transition>();

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnStateMachineStop::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        hfsm::VeryVerbose(TEXT("[EXIT][STATEMACHINE] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_StateMachine_Transition::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_StateMachine_Current& InCurrent) const
        -> void
    {
        const auto PreviousStateHandle = UCk_Utils_State_UE::CastChecked(InCurrent.Get_PreviousState());

        // Wait for previous state to fully exit
        if (PreviousStateHandle.Has<ck::FTag_State_Exit>())
        {
            return;
        }

        auto CurrentStateHandle = UCk_Utils_State_UE::CastChecked(InCurrent.Get_CurrentState());
        UCk_Utils_State_UE::Request_Enter(CurrentStateHandle);
        InHandle.Remove<FTag_StateMachine_Transition>();

        hfsm::VeryVerbose(TEXT("[TRANSITION][STATEMACHINE] From [{}] To [{}]"), PreviousStateHandle, CurrentStateHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_StateMachine_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_StateMachine_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_StateMachine_Evaluate_StateMachine>();

        const auto CurrentStateHandle = UCk_Utils_State_UE::CastChecked(InCurrent.Get_CurrentState());

        if (UCk_Utils_State_UE::Get_IsReadyToTransition(CurrentStateHandle))
        {
            InCurrent._PreviousState = InCurrent.Get_CurrentState();
            InCurrent._CurrentState = UCk_Utils_State_UE::Get_NextState(CurrentStateHandle);

            auto PreviousStateHandle = UCk_Utils_State_UE::CastChecked(InCurrent.Get_PreviousState());
            UCk_Utils_State_UE::Request_Exit(PreviousStateHandle);

            InHandle.AddOrGet<FTag_StateMachine_Transition>();

            {
#if STATS
                auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
                UUtils_Signal_OnStateMachineTransition::Broadcast(InHandle,
                    MakePayload(InHandle, PreviousStateHandle, CurrentStateHandle));
            }

            hfsm::VeryVerbose(TEXT("[STATEMACHINE][Ready-To-Transition] From [{}] To [{}]"), InCurrent.Get_PreviousState(), InCurrent.Get_CurrentState());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------