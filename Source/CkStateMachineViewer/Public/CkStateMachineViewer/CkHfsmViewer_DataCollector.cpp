#include "CkHfsmViewer_DataCollector.h"

#include "CkStateMachineViewer/CkHfsmViewer_Log.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkStateMachine/CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/EntityScripts/CkSmState_EntityScript.h"

// --------------------------------------------------------------------------------------------------------------------

static auto
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

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    Collect(
        UWorld* InWorld)
    -> void
{
    _StateMachines.Reset();

    if (NOT IsValid(InWorld))
    { return; }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(InWorld);

    if (NOT ck::IsValid(TransientEntity))
    { return; }

    TransientEntity.View<ck::FFragment_Sm_Current, ck::FFragment_Sm_Params>().ForEach(
        [this, &TransientEntity](FCk_Entity InEntity, const ck::FFragment_Sm_Current&, const ck::FFragment_Sm_Params&)
        {
            auto Handle = ck::MakeHandle(InEntity, TransientEntity);
            _StateMachines.Add(CollectStateMachine(Handle));
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    Get_AllStateMachines() const
    -> const TArray<FCkHfsmViewer_SmInfo>&
{
    return _StateMachines;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    CollectStateMachine(
        const FCk_Handle& InSmHandle)
    -> FCkHfsmViewer_SmInfo
{
    auto SmInfo = FCkHfsmViewer_SmInfo{};
    SmInfo.Handle = ck::StaticCast<FCk_Handle_StateMachine>(InSmHandle);

    const auto& Current = InSmHandle.Get<ck::FFragment_Sm_Current>();
    SmInfo.RunStatus = Current.Get_RunStatus();
    SmInfo.CurrentStateClass = Current.Get_CurrentStateClass();
    SmInfo.IsTransitionQueued = InSmHandle.Has<ck::FTag_Sm_TransitionQueued>();

    if (InSmHandle.Has<ck::FFragment_Sm_Context>())
    {
        SmInfo.GameEntity = InSmHandle.Get<ck::FFragment_Sm_Context>().Get_GameEntityHandle();
    }

    if (ck::IsValid(SmInfo.GameEntity))
    {
        SmInfo.DebugName = UCk_Utils_Handle_UE::Get_DebugName(SmInfo.GameEntity).ToString();
    }

    if (SmInfo.DebugName.IsEmpty())
    {
        SmInfo.DebugName = UCk_Utils_Handle_UE::Get_DebugName(InSmHandle).ToString();
    }

    if (SmInfo.DebugName.IsEmpty())
    {
        SmInfo.DebugName = TEXT("(unnamed)");
    }

    // Build the graph from the debug fragment's cached data
    if (NOT InSmHandle.Has<ck::FFragment_Sm_Debug>())
    {
        return SmInfo;
    }

    const auto& Debug = InSmHandle.Get<ck::FFragment_Sm_Debug>();
    const auto& CachedStates = Debug.Get_CachedStates();

    auto StateClassToIndex = TMap<TSubclassOf<UCk_SmState_EntityScript>, int32>{};

    // Build state nodes from cache
    for (const auto& [StateClass, CachedState] : CachedStates)
    {
        auto StateInfo = FCkHfsmViewer_StateInfo{};
        StateInfo.StateClass = StateClass;
        StateInfo.StateName = CachedState.StateName;
        StateInfo.IsCurrentState = (StateClass == SmInfo.CurrentStateClass);

        // Copy cached tasks
        for (const auto& CachedTask : CachedState.Tasks)
        {
            auto TaskInfo = FCkHfsmViewer_TaskInfo{};
            TaskInfo.ClassName = CachedTask.ClassName;
            TaskInfo.Mode = CachedTask.Mode;
            StateInfo.Tasks.Add(MoveTemp(TaskInfo));
        }

        auto Index = SmInfo.States.Num();
        StateClassToIndex.Add(StateClass, Index);
        SmInfo.States.Add(MoveTemp(StateInfo));

        if (StateClass == SmInfo.CurrentStateClass)
        {
            SmInfo.CurrentStateIndex = Index;
        }
    }

    // Build transitions from cache
    for (const auto& [StateClass, CachedState] : CachedStates)
    {
        auto* SourceIndex = StateClassToIndex.Find(StateClass);

        if (NOT SourceIndex)
        { continue; }

        for (const auto& CachedTransition : CachedState.Transitions)
        {
            auto TransInfo = FCkHfsmViewer_TransitionInfo{};
            TransInfo.SourceStateIndex = *SourceIndex;
            TransInfo.Order = CachedTransition.Order;
            TransInfo.TargetStateClass = CachedTransition.TargetStateClass;

            if (IsValid(CachedTransition.TargetStateClass))
            {
                TransInfo.TargetStateName = GetCleanClassName(CachedTransition.TargetStateClass);
            }

            auto* TargetIndex = StateClassToIndex.Find(CachedTransition.TargetStateClass);
            TransInfo.TargetStateIndex = TargetIndex ? *TargetIndex : -1;

            // Build conditions from cache (no live satisfaction data yet)
            for (const auto& CachedCondition : CachedTransition.Conditions)
            {
                auto CondInfo = FCkHfsmViewer_ConditionInfo{};
                CondInfo.ClassName = CachedCondition.ClassName;
                CondInfo.Mode = CachedCondition.Mode;
                CondInfo.ResetBehavior = CachedCondition.ResetBehavior;
                CondInfo.IsSatisfied = false;
                TransInfo.Conditions.Add(MoveTemp(CondInfo));
            }

            TransInfo.TotalCount = TransInfo.Conditions.Num();
            TransInfo.SatisfiedCount = 0;
            TransInfo.AreAllConditionsSatisfied = false;

            SmInfo.Transitions.Add(MoveTemp(TransInfo));
        }
    }

    // Overlay live condition satisfaction for the current state's transitions
    auto CurrentStateHandle = Current.Get_CurrentStateHandle();

    if (ck::IsValid(CurrentStateHandle) && IsValid(SmInfo.CurrentStateClass))
    {
        OverlayLiveData(
            CurrentStateHandle,
            SmInfo.CurrentStateIndex,
            StateClassToIndex,
            SmInfo);
    }

    // Copy history
    for (const auto& HistoryEntry : Debug.Get_History())
    {
        auto ViewerEntry = FCkHfsmViewer_HistoryEntry{};
        ViewerEntry.FromStateName = HistoryEntry.FromStateName;
        ViewerEntry.ToStateName = HistoryEntry.ToStateName;
        ViewerEntry.FrameNumber = HistoryEntry.FrameNumber;
        SmInfo.History.Add(MoveTemp(ViewerEntry));
    }

    return SmInfo;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_DataCollector::
    OverlayLiveData(
        const FCk_Handle& InStateHandle,
        int32 InCurrentStateIndex,
        const TMap<TSubclassOf<UCk_SmState_EntityScript>, int32>& InStateClassToIndex,
        FCkHfsmViewer_SmInfo& InOutSmInfo)
    -> void
{
    auto StateChildren = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InStateHandle);

    auto TaskIndex = 0;

    for (const auto& ChildHandle : StateChildren)
    {
        // Overlay live condition data on transitions
        if (ChildHandle.Has<ck::FFragment_SmTransition_Params>())
        {
            const auto& TransParams = ChildHandle.Get<ck::FFragment_SmTransition_Params>();
            auto TargetClass = TransParams.Get_TargetStateClass();
            auto Order = TransParams.Get_Order();

            auto* MatchingTransition = static_cast<FCkHfsmViewer_TransitionInfo*>(nullptr);

            for (auto& Trans : InOutSmInfo.Transitions)
            {
                if (Trans.SourceStateIndex == InCurrentStateIndex
                    && Trans.TargetStateClass == TargetClass
                    && Trans.Order == Order)
                {
                    MatchingTransition = &Trans;
                    break;
                }
            }

            if (NOT MatchingTransition)
            { continue; }

            MatchingTransition->Handle = ChildHandle;

            auto TransChildren = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(ChildHandle);
            auto SatisfiedCount = 0;
            auto TotalCount = 0;
            auto ConditionIndex = 0;

            for (const auto& CondHandle : TransChildren)
            {
                if (NOT CondHandle.Has<ck::FFragment_SmCondition_Current>())
                { continue; }

                const auto& CondCurrent = CondHandle.Get<ck::FFragment_SmCondition_Current>();
                auto IsSatisfied = CondCurrent.Get_IsSatisfied();

                ++TotalCount;

                if (IsSatisfied)
                {
                    ++SatisfiedCount;
                }

                if (ConditionIndex < MatchingTransition->Conditions.Num())
                {
                    MatchingTransition->Conditions[ConditionIndex].Handle = CondHandle;
                    MatchingTransition->Conditions[ConditionIndex].IsSatisfied = IsSatisfied;
                }

                ++ConditionIndex;
            }

            MatchingTransition->TotalCount = TotalCount;
            MatchingTransition->SatisfiedCount = SatisfiedCount;
            MatchingTransition->AreAllConditionsSatisfied = (TotalCount > 0 && SatisfiedCount == TotalCount);
        }

        // Overlay live task results
        if (ChildHandle.Has<ck::FFragment_SmTask_Current>())
        {
            auto& CurrentState = InOutSmInfo.States[InCurrentStateIndex];

            if (TaskIndex < CurrentState.Tasks.Num())
            {
                CurrentState.Tasks[TaskIndex].Handle = ChildHandle;
                CurrentState.Tasks[TaskIndex].LastResult = ChildHandle.Get<ck::FFragment_SmTask_Current>().Get_LastResult();
            }

            ++TaskIndex;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
