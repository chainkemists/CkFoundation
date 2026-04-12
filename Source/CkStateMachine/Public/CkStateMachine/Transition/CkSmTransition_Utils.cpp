#include "CkSmTransition_Utils.h"

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
