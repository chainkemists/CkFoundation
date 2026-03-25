#include "CkStateMachine_Debug_Processor.h"

#if CK_BUILD_SM_GRAPH_WALK
#include "CkStateMachine/CkStateMachine_Debug_GraphWalk_Fragment.h"
#endif

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

#include "CkStateMachine/EntityScripts/CkSmState_EntityScript.h"

// --------------------------------------------------------------------------------------------------------------------

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

            constexpr auto MaxCompletedRuns = 10;

            if (Debug._CompletedRuns.Num() > MaxCompletedRuns)
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
            const auto& GraphDef = InHandle.Get<FFragment_Sm_Debug_GraphDefinition>();

            if (GraphDef.Get_IsComplete())
            {
                for (const auto& [StateClass, StateDef] : GraphDef.Get_StateDefinitions())
                {
                    if (Debug._CachedStates.Contains(StateClass))
                    { continue; }

                    auto CachedState = FCk_SmDebug_CachedState{};
                    CachedState.StateClass = StateClass;
                    CachedState.StateName = StateDef.StateName;

                    for (const auto& TransDef : StateDef.Transitions)
                    {
                        auto CachedTrans = FCk_SmDebug_CachedTransition{};
                        CachedTrans.SourceStateClass = StateClass;
                        CachedTrans.TargetStateClass = TransDef.TargetStateClass;
                        CachedTrans.Order = TransDef.Order;
                        CachedState.Transitions.Add(MoveTemp(CachedTrans));
                    }

                    for (const auto& TaskDef : StateDef.Tasks)
                    {
                        auto CachedTask = FCk_SmDebug_CachedTask{};
                        CachedTask.ClassName = TaskDef.ClassName;
                        CachedTask.Mode = TaskDef.Mode;
                        CachedTask.HasSubStateMachine = TaskDef.HasSubStateMachine;
                        CachedTask.SubSmInitialStateClass = TaskDef.SubSmInitialStateClass;
                        CachedState.Tasks.Add(MoveTemp(CachedTask));
                    }

                    Debug._CachedStates.Add(StateClass, MoveTemp(CachedState));
                }
            }
        }
#endif

        auto CurrentStateClass = InCurrent.Get_CurrentStateClass();

        // Ensure initial state class always has a cache entry
        auto InitialStateClass = InParams.Get_InitialStateClass();

        if (IsValid(InitialStateClass))
        {
            if (NOT Debug._CachedStates.Contains(InitialStateClass))
            {
                auto CachedState = FCk_SmDebug_CachedState{};
                CachedState.StateClass = InitialStateClass;
                CachedState.StateName = GetCleanClassName(InitialStateClass);
                Debug._CachedStates.Add(InitialStateClass, MoveTemp(CachedState));
            }
        }

        // Detect state change and record history
        if (IsValid(CurrentStateClass) && CurrentStateClass != Debug._LastObservedStateClass)
        {
            if (IsValid(Debug._LastObservedStateClass))
            {
                auto Entry = FCk_SmDebug_HistoryEntry{};
                Entry.FromStateClass = Debug._LastObservedStateClass;
                Entry.ToStateClass = CurrentStateClass;
                Entry.FromStateName = GetCleanClassName(Debug._LastObservedStateClass);
                Entry.ToStateName = GetCleanClassName(CurrentStateClass);
                Entry.FrameNumber = GFrameCounter;

#if !UE_BUILD_SHIPPING
                if (InHandle.Has<FFragment_Sm_Debug_LastFiredTransition>())
                {
                    const auto& LastFired = InHandle.Get<FFragment_Sm_Debug_LastFiredTransition>();
                    Entry.TransitionOrder = LastFired.Order;
                    Entry.TransitionConditionNames = LastFired.ConditionNames;
                    Entry.RealTimeSeconds = LastFired.RealTimeSeconds;
                    InHandle.Remove<FFragment_Sm_Debug_LastFiredTransition>();
                }
                else
                {
                    Entry.TransitionOrder = -1;
                    Entry.RealTimeSeconds = FPlatformTime::Seconds();
                }
#endif

                if (Debug._CachedStates.Contains(Debug._LastObservedStateClass))
                {
                    const auto& CachedFrom = Debug._CachedStates[Debug._LastObservedStateClass];

                    for (const auto& Task : CachedFrom.Tasks)
                    {
                        auto Snapshot = FCk_SmDebug_HistoryTaskSnapshot{};
                        Snapshot.TaskName = Task.ClassName;
                        Snapshot.Result = Task.LastResult;
                        Entry.TaskSnapshots.Add(MoveTemp(Snapshot));
                    }
                }

                Debug._History.Add(MoveTemp(Entry));
            }

            Debug._LastObservedStateClass = CurrentStateClass;
            Debug._CurrentStateEnteredAtRealTime = FPlatformTime::Seconds();
        }

        // Cache current state data from live entities
        if (IsValid(CurrentStateClass) && ck::IsValid(InCurrent.Get_CurrentStateHandle()))
        {
            DoCacheCurrentState(InHandle, Debug, InCurrent);
        }
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
        auto CurrentStateClass = InCurrent.Get_CurrentStateClass();
        auto StateHandle = static_cast<FCk_Handle>(InCurrent.Get_CurrentStateHandle());

        auto& CachedState = InDebug._CachedStates.FindOrAdd(CurrentStateClass);
        CachedState.StateClass = CurrentStateClass;
        CachedState.StateName = GetCleanClassName(CurrentStateClass);
        CachedState.Transitions.Reset();
        CachedState.Tasks.Reset();

        auto StateChildren = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(StateHandle);

        for (const auto& ChildHandle : StateChildren)
        {
            // Cache transitions
            if (ChildHandle.Has<FFragment_SmTransition_Params>())
            {
                const auto& TransParams = ChildHandle.Get<FFragment_SmTransition_Params>();

                auto CachedTransition = FCk_SmDebug_CachedTransition{};
                CachedTransition.SourceStateClass = CurrentStateClass;
                CachedTransition.TargetStateClass = TransParams.Get_TargetStateClass();
                CachedTransition.Order = TransParams.Get_Order();

                // Cache conditions on this transition
                auto TransChildren = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(ChildHandle);

                for (const auto& CondHandle : TransChildren)
                {
                    if (NOT CondHandle.Has<FFragment_SmCondition_Current>())
                    { continue; }

                    auto CachedCondition = FCk_SmDebug_CachedCondition{};
                    CachedCondition.ResetBehavior = CondHandle.Get<FFragment_SmCondition_Current>().Get_ResetBehavior();

                    if (CondHandle.Has<FTag_SmCondition_Polled>())
                    {
                        CachedCondition.Mode = ECk_SmConditionMode::Polled;
                    }
                    else
                    {
                        CachedCondition.Mode = ECk_SmConditionMode::EventDriven;
                    }

                    if (CondHandle.Has<FFragment_EntityScript_Current>())
                    {
                        auto* CondScript = CondHandle.Get<FFragment_EntityScript_Current>().Get_Script().Get();

                        if (IsValid(CondScript))
                        {
                            CachedCondition.ClassName = GetCleanClassName(CondScript->GetClass());
                        }
                    }

                    CachedTransition.Conditions.Add(MoveTemp(CachedCondition));
                }

                CachedState.Transitions.Add(MoveTemp(CachedTransition));

                // Ensure target state class has a cache entry
                auto TargetClass = TransParams.Get_TargetStateClass();

                if (IsValid(TargetClass) && NOT InDebug._CachedStates.Contains(TargetClass))
                {
                    auto TargetCachedState = FCk_SmDebug_CachedState{};
                    TargetCachedState.StateClass = TargetClass;
                    TargetCachedState.StateName = GetCleanClassName(TargetClass);
                    InDebug._CachedStates.Add(TargetClass, MoveTemp(TargetCachedState));
                }
            }

            // Cache tasks
            if (ChildHandle.Has<FFragment_SmTask_Current>())
            {
                auto CachedTask = FCk_SmDebug_CachedTask{};

                if (ChildHandle.Has<FTag_SmTask_Tick>())
                {
                    CachedTask.Mode = ECk_SmTaskMode::Tick;
                }
                else
                {
                    CachedTask.Mode = ECk_SmTaskMode::EnterExitOnly;
                }

                if (ChildHandle.Has<FFragment_EntityScript_Current>())
                {
                    auto* TaskScript = ChildHandle.Get<FFragment_EntityScript_Current>().Get_Script().Get();

                    if (IsValid(TaskScript))
                    {
                        CachedTask.ClassName = GetCleanClassName(TaskScript->GetClass());
                    }
                }

                if (ChildHandle.Has<FFragment_SmTask_SubStateMachine>())
                {
                    const auto& SubSmFrag = ChildHandle.Get<FFragment_SmTask_SubStateMachine>();
                    CachedTask.HasSubStateMachine = true;
                    CachedTask.SubSmHandle = SubSmFrag.Get_SubStateMachineHandle();

                    if (ck::IsValid(CachedTask.SubSmHandle)
                        && static_cast<FCk_Handle>(CachedTask.SubSmHandle).Has<FFragment_Sm_Params>())
                    {
                        CachedTask.SubSmInitialStateClass =
                            static_cast<FCk_Handle>(CachedTask.SubSmHandle)
                                .Get<FFragment_Sm_Params>().Get_InitialStateClass();
                    }
                }

                CachedTask.LastResult = ChildHandle.Get<FFragment_SmTask_Current>().Get_LastResult();

                CachedState.Tasks.Add(MoveTemp(CachedTask));
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_Debug::
        GetCleanClassName(
            const UClass* InClass)
        -> FString
    {
        if (NOT IsValid(InClass))
        { return TEXT("(unknown)"); }

        auto Name = InClass->GetName();
        Name.RemoveFromStart(TEXT("BP_"));
        Name.RemoveFromEnd(TEXT("_C"));
        return Name;
    }
}

// --------------------------------------------------------------------------------------------------------------------
