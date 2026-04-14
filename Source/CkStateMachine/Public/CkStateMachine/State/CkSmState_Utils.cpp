#include "CkSmState_Utils.h"

#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmState_UE::
    TryCheckTransitionBreakpoint(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass)
    -> void
{
#if !UE_BUILD_SHIPPING
    if (NOT InStateMachine.Has<ck::FFragment_Sm_Breakpoints>())
    { return; }

    const auto& SmCurrent = InStateMachine.Get<ck::FFragment_Sm_Current>();
    const auto TransitionKey = ck::FFragment_Sm_Breakpoints::FTransitionKey{
        SmCurrent.Get_CurrentStateClass(), InTargetStateClass};

    if (NOT InStateMachine.Get<ck::FFragment_Sm_Breakpoints>().Get_TransitionBreakpoints().Contains(TransitionKey))
    { return; }

    auto& [Description, RealTimeSeconds] = InStateMachine.AddOrGet<ck::FFragment_Sm_Debug_BreakpointHit>();
    Description = ck::Format_UE(TEXT("Transition: {} \u2192 {}"),
        SmCurrent.Get_CurrentStateClass()->GetName(), InTargetStateClass->GetName());
    RealTimeSeconds = FPlatformTime::Seconds();
    UCk_Utils_EditorOnly_UE::Request_DebugPauseExecution();
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmState_UE::
    TryRecordLastFiredTransition(
        FCk_Handle_StateMachine& InStateMachine,
        FCk_Handle_SmTransition& InTransition)
    -> void
{
#if !UE_BUILD_SHIPPING
    auto& [ConditionNames, RealTimeSeconds] = InStateMachine.AddOrGet<ck::FFragment_Sm_Debug_LastFiredTransition>();
    RealTimeSeconds = FPlatformTime::Seconds();
    ConditionNames.Reset();

    UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_ValidEntry(InTransition,
    [&](FCk_Handle_SmCondition InCondition) -> ECk_Record_ForEachIterationResult
    {
        if (NOT InCondition.Has<ck::FFragment_EntityScript_Current>())
        { return ECk_Record_ForEachIterationResult::Continue; }

        if (auto* CondScript = InCondition.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get();
            ck::IsValid(CondScript))
        {
            ConditionNames.Add(UCk_Utils_Object_UE::Get_CleanClassName(CondScript->GetClass()));
        }

        return ECk_Record_ForEachIterationResult::Continue;
    });
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmState_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_RecordOfSmTransitions>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmState_UE::
    Create(
        FCk_Handle_StateMachine& InOwnerStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass)
    -> FCk_Handle_SmState
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStateClass),
        TEXT("Invalid state class in SmState Create"))
    { return {}; }

    const auto ResolvedClass = Get_ResolvedStateClass(InOwnerStateMachine, InStateClass);

    auto StateEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwnerStateMachine);

    UCk_Utils_Handle_UE::Set_DebugName(StateEntity, ResolvedClass->GetFName());

    if (InOwnerStateMachine.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = InOwnerStateMachine.Get<ck::FFragment_Sm_Context>();
        StateEntity.Add<ck::FFragment_Sm_Context>(Context.Get_GameEntityHandle());
    }

    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(StateEntity, InOwnerStateMachine);

    StateEntity.Add<ck::FTag_SmState_FullyEventDriven>();
    StateEntity.Add<ck::FTag_SmState_Active>();
    StateEntity.Add<ck::FFragment_SmState_Hierarchy>(
        DoBuildProspectiveHierarchy(InOwnerStateMachine, ResolvedClass));

    UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::AddIfMissing(StateEntity);

    auto StateEntityTyped = CastChecked(StateEntity);

    UCk_Utils_StateMachine_UE::RecordOfSmStates_Utils::AddIfMissing(InOwnerStateMachine);
    UCk_Utils_StateMachine_UE::RecordOfSmStates_Utils::Request_Connect(
        InOwnerStateMachine, StateEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

    UCk_Utils_EntityScript_UE::Add(StateEntity, ResolvedClass, FInstancedStruct{});

    return StateEntityTyped;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmState_UE::
    DoBuildProspectiveHierarchy(
        const FCk_Handle_StateMachine& InOwnerStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass)
    -> TArray<FGameplayTag>
{
    auto Hierarchy = TArray<FGameplayTag>{};

    if (InOwnerStateMachine.Has<ck::FFragment_Sm_ParentHierarchy>())
    {
        Hierarchy = InOwnerStateMachine.Get<ck::FFragment_Sm_ParentHierarchy>().Get_ParentHierarchy();
    }

    Hierarchy.Add(UCk_SmState_EntityScript::Get_StateTagForClass(InStateClass));
    return Hierarchy;
}

auto
    UCk_Utils_SmState_UE::
    Get_ResolvedStateClass(
        const FCk_Handle_StateMachine& InOwnerStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InRequestedClass)
    -> TSubclassOf<UCk_SmState_EntityScript>
{
    if (NOT InOwnerStateMachine.Has<ck::FFragment_Sm_StateOverrides>())
    { return InRequestedClass; }

    const auto& Overrides = InOwnerStateMachine.Get<ck::FFragment_Sm_StateOverrides>().Get_Overrides();
    if (Overrides.IsEmpty())
    { return InRequestedClass; }

    const auto Prospective = DoBuildProspectiveHierarchy(InOwnerStateMachine, InRequestedClass);
    const auto ProspectiveLeaf = Prospective.Last();

    for (const auto& Entry : Overrides)
    {
        if (ck::Is_NOT_Valid(Entry._OverridingStateClass))
        { continue; }

        const auto& EntryHierarchy = Entry._OverriddenStateHierarchy;

        if (EntryHierarchy.Num() == 1)
        {
            if (EntryHierarchy[0] == ProspectiveLeaf)
            { return Entry._OverridingStateClass; }
        }
        else if (EntryHierarchy.Num() > 1)
        {
            if (EntryHierarchy == Prospective)
            { return Entry._OverridingStateClass; }
        }
    }

    return InRequestedClass;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmState_UE::
    Request_Evaluate(
        FCk_Handle_SmState& InState)
    -> FCk_Handle_SmState
{
    InState.AddOrGet<ck::FTag_SmState_NeedsEvaluation>();
    return InState;
}

auto
    UCk_Utils_SmState_UE::
    Get_IsFullyEventDriven(
        const FCk_Handle_SmState& InState)
    -> bool
{
    return InState.Has<ck::FTag_SmState_FullyEventDriven>();
}

auto
    UCk_Utils_SmState_UE::
    Request_MarkState_AsNotFullyEventDriven(
        FCk_Handle_SmState& InState)
    -> FCk_Handle_SmState
{
    InState.Try_Remove<ck::FTag_SmState_FullyEventDriven>();
    return InState;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmState_UE::
    Get_IsReadyToTransition(
        const FCk_Handle_SmState& InState)
    -> bool
{
    return UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::AnyOf(InState,
    [](const FCk_Handle_SmTransition& InTransition)
    {
        return UCk_Utils_SmTransition_UE::Get_EvaluationResult(InTransition) == ECk_SmTransitionResult::Pass;
    });
}

auto
    UCk_Utils_SmState_UE::
    Get_OwningStateMachine(
        const FCk_Handle_SmState& InState)
    -> FCk_Handle_StateMachine
{
    return ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InState);
}

auto
    UCk_Utils_SmState_UE::
    Get_Hierarchy(
        const FCk_Handle_SmState& InState)
    -> TArray<FGameplayTag>
{
    if (ck::Is_NOT_Valid(InState) || NOT InState.Has<ck::FFragment_SmState_Hierarchy>())
    { return {}; }

    return InState.Get<ck::FFragment_SmState_Hierarchy>().Get_Hierarchy();
}

// --------------------------------------------------------------------------------------------------------------------
