#include "CkHfsm/Condition/CkCondition_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Condition_UE::
    Create(
        FCk_Handle_Transition& InTransitionHandle,
        const FCk_Fragment_Condition_ParamsData& InParams)
    -> FCk_Handle_Condition
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InTransitionHandle, [&](FCk_Handle InNew)
    {
        InNew.Add<ck::FFragment_Condition_Params>(InParams);
        InNew.Add<ck::FFragment_Condition_Current>();
        InNew.Add<ck::FTag_Condition_Setup>();
    });

    return CastChecked(NewEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Condition_UE, FCk_Handle_Condition,
    ck::FFragment_Condition_Params, ck::FFragment_Condition_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Condition_UE::
    Request_StartOrResumeEvaluating(
        FCk_Handle_Condition& InHandle)
    -> FCk_Handle_Condition
{
    if (InHandle.Has<ck::FTag_Condition_EvaluationPaused>())
    {
        InHandle.Remove<ck::FTag_Condition_EvaluationPaused>();
    }
    else
    {
        InHandle.AddOrGet<ck::FTag_Condition_Enter>();
    }

    InHandle.AddOrGet<ck::FTag_StateMachine_Evaluate_TransitionOrCondition>();
    InHandle.Remove<ck::FTag_Condition_EvaluationPassed>();
    InHandle.Remove<ck::FTag_Condition_EvaluationFailed>();

    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    Request_PauseEvaluation(
        FCk_Handle_Condition& InHandle)
    -> FCk_Handle_Condition
{
    InHandle.Remove<ck::FTag_StateMachine_Evaluate_TransitionOrCondition>();
    InHandle.AddOrGet<ck::FTag_Condition_EvaluationPaused>();

    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    Request_StopEvaluating(
        FCk_Handle_Condition& InHandle)
    -> FCk_Handle_Condition
{
    InHandle.AddOrGet<ck::FTag_Condition_Exit>();
    InHandle.Remove<ck::FTag_Condition_EvaluationPaused>();

    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    Request_MarkResult(
        FCk_Handle_Condition& InHandle,
        ECk_Condition_MarkResult InResult)
    -> FCk_Handle_Condition
{
    switch (InResult)
    {
        case ECk_Condition_MarkResult::Passed:
        {
            InHandle.AddOrGet<ck::FTag_Condition_EvaluationPassed>();
            InHandle.Remove<ck::FTag_Condition_EvaluationFailed>();

            {
#if STATS
                auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
                UUtils_Signal_OnConditionPassed::Broadcast(InHandle, MakePayload(InHandle, FCk_Time{}));
            }

            // Notify parent transition to evaluate
            const auto ParentEntity = UCk_Utils_ContextOwner_UE::Get_Owner(InHandle);
            if (ck::IsValid(ParentEntity))
            {
                ParentEntity.AddOrGet<ck::FTag_StateMachine_Evaluate_TransitionOrCondition>();
            }

            break;
        }
        case ECk_Condition_MarkResult::Failed:
        {
            InHandle.AddOrGet<ck::FTag_Condition_EvaluationFailed>();
            InHandle.Remove<ck::FTag_Condition_EvaluationPassed>();

            {
#if STATS
                auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
                UUtils_Signal_OnConditionFailed::Broadcast(InHandle, MakePayload(InHandle, FCk_Time{}));
            }
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InResult);
            break;
        }
    }

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Condition_UE::
    Get_EvaluationResult(
        const FCk_Handle_Condition& InHandle)
    -> ECk_Condition_Result
{
    const auto& NegateResult = InHandle.Get<ck::FFragment_Condition_Params>().Get_NegateResult();

    if (InHandle.Has<ck::FTag_Condition_EvaluationPassed>())
    {
        return NegateResult ? ECk_Condition_Result::Fail : ECk_Condition_Result::Pass;
    }

    if (InHandle.Has<ck::FTag_Condition_EvaluationFailed>() ||
        InHandle.Has<ck::FTag_Condition_IsEventDriven>())
    {
        return NegateResult ? ECk_Condition_Result::Pass : ECk_Condition_Result::Fail;
    }

    return ECk_Condition_Result::Undetermined;
}

auto
    UCk_Utils_Condition_UE::
    Get_IsResultNegated(
        const FCk_Handle_Condition& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_Condition_NegateResult>();
}

auto
    UCk_Utils_Condition_UE::
    Get_IsEventDriven(
        const FCk_Handle_Condition& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_Condition_IsEventDriven>();
}

auto
    UCk_Utils_Condition_UE::
    Get_IsNotEventDriven(
        const FCk_Handle_Condition& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_Condition_IsNotEventDriven>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Condition_UE::
    BindTo_OnEnter(
        FCk_Handle_Condition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Condition& InDelegate)
    -> FCk_Handle_Condition
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnConditionEnter, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    BindTo_OnExit(
        FCk_Handle_Condition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Condition& InDelegate)
    -> FCk_Handle_Condition
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnConditionExit, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    BindTo_OnPassed(
        FCk_Handle_Condition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Condition& InDelegate)
    -> FCk_Handle_Condition
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnConditionPassed, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    BindTo_OnFailed(
        FCk_Handle_Condition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Condition& InDelegate)
    -> FCk_Handle_Condition
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnConditionFailed, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    UnbindFrom_OnEnter(
        FCk_Handle_Condition& InHandle,
        const FCk_Delegate_Condition& InDelegate)
    -> FCk_Handle_Condition
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnConditionEnter, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    UnbindFrom_OnExit(
        FCk_Handle_Condition& InHandle,
        const FCk_Delegate_Condition& InDelegate)
    -> FCk_Handle_Condition
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnConditionExit, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    UnbindFrom_OnPassed(
        FCk_Handle_Condition& InHandle,
        const FCk_Delegate_Condition& InDelegate)
    -> FCk_Handle_Condition
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnConditionPassed, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    UnbindFrom_OnFailed(
        FCk_Handle_Condition& InHandle,
        const FCk_Delegate_Condition& InDelegate)
    -> FCk_Handle_Condition
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnConditionFailed, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------