#include "CkSmCondition_Timer.h"

#include "CkTimer/CkTimer_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_Timer::
    BeginPlay()
    -> void
{
    auto ScriptEntity = DoGet_ScriptEntity();

    const auto TimerParams = FCk_Fragment_Timer_ParamsData{_Duration}
        .Set_CountDirection(ECk_Timer_CountDirection::CountDown)
        .Set_Behavior(ECk_Timer_Behavior::PauseOnDone)
        .Set_StartingState(ECk_Timer_State::Running);

    auto TimerHandle = UCk_Utils_Timer_UE::Add(ScriptEntity, TimerParams);

    auto Delegate = FCk_Delegate_Timer{};
    Delegate.BindDynamic(this, &ThisType::OnTimerComplete);
    UCk_Utils_Timer_UE::BindTo_OnDone(TimerHandle, Delegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame);

    Super::BeginPlay();
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
