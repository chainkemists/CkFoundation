#include "CkTimer_Utils.h"

#include "CkTimer/CkTimer_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Timer_ParamsData& InData)
    -> void
{
    InHandle.Add<ck::FFragment_Timer_Params>(InData);
    InHandle.Add<ck::FFragment_Timer_Current>(FCk_Chrono{InData.Get_Duration()});
}

auto
    UCk_Utils_Timer_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_Timer_Current, ck::FFragment_Timer_Params>();
}

auto
    UCk_Utils_Timer_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Timer Feature."), InHandle)
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Request_Reset(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    Request_Stop(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    Request_Pause(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    Request_Resume(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerReset(
        FCk_Handle                InHandle,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerStop(
        FCk_Handle                InHandle,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerPause(
        FCk_Handle                InHandle,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerResume(
        FCk_Handle                InHandle,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerDone(
        FCk_Handle                InHandle,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerUpdate(
        FCk_Handle                InHandle,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindTo_OnTimerReset(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindTo_OnTimerStop(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindTo_OnTimerPause(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindTo_OnTimerResume(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindTo_OnTimerDone(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindTo_OnTimerUpdate(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}
