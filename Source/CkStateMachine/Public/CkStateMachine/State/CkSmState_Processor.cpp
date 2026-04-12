#include "CkSmState_Processor.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment_Data.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmState_Evaluate);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_SmState_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
        -> void
    {
        auto InSmHandle = TUtils_Sm_OwningStateMachine::Get_StoredEntity(InHandle);

        if (ck::Is_NOT_Valid(InSmHandle))
        { return; }

        if (NOT InSmHandle.Has<FTag_Sm_Running>()
            || InSmHandle.Has<FTag_Sm_Paused>()
            || InSmHandle.Has<FTag_Sm_TransitionQueued>())
        { return; }

        // ---- Walk transitions in record order (insertion order = priority order) ----

        UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::ForEach_ValidEntry(InHandle,
            [&](FCk_Handle_SmTransition InTransition) -> ECk_Record_ForEachIterationResult
            {
                if (NOT InTransition.Has<FFragment_SmTransition_Params>())
                { return ECk_Record_ForEachIterationResult::Continue; }

                switch (UCk_Utils_SmTransition_UE::Get_EvaluationResult(InTransition))
                {
                case ECk_SmTransitionResult::Undetermined:
                {
                    UCk_Utils_SmTransition_UE::Request_StartOrResumeEvaluating(InTransition);
                    return ECk_Record_ForEachIterationResult::Break;
                }
                case ECk_SmTransitionResult::Fail:
                {
                    return ECk_Record_ForEachIterationResult::Continue;
                }
                case ECk_SmTransitionResult::Pass:
                {
                    const auto TargetStateClass = UCk_Utils_SmTransition_UE::Get_TargetStateClass(InTransition);

                    // ---- Debug breakpoint ----
#if !UE_BUILD_SHIPPING
                    if (InSmHandle.Has<FFragment_Sm_Breakpoints>())
                    {
                        const auto& SmCurrent = InSmHandle.Get<FFragment_Sm_Current>();
                        const auto TransitionKey = FFragment_Sm_Breakpoints::FTransitionKey{
                            SmCurrent.Get_CurrentStateClass(), TargetStateClass};

                        if (InSmHandle.Get<FFragment_Sm_Breakpoints>().Get_TransitionBreakpoints().Contains(TransitionKey))
                        {
                            auto& HitFrag = InSmHandle.AddOrGet<FFragment_Sm_Debug_BreakpointHit>();
                            HitFrag.Description = ck::Format_UE(TEXT("Transition: {} \u2192 {}"),
                                SmCurrent.Get_CurrentStateClass()->GetName(), TargetStateClass->GetName());
                            HitFrag.RealTimeSeconds = FPlatformTime::Seconds();
                            UCk_Utils_EditorOnly_UE::Request_DebugPauseExecution();
                        }
                    }
#endif

                    // ---- Queue the transition ----
                    UCk_Utils_StateMachine_UE::Request_Transition(InSmHandle, TargetStateClass);
                    InSmHandle.AddOrGet<FTag_Sm_TransitionQueued>();

                    // ---- Debug last-fired info ----
#if !UE_BUILD_SHIPPING
                    {
                        auto& LastFired = InSmHandle.AddOrGet<FFragment_Sm_Debug_LastFiredTransition>();
                        LastFired.RealTimeSeconds = FPlatformTime::Seconds();
                        LastFired.ConditionNames.Reset();

                        UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_ValidEntry(InTransition,
                            [&](FCk_Handle_SmCondition InCondition) -> ECk_Record_ForEachIterationResult
                            {
                                if (NOT InCondition.Has<FFragment_EntityScript_Current>())
                                { return ECk_Record_ForEachIterationResult::Continue; }

                                auto* CondScript = InCondition.Get<FFragment_EntityScript_Current>().Get_Script().Get();

                                if (ck::IsValid(CondScript))
                                {
                                    auto CondName = CondScript->GetClass()->GetName();
                                    CondName.RemoveFromStart(TEXT("BP_"));
                                    CondName.RemoveFromEnd(TEXT("_C"));
                                    LastFired.ConditionNames.Add(MoveTemp(CondName));
                                }

                                return ECk_Record_ForEachIterationResult::Continue;
                            });
                    }
#endif

                    ck::sm::Verbose(TEXT("SM [{}] transition queued to [{}]"),
                        InSmHandle, TargetStateClass->GetName());

                    return ECk_Record_ForEachIterationResult::Break;
                }
                }

                return ECk_Record_ForEachIterationResult::Continue;
            });
    }
}

// --------------------------------------------------------------------------------------------------------------------
