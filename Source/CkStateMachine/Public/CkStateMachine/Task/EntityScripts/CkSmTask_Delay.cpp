#include "CkSmTask_Delay.h"

#include "CkTimer/CkTimer_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_Delay::
    BeginPlay()
    -> void
{
    auto ScriptEntity = DoGet_ScriptEntity();

    const auto TimerParams = FCk_Fragment_Timer_ParamsData{_Duration}
        .Set_CountDirection(ECk_Timer_CountDirection::CountDown)
        .Set_Behavior(ECk_Timer_Behavior::PauseOnDone)
        .Set_StartingState(ECk_Timer_State::Running);

    _DelayTimerHandle = UCk_Utils_Timer_UE::Add(ScriptEntity, TimerParams);

    auto Delegate = FCk_Delegate_Timer{};
    Delegate.BindDynamic(this, &ThisType::OnDelayTimerComplete);
    UCk_Utils_Timer_UE::BindTo_OnDone(_DelayTimerHandle, Delegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame);

    Super::BeginPlay();
}

auto
    UCk_SmTask_Delay::
    EndPlay()
    -> void
{
    if (ck::IsValid(_DelayTimerHandle))
    {
        auto Delegate = FCk_Delegate_Timer{};
        Delegate.BindDynamic(this, &ThisType::OnDelayTimerComplete);
        UCk_Utils_Timer_UE::UnbindFrom_OnDone(_DelayTimerHandle, Delegate);
        _DelayTimerHandle = {};
    }

    Super::EndPlay();
}

void
    UCk_SmTask_Delay::
    OnDelayTimerComplete(
        FCk_Handle_Timer InHandle,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT)
{
    Mark_Result(ECk_SmTaskResult::Succeeded);
}

// --------------------------------------------------------------------------------------------------------------------
