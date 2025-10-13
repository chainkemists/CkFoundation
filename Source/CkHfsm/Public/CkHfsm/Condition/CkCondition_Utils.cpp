#include "CkCondition_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/ContextOwner/CkContextOwner_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Condition_UE::
    Create(
        FCk_Handle_Transition& InTransitionHandle,
        const FCk_Fragment_Condition_ParamsData& InParams)
    -> FCk_Handle_Condition
{
    // Convert typesafe handle to generic handle for entity creation
    FCk_Handle& GenericHandle = InTransitionHandle;
    
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(GenericHandle, [&](FCk_Handle InNew)
    {
        InNew.Add<ck::FFragment_Condition_Params>(InParams);
        InNew.Add<ck::FFragment_Condition_Current>();
        InNew.AddOrGet<ck::FTag_Condition_Setup>();

        // Set context owner to parent transition
        UCk_Utils_ContextOwner_UE::Set_Owner(InNew, GenericHandle);
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
    InHandle.AddOrGet<ck::FFragment_Condition_Requests>()._Requests.Emplace(
        FCk_Request_Condition_Command{ECk_Condition_Command::StartOrResumeEvaluating});

    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    Request_PauseEvaluation(
        FCk_Handle_Condition& InHandle)
    -> FCk_Handle_Condition
{
    InHandle.AddOrGet<ck::FFragment_Condition_Requests>()._Requests.Emplace(
        FCk_Request_Condition_Command{ECk_Condition_Command::PauseEvaluation});

    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    Request_StopEvaluating(
        FCk_Handle_Condition& InHandle)
    -> FCk_Handle_Condition
{
    InHandle.AddOrGet<ck::FFragment_Condition_Requests>()._Requests.Emplace(
        FCk_Request_Condition_Command{ECk_Condition_Command::StopEvaluating});

    return InHandle;
}

auto
    UCk_Utils_Condition_UE::
    Request_MarkResult(
        FCk_Handle_Condition& InHandle,
        ECk_Condition_MarkResult InResult)
    -> FCk_Handle_Condition
{
    InHandle.AddOrGet<ck::FFragment_Condition_Requests>()._Requests.Emplace(
        FCk_Request_Condition_MarkResult{InResult});

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