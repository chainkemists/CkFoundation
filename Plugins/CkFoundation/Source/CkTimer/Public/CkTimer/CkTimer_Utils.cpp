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

    TryAddReplicatedFragment<UCk_Fragment_Timer_Rep>(InHandle);

    if (InData.Get_StartingState() == ECk_Timer_State::Running)
    {
        InHandle.Add<ck::FTag_Timer_NeedsUpdate>();
    }
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
    UnbindFrom_OnTimerReset(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerStop(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerPause(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerResume(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerDone(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerUpdate(
        FCk_Handle                InHandle,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Request_OverrideTimer(
        FCk_Handle InHandle,
        const FCk_Chrono& InNewTimer)
    -> void
{
    InHandle.Get<ck::FFragment_Timer_Current>() = ck::FFragment_Timer_Current{InNewTimer};
}

// --------------------------------------------------------------------------------------------------------------------
