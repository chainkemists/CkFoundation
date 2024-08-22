#include "CkSubstep_Utils.h"

#include "CkSubstep/CkSubstep_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Substep_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Substep_ParamsData& InParams)
    -> FCk_Handle_Substep
{
    InHandle.Add<ck::FFragment_Substep_Params>(InParams);
    InHandle.Add<ck::FFragment_Substep_Current>();
    InHandle.Add<ck::FTag_Substep_FirstUpdate>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Substep_UE, FCk_Handle_Substep, ck::FFragment_Substep_Params, ck::FFragment_Substep_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Substep_UE::
    Request_Pause(
        FCk_Handle_Substep& InHandle)
    -> FCk_Handle_Substep
{
    InHandle.Try_Remove<ck::FTag_Substep_Update>();

    return InHandle;
}

auto
    UCk_Utils_Substep_UE::
    Request_Resume(
        FCk_Handle_Substep& InHandle)
    -> FCk_Handle_Substep
{
    InHandle.AddOrGet<ck::FTag_Substep_Update>();

    return InHandle;
}

auto
    UCk_Utils_Substep_UE::
    Request_Reset(
        FCk_Handle_Substep& InHandle,
        ECk_Substep_State InState)
    -> FCk_Handle_Substep
{
    switch(InState)
    {
        case ECk_Substep_State::Paused: { Request_Pause(InHandle); break; }
        case ECk_Substep_State::Running: { Request_Resume(InHandle); break; }
    }

    InHandle.AddOrGet<ck::FTag_Substep_FirstUpdate>();

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Substep_UE::
    BindTo_OnFirstUpdate(
        FCk_Handle_Substep& InSubstepHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Substep_OnFirstUpdate& InDelegate)
    -> FCk_Handle_Substep
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnSubstepFirstUpdate, InSubstepHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InSubstepHandle;
}

auto
    UCk_Utils_Substep_UE::
    UnbindFrom_OnFirstUpdate(
        FCk_Handle_Substep& InSubstepHandle,
        const FCk_Delegate_Substep_OnFirstUpdate& InDelegate)
    -> FCk_Handle_Substep
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnSubstepFirstUpdate, InSubstepHandle, InDelegate);
    return InSubstepHandle;
}

auto
    UCk_Utils_Substep_UE::
    BindTo_OnUpdate(
        FCk_Handle_Substep& InSubstepHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Substep_OnUpdate& InDelegate)
    -> FCk_Handle_Substep
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnSubstepUpdate, InSubstepHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InSubstepHandle;
}

auto
    UCk_Utils_Substep_UE::
    UnbindFrom_OnUpdate(
        FCk_Handle_Substep& InSubstepHandle,
        const FCk_Delegate_Substep_OnUpdate& InDelegate)
    -> FCk_Handle_Substep
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnSubstepUpdate, InSubstepHandle, InDelegate);
    return InSubstepHandle;
}

auto
    UCk_Utils_Substep_UE::
    BindTo_OnFrameEnd(
        FCk_Handle_Substep& InSubstepHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Substep_OnFrameEnd& InDelegate)
    -> FCk_Handle_Substep
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnSubstepFrameEnd, InSubstepHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InSubstepHandle;
}

auto
    UCk_Utils_Substep_UE::
    UnbindFrom_OnFrameEnd(
        FCk_Handle_Substep& InSubstepHandle,
        const FCk_Delegate_Substep_OnFrameEnd& InDelegate)
    -> FCk_Handle_Substep
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnSubstepFrameEnd, InSubstepHandle, InDelegate);
    return InSubstepHandle;
}

// --------------------------------------------------------------------------------------------------------------------
