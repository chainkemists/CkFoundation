#include "CkStateMachine_Debug_Utils.h"

#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_state_machine_debug_utils
{
    // GFrameCounter stamp of the last debugger read. Game thread only (Slate ticks + ECS scheduler).
    static uint64 LastConsumedFrame = 0;

    // Above the debugger windows' refresh-gate interval so the poll gate doesn't flap between reads.
    constexpr uint64 ConsumedGraceFrames = 300;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    Request_RecordTransition(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Request_SmDebug_RecordTransition& InRequest)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, InRequest);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    DoAddRequest(
        FCk_Handle_StateMachine& InStateMachine,
        const auto& InRequest)
    -> FCk_Handle_StateMachine
{
    auto& Requests = InStateMachine.AddOrGet<ck::FFragment_SmDebug_Requests>();
    Requests._Requests.Add(InRequest);
    return InStateMachine;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    NotifyDebugDataConsumed()
    -> void
{
    ck_state_machine_debug_utils::LastConsumedFrame = GFrameCounter;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    Get_IsDebugDataDesired()
    -> bool
{
    const auto LastConsumed = ck_state_machine_debug_utils::LastConsumedFrame;

    if (LastConsumed != 0 && GFrameCounter - LastConsumed <= ck_state_machine_debug_utils::ConsumedGraceFrames)
    { return true; }

    // The on-screen entity debug overlay reads SM debug data every tick while enabled. Resolved by
    // name — and re-probed until found, since the debugger plugin may load after this module — so
    // CkStateMachine takes no dependency on the debugger modules.
    static IConsoleVariable* OverlayCVar = nullptr;

    if (OverlayCVar == nullptr)
    { OverlayCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.DebugOverlay")); }

    return OverlayCVar != nullptr && OverlayCVar->GetInt() != 0;
}
