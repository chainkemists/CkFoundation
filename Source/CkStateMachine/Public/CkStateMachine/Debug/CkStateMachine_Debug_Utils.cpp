#include "CkStateMachine_Debug_Utils.h"

#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_state_machine_debug_utils
{
    // GFrameCounter stamp of the last debugger read. Game thread only (Slate ticks + ECS scheduler).
    static uint64 LastConsumedFrame = 0;

    // Above the debugger windows' refresh-gate interval so the poll gate doesn't flap between reads.
    constexpr uint64 ConsumedGraceFrames = 300;

    // Slate owns this value through Set_IsDebuggerCaptureVisible. It deliberately has no grace
    // period: closing or backgrounding the SM debugger immediately disarms history capture.
    static bool GIsDebuggerCaptureVisible = false;
    static uint64 GDebuggerCaptureGeneration = 0;
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
    if (NOT Get_IsDebuggerCaptureActive(InStateMachine))
    { return InStateMachine; }

    auto& Requests = InStateMachine.AddOrGet<ck::FFragment_SmDebug_Requests>();

    const auto CaptureGeneration = Get_DebuggerCaptureGeneration();
    if (Requests._CaptureGeneration != CaptureGeneration)
    {
        Requests._Requests.Reset();
        Requests._CaptureGeneration = CaptureGeneration;
    }
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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    Set_IsDebuggerCaptureVisible(const bool InIsVisible)
    -> void
{
    if (ck_state_machine_debug_utils::GIsDebuggerCaptureVisible == InIsVisible)
    { return; }

    ck_state_machine_debug_utils::GIsDebuggerCaptureVisible = InIsVisible;
    if (InIsVisible)
    { ++ck_state_machine_debug_utils::GDebuggerCaptureGeneration; }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    Get_IsDebuggerCaptureVisible()
    -> bool
{
    return ck_state_machine_debug_utils::GIsDebuggerCaptureVisible;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    Get_DebuggerCaptureGeneration()
    -> uint64
{
    return ck_state_machine_debug_utils::GDebuggerCaptureGeneration;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    BeginDebuggerCapture(FCk_Handle_StateMachine& InStateMachine)
    -> void
{
    if (NOT Get_IsDebuggerCaptureVisible() || ck::Is_NOT_Valid(InStateMachine))
    { return; }

    const auto CaptureGeneration = Get_DebuggerCaptureGeneration();
    auto& Debug = InStateMachine.AddOrGet<ck::FFragment_Sm_Debug>();
    if (Debug._CaptureGeneration == CaptureGeneration)
    { return; }

    Debug._CachedStates.Reset();
    Debug._History.Reset();
    Debug._LastObservedStateClass = nullptr;
    Debug._CurrentStateEnteredAtRealTime = 0.0;
    Debug._CompletedRuns.Reset();
    Debug._RunCounter = 0;
    Debug._LastObservedRunStatus = ECk_SmRunStatus::Stopped;
    Debug._CurrentRunStartRealTime = 0.0;
    Debug._CaptureGeneration = CaptureGeneration;

#if CK_BUILD_SM_GRAPH_WALK
    if (NOT InStateMachine.Has<ck::FFragment_Sm_Debug_GraphDefinition>())
    { InStateMachine.AddOrGet<ck::FTag_Sm_Debug_RequiresGraphWalk>(); }
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    Get_IsDebuggerCaptureActive(const FCk_Handle_StateMachine& InStateMachine)
    -> bool
{
    if (NOT Get_IsDebuggerCaptureVisible() || ck::Is_NOT_Valid(InStateMachine))
    { return false; }

    return InStateMachine.Has<ck::FFragment_Sm_Debug>()
        && InStateMachine.Get<ck::FFragment_Sm_Debug>().Get_CaptureGeneration()
            == Get_DebuggerCaptureGeneration();
}
