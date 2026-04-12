#include "CkStateMachine_Debug_GraphWalk_Processor.h"

#if CK_BUILD_SM_GRAPH_WALK

#include "CkStateMachine/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/EntityScripts/CkSmTask_SubStateMachine.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_Debug_GraphWalk);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_Debug_GraphWalk_Iterate);

namespace ck
{
    // ================================================================================================================
    // ONE-SHOT KICKOFF
    // ================================================================================================================

    auto
        FProcessor_Sm_Debug_GraphWalk::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams)
        -> void
    {
        InHandle.Remove<FTag_Sm_Debug_RequiresGraphWalk>();

        auto InitialStateClass = InParams.Get_InitialStateClass();

        if (NOT IsValid(InitialStateClass))
        { return; }

        auto& Progress = InHandle.AddOrGet<FFragment_Sm_Debug_GraphWalk_Progress>();
        Progress._PendingDiscovery.Add(InitialStateClass);

        CreateBatch(Progress, InHandle);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_Debug_GraphWalk::
        CreateBatch(
            FFragment_Sm_Debug_GraphWalk_Progress& InOutProgress,
            HandleType InSmHandle)
        -> void
    {
        auto SmHandle = static_cast<FCk_Handle>(InSmHandle);

        for (const auto& StateClass : InOutProgress._PendingDiscovery)
        {
            if (NOT IsValid(StateClass))
            { continue; }

            if (InOutProgress._Visited.Contains(StateClass))
            { continue; }

            InOutProgress._Visited.Add(StateClass);

            auto TempEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(SmHandle);

            UCk_Utils_EntityScript_UE::Add(
                TempEntity,
                StateClass,
                FInstancedStruct{});

            auto PendingEntry = FFragment_Sm_Debug_GraphWalk_Progress::FPendingEntity{};
            PendingEntry.StateClass = StateClass;
            PendingEntry.EntityHandle = TempEntity;
            InOutProgress._PendingEntities.Add(MoveTemp(PendingEntry));
        }

        InOutProgress._PendingDiscovery.Reset();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_Debug_GraphWalk::
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

    // ================================================================================================================
    // ITERATE — Runs each frame while walk is in progress
    // ================================================================================================================

    auto
        FProcessor_Sm_Debug_GraphWalk_Iterate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Debug_GraphWalk_Progress& /*InProgress*/)
        -> void
    {
        auto& Progress = InHandle.Get<FFragment_Sm_Debug_GraphWalk_Progress>();

        if (Progress._PendingEntities.IsEmpty())
        { return; }

        // Check if ALL pending entities have been constructed (script attached)
        for (const auto& Pending : Progress._PendingEntities)
        {
            if (NOT static_cast<FCk_Handle>(Pending.EntityHandle).Has<FFragment_EntityScript_Current>())
            { return; }
        }

        // All state scripts constructed — DefineState has run, children are readable

        for (const auto& Pending : Progress._PendingEntities)
        {
            auto StateDef = FCk_SmDebug_StateDefinition{};
            StateDef.StateClass = Pending.StateClass;
            StateDef.StateName = GetCleanClassName(Pending.StateClass);

            auto StateChildren = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(
                static_cast<FCk_Handle>(Pending.EntityHandle));

            for (const auto& ChildHandle : StateChildren)
            {
                // --- Transitions (FFragment_SmTransition_Params is added immediately by AddTransition) ---
                if (ChildHandle.Has<FFragment_SmTransition_Params>())
                {
                    const auto& TransParams = ChildHandle.Get<FFragment_SmTransition_Params>();

                    auto TransDef = FCk_SmDebug_StateDefinition::FTransitionDef{};
                    TransDef.TargetStateClass = TransParams.Get_TargetStateClass();
                    TransDef.Order = TransParams.Get_Order();
                    StateDef.Transitions.Add(MoveTemp(TransDef));

                    if (IsValid(TransParams.Get_TargetStateClass()))
                    {
                        Progress._PendingDiscovery.Add(TransParams.Get_TargetStateClass());
                    }
                }

                // --- Tasks (FFragment_SmTask_Current + mode tags added immediately by AddTask) ---
                if (ChildHandle.Has<FFragment_SmTask_Current>())
                {
                    auto TaskDef = FCk_SmDebug_StateDefinition::FTaskDef{};

                    if (ChildHandle.Has<FTag_SmTask_Tick>())
                    {
                        TaskDef.Mode = ECk_SmTaskMode::Tick;
                    }

                    // Read task class from the request entity (child of task entity).
                    // AddTask calls UCk_Utils_EntityScript_UE::Add() which creates a request entity
                    // containing _EntityScriptClassArchetype (the CDO). We read from the CDO directly.
                    auto TaskChildren = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(ChildHandle);

                    for (const auto& RequestChild : TaskChildren)
                    {
                        if (NOT RequestChild.Has<FFragment_EntityScript_RequestSpawnEntity>())
                        { continue; }

                        const auto& Request = RequestChild.Get<FFragment_EntityScript_RequestSpawnEntity>();
                        auto* Archetype = Request.Get_EntityScriptClassArchetype().Get();

                        if (NOT IsValid(Archetype))
                        { continue; }

                        TaskDef.ClassName = GetCleanClassName(Archetype->GetClass());

                        auto* SubSmTask = Cast<UCk_SmTask_SubStateMachine>(Archetype);

                        if (IsValid(SubSmTask) && IsValid(SubSmTask->Get_InitialStateClass()))
                        {
                            TaskDef.HasSubStateMachine = true;
                            TaskDef.SubSmInitialStateClass = SubSmTask->Get_InitialStateClass();
                            Progress._PendingSubSmWalks.Add(
                                {Pending.StateClass, SubSmTask->Get_InitialStateClass()});
                        }
                    }

                    StateDef.Tasks.Add(MoveTemp(TaskDef));
                }
            }

            Progress._StateDefinitions.Add(Pending.StateClass, MoveTemp(StateDef));

            auto TempHandle = static_cast<FCk_Handle>(Pending.EntityHandle);
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(TempHandle);
        }

        Progress._PendingEntities.Reset();

        // Process sub-SM walk queue
        for (const auto& [ParentClass, SubSmInitial] : Progress._PendingSubSmWalks)
        {
            if (Progress._SubSmDefinitions.Contains(ParentClass))
            { continue; }

            auto SubDef = FCk_SmDebug_SubSmDefinition{};
            SubDef.ParentStateClass = ParentClass;
            SubDef.InitialStateClass = SubSmInitial;
            Progress._SubSmDefinitions.Add(ParentClass, MoveTemp(SubDef));

            Progress._PendingDiscovery.Add(SubSmInitial);
        }

        Progress._PendingSubSmWalks.Reset();

        // Create next batch of temp entities for newly discovered states
        CreateBatch(Progress, InHandle);

        // If no more pending entities, walk is complete
        if (Progress._PendingEntities.IsEmpty())
        {
            AssignSubSmStateDefinitions(Progress);

            auto& GraphDef = InHandle.AddOrGet<FFragment_Sm_Debug_GraphDefinition>();
            GraphDef._StateDefinitions = MoveTemp(Progress._StateDefinitions);
            GraphDef._SubSmDefinitions = MoveTemp(Progress._SubSmDefinitions);
            GraphDef._IsComplete = true;

            InHandle.Remove<FFragment_Sm_Debug_GraphWalk_Progress>();
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_Debug_GraphWalk_Iterate::
        CreateBatch(
            FFragment_Sm_Debug_GraphWalk_Progress& InOutProgress,
            HandleType InSmHandle)
        -> void
    {
        FProcessor_Sm_Debug_GraphWalk::CreateBatch(InOutProgress, InSmHandle);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_Debug_GraphWalk_Iterate::
        AssignSubSmStateDefinitions(
            FFragment_Sm_Debug_GraphWalk_Progress& InOutProgress)
        -> void
    {
        // For each sub-SM definition, find all states reachable from its initial state
        // via BFS on transitions, and assign them to the sub-SM's StateDefinitions map.
        // States assigned to a sub-SM are removed from the top-level _StateDefinitions
        // so the parent SM only contains its own states.

        for (auto& [ParentClass, SubSmDef] : InOutProgress._SubSmDefinitions)
        {
            if (NOT IsValid(SubSmDef.InitialStateClass))
            { continue; }

            auto Queue = TArray<TSubclassOf<UCk_SmState_EntityScript>>{};
            auto SubSmVisited = TSet<TSubclassOf<UCk_SmState_EntityScript>>{};

            Queue.Add(SubSmDef.InitialStateClass);
            SubSmVisited.Add(SubSmDef.InitialStateClass);

            while (NOT Queue.IsEmpty())
            {
                auto CurrentClass = Queue[0];
                Queue.RemoveAt(0);

                auto* StateDef = InOutProgress._StateDefinitions.Find(CurrentClass);

                if (NOT StateDef)
                { continue; }

                SubSmDef.StateDefinitions.Add(CurrentClass, *StateDef);

                for (const auto& TransDef : StateDef->Transitions)
                {
                    if (IsValid(TransDef.TargetStateClass)
                        && NOT SubSmVisited.Contains(TransDef.TargetStateClass))
                    {
                        SubSmVisited.Add(TransDef.TargetStateClass);
                        Queue.Add(TransDef.TargetStateClass);
                    }
                }
            }

            // Remove sub-SM states from the top-level definitions
            for (const auto& [StateClass, _] : SubSmDef.StateDefinitions)
            {
                InOutProgress._StateDefinitions.Remove(StateClass);
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_Debug_GraphWalk_Iterate::
        GetCleanClassName(
            const UClass* InClass)
        -> FString
    {
        return FProcessor_Sm_Debug_GraphWalk::GetCleanClassName(InClass);
    }
}

#endif // CK_BUILD_SM_GRAPH_WALK

// --------------------------------------------------------------------------------------------------------------------
