#include "CkHfsm/Transition/CkTransition_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkHfsm/Condition/CkCondition_Utils.h"
#include "CkHfsm/State/CkState_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    struct RecordOfTransitions_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfTransitions> {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transition_UE::
    Create(
        FCk_Handle_State& InStateHandle,
        const FCk_Fragment_Transition_ParamsData& InParams)
    -> FCk_Handle_Transition
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InStateHandle, [&](FCk_Handle InNew)
    {
        InNew.Add<ck::FFragment_Transition_Params>(InParams);
        InNew.Add<ck::FFragment_Transition_Current>();
        InNew.Add<ck::FTag_Transition_Setup>();
    });

    // Connect to parent state's record
    RecordOfTransitions_Utils::AddIfMissing(InStateHandle, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfTransitions_Utils::Request_Connect(InStateHandle, CastChecked(NewEntity),
        ECk_Record_LabelRequirementPolicy::Optional);

    return CastChecked(NewEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Transition_UE, FCk_Handle_Transition,
    ck::FFragment_Transition_Params, ck::FFragment_Transition_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transition_UE::
    Request_StartEvaluating(
        FCk_Handle_Transition& InHandle)
    -> FCk_Handle_Transition
{
    InHandle.AddOrGet<ck::FTag_Transition_Enter>();
    InHandle.AddOrGet<ck::FTag_StateMachine_Evaluate_TransitionOrCondition>();

    return InHandle;
}

auto
    UCk_Utils_Transition_UE::
    Request_StopEvaluating(
        FCk_Handle_Transition& InHandle)
    -> FCk_Handle_Transition
{
    InHandle.AddOrGet<ck::FTag_Transition_Exit>();

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transition_UE::
    Get_Name(
        const FCk_Handle_Transition& InHandle)
    -> FGameplayTag
{
    return InHandle.Get<ck::FFragment_Transition_Params>().Get_Name();
}

auto
    UCk_Utils_Transition_UE::
    Get_EvaluationResult(
        const FCk_Handle_Transition& InHandle)
    -> ECk_Transition_Result
{
    if (InHandle.Has<ck::FTag_Transition_EvaluationPassed>())
    {
        return ECk_Transition_Result::Pass;
    }

    if (InHandle.Has<ck::FTag_Transition_EvaluationFailed>() ||
        InHandle.Has<ck::FTag_Transition_IsEventDriven>())
    {
        return ECk_Transition_Result::Fail;
    }

    return ECk_Transition_Result::Undetermined;
}

auto
    UCk_Utils_Transition_UE::
    Get_TargetState(
        const FCk_Handle_Transition& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_Transition_Params>().Get_TargetState();
}

auto
    UCk_Utils_Transition_UE::
    Get_TransitionCondition(
        const FCk_Handle_Transition& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_Transition_Params>().Get_TransitionCondition();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transition_UE::
    BindTo_OnEnter(
        FCk_Handle_Transition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Transition& InDelegate)
    -> FCk_Handle_Transition
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnTransitionEnter, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Transition_UE::
    BindTo_OnExit(
        FCk_Handle_Transition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Transition& InDelegate)
    -> FCk_Handle_Transition
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnTransitionExit, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Transition_UE::
    BindTo_OnPassed(
        FCk_Handle_Transition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Transition& InDelegate)
    -> FCk_Handle_Transition
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnTransitionPassed, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Transition_UE::
    BindTo_OnFailed(
        FCk_Handle_Transition& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Transition& InDelegate)
    -> FCk_Handle_Transition
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnTransitionFailed, InHandle, InDelegate, InBindingPolicy);
    return InHandle;
}

auto
    UCk_Utils_Transition_UE::
    UnbindFrom_OnEnter(
        FCk_Handle_Transition& InHandle,
        const FCk_Delegate_Transition& InDelegate)
    -> FCk_Handle_Transition
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnTransitionEnter, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Transition_UE::
    UnbindFrom_OnExit(
        FCk_Handle_Transition& InHandle,
        const FCk_Delegate_Transition& InDelegate)
    -> FCk_Handle_Transition
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnTransitionExit, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Transition_UE::
    UnbindFrom_OnPassed(
        FCk_Handle_Transition& InHandle,
        const FCk_Delegate_Transition& InDelegate)
    -> FCk_Handle_Transition
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnTransitionPassed, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Transition_UE::
    UnbindFrom_OnFailed(
        FCk_Handle_Transition& InHandle,
        const FCk_Delegate_Transition& InDelegate)
    -> FCk_Handle_Transition
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnTransitionFailed, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------