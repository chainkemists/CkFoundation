#include "CkSmTransition_Utils.h"

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
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass,
        int32 InOrder)
    -> FCk_Handle_SmTransition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerState),
        TEXT("Invalid state handle in SmTransition Create"))
    { return {}; }

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
        ck::FFragment_SmTransition_Params{InTargetStateClass, InOrder});
    TransitionEntity.Add<ck::FFragment_SmTransition_Current>();

    auto TransitionEntityTyped = CastChecked(TransitionEntity);

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
    MarkTransitionAs_StartEvaluating(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTransition),
        TEXT("Invalid transition handle in MarkTransitionAs_StartEvaluating"))
    { return InTransition; }

    auto& Current = InTransition.AddOrGet<ck::FFragment_SmTransition_Current>();
    Current.Set_Result(ECk_SmTransitionResult::Undetermined);
    InTransition.AddOrGet<ck::FTag_SmTransition_Evaluating>();

    return InTransition;
}

auto
    UCk_Utils_SmTransition_UE::
    MarkTransitionAs_EvaluationPassed(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTransition),
        TEXT("Invalid transition handle in MarkTransitionAs_EvaluationPassed"))
    { return InTransition; }

    auto& Current = InTransition.AddOrGet<ck::FFragment_SmTransition_Current>();
    Current.Set_Result(ECk_SmTransitionResult::Pass);
    InTransition.Try_Remove<ck::FTag_SmTransition_Evaluating>();

    return InTransition;
}

auto
    UCk_Utils_SmTransition_UE::
    MarkTransitionAs_EvaluationFailed(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTransition),
        TEXT("Invalid transition handle in MarkTransitionAs_EvaluationFailed"))
    { return InTransition; }

    auto& Current = InTransition.AddOrGet<ck::FFragment_SmTransition_Current>();
    Current.Set_Result(ECk_SmTransitionResult::Fail);
    InTransition.Try_Remove<ck::FTag_SmTransition_Evaluating>();

    return InTransition;
}

auto
    UCk_Utils_SmTransition_UE::
    MarkTransitionAs_ReadyToTransition(
        FCk_Handle_SmTransition& InTransition)
    -> FCk_Handle_SmTransition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTransition),
        TEXT("Invalid transition handle in MarkTransitionAs_ReadyToTransition"))
    { return InTransition; }

    auto& Current = InTransition.AddOrGet<ck::FFragment_SmTransition_Current>();
    Current.Set_Result(ECk_SmTransitionResult::Pass);
    InTransition.Try_Remove<ck::FTag_SmTransition_Evaluating>();

    return InTransition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmTransition_UE::
    Get_EvaluationResult(
        const FCk_Handle_SmTransition& InTransition)
    -> ECk_SmTransitionResult
{
    if (ck::Is_NOT_Valid(InTransition))
    { return ECk_SmTransitionResult::Undetermined; }

    if (NOT InTransition.Has<ck::FFragment_SmTransition_Current>())
    { return ECk_SmTransitionResult::Undetermined; }

    return InTransition.Get<ck::FFragment_SmTransition_Current>().Get_Result();
}

auto
    UCk_Utils_SmTransition_UE::
    Get_TargetStateClass(
        const FCk_Handle_SmTransition& InTransition)
    -> TSubclassOf<UCk_SmState_EntityScript>
{
    if (ck::Is_NOT_Valid(InTransition))
    { return nullptr; }

    if (NOT InTransition.Has<ck::FFragment_SmTransition_Params>())
    { return nullptr; }

    return InTransition.Get<ck::FFragment_SmTransition_Params>().Get_TargetStateClass();
}

// --------------------------------------------------------------------------------------------------------------------
