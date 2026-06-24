#include "CkStateMachine_Debug_GraphWalk_Processor.h"

#if CK_BUILD_SM_GRAPH_WALK

#include "CkStateMachine/CkStateMachine_Stats.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/Task/CkSmTask_Utils.h"
#include "CkStateMachine/Task/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/Task/EntityScripts/CkSmTask_SubStateMachine.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_Debug_GraphWalk);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_Debug_GraphWalk_Iterate);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("SmDebug::GraphWalk (kickoff)"), STAT_Sm_Debug_GraphWalk, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmDebug::GraphWalk::Iterate"), STAT_Sm_Debug_GraphWalk_Iterate, STATGROUP_CkStateMachine);

// --------------------------------------------------------------------------------------------------------------------

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
        SCOPE_CYCLE_COUNTER(STAT_Sm_Debug_GraphWalk);

        InHandle.Remove<FTag_Sm_Debug_RequiresGraphWalk>();

        auto InitialStateClass = InParams.Get_InitialStateClass();

        if (ck::Is_NOT_Valid(InitialStateClass))
        { return; }

        // Resolve through override map so the graph uses the same class that runtime will spawn
        InitialStateClass = UCk_Utils_SmState_UE::Get_ResolvedStateClass(InHandle, InitialStateClass);

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
        for (const auto& StateClass : InOutProgress._PendingDiscovery)
        {
            if (ck::Is_NOT_Valid(StateClass))
            { continue; }

            if (InOutProgress._Visited.Contains(StateClass))
            { continue; }

            InOutProgress._Visited.Add(StateClass);

            auto TempEntity = UCk_Utils_SmState_UE::Create(InSmHandle, StateClass);

            // Tag before the deferred script attach runs so DefineState's children
            // (transitions/conditions/tasks) cascade the tag via their Create utilities.
            TempEntity.Add<FTag_Sm_Debug_GraphWalkEntity>();

            auto PendingEntry = FFragment_Sm_Debug_GraphWalk_Progress::FPendingEntity{};
            PendingEntry.StateClass = StateClass;
            PendingEntry.EntityHandle = TempEntity;
            InOutProgress._PendingEntities.Add(MoveTemp(PendingEntry));
        }

        InOutProgress._PendingDiscovery.Reset();
    }

    // ----------------------------------------------------------------------------------------------------------------


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
        SCOPE_CYCLE_COUNTER(STAT_Sm_Debug_GraphWalk_Iterate);

        auto& Progress = InHandle.Get<FFragment_Sm_Debug_GraphWalk_Progress>();

        if (Progress._PendingEntities.IsEmpty())
        { return; }

        // Check if ALL pending entities have been constructed (script attached)
        for (const auto& Pending : Progress._PendingEntities)
        {
            if (NOT Pending.EntityHandle.Has<FFragment_EntityScript_Current>())
            { return; }
        }

        // All state scripts constructed — DefineState has run, children are readable

        for (const auto& Pending : Progress._PendingEntities)
        {
            auto StateDef = FCk_SmDebug_StateDefinition{};
            StateDef.StateClass = Pending.StateClass;
            StateDef.StateName = UCk_Utils_Object_UE::Get_CleanClassName(Pending.StateClass);
            auto PendingStateHandle = UCk_Utils_SmState_UE::CastChecked(Pending.EntityHandle);
            StateDef.ScriptClass = UCk_Utils_SmState_UE::Get_ScriptClass(PendingStateHandle);
            StateDef.RequestedScriptClass = UCk_Utils_SmState_UE::Get_RequestedScriptClass(PendingStateHandle);

            // ---- Transitions via record ----

            UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::ForEach_ValidEntry(Pending.EntityHandle,
            [&](FCk_Handle_SmTransition InTransition)
            {
                const auto& TransParams = InTransition.Get<FFragment_SmTransition_Params>();

                // Resolve target class through override map so graph topology matches runtime
                const auto ResolvedTarget = UCk_Utils_SmState_UE::Get_ResolvedStateClass(
                    InHandle, TransParams.Get_TargetStateClass());

                auto TransDef = FCk_SmDebug_StateDefinition::FTransitionDef{};
                TransDef.TargetStateClass = ResolvedTarget;

                // Capture each condition's static definition (class + mode) so the
                // debugger can show event-driven vs polled on transitions whose
                // source state hasn't become current yet.
                UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_ValidEntry(InTransition,
                [&](FCk_Handle_SmCondition InCondition)
                {
                    auto CondDef = FCk_SmDebug_StateDefinition::FConditionDef{};
                    CondDef.Mode = InCondition.Has<FTag_SmCondition_Polled>()
                        ? ECk_SmConditionMode::Polled
                        : ECk_SmConditionMode::EventDriven;
                    CondDef.ScriptClass = UCk_Utils_SmCondition_UE::Get_ScriptClass(InCondition);
                    if (ck::IsValid(CondDef.ScriptClass))
                    {
                        CondDef.ClassName = UCk_Utils_Object_UE::Get_CleanClassName(CondDef.ScriptClass);
                    }
                    TransDef.Conditions.Add(MoveTemp(CondDef));
                });

                StateDef.Transitions.Add(MoveTemp(TransDef));

                if (ck::IsValid(ResolvedTarget))
                {
                    Progress._PendingDiscovery.Add(ResolvedTarget);
                }
            });

            // ---- Tasks via record ----

            UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::ForEach_ValidEntry(Pending.EntityHandle,
            [&](FCk_Handle_SmTask InTask)
            {
                auto TaskDef = FCk_SmDebug_StateDefinition::FTaskDef{};

                if (InTask.Has<FTag_SmTask_Tick>())
                {
                    TaskDef.Mode = ECk_SmTaskMode::Tick;
                }

                // Read the task's script class directly — same path the live cache uses.
                // Relying on the RequestSpawnEntity child fragment leaves ClassName empty
                // once the spawn request has been consumed.
                TaskDef.ScriptClass = UCk_Utils_SmTask_UE::Get_ScriptClass(InTask);
                if (ck::IsValid(TaskDef.ScriptClass))
                {
                    TaskDef.ClassName = UCk_Utils_Object_UE::Get_CleanClassName(TaskDef.ScriptClass);
                }

                // Sub-SM detection still needs the archetype (to read its InitialStateClass).
                auto TaskChildren = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InTask);
                for (const auto& RequestChild : TaskChildren)
                {
                    if (NOT RequestChild.Has<FFragment_EntityScript_RequestSpawnEntity>())
                    { continue; }

                    const auto& Request = RequestChild.Get<FFragment_EntityScript_RequestSpawnEntity>();
                    auto* Archetype = Request.Get_EntityScriptClassArchetype().Get();

                    if (ck::Is_NOT_Valid(Archetype))
                    { continue; }

                    auto* SubSmTask = Cast<UCk_SmTask_SubStateMachine>(Archetype);
                    if (ck::IsValid(SubSmTask) && ck::IsValid(SubSmTask->Get_InitialStateClass()))
                    {
                        TaskDef.HasSubStateMachine = true;
                        TaskDef.SubSmInitialStateClass = SubSmTask->Get_InitialStateClass();
                        Progress._PendingSubSmWalks.Add(
                            {Pending.StateClass, SubSmTask->Get_InitialStateClass()});
                    }
                }

                StateDef.Tasks.Add(MoveTemp(TaskDef));
            });

            Progress._StateDefinitions.Add(Pending.StateClass, MoveTemp(StateDef));

            auto PendingEntityHandle = Pending.EntityHandle;
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PendingEntityHandle);
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
            if (ck::Is_NOT_Valid(SubSmDef.InitialStateClass))
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
                    if (ck::IsValid(TransDef.TargetStateClass)
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

}

#endif // CK_BUILD_SM_GRAPH_WALK

// --------------------------------------------------------------------------------------------------------------------
