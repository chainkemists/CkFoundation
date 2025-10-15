namespace utils_timer
{

    FCk_Handle_Timer
    Create_Tick(
        FCk_Handle InAnyHandle,
        FCk_Delegate_Timer InDelegate)
    {
        auto Handle = InAnyHandle;

        auto Params = FCk_Fragment_Timer_ParamsData();
        Params.Set_StartingState(ECk_Timer_State::Running);
        Params.Set_Behavior(ECk_Timer_Behavior::ResetOnDone);

        auto Timer = utils_timer::Add(Handle, Params);
        utils_timer::BindTo_OnUpdate(Timer, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame, InDelegate);

        return Timer;
    }
}