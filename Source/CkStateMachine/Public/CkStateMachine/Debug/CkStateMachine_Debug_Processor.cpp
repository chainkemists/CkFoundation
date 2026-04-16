#include "CkStateMachine_Debug_Processor.h"

#if CK_BUILD_SM_GRAPH_WALK
#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"
#endif

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/Task/CkSmTask_Utils.h"
#include "CkStateMachine/Condition/CkSmCondition_Utils.h"

#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/Task/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_Debug);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmDebug_HandleRequests);

namespace ck
{
    auto
        FProcessor_Sm_Debug::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Current& InCurrent,
            const FFragment_Sm_Params& InParams)
        -> void
    {
        auto& Debug = InHandle.AddOrGet<FFragment_Sm_Debug>();

        // Run lifecycle detection
        auto RunStatus = InCurrent.Get_RunStatus();

        if (Debug._LastObservedRunStatus == ECk_SmRunStatus::Stopped
            && RunStatus == ECk_SmRunStatus::Running)
        {
            Debug._RunCounter++;
            Debug._CurrentRunStartRealTime = FPlatformTime::Seconds();
            Debug._History.Reset();
            Debug._LastObservedStateClass = nullptr;
        }
        else if (Debug._LastObservedRunStatus != ECk_SmRunStatus::Stopped
            && RunStatus == ECk_SmRunStatus::Stopped)
        {
            auto RunInfo = FCk_SmDebug_RunInfo{};
            RunInfo.RunIndex = Debug._RunCounter;
            RunInfo.StartRealTimeSeconds = Debug._CurrentRunStartRealTime;
            RunInfo.EndRealTimeSeconds = FPlatformTime::Seconds();
            RunInfo.History = Debug._History;
            Debug._CompletedRuns.Add(MoveTemp(RunInfo));

            if (constexpr auto MaxCompletedRuns = 10;
                Debug._CompletedRuns.Num() > MaxCompletedRuns)
            {
                Debug._CompletedRuns.RemoveAt(0);
            }

            Debug._History.Reset();
            Debug._LastObservedStateClass = nullptr;
        }

        Debug._LastObservedRunStatus = RunStatus;

#if CK_BUILD_SM_GRAPH_WALK
        if (InHandle.Has<FFragment_Sm_Debug_GraphDefinition>())
        {
            if (const auto& GraphDef = InHandle.Get<FFragment_Sm_Debug_GraphDefinition>();
                GraphDef.Get_IsComplete())
            {
                for (const auto& [StateClass, StateDef] : GraphDef.Get_StateDefinitions())
                {
                    // Resolve through override map so cache uses the same class runtime produces
                    const auto ResolvedClass = UCk_Utils_SmState_UE::Get_ResolvedStateClass(
                        InHandle, StateClass);

                    if (Debug._CachedStates.Contains(ResolvedClass))
                    { continue; }

                    auto CachedState = FCk_SmDebug_CachedState{};
                    CachedState.StateClass = ResolvedClass;
                    CachedState.StateName = UCk_Utils_Object_UE::Get_CleanClassName(ResolvedClass);

                    for (const auto& [TargetStateClass] : StateDef.Transitions)
                    {
                        const auto ResolvedTarget = UCk_Utils_SmState_UE::Get_ResolvedStateClass(
                            InHandle, TargetStateClass);

                        auto CachedTrans = FCk_SmDebug_CachedTransition{};
                        CachedTrans.SourceStateClass = ResolvedClass;
                        CachedTrans.TargetStateClass = ResolvedTarget;
                        CachedState.Transitions.Add(MoveTemp(CachedTrans));
                    }

                    for (const auto& [ClassName, Mode, HasSubStateMachine, SubSmInitialStateClass] : StateDef.Tasks)
                    {
                        auto CachedTask = FCk_SmDebug_CachedTask{};
                        CachedTask.ClassName = ClassName;
                        CachedTask.Mode = Mode;
                        CachedTask.HasSubStateMachine = HasSubStateMachine;
                        CachedTask.SubSmInitialStateClass = SubSmInitialStateClass;
                        CachedState.Tasks.Add(MoveTemp(CachedTask));
                    }

                    Debug._CachedStates.Add(ResolvedClass, MoveTemp(CachedState));
                }
            }
        }
#endif

        auto CurrentStateClass = InCurrent.Get_CurrentStateClass();

        // Ensure initial state class always has a cache entry (resolved through overrides)

        if (auto InitialStateClass = InParams.Get_InitialStateClass();
            ck::IsValid(InitialStateClass))
        {
            const auto ResolvedInitial = UCk_Utils_SmState_UE::Get_ResolvedStateClass(
                InHandle, InitialStateClass);

            if (NOT Debug._CachedStates.Contains(ResolvedInitial))
            {
                auto CachedState = FCk_SmDebug_CachedState{};
                CachedState.StateClass = ResolvedInitial;
                CachedState.StateName = UCk_Utils_Object_UE::Get_CleanClassName(ResolvedInitial);
                Debug._CachedStates.Add(ResolvedInitial, MoveTemp(CachedState));
            }
        }

        // Track the observed current state for caching / initial-state bookkeeping. History
        // is now fully driven by FFragment_SmDebug_Requests — the debug handle-requests
        // processor drains them, so we don't poll-and-record here.
        if (ck::IsValid(CurrentStateClass) && CurrentStateClass != Debug._LastObservedStateClass)
        {
            Debug._LastObservedStateClass = CurrentStateClass;
            Debug._CurrentStateEnteredAtRealTime = FPlatformTime::Seconds();
        }

        // Cache current state data from live entities
        if (ck::IsValid(CurrentStateClass) && ck::IsValid(InCurrent.Get_CurrentStateHandle()))
        {
            DoCacheCurrentState(InHandle, Debug, InCurrent);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SmDebug_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SmDebug_Requests& InRequests) const
        -> void
    {
        InHandle.CopyAndRemove(InRequests, [&](FFragment_SmDebug_Requests& InRequestsCopy)
        {
            algo::ForEachRequest(InRequestsCopy._Requests, Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InRequest);
            }));
        });
    }

    auto
        FProcessor_SmDebug_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_SmDebug_RecordTransition& InRequest)
        -> void
    {
        // No debug fragment yet means the polling processor hasn't initialized caches —
        // we still want the history so AddOrGet here. Cache-dependent fields just miss.
        auto& DebugFragment = InHandle.AddOrGet<FFragment_Sm_Debug>();
        const auto FromClass = InRequest.Get_FromStateClass();
        const auto ToClass   = InRequest.Get_ToStateClass();

        if (ck::Is_NOT_Valid(ToClass))
        { return; }

        // A record with no FromClass represents a sub-SM's initial-state entry routed up to this
        // parent SM. Emit a bare history entry (no task snapshots, no sub-SM history copy, and
        // don't touch _LastObservedStateClass since this record is *about* the sub-SM, not this SM).
        if (ck::Is_NOT_Valid(FromClass))
        {
            auto SubSmEntry = FCk_SmDebug_HistoryEntry{};
            SubSmEntry.ToStateClass             = ToClass;
            SubSmEntry.ToStateName              = UCk_Utils_Object_UE::Get_CleanClassName(ToClass);
            SubSmEntry.FromStateName            = TEXT("(start)");
            SubSmEntry.FrameNumber              = static_cast<uint64>(InRequest.Get_FrameNumber());
            SubSmEntry.TransitionConditionNames = InRequest.Get_ConditionNames();
            SubSmEntry.RealTimeSeconds          = InRequest.Get_RealTimeSeconds();
            SubSmEntry.SubSmParentStateName     = InRequest.Get_SubSmParentStateName();

            DebugFragment._History.Add(MoveTemp(SubSmEntry));
            return;
        }

        auto Entry = FCk_SmDebug_HistoryEntry{};
        Entry.FromStateClass = FromClass;
        Entry.ToStateClass   = ToClass;
        Entry.FromStateName  = UCk_Utils_Object_UE::Get_CleanClassName(FromClass);
        Entry.ToStateName    = UCk_Utils_Object_UE::Get_CleanClassName(ToClass);
        Entry.FrameNumber    = static_cast<uint64>(InRequest.Get_FrameNumber());
        Entry.TransitionConditionNames = InRequest.Get_ConditionNames();
        Entry.RealTimeSeconds          = InRequest.Get_RealTimeSeconds();

        if (DebugFragment._CachedStates.Contains(FromClass))
        {
            for (const auto& CachedFrom = DebugFragment._CachedStates[FromClass];
                const auto& Task : CachedFrom.Tasks)
            {
                auto Snapshot = FCk_SmDebug_HistoryTaskSnapshot{};
                Snapshot.TaskName = Task.ClassName;
                Snapshot.Result   = Task.LastResult;
                Entry.TaskSnapshots.Add(MoveTemp(Snapshot));

                // Persist sub-SM history into the parent before the sub-SM entity is destroyed
                if (Task.HasSubStateMachine
                    && ck::IsValid(Task.SubSmHandle)
                    && Task.SubSmHandle.Has<FFragment_Sm_Debug>())
                {
                    for (auto SubEntry : Task.SubSmHandle.Get<FFragment_Sm_Debug>().Get_History())
                    {
                        SubEntry.SubSmParentStateName = CachedFrom.StateName;
                        DebugFragment._History.Add(MoveTemp(SubEntry));
                    }
                }
            }
        }

        DebugFragment._History.Add(MoveTemp(Entry));
        DebugFragment._LastObservedStateClass = ToClass;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_Debug::
        DoCacheCurrentState(
            HandleType InHandle,
            FFragment_Sm_Debug& InDebug,
            const FFragment_Sm_Current& InCurrent)
        -> void
    {
        const auto CurrentStateClass = InCurrent.Get_CurrentStateClass();
        auto StateHandle = InCurrent.Get_CurrentStateHandle();

        auto& [StateClass, ScriptClass, RequestedScriptClass, StateName, Transitions, Tasks] =
            InDebug._CachedStates.FindOrAdd(CurrentStateClass);
        StateClass = CurrentStateClass;
        StateName = UCk_Utils_Object_UE::Get_CleanClassName(CurrentStateClass);
        Transitions.Reset();
        Tasks.Reset();

        // Capture the override-aware actual entity script class plus the originally-requested one.
        // ScriptClass may differ from CurrentStateClass / RequestedScriptClass when
        // FFragment_Sm_StateOverrides remaps the state at runtime.
        ScriptClass = UCk_Utils_SmState_UE::Get_ScriptClass(StateHandle);
        RequestedScriptClass = UCk_Utils_SmState_UE::Get_RequestedScriptClass(StateHandle);

        // ---- Cache transitions via record ----

        UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::ForEach_ValidEntry(StateHandle,
        [&](FCk_Handle_SmTransition InTransition)
        {
            const auto& TransParams = InTransition.Get<FFragment_SmTransition_Params>();

            // Resolve the transition target through the SM's override table. Transitions are declared
            // against the *requested* state class (e.g. Interactable_Focused) but the SM actually enters
            // the *resolved* class (e.g. Shelf_Stock_Focused). Without this, the cache ends up with
            // separate entries for the requested and resolved classes — visible as a duplicate node
            // in the debugger graph.
            const auto ResolvedTargetClass = UCk_Utils_SmState_UE::Get_ResolvedStateClass(
                InHandle, TransParams.Get_TargetStateClass());

            auto CachedTransition = FCk_SmDebug_CachedTransition{};
            CachedTransition.SourceStateClass = CurrentStateClass;
            CachedTransition.TargetStateClass = ResolvedTargetClass;

            // Cache conditions on this transition

            UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_ValidEntry(InTransition,
            [&](FCk_Handle_SmCondition InCondition)
            {
                auto CachedCondition = FCk_SmDebug_CachedCondition{};

                if (InCondition.Has<FTag_SmCondition_Polled>())
                {
                    CachedCondition.Mode = ECk_SmConditionMode::Polled;
                }
                else
                {
                    CachedCondition.Mode = ECk_SmConditionMode::EventDriven;
                }

                CachedCondition.ScriptClass = UCk_Utils_SmCondition_UE::Get_ScriptClass(InCondition);
                if (ck::IsValid(CachedCondition.ScriptClass))
                {
                    CachedCondition.ClassName = UCk_Utils_Object_UE::Get_CleanClassName(CachedCondition.ScriptClass);
                }

                CachedTransition.Conditions.Add(MoveTemp(CachedCondition));
            });

            Transitions.Add(MoveTemp(CachedTransition));

            // Ensure target state class has a cache entry. Use the override-resolved class so a
            // transition and its eventually-entered state share a single cache entry.

            if (ck::IsValid(ResolvedTargetClass) && NOT InDebug._CachedStates.Contains(ResolvedTargetClass))
            {
                auto TargetCachedState = FCk_SmDebug_CachedState{};
                TargetCachedState.StateClass = ResolvedTargetClass;
                TargetCachedState.StateName = UCk_Utils_Object_UE::Get_CleanClassName(ResolvedTargetClass);
                InDebug._CachedStates.Add(ResolvedTargetClass, MoveTemp(TargetCachedState));
            }
        });

        // ---- Cache tasks via record ----

        UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::ForEach_ValidEntry(StateHandle,
        [&](FCk_Handle_SmTask InTask)
        {
            auto CachedTask = FCk_SmDebug_CachedTask{};

            if (InTask.Has<FTag_SmTask_Tick>())
            {
                CachedTask.Mode = ECk_SmTaskMode::Tick;
            }
            else
            {
                CachedTask.Mode = ECk_SmTaskMode::EnterExitOnly;
            }

            CachedTask.ScriptClass = UCk_Utils_SmTask_UE::Get_ScriptClass(InTask);
            if (ck::IsValid(CachedTask.ScriptClass))
            {
                CachedTask.ClassName = UCk_Utils_Object_UE::Get_CleanClassName(CachedTask.ScriptClass);
            }

            if (InTask.Has<FFragment_SmTask_SubStateMachine>())
            {
                const auto& SubSmFrag = InTask.Get<FFragment_SmTask_SubStateMachine>();
                CachedTask.HasSubStateMachine = true;
                CachedTask.SubSmHandle = SubSmFrag.Get_SubStateMachineHandle();

                if (ck::IsValid(CachedTask.SubSmHandle)
                    && CachedTask.SubSmHandle.Has<FFragment_Sm_Params>())
                {
                    CachedTask.SubSmInitialStateClass =
                        CachedTask.SubSmHandle.Get<FFragment_Sm_Params>().Get_InitialStateClass();
                }
            }

            CachedTask.LastResult = InTask.Get<FFragment_SmTask_Current>().Get_LastResult();

            Tasks.Add(MoveTemp(CachedTask));
        });
    }

    // ----------------------------------------------------------------------------------------------------------------

}

// --------------------------------------------------------------------------------------------------------------------
