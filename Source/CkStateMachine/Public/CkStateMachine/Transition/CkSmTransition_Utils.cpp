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

    auto TransHandle = static_cast<FCk_Handle>(InTransition);
    auto& Current = TransHandle.AddOrGet<ck::FFragment_SmTransition_Current>();
    Current.Set_Result(ECk_SmTransitionResult::Undetermined);
    TransHandle.AddOrGet<ck::FTag_SmTransition_Evaluating>();

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

    auto TransHandle = static_cast<FCk_Handle>(InTransition);
    auto& Current = TransHandle.AddOrGet<ck::FFragment_SmTransition_Current>();
    Current.Set_Result(ECk_SmTransitionResult::Pass);
    TransHandle.Try_Remove<ck::FTag_SmTransition_Evaluating>();

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

    auto TransHandle = static_cast<FCk_Handle>(InTransition);
    auto& Current = TransHandle.AddOrGet<ck::FFragment_SmTransition_Current>();
    Current.Set_Result(ECk_SmTransitionResult::Fail);
    TransHandle.Try_Remove<ck::FTag_SmTransition_Evaluating>();

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

    auto TransHandle = static_cast<FCk_Handle>(InTransition);
    auto& Current = TransHandle.AddOrGet<ck::FFragment_SmTransition_Current>();
    Current.Set_Result(ECk_SmTransitionResult::Pass);
    TransHandle.Try_Remove<ck::FTag_SmTransition_Evaluating>();

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

    auto TransHandle = static_cast<FCk_Handle>(InTransition);

    if (NOT TransHandle.Has<ck::FFragment_SmTransition_Current>())
    { return ECk_SmTransitionResult::Undetermined; }

    return TransHandle.Get<ck::FFragment_SmTransition_Current>().Get_Result();
}

auto
    UCk_Utils_SmTransition_UE::
    Get_TargetStateClass(
        const FCk_Handle_SmTransition& InTransition)
    -> TSubclassOf<UCk_SmState_EntityScript>
{
    if (ck::Is_NOT_Valid(InTransition))
    { return nullptr; }

    auto TransHandle = static_cast<FCk_Handle>(InTransition);

    if (NOT TransHandle.Has<ck::FFragment_SmTransition_Params>())
    { return nullptr; }

    return TransHandle.Get<ck::FFragment_SmTransition_Params>().Get_TargetStateClass();
}

// --------------------------------------------------------------------------------------------------------------------
