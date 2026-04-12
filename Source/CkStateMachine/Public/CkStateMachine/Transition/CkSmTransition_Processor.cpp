#include "CkSmTransition_Processor.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmTransition_EvaluateFromConditions);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmTransition_TryFire);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // TRANSITION EVALUATE FROM CONDITIONS
    // ================================================================================================================

    auto
        FProcessor_SmTransition_EvaluateFromConditions::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmTransition_Current& InCurrent)
        -> void
    {
        auto HasAnyConditions = false;
        auto AnyUndetermined = false;
        auto AnyFail = false;

        UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_ValidEntry(InHandle,
            [&](FCk_Handle_SmCondition InCondition)
            {
                if (NOT InCondition.Has<FFragment_SmCondition_Current>())
                { return; }

                HasAnyConditions = true;

                const auto& ConditionCurrent = InCondition.Get<FFragment_SmCondition_Current>();

                switch (ConditionCurrent.Get_Result())
                {
                case ECk_SmConditionResult::Undetermined:
                {
                    AnyUndetermined = true;
                    break;
                }
                case ECk_SmConditionResult::Fail:
                {
                    AnyFail = true;
                    break;
                }
                case ECk_SmConditionResult::Pass:
                {
                    break;
                }
                }
            });

        // ----

        if (NOT HasAnyConditions)
        {
            // Vacuous AND — no conditions means always pass
            InCurrent._Result = ECk_SmTransitionResult::Pass;
            InHandle.Remove<FTag_SmTransition_Evaluating>();
            return;
        }

        if (AnyUndetermined)
        {
            // Not all conditions have resolved yet — stay undetermined, wait for next pump
            return;
        }

        if (AnyFail)
        {
            InCurrent._Result = ECk_SmTransitionResult::Fail;
            InHandle.Remove<FTag_SmTransition_Evaluating>();
            return;
        }

        // All conditions passed
        InCurrent._Result = ECk_SmTransitionResult::Pass;
        InHandle.Remove<FTag_SmTransition_Evaluating>();
    }

    // ================================================================================================================
    // TRY FIRE
    // ================================================================================================================

    auto
        FProcessor_SmTransition_TryFire::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Current& InCurrent)
        -> void
    {
        auto StateHandle = InCurrent.Get_CurrentStateHandle();

        if (ck::Is_NOT_Valid(StateHandle))
        { return; }

        const auto IsStateTicking = StateHandle.Has<FTag_SmState_Ticking>();

        struct FTransitionCandidate
        {
            FCk_Handle Handle;
            int32 Order;
            TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;
        };

        auto Transitions = TArray<FTransitionCandidate>{};

        UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::ForEach_ValidEntry(StateHandle,
            [&](FCk_Handle_SmTransition InTransition)
            {
                if (NOT InTransition.Has<FFragment_SmTransition_Params>())
                { return; }

                const auto& TransitionParams = InTransition.Get<FFragment_SmTransition_Params>();
                Transitions.Add(FTransitionCandidate{
                    InTransition,
                    TransitionParams.Get_Order(),
                    TransitionParams.Get_TargetStateClass()
                });
            });

        Transitions.Sort([](const FTransitionCandidate& A, const FTransitionCandidate& B)
        {
            return A.Order < B.Order;
        });

        for (const auto& Transition : Transitions)
        {
            if (NOT Transition.Handle.Has<FFragment_SmTransition_Current>())
            {
                if (IsStateTicking)
                {
                    // Ticking state: transition has not yet been set up — start evaluating
                    DoMarkTransitionAs_StartEvaluating(Transition.Handle);
                }
                // Event-driven state: transitions are woken externally via MarkConditionAs_Satisfied.
                // Do not kick off polling.
                continue;
            }

            const auto& TransitionCurrent = Transition.Handle.Get<FFragment_SmTransition_Current>();

            switch (TransitionCurrent.Get_Result())
            {
            case ECk_SmTransitionResult::Undetermined:
            {
                if (IsStateTicking)
                {
                    // Ticking state: still evaluating — kick off evaluation if not already in progress
                    if (NOT Transition.Handle.Has<FTag_SmTransition_Evaluating>())
                    {
                        DoMarkTransitionAs_StartEvaluating(Transition.Handle);
                    }
                }
                // Event-driven state: Undetermined means no external wake has occurred yet. Do nothing.
                continue;
            }
            case ECk_SmTransitionResult::Fail:
            {
                continue;
            }
            case ECk_SmTransitionResult::Pass:
            {
#if !UE_BUILD_SHIPPING
                if (InHandle.Has<FFragment_Sm_Breakpoints>())
                {
                    const auto& Breakpoints = InHandle.Get<FFragment_Sm_Breakpoints>();
                    auto TransitionKey = FFragment_Sm_Breakpoints::FTransitionKey{
                        InCurrent.Get_CurrentStateClass(), Transition.TargetStateClass};

                    if (Breakpoints.Get_TransitionBreakpoints().Contains(TransitionKey))
                    {
                        auto& HitFrag = InHandle.AddOrGet<FFragment_Sm_Debug_BreakpointHit>();
                        HitFrag.Description = TEXT("Transition: ")
                            + InCurrent.Get_CurrentStateClass()->GetName()
                            + TEXT(" \u2192 ")
                            + Transition.TargetStateClass->GetName();
                        HitFrag.RealTimeSeconds = FPlatformTime::Seconds();
                        UCk_Utils_EditorOnly_UE::Request_DebugPauseExecution();
                    }
                }
#endif

                auto& SmRequests = InHandle.AddOrGet<FFragment_Sm_Requests>();
                SmRequests._Requests.Add(FCk_Request_Sm_Transition{Transition.TargetStateClass});

                InHandle.Add<FTag_Sm_TransitionQueued>();

#if !UE_BUILD_SHIPPING
                {
                    auto& LastFired = InHandle.AddOrGet<FFragment_Sm_Debug_LastFiredTransition>();
                    LastFired.Order = Transition.Order;
                    LastFired.RealTimeSeconds = FPlatformTime::Seconds();
                    LastFired.ConditionNames.Reset();

                    auto TransitionHandleCopy = Transition.Handle;
                    UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_ValidEntry(TransitionHandleCopy,
                        [&](FCk_Handle_SmCondition InCondition)
                        {
                            if (NOT InCondition.Has<FFragment_EntityScript_Current>())
                            { return; }

                            auto* CondScript = InCondition.Get<FFragment_EntityScript_Current>().Get_Script().Get();

                            if (ck::IsValid(CondScript))
                            {
                                auto CondName = CondScript->GetClass()->GetName();
                                CondName.RemoveFromStart(TEXT("BP_"));
                                CondName.RemoveFromEnd(TEXT("_C"));
                                LastFired.ConditionNames.Add(MoveTemp(CondName));
                            }
                        });
                }
#endif

                ck::sm::Verbose(TEXT("SM [{}] transition queued to [{}] (order [{}])"),
                    InHandle, Transition.TargetStateClass->GetName(), Transition.Order);

                return;
            }
            }
        }
    }

    auto
        FProcessor_SmTransition_TryFire::
        DoMarkTransitionAs_StartEvaluating(
            FCk_Handle InTransitionHandle)
        -> void
    {
        auto& TransitionCurrent = InTransitionHandle.AddOrGet<FFragment_SmTransition_Current>();
        TransitionCurrent._Result = ECk_SmTransitionResult::Undetermined;
        InTransitionHandle.AddOrGet<FTag_SmTransition_Evaluating>();

        // Reset ResetEveryFrame conditions so the pump evaluates them fresh
        UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_ValidEntry(InTransitionHandle,
            [](FCk_Handle_SmCondition InCondition)
            {
                if (NOT InCondition.Has<FFragment_SmCondition_Current>())
                { return; }

                auto& ConditionCurrent = InCondition.Get<FFragment_SmCondition_Current>();

                if (ConditionCurrent.Get_ResetBehavior() == ECk_SmConditionResetBehavior::ResetEveryFrame)
                {
                    ConditionCurrent._Result = ECk_SmConditionResult::Undetermined;
                }
            });
    }
}

// --------------------------------------------------------------------------------------------------------------------
