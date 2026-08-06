#include "CkSmCondition_Timer.h"

#include "CkTimer/CkTimer_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_Timer::
    EnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    Super::EnterCondition(InHandle, InNetContext);

    auto ScriptEntity = DoGet_ScriptEntity();

    const auto TimerParams = FCk_Timer_Spec{_Duration}
        .Set_CountDirection(ECk_Timer_CountDirection::CountDown)
        .Set_Behavior(ECk_Timer_Behavior::PauseOnDone)
        .Set_StartingState(ECk_Timer_State::Running);

    _TimerHandle = UCk_Utils_Timer_UE::Add(ScriptEntity, TimerParams);

    auto Delegate = FCk_Delegate_Timer{};
    Delegate.BindDynamic(this, &ThisType::OnTimerComplete);
    UCk_Utils_Timer_UE::BindTo_OnDone(_TimerHandle, Delegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame);
}

auto
    UCk_SmCondition_Timer::
    ExitCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    if (ck::IsValid(_TimerHandle))
    {
        auto Delegate = FCk_Delegate_Timer{};
        Delegate.BindDynamic(this, &ThisType::OnTimerComplete);
        UCk_Utils_Timer_UE::UnbindFrom_OnDone(_TimerHandle, Delegate);
        _TimerHandle = {};
    }

    Super::ExitCondition(InHandle, InNetContext);
}

void
    UCk_SmCondition_Timer::
    OnTimerComplete(
        FCk_Handle_Timer InHandle,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT)
{
    MarkSatisfied();
}

// --------------------------------------------------------------------------------------------------------------------
