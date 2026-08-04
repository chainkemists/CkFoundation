#pragma once

#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment_Data.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkStateMachine_Debug_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Debug-side mirror of UCk_Utils_StateMachine_UE: the core state machine calls these Request_*
// entry points instead of reaching into the debug processor directly.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKSTATEMACHINE_API UCk_Utils_StateMachineDebug_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Called the moment a transition fires; same-frame pumps (A→B→C) each queue their own record
    // and are drained in order by the debug handle-requests processor.
    static auto
    Request_RecordTransition(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Request_SmDebug_RecordTransition& InRequest) -> FCk_Handle_StateMachine;

    // Demand tracking for the Sm_Debug poll processor: pull-based debugger consumers stamp each read
    // of FFragment_Sm_Debug here, and the poll skips its whole view iteration when nothing consumed
    // recently AND the on-screen debug overlay (ck.DebugOverlay) is off. Game thread only.
    static auto
    NotifyDebugDataConsumed() -> void;

    static auto
    Get_IsDebugDataDesired() -> bool;

    // Dedicated State Machine debugger capture contract. This is intentionally separate from
    // Get_IsDebugDataDesired(): the on-screen overlay may need current-state polling without
    // authorizing graph discovery or transition-history retention.
    static auto
    Set_IsDebuggerCaptureVisible(bool InIsVisible) -> void;

    static auto
    Get_IsDebuggerCaptureVisible() -> bool;

    // Start (or refresh) a capture for one machine. A new visible session resets its debug
    // history, so opening the debugger after transitions never backfills old history.
    static auto
    BeginDebuggerCapture(FCk_Handle_StateMachine& InStateMachine) -> void;

    static auto
    Get_IsDebuggerCaptureActive(const FCk_Handle_StateMachine& InStateMachine) -> bool;

    static auto
    Get_DebuggerCaptureGeneration() -> uint64;

private:
    static auto
    DoAddRequest(
        FCk_Handle_StateMachine& InStateMachine,
        const auto& InRequest) -> FCk_Handle_StateMachine;
};

// --------------------------------------------------------------------------------------------------------------------
