#include "CkCueBase_EntityScript.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkTimer/CkTimer_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueBase_EntityScript::
    Construct(FCk_Handle& InHandle, const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow
{
    if (_LifetimeBehavior == ECk_Cue_LifetimeBehavior::Timed)
    {
        auto TimerParams = FCk_Fragment_Timer_ParamsData{_LifetimeDuration}
            .Set_TimerName(FGameplayTag::RequestGameplayTag("Cue.Lifetime"))
            .Set_Behavior(ECk_Timer_Behavior::StopOnDone)
            .Set_StartingState(ECk_Timer_State::Running);

        auto LifetimeTimer = UCk_Utils_Timer_UE::Add(_AssociatedEntity, TimerParams);

        // Bind to timer completion to self-destruct
        auto OnDoneDelegate = FCk_Delegate_Timer();
        OnDoneDelegate.BindUFunction(this, FName("OnLifetimeExpired"));
        UCk_Utils_Timer_UE::BindTo_OnDone(LifetimeTimer, ECk_Signal_BindingPolicy::IgnorePayloadInFlight, OnDoneDelegate);
    }

    return Super::Construct(InHandle, InSpawnParams);
}

auto
    UCk_CueBase_EntityScript::
    OnLifetimeExpired(FCk_Handle_Timer InTimer, FCk_Chrono InChrono, FCk_Time InDeltaT) -> void
{
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_AssociatedEntity);
}

// --------------------------------------------------------------------------------------------------------------------
