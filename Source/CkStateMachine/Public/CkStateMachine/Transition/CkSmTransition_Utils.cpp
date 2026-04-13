#include "CkSmTransition_Utils.h"

#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_SmTransition_Params>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Create(
        FCk_Handle_SmState& InOwnerState,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass)
    -> FCk_Handle_SmTransition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTargetStateClass),
        TEXT("Invalid target state class in SmTransition Create"))
    { return {}; }

    auto TransitionEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwnerState);

    UCk_Utils_Handle_UE::Set_DebugName(TransitionEntity,
        *ck::Format_UE(TEXT("Transition -> {}"), InTargetStateClass->GetFName()));

    if (InOwnerState.Has<ck::FFragment_Sm_Context>())
    {
        const auto& Context = InOwnerState.Get<ck::FFragment_Sm_Context>();
        TransitionEntity.Add<ck::FFragment_Sm_Context>(Context.Get_GameEntityHandle());
    }

    TransitionEntity.Add<ck::FFragment_SmTransition_Params>(
        ck::FFragment_SmTransition_Params{InTargetStateClass});

    auto TransitionEntityTyped = CastChecked(TransitionEntity);
    TransitionEntityTyped.Add<ck::FTag_SmTransition_FullyEventDriven>();

    UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::AddIfMissing(InOwnerState);
    UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::Request_Connect(
        InOwnerState, TransitionEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

    ck::TUtils_Sm_ParentState::AddOrReplace(TransitionEntity, InOwnerState);

    if (ck::TUtils_Sm_OwningStateMachine::Has(InOwnerState))
    {
        const auto OwningSm = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InOwnerState);
        ck::TUtils_Sm_OwningStateMachine::AddOrReplace(TransitionEntity, OwningSm);
    }

    return TransitionEntityTyped;
}

 // --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Request_MarkTransition_AsNotFullyEventDriven(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    InTransition.Try_Remove<ck::FTag_SmTransition_FullyEventDriven>();

    if (ck::TUtils_Sm_ParentState::Has(InTransition))
    {
        auto ParentState = UCk_Utils_SmState_UE::CastChecked(
            ck::TUtils_Sm_ParentState::Get_StoredEntity(InTransition));
        UCk_Utils_SmState_UE::Request_MarkState_AsNotFullyEventDriven(ParentState);
    }

    return InTransition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Request_StartEvaluating(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    InTransition.Get<ck::FFragment_SmTransition_Current>().Set_Result(ECk_SmTransitionResult::Undetermined);
    InTransition.AddOrGet<ck::FTag_SmTransition_Evaluating>();

    return InTransition;
}

auto
    UCk_Utils_SmTransition_UE::
    Request_UpdateTransitionResult(
        FCk_Handle_SmTransition& InTransition,
        ECk_SmTransitionResult InResult)
    -> FCk_Handle_SmTransition
{
    InTransition.Get<ck::FFragment_SmTransition_Current>().Set_Result(InResult);
    InTransition.Try_Remove<ck::FTag_SmTransition_Evaluating>();

    return InTransition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Is_FullyEventDriven(
        const FCk_Handle_SmTransition& InTransition)
    -> bool
{
    return InTransition.Has<ck::FTag_SmTransition_FullyEventDriven>();
}

auto
    UCk_Utils_SmTransition_UE::
    Get_EvaluationResult(
        const FCk_Handle_SmTransition& InTransition)
    -> ECk_SmTransitionResult
{
    return InTransition.Get<ck::FFragment_SmTransition_Current>().Get_Result();
}

auto
    UCk_Utils_SmTransition_UE::
    Get_TargetStateClass(
        const FCk_Handle_SmTransition& InTransition)
    -> TSubclassOf<UCk_SmState_EntityScript>
{
    return InTransition.Get<ck::FFragment_SmTransition_Params>().Get_TargetStateClass();
}

auto
    UCk_Utils_SmTransition_UE::
    Get_OwningStateMachine(
        const FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_StateMachine
{
    return ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InTransition);
}

// --------------------------------------------------------------------------------------------------------------------
