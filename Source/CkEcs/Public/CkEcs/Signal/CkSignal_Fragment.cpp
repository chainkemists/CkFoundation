#include "CkSignal_Fragment.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    static TAutoConsoleVariable<bool> CVar_StatSignalListeners(
        TEXT("ck.Signal.StatListeners"),
        false,
        TEXT("When enabled, each signal listener (bound delegate) invoked during a broadcast is timed under ")
        TEXT("`stat CkSignals_Listeners`, named ClassName::FunctionName. Off by default; opt-in for diagnosing ")
        TEXT("WHICH listener of a hot signal (e.g. OnTimerUpdate) is expensive."),
        ECVF_Default);

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Get_ShouldStat_SignalListeners()
        -> bool
    {
        return CVar_StatSignalListeners.GetValueOnAnyThread();
    }

    auto
        Make_SignalListenerStatId(
            const FString& InListenerName)
        -> TStatId
    {
        return CK_CREATE_DYNAMIC_STAT_ID(STATGROUP_CkSignals_Listeners, InListenerName);
    }
}

// --------------------------------------------------------------------------------------------------------------------
